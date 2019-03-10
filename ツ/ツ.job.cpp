#include "ツ.job.h"
#include "ツ.intrinsic.h"

struct Fiber
{
    void* context;
    u8*   stack;
    u64   stack_size;

    Fiber* next;
    Fiber* previous;

    Job_Counter* job_counter;
    u64          finished_job_count;
};

struct Fiber_List
{
    Fiber        sentinel;
    volatile u32 lock;
};

struct Thread
{
    void* handle;
    u32   id;

    Fiber*      current_fiber;
    Fiber*      previous_fiber;
    Fiber_List* list_to_add_previous_fiber;
};

#if defined(_WIN32)
    #define THREAD_PROC(name) DWORD WINAPI name(LPVOID data)
    #define create_semaphore(max_count) (void*)CreateSemaphore(NULL, (max_count), (max_count), NULL)
    #define semaphore_signal(semaphore_handle) ReleaseSemaphore((semaphore_handle), 1, NULL)
    #define semaphore_wait(semaphore_handle) WaitForSingleObject((semaphore_handle), INFINITE)
    #define create_suspended_thread(thread_proc, data) (void*)CreateThread(NULL, megabytes(1), (LPTHREAD_START_ROUTINE)(thread_proc), (data), CREATE_SUSPENDED, NULL)
    #define resume_thread(thread_handle) ResumeThread((HANDLE)(thread_handle))
    #define bind_thread_to_core(thread_handle, core) SetThreadAffinityMask((thread_handle) ? (HANDLE)(thread_handle) : GetCurrentThread(), (u64)1 << (core))

    internal DWORD thread_local_storage_index;
    #define init_thread_local_storage() thread_local_storage_index = TlsAlloc()
    #define set_thread_local_storage(value) TlsSetValue(thread_local_storage_index, (LPVOID)(value))
    #define get_thread_local_storage() TlsGetValue(thread_local_storage_index)

    #if defined(__clang__)
        #define FIBER_PROC(name) void name()
        typedef void (*Fiber_Proc)();

        internal void init_fiber(Fiber* fiber, u8* stack, u64 stack_size, Fiber_Proc fiber_proc)
        {
            fiber->stack = stack;
            fiber->stack_size = stack_size;

            uintptr_t* stack_pointer = (uintptr_t*)(((uintptr_t)stack + stack_size) & (uintptr_t)-16) - 2;
            *stack_pointer = (uintptr_t)fiber_proc;
            stack_pointer -= 8;

            *--stack_pointer = 0;
            *--stack_pointer = (uintptr_t)(stack + stack_size);
            *--stack_pointer = (uintptr_t)stack;

            fiber->context = (void*)(stack_pointer);
        }

        // TODO: We also have to save xmm6-xmm15 registers
        external void swap_context(void** current, void* dest) asm("_swap_context");
        asm(R"(
            .text
            _swap_context:

            pushq %r12
            pushq %r13
            pushq %r14
            pushq %r15
            pushq %rsi
            pushq %rdi
            pushq %rbp
            pushq %rbx
            pushq %gs:0x00
            pushq %gs:0x08
            pushq %gs:0x10

            movq %rsp, (%rcx)
            movq %rdx, %rsp

            popq %gs:0x10
            popq %gs:0x08
            popq %gs:0x00
            popq %rbx
            popq %rbp
            popq %rdi
            popq %rsi
            popq %r15
            popq %r14
            popq %r13
            popq %r12

            ret
        )");

        internal void switch_fiber(Fiber* current, Fiber* next)
        {
            void* dest = next->context;
            next->context = NULL;

            assert(dest);
            swap_context(&current->context, dest);
        }
    #endif
#elif defined(__APPLE__) && defined(__MACH__)
    // TODO: Define thread and fiber stuff for other platforms
#elif defined(__linux__)
    // TODO: Define thread and fiber stuff for other platforms
#endif

struct Job_Queue_Item
{
    volatile u64 index;
    Job          job;
};

struct Job_Queue
{
    Job_Queue_Item jobs[256];
    volatile u64   enqueue_index;
    volatile u64   dequeue_index;
    u64            mask;
};

internal void init_job_queue(Job_Queue* queue)
{
    u64 max_jobs = array_count(queue->jobs);

    queue->enqueue_index = 0;
    queue->dequeue_index = 0;
    queue->mask = max_jobs - 1;

    for (u64 i = 0; i < max_jobs; ++i)
    {
        queue->jobs[i].job = {};
        queue->jobs[i].index = i;
    }
}

internal b32 enqueue_job(Job_Queue* queue, Job job)
{
    u64 enqueue_index = queue->enqueue_index;
    while (true)
    {
        Job_Queue_Item* queue_item = &queue->jobs[enqueue_index & queue->mask];
        u64 index = queue_item->index;
        intptr_t diff = (intptr_t)index - (intptr_t)enqueue_index;

        if (diff == 0)
        {
            if (atomic_compare_exchange(&queue->enqueue_index, enqueue_index, enqueue_index + 1))
            {
                queue_item->job = job;

                READ_WRITE_BARRIER

                queue_item->index++;
                return true;
            }
        }
        else if (diff < 0)
        {
            return false;
        }
        else
        {
            enqueue_index = queue->enqueue_index;
        }
    }
    return false;
}

internal b32 dequeue_job(Job_Queue* queue, Job* job)
{
    u64 dequeue_index = queue->dequeue_index;
    while (true)
    {
        Job_Queue_Item* queue_item = &queue->jobs[dequeue_index & queue->mask];
        u64 index = queue_item->index;
        intptr_t diff = (intptr_t)index - (intptr_t)(dequeue_index + 1);

        if (diff == 0)
        {
            if (atomic_compare_exchange(&queue->dequeue_index, dequeue_index, dequeue_index + 1))
            {
                *job = queue_item->job;

                READ_WRITE_BARRIER

                queue_item->index = dequeue_index + queue->mask + 1;
                return true;
            }
        }
        else if (diff < 0)
        {
            return false;
        }
        else
        {
            dequeue_index = queue->dequeue_index;
        }
    }
    return false;
}

internal void spinlock_begin(volatile u32* lock)
{
    while (true)
    {
        if (*lock == 0)
        {
            READ_WRITE_BARRIER

            if (atomic_compare_exchange(lock, 0, 1))
            {
                break;
            }
        }
        _mm_pause();
    }
}

internal void spinlock_end(volatile u32* lock)
{
    _mm_sfence();
    *lock = 0;
}

#define FIBER_STACK_SIZE           kilobytes(64)
#define MAX_FIBER_COUNT            256
#define MAX_WORKER_THREAD_COUNT    16
#define MAX_DEDICATED_THREAD_COUNT 4

struct Scheduler
{
    Thread worker_threads[MAX_WORKER_THREAD_COUNT];
    u32    worker_thread_count;

    Thread dedicated_threads[MAX_DEDICATED_THREAD_COUNT];
    u32    dedicated_thread_count;

    Fiber      fibers[MAX_FIBER_COUNT];
    Fiber_List fiber_wait_list;
    Fiber_List fiber_free_list;

    Job_Queue worker_threads_queue;
    Job_Queue dedicated_threads_queue;

    void* worker_threads_semaphore_handle;
    void* dedicated_threads_semaphore_handle;
};

internal Scheduler scheduler;

internal void add_fiber_to_list(Fiber_List* list, Fiber* fiber)
{
    fiber->previous = &list->sentinel;
    fiber->next = list->sentinel.next;
    if (fiber->next)
    {
        fiber->next->previous = fiber;
    }
    list->sentinel.next = fiber;
}

internal void remove_fiber_from_list(Fiber* fiber)
{
    fiber->previous->next = fiber->next;
    if (fiber->next)
    {
        fiber->next->previous = fiber->previous;
    }

    fiber->previous = NULL;
    fiber->next = NULL;
}

internal Fiber* get_free_fiber()
{
    spinlock_begin(&scheduler.fiber_free_list.lock);

    Fiber* fiber = scheduler.fiber_free_list.sentinel.next;
    remove_fiber_from_list(fiber);

    spinlock_end(&scheduler.fiber_free_list.lock);
    return fiber;
}

internal Fiber* get_finished_waiting_fiber()
{
    spinlock_begin(&scheduler.fiber_wait_list.lock);

    Fiber* fiber = scheduler.fiber_wait_list.sentinel.next;
    while (fiber)
    {
        if (*fiber->job_counter == fiber->finished_job_count)
        {
            break;
        }
        fiber = fiber->next;
    }

    if (fiber)
    {
        remove_fiber_from_list(fiber);
    }

    spinlock_end(&scheduler.fiber_wait_list.lock);
    return fiber;
}

FIBER_PROC(fiber_proc)
{
    Thread* thread = (Thread*)get_thread_local_storage();
    if (thread->previous_fiber)
    {
        assert(thread->list_to_add_previous_fiber);

        spinlock_begin(&thread->list_to_add_previous_fiber->lock);
        add_fiber_to_list(thread->list_to_add_previous_fiber, thread->previous_fiber);
        spinlock_end(&thread->list_to_add_previous_fiber->lock);
    }

    u32 retry_count = 0;
    while (true)
    {
        Fiber* fiber = get_finished_waiting_fiber();
        if (fiber)
        {
            thread = (Thread*)get_thread_local_storage();
            assert(thread && thread->current_fiber);

            thread->list_to_add_previous_fiber = &scheduler.fiber_free_list;
            thread->previous_fiber = thread->current_fiber;
            thread->current_fiber = fiber;
            switch_fiber(thread->previous_fiber, fiber);

            thread = (Thread*)get_thread_local_storage();
            spinlock_begin(&thread->list_to_add_previous_fiber->lock);
            add_fiber_to_list(thread->list_to_add_previous_fiber, thread->previous_fiber);
            spinlock_end(&thread->list_to_add_previous_fiber->lock);
        }

        Job job;
        if (!dequeue_job(&scheduler.worker_threads_queue, &job))
        {
            // TODO: Fix this!! See if there is a better way to fix putting the thread to sleep when there is no work to do!
            if (++retry_count >= 100)
            {
                semaphore_wait(scheduler.worker_threads_semaphore_handle);
                retry_count = 0;
            }
        }
        else
        {
            job.entry_point(job.data);
            atomic_decrement(job.counter);

            semaphore_signal(scheduler.worker_threads_semaphore_handle);
        }
    }
}

THREAD_PROC(worker_thread_proc)
{
    Fiber* fiber = get_free_fiber();
    assert(fiber);

    Thread* thread = (Thread*)data;
    thread->current_fiber = fiber;
    thread->previous_fiber = NULL;
    thread->list_to_add_previous_fiber = NULL;

    set_thread_local_storage(thread);

    fiber_proc();
    return 0;
}

THREAD_PROC(dedicated_thread_proc)
{
    while (true)
    {
        Job job;
        if (!dequeue_job(&scheduler.dedicated_threads_queue, &job))
        {
            semaphore_wait(scheduler.dedicated_threads_semaphore_handle);
        }
        else
        {
            job.entry_point(job.data);
            atomic_decrement(job.counter);

            // TODO: We may want to wake up the dedicated threads as well.
            // Seems to work without doing that (for now)!
            semaphore_signal(scheduler.worker_threads_semaphore_handle);
        }
    }

    return 0;
}

void init_job_system(u32 worker_thread_count, u32 dedicated_thread_count, Memory_Arena* memory_arena)
{
    init_thread_local_storage();

    for (u32 i = 0; i < array_count(scheduler.fibers); ++i)
    {
        u64 stack_size = kilobytes(64);
        init_fiber(&scheduler.fibers[i], memory_arena_reserve(memory_arena, stack_size), stack_size, fiber_proc);
        add_fiber_to_list(&scheduler.fiber_free_list, &scheduler.fibers[i]);
    }

    init_job_queue(&scheduler.worker_threads_queue);
    init_job_queue(&scheduler.dedicated_threads_queue);

    scheduler.worker_threads_semaphore_handle = create_semaphore(worker_thread_count);
    scheduler.dedicated_threads_semaphore_handle = create_semaphore(dedicated_thread_count);

    // NOTE: worker_thread_count should be the amount of worker threads wanted minus the main thread
    scheduler.worker_thread_count = worker_thread_count + 1;
    for (u32 i = 1; i < scheduler.worker_thread_count; ++i)
    {
        Thread* thread = &scheduler.worker_threads[i];
        thread->id = i;
        thread->handle = create_suspended_thread(&worker_thread_proc, thread);
        assert(thread->handle);

        bind_thread_to_core(thread->handle, i);
        resume_thread(thread->handle);
    }

    Fiber* fiber = get_free_fiber();
    assert(fiber);

    scheduler.worker_threads[0].id = 0;
    scheduler.worker_threads[0].current_fiber = fiber;
    scheduler.worker_threads[0].previous_fiber = NULL;
    scheduler.worker_threads[0].list_to_add_previous_fiber = NULL;
    bind_thread_to_core(NULL, 0);

    set_thread_local_storage(&scheduler.worker_threads[0]);

    scheduler.dedicated_thread_count = dedicated_thread_count;
    for (u32 i = 0; i < scheduler.dedicated_thread_count; ++i)
    {
        Thread* thread = &scheduler.dedicated_threads[i];
        thread->id = i;
        thread->handle = create_suspended_thread(&dedicated_thread_proc, thread);
        assert(thread->handle);

        resume_thread(thread->handle);
    }
}

internal void run_jobs(Job* jobs, u64 job_count, Job_Counter* counter, Job_Queue* queue, void* semaphore_handle)
{
    *counter = job_count;
    for (u64 i = 0; i < job_count;)
    {
        jobs[i].counter = counter;
        if (enqueue_job(queue, jobs[i]))
        {
            i++;
            semaphore_signal(semaphore_handle);
        }
    }
}

void run_jobs(Job* jobs, u64 job_count, Job_Counter* counter)
{
    run_jobs(jobs, job_count, counter, &scheduler.worker_threads_queue, scheduler.worker_threads_semaphore_handle);
}

void run_jobs_on_dedicated_thread(Job* jobs, u64 job_count, Job_Counter* counter)
{
    run_jobs(jobs, job_count, counter, &scheduler.dedicated_threads_queue, scheduler.dedicated_threads_semaphore_handle);
}

void wait_for_counter(Job_Counter* counter, u64 count)
{
    Thread* thread = (Thread*)get_thread_local_storage();
    assert(thread && thread->current_fiber);

    thread->current_fiber->job_counter = counter;
    thread->current_fiber->finished_job_count = count;

    Fiber* next_fiber = get_finished_waiting_fiber();
    if (!next_fiber)
    {
        next_fiber = get_free_fiber();
    }

    thread->list_to_add_previous_fiber = &scheduler.fiber_wait_list;
    thread->previous_fiber = thread->current_fiber;
    thread->current_fiber = next_fiber;
    switch_fiber(thread->previous_fiber, next_fiber);

    thread = (Thread*)get_thread_local_storage();
    spinlock_begin(&thread->list_to_add_previous_fiber->lock);
    add_fiber_to_list(thread->list_to_add_previous_fiber, thread->previous_fiber);
    spinlock_end(&thread->list_to_add_previous_fiber->lock);
}
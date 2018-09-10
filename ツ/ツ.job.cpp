#include "ツ.job.h"

struct Fiber
{
    void* handle;
    u64   stack_size;
};

// Platform specific threads and fibers
#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>

    #define FIBER_PROC(name) void CALLBACK name(PVOID data)
    #define THREAD_PROC(name) DWORD WINAPI name(LPVOID data)

    #define create_semaphore(max_count) (void*)CreateSemaphore(NULL, (max_count), (max_count), NULL)
    #define semaphore_signal(semaphore_handle) ReleaseSemaphore((semaphore_handle), 1, NULL)
    #define semaphore_wait(semaphore_handle) WaitForSingleObject((semaphore_handle), INFINITE)
    #define create_suspended_thread(thread_proc, data) (void*)CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(thread_proc), (data), CREATE_SUSPENDED, NULL)
    #define resume_thread(thread_handle) ResumeThread((HANDLE)(thread_handle))
    #define bind_thread_to_core(thread_handle, core) SetThreadAffinityMask((thread_handle) ? (HANDLE)(thread_handle) : GetCurrentThread(), (u64)1 << (core))
    #define convert_thread_to_fiber() ConvertThreadToFiber(NULL)
    #define switch_fiber(current, next) SwitchToFiber((next)->handle)

    internal void init_fiber(Fiber* fiber, void* fiber_proc, void* data)
    {
        if (fiber->handle)
        {
            DeleteFiber(fiber->handle);
        }

        fiber->handle = CreateFiber(fiber->stack_size, (LPFIBER_START_ROUTINE)fiber_proc, data);
        if (!fiber->handle)
        {
            // TODO: Logging
            printf("Unable to create fiber!\n");
        }
    };
#elif defined(__APPLE__) && defined(__MACH__)
    #define FIBER_PROC(name) void name(void* data)
    #define THREAD_PROC(name) void* thread_proc(void* data)

    // TODO: Define thread and fiber stuff for other platforms
#elif defined(__linux__)
    // TODO: Define thread and fiber stuff for other platforms
#endif

// Compiler specific atomic operations
#if defined(_MSC_VER)
    #include <intrin.h>

    #define READ_WRITE_BARRIER _ReadWriteBarrier();
    #define atomic_compare_exchange(dest, old_value, new_value) InterlockedCompareExchange((dest), (new_value), (old_value)) == (old_value)
    #define atomic_increment(dest) InterlockedIncrement((dest))
    #define atomic_decrement(dest) InterlockedDecrement((dest))
#elif defined(__clang__)
    #define READ_WRITE_BARRIER __asm__ __volatile__ ("" ::: "memory");
    #define atomic_compare_exchange(dest, old_value, new_value) __sync_bool_compare_and_swap((dest), (old_value), (new_value)) 
    #define atomic_increment(dest) __sync_add_and_fetch((dest), 1)
    #define atomic_decrement(dest) __sync_sub_and_fetch((dest), 1)
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
    u64 max_jobs = sizeof(queue->jobs) / sizeof(queue->jobs[0]);

    queue->enqueue_index = 0;
    queue->dequeue_index = 0;
    queue->mask = max_jobs - 1;

    for (u64 i = 0; i < max_jobs; ++i)
    {
        queue->jobs[i].index = i;
    }
}

internal bool enqueue_job(Job_Queue* queue, Job job)
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

internal bool dequeue_job(Job_Queue* queue, Job* job)
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
}

struct Job_Scheduler_Fiber
{
    Job_Scheduler_Fiber* next;
    Job_Scheduler_Fiber* previous;

    Fiber        fiber;
    Job_Counter* job_counter;
    u64          finished_job_count;
};

struct Job_Scheduler_Thread
{
    u32   id;
    void* handle;
    
    Job_Scheduler_Fiber*  current_fiber;
    Job_Scheduler_Fiber*  previous_fiber;
    Job_Scheduler_Fiber** previous_fiber_list;
    volatile u32*         previous_fiber_list_lock;
};

#define MAX_THREAD_COUNT 16
#define MAX_QUEUE_COUNT  JOB_PRIORITY_HIGH + 1
#define MAX_FIBER_COUNT  256

internal Job_Scheduler_Thread threads[MAX_THREAD_COUNT];
internal Job_Queue            queues[MAX_QUEUE_COUNT];
internal Job_Scheduler_Fiber  fibers[MAX_FIBER_COUNT];

struct Job_Scheduler
{
    u64                  thread_count;
    u64                  current_fiber_index = 0;

    Job_Scheduler_Fiber* fibers_free_list      = NULL;
    volatile u32         fibers_free_list_lock = 0;

    Job_Scheduler_Fiber* fibers_wait_list      = NULL;
    volatile u32         fibers_wait_list_lock = 0;

    void* semaphore_handle;
};

internal Job_Scheduler scheduler;
internal thread_local Job_Scheduler_Thread* scheduler_thread = NULL;

internal void insert_scheduler_fiber_into_list(Job_Scheduler_Fiber** list, Job_Scheduler_Fiber* fiber)
{
    if (!*list)
    {
        *list = fiber;
        fiber->previous = NULL;
        fiber->next = NULL;
    }
    else
    {
        fiber->previous = NULL;
        fiber->next = *list;
        (*list)->previous = fiber;
        *list = fiber;
    }
}

internal void remove_scheduler_fiber_from_list(Job_Scheduler_Fiber** list, Job_Scheduler_Fiber* fiber)
{
    if (fiber->next)
    {
        fiber->next->previous = fiber->previous;
    }
    if (fiber->previous)
    {
        fiber->previous->next = fiber->next;
    }
    if (*list == fiber)
    {
        *list = fiber->next;
    }

    fiber->next = NULL;
    fiber->previous = NULL;
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

internal Job_Scheduler_Fiber* get_free_fiber()
{
    Job_Scheduler_Fiber* fiber = NULL;

    spinlock_begin(&scheduler.fibers_free_list_lock);
    if (scheduler.current_fiber_index < MAX_FIBER_COUNT)
    {
        fiber = &fibers[scheduler.current_fiber_index++];
    }
    else if (scheduler.fibers_free_list)
    {
        fiber = scheduler.fibers_free_list;
        remove_scheduler_fiber_from_list(&scheduler.fibers_free_list, fiber);
    }

    spinlock_end(&scheduler.fibers_free_list_lock);
    return fiber;
}

internal Job_Scheduler_Fiber* get_finished_waiting_fiber()
{
    spinlock_begin(&scheduler.fibers_wait_list_lock);

    Job_Scheduler_Fiber* fiber = scheduler.fibers_wait_list;
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
        remove_scheduler_fiber_from_list(&scheduler.fibers_wait_list, fiber);
    }

    spinlock_end(&scheduler.fibers_wait_list_lock);
    return fiber;
}

FIBER_PROC(fiber_proc)
{
    if (data)
    {
        Job_Scheduler_Fiber* previous_fiber = (Job_Scheduler_Fiber*)data;
        spinlock_begin(&scheduler.fibers_wait_list_lock);
        insert_scheduler_fiber_into_list(&scheduler.fibers_wait_list, previous_fiber);
        spinlock_end(&scheduler.fibers_wait_list_lock);
    }

    while (true)
    {
        Job_Scheduler_Fiber* fiber = get_finished_waiting_fiber();
        if (fiber)
        {
            scheduler_thread->previous_fiber = scheduler_thread->current_fiber;
            scheduler_thread->previous_fiber_list = &scheduler.fibers_free_list;
            scheduler_thread->previous_fiber_list_lock = &scheduler.fibers_free_list_lock;

            scheduler_thread->current_fiber = fiber;
            switch_fiber(&scheduler_thread->previous_fiber->fiber, &scheduler_thread->current_fiber->fiber);
        }

        Job job;
        if (!dequeue_job(&queues[JOB_PRIORITY_HIGH], &job) &&
            !dequeue_job(&queues[JOB_PRIORITY_MEDIUM], &job) &&
            !dequeue_job(&queues[JOB_PRIORITY_LOW], &job))
        {
            semaphore_wait(scheduler.semaphore_handle);
        }
        else
        {
            scheduler_thread->current_fiber->job_counter = job.counter;
            job.entry_point(job.data);
            atomic_decrement(job.counter);
        }
    }
}

THREAD_PROC(thread_proc)
{
    scheduler_thread = (Job_Scheduler_Thread*)data;
    scheduler_thread->current_fiber = get_free_fiber();
    scheduler_thread->current_fiber->fiber.handle = convert_thread_to_fiber();

    fiber_proc(NULL);
    return 0;
}

bool init_job_system(u32 thread_count)
{
    scheduler.semaphore_handle = create_semaphore(thread_count);

    for (u64 i = 0; i < MAX_QUEUE_COUNT; ++i)
    {
        init_job_queue(&queues[i]);
    }

    u64 stack_size = 64 * 1024 * 1024;
    for (u64 i = 0; i < MAX_FIBER_COUNT; ++i)
    {
        fibers[i].next = NULL;
        fibers[i].previous = NULL;
        fibers[i].fiber.stack_size = stack_size;
        fibers[i].fiber.handle = NULL;
    }

    scheduler.thread_count = thread_count;
    for (u32 i = 1; i < scheduler.thread_count; ++i)
    {
        Job_Scheduler_Thread* thread = &threads[i];
        thread->id = i;
        thread->handle = create_suspended_thread(&thread_proc, thread);

        if (!thread->handle)
        {
            // TODO: Logging
            printf("Unable to create thread!\n");
            return false;
        }

        bind_thread_to_core(thread->handle, i);
        resume_thread(thread->handle);
    }

    threads[0].id = 0;
    bind_thread_to_core(NULL, 0);

    threads[0].current_fiber = get_free_fiber();
    threads[0].current_fiber->fiber.handle = convert_thread_to_fiber();
    scheduler_thread = &threads[0];

    return true;
}

void run_jobs(Job* jobs, u64 job_count, Job_Counter* counter)
{
    run_jobs(jobs, job_count, counter, JOB_PRIORITY_MEDIUM);
}

void run_jobs(Job* jobs, u64 job_count, Job_Counter* counter, Job_Priority priority)
{
    Job_Queue* queue = &queues[priority];
    *counter = job_count;

    u64 i = 0;
    while (i < job_count)
    {
        jobs[i].counter = counter;
        if (enqueue_job(queue, jobs[i]))
        {
            i++;
            semaphore_signal(scheduler.semaphore_handle);
        }
    }
}

void wait_for_counter(Job_Counter* counter, u64 count)
{
    scheduler_thread->previous_fiber = scheduler_thread->current_fiber;
    scheduler_thread->previous_fiber->finished_job_count = count;
    scheduler_thread->previous_fiber->job_counter = counter;

    scheduler_thread->previous_fiber_list = &scheduler.fibers_wait_list;
    scheduler_thread->previous_fiber_list_lock = &scheduler.fibers_wait_list_lock;
    scheduler_thread->current_fiber = get_finished_waiting_fiber();

    if (!scheduler_thread->current_fiber)
    {
        scheduler_thread->current_fiber = get_free_fiber();
        init_fiber(&scheduler_thread->current_fiber->fiber, fiber_proc, scheduler_thread->previous_fiber);
    }

    switch_fiber(&scheduler_thread->previous_fiber->fiber, &scheduler_thread->current_fiber->fiber);

    spinlock_begin(scheduler_thread->previous_fiber_list_lock);
    insert_scheduler_fiber_into_list(scheduler_thread->previous_fiber_list, scheduler_thread->previous_fiber);
    spinlock_end(scheduler_thread->previous_fiber_list_lock);
}


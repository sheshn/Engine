#include "ツ.job.h"
#include "ツ.intrinsic.h"

struct Fiber
{
    void* handle;
    u64   stack_size;
};

struct Thread
{
    void* handle;
    u32   id;
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
    #define THREAD_PROC(name) void* name(void* data)

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
    u64 max_jobs = sizeof(queue->jobs) / sizeof(queue->jobs[0]);

    queue->enqueue_index = 0;
    queue->dequeue_index = 0;
    queue->mask = max_jobs - 1;

    for (u64 i = 0; i < max_jobs; ++i)
    {
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
    Thread thread;

    Job_Scheduler_Fiber*  current_fiber;
    Job_Scheduler_Fiber*  previous_fiber;
    Job_Scheduler_Fiber** previous_fiber_list;
    volatile u32*         previous_fiber_list_lock;
};

#define MAX_THREAD_COUNT  16
#define MAX_QUEUE_COUNT  (JOB_PRIORITY_HIGH + 1)
#define MAX_FIBER_COUNT   256
#define FIBER_STACK_SIZE (64 * 1024 * 1024)

#define MAX_DEDICATED_THREAD_COUNT 4

struct Job_Scheduler
{
    Job_Scheduler_Thread threads[MAX_THREAD_COUNT];
    Job_Queue            queues[MAX_QUEUE_COUNT];
    Job_Scheduler_Fiber  fibers[MAX_FIBER_COUNT];

    u32 thread_count;
    u32 current_fiber_index = 0;

    Job_Scheduler_Fiber* fibers_free_list      = NULL;
    volatile u32         fibers_free_list_lock = 0;

    Job_Scheduler_Fiber* fibers_wait_list      = NULL;
    volatile u32         fibers_wait_list_lock = 0;

    void* semaphore_handle;

    // TODO: Maybe come up with a better name for this?
    Thread    dedicated_threads[MAX_DEDICATED_THREAD_COUNT];
    Job_Queue dedicated_thread_queue;

    u32   dedicated_thread_count;
    void* dedicated_thread_semaphore_handle;
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
        fiber = &scheduler.fibers[scheduler.current_fiber_index++];
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
        if (!dequeue_job(&scheduler.queues[JOB_PRIORITY_HIGH], &job) &&
            !dequeue_job(&scheduler.queues[JOB_PRIORITY_MEDIUM], &job) &&
            !dequeue_job(&scheduler.queues[JOB_PRIORITY_LOW], &job))
        {
            // TODO: Fix this. If all threads are asleep, no threads will ever wake up!
            // But we also want to sleep if there is no work to be done!
            // It works now, but might fail in the future. Or maybe it's already not working properly!
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

THREAD_PROC(dedicated_thread_proc)
{
    while (true)
    {
        Job job;
        if (!dequeue_job(&scheduler.dedicated_thread_queue, &job))
        {
            semaphore_wait(scheduler.dedicated_thread_semaphore_handle);
        }
        else
        {
            job.entry_point(job.data);
            atomic_decrement(job.counter);
        }
    }
}

u32 get_thread_id()
{
    // TODO: Take into account if the calling thread is one of the dedicated threads
    assert(scheduler_thread != NULL);
    return scheduler_thread->thread.id;
}

// TODO: Better name? Should indicate that it is the threads using fibers.
u64 get_thread_count()
{
    assert(scheduler_thread != NULL);
    return scheduler.thread_count;
}

b32 init_job_system(u32 thread_count, u32 dedicated_thread_count)
{
    assert(thread_count <= MAX_THREAD_COUNT);
    scheduler.semaphore_handle = create_semaphore(thread_count);

    for (u64 i = 0; i < MAX_QUEUE_COUNT; ++i)
    {
        init_job_queue(&scheduler.queues[i]);
    }
    for (u64 i = 0; i < MAX_FIBER_COUNT; ++i)
    {
        scheduler.fibers[i].next = NULL;
        scheduler.fibers[i].previous = NULL;
        scheduler.fibers[i].fiber.stack_size = FIBER_STACK_SIZE;
        scheduler.fibers[i].fiber.handle = NULL;
    }

    // NOTE: The main thread already counts as one thread
    // (thread_count - 1) additional threads will be created
    scheduler.thread_count = thread_count;
    for (u32 i = 1; i < scheduler.thread_count; ++i)
    {
        Job_Scheduler_Thread* thread = &scheduler.threads[i];
        thread->thread.id = i;
        thread->thread.handle = create_suspended_thread(&thread_proc, thread);

        if (!thread->thread.handle)
        {
            // TODO: Logging
            printf("Unable to create thread!\n");
            return false;
        }

        bind_thread_to_core(thread->thread.handle, i);
        resume_thread(thread->thread.handle);
    }

    scheduler.threads[0].thread.id = 0;
    bind_thread_to_core(NULL, 0);

    scheduler.threads[0].current_fiber = get_free_fiber();
    scheduler.threads[0].current_fiber->fiber.handle = convert_thread_to_fiber();
    scheduler_thread = &scheduler.threads[0];

    assert(dedicated_thread_count <= MAX_DEDICATED_THREAD_COUNT);
    scheduler.dedicated_thread_semaphore_handle = create_semaphore(dedicated_thread_count);
    init_job_queue(&scheduler.dedicated_thread_queue);

    scheduler.dedicated_thread_count = dedicated_thread_count;
    for (u32 i = 0; i < scheduler.dedicated_thread_count; ++i)
    {
        Thread* thread = &scheduler.dedicated_threads[i];
        thread->id = scheduler.thread_count + i;
        thread->handle = create_suspended_thread(&dedicated_thread_proc, thread);

        if (!thread->handle)
        {
            // TODO: Logging
            printf("Unable to create dedicated thread!\n");
            return false;
        }
        resume_thread(thread->handle);
    }

    return true;
}

void run_jobs(Job* jobs, u64 job_count, Job_Counter* counter)
{
    run_jobs(jobs, job_count, counter, JOB_PRIORITY_MEDIUM);
}

void run_jobs(Job* jobs, u64 job_count, Job_Counter* counter, Job_Priority priority)
{
    Job_Queue* queue = &scheduler.queues[priority];
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

void run_jobs_on_dedicated_thread(Job* jobs, u64 job_count, Job_Counter* counter)
{
    *counter = job_count;

    u64 i = 0;
    while (i < job_count)
    {
        jobs[i].counter = counter;
        if (enqueue_job(&scheduler.dedicated_thread_queue, jobs[i]))
        {
            i++;

            // TODO: Maybe only signal one thread to wake up?
            semaphore_signal(scheduler.dedicated_thread_semaphore_handle);
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
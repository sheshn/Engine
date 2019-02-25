#pragma once

#include "ツ.types.h"

#define JOB_ENTRY_POINT(name) void name(void* data)

typedef void (*Job_Entry_Point)(void* data);
typedef volatile u64 Job_Counter;

enum Job_Priority
{
    JOB_PRIORITY_LOW,
    JOB_PRIORITY_MEDIUM,
    JOB_PRIORITY_HIGH
};

struct Job
{
    Job_Entry_Point entry_point;
    void*           data;
    Job_Counter*    counter;
};

b32  init_job_system(u32 thread_count, u32 dedicated_thread_count);
void run_jobs(Job* jobs, u64 job_count, Job_Counter* counter);
void run_jobs(Job* jobs, u64 job_count, Job_Counter* counter, Job_Priority priority);
void run_jobs_on_dedicated_thread(Job* jobs, u64 job_count, Job_Counter* counter);
void wait_for_counter(Job_Counter* counter, u64 count);
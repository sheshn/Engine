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
    Job_Entry_Point entry_point = nullptr;
    void*           data        = nullptr;
    Job_Counter*    counter     = nullptr;
};

u32  get_thread_id();
u64  get_thread_count();
bool init_job_system(u32 threads);
void run_jobs(Job* jobs, u64 job_count, Job_Counter* counter);
void run_jobs(Job* jobs, u64 job_count, Job_Counter* counter, Job_Priority priority);
void wait_for_counter(Job_Counter* counter, u64 count);


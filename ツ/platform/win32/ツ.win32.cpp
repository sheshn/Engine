#include <stdio.h>

// NOTE: Unity build
#include "../../ツ.job.cpp"

JOB_ENTRY_POINT(job1_entry_point)
{
    int i = *(int*)data;
    printf("Job 1: %d\n", i);
}

JOB_ENTRY_POINT(job2_entry_point)
{
    Job_Counter job_counter1;
    Job jobs[1024];
    for (u64 i = 0; i < 1024; ++i)
    {
        jobs[i].entry_point = job1_entry_point;
        jobs[i].data = data;
    }
    run_jobs(&jobs[0], 1024, &job_counter1);
    wait_for_counter(&job_counter1, 0);

    int i = *(int*)data;
    printf("Job 2: %d\n", i);
}

int main()
{
    printf("Hello World!\n");

    if (!init_job_system(8))
    {
        printf("Failed to initialize job system!\n");
        return 1;
    }
    
    int i = 52;
    Job_Counter job_counter2;
    Job job2 = {job2_entry_point, &i, NULL}; 
    run_jobs(&job2, 1, &job_counter2);
    wait_for_counter(&job_counter2, 0);
    return 0;
}


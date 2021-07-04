#define TC_THREAD_IMPLEMENTATION
#include "../tc_thread.h"

#include <stdio.h>

// number of threads doing things
#define NUM_WORKERS 5

// mutex-locked printf
static tcthread_mutex_t mprint_mutex;
void mprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    tcthread_mutex_lock(mprint_mutex);
    vprintf(format, args);
    fflush(stdout);
    tcthread_mutex_unlock(mprint_mutex);
    va_end(args);
}

static int work_amount;
static tcthread_cond_t work_cond;
static tcthread_mutex_t work_mutex;
static void* worker(void* param)
{
    unsigned int worker_id = *(const unsigned int*)param;
    tcthread_mutex_lock(work_mutex);
    // srand() might or might not be thread-safe --- so we use a mutex just in case
    srand(worker_id);
    tcthread_mutex_unlock(work_mutex);
    mprintf("W%u: Started.\n", worker_id);
    for(;;)
    {
        tcthread_mutex_lock(work_mutex);
        if(!work_amount)
            mprintf("W%u: Waiting for work...\n", worker_id);
        while(!work_amount) // wait for work (or exit)
            tcthread_cond_wait(work_cond, work_mutex);
        int amount = work_amount--; // grab a work item
        // mostly unrelated to the mutex, but within it just in case (for the same reason as srand())
        unsigned int sleep_time = 1 + rand() % 4;
        tcthread_mutex_unlock(work_mutex);
        if(amount < 0)
            break;  // exit requested
        mprintf("W%u: Doing work for [%u] seconds. [%d work items remain]\n", worker_id, sleep_time, amount - 1);
        tcthread_sleep(sleep_time * 1000u); // simulate some work
        mprintf("W%u: Work done!\n", worker_id);
    }
    mprintf("W%u: Stopped.\n", worker_id);
    return NULL;
}

int main(void)
{
    mprint_mutex = tcthread_mutex_create(false);

    mprintf("# of cores: %u\n", tcthread_get_cpu_count());

    work_amount = 0;
    work_cond = tcthread_cond_create();
    work_mutex = tcthread_mutex_create(false);

    mprintf("Starting %d workers...\n", NUM_WORKERS);
    unsigned int worker_ids[NUM_WORKERS];
    tcthread_t workers[NUM_WORKERS];
    for(int i = 0; i < NUM_WORKERS; i++)
    {
        worker_ids[i] = i;
        workers[i] = tcthread_create(0, worker, &worker_ids[i]);
    }

    bool done = false;
    while(!done)
    {
        mprintf("Enter command [q=exit, 1=schedule 1 job, a=schedule %d jobs]:\n", NUM_WORKERS);
        char line[256];
        fgets(line, sizeof(line), stdin);
        switch(line[0])
        {
        case '1':   // wake 1 worker (because we have work items for 1)
            tcthread_mutex_lock(work_mutex);
            work_amount += 1;
            tcthread_mutex_unlock(work_mutex);
            tcthread_cond_signal(work_cond);
            break;
        case 'a':   // wake all workers (because we have enough work items for all)
            tcthread_mutex_lock(work_mutex);
            work_amount += NUM_WORKERS;
            tcthread_mutex_unlock(work_mutex);
            tcthread_cond_broadcast(work_cond);
            break;
        case 'q':
        case 0: // EOF
            done = true;
            break;
        }
    }
    mprintf("Exiting requested.\n");

    mprintf("Stopping workers...\n");
    // signal program end
    tcthread_mutex_lock(work_mutex);
    work_amount = -1;
    tcthread_mutex_unlock(work_mutex);
    tcthread_cond_broadcast(work_cond);
    // now wait for thread to join
    for(int i = 0; i < NUM_WORKERS; i++)
        tcthread_join(workers[i], NULL);
    mprintf("Done.\n");

    tcthread_cond_destroy(work_cond);
    tcthread_mutex_destroy(work_mutex);

    tcthread_mutex_destroy(mprint_mutex);
    return 0;
}

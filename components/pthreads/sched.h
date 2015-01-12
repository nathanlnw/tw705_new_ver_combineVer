#ifndef __SCHED_H__
#define __SCHED_H__

#include <rtthread.h>
#include <pthread.h>

/* Thread scheduling policies */
enum
{
    SCHED_OTHER = 0,
    SCHED_FIFO,
    SCHED_RR,
    SCHED_MIN = SCHED_OTHER,
    SCHED_MAX = SCHED_RR
};

struct sched_param
{
    int sched_priority;
};

#ifdef __cplusplus
extern "C"
{
#endif

    int sched_yield(void);
    int sched_get_priority_min(int policy);
    int sched_get_priority_max(int policy);
    int sched_setscheduler(pid_t pid, int policy);

#ifdef __cplusplus
}
#endif

#endif
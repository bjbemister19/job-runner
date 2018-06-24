#ifndef __JOB_RUNNER__
#define __JOB_RUNNER__

#include "stdint.h"

#define JOB_RUNNER_SHUTDOWN_COMPLETE 1

typedef enum {
    JR_INVALID_SHUTDOWN_CODE   =   -8,
    JR_TIMEOUT                 =   -7,
    JR_QUEUE_FULL              =   -6,
    JR_INVALID_DESTRUCTOR      =   -5,
    JR_INVALID_CMD             =   -4,
    JR_NULL_POINTER            =   -3,
    JR_MEMORY_ALLOC_FAIL       =   -2,
    JR_FAIL                    =   -1,
    JR_SUCCESS                 =    0
} jrerr_t ;

typedef enum {
    JOB_RUNNER_KEEP_ALIVE,
    JOB_RUNNER_IM_DONE,
    JOB_RUNNER_OK,
    JOB_RUNNER_SHUT_DOWN,
} job_runner_state_t;

typedef void* job_runner_shutdown_response_handle_t;

struct job_runner;

jrerr_t job_runner_add_job(struct job_runner* runner, void* job_callback, uint32_t repeat_delay);

jrerr_t job_runner_execute();

jrerr_t job_runner_shutdown(struct job_runner* runner);

jrerr_t job_runner_shutdown_async(struct job_runner* runner, job_runner_shutdown_response_handle_t* shutdown_resp_channel);

jrerr_t job_runner_await_shutdown(job_runner_shutdown_response_handle_t shutdown_resp_channel, uint32_t ticks_to_wait);

jrerr_t job_runner_get_shutdown_response_handle(struct job_runner* runner, job_runner_shutdown_response_handle_t* shutdown_resp_channel);

jrerr_t job_runner_create( struct job_runner** new_runner, uint32_t loop_delay );

#endif // __JOB_RUNNER__
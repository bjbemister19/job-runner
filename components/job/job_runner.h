/*
 * Job Runner
 *
 * Copyright (c) 2018 Brandon Bemister. All rights reserved.
 * https://github.com/bjbemister19/job-runner
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Brandon Bemister
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __JOB_RUNNER__
#define __JOB_RUNNER__

#include "stdint.h"


#define JOB_RUNNER_SHUTDOWN_COMPLETE 1

typedef enum {

    JR_NOT_STARTED             =  -10,
    JR_ALREADY_STARTED         =   -9,
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
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

#include "job_runner.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#define SAFE_FREE(ptr) if(ptr){free(ptr);}

struct job_runner_job {

    job_runner_state_t (*job_callback) (job_runner_state_t state);
    TickType_t last_run;
    TickType_t repeat_delay;

    struct job_runner_job* next;

};

struct job_runner {

    TaskHandle_t task_hnd;
    struct job_runner_job* jobs;
    TickType_t loop_delay;
    job_runner_state_t state;
    xQueueHandle* cmd_queue;
    xQueueHandle* shutdown_resp_channel;

};

enum cmd_type {
    JR_CMD_TYPE_SHUTDOWN,
};

struct job_cmd {

    enum cmd_type type;
    void* cmd_data;
    void (*cmd_dtor)(void* cmd_data);

};

static void __job_runner_free_job(struct job_runner_job* job){
    SAFE_FREE(job);
}

static void __job_runner_free_runner(struct job_runner* runner){

    if(runner){
        if(runner->cmd_queue){
            vQueueDelete(runner->cmd_queue);
        }
    }

    SAFE_FREE(runner);
}

static jrerr_t __job_runner_process_current(struct job_runner* runner, struct job_runner_job** in_current, struct job_runner_job** in_previous){

    if(runner == NULL){
        return JR_NULL_POINTER;
    }

    if(*in_current == NULL){
        *in_previous = *in_current;
        *in_current = runner->jobs;
    }

    struct job_runner_job* previous = *in_previous;
    struct job_runner_job* current = *in_current;

    if(current == NULL){
        return JR_NULL_POINTER;
    }

    TickType_t now = xTaskGetTickCount();
    TickType_t elapsed = now - current->last_run;

    if( (elapsed >= current->repeat_delay) || (runner->state == JOB_RUNNER_SHUT_DOWN) ){

        job_runner_state_t job_state = current->job_callback(runner->state);
        if(job_state == JOB_RUNNER_IM_DONE){
            if(previous == NULL){
                // We must be at the head
                runner->jobs = current->next;
                __job_runner_free_job(current);
                current = runner->jobs;
            } 
            else {
                previous->next = current->next;
                __job_runner_free_job(current);
                current = previous->next;
            }
            
            *in_previous = previous;
            *in_current = current;

        } 
        else {

            current->last_run = xTaskGetTickCount();
            *in_previous = current;
            *in_current = current->next;

        }

    } 
    else {

        *in_previous = current;
        *in_current = current->next;
    
    }

    return JR_SUCCESS;

}

static jrerr_t __job_runner_process_command(struct job_runner* runner){
    
    if(runner == NULL){
        return JR_NULL_POINTER;
    }

    if(runner->cmd_queue == NULL){
        return JR_NULL_POINTER;
    }

    jrerr_t err = JR_SUCCESS;

    struct job_cmd cmd = {0};
    BaseType_t rcvd = xQueueReceive(runner->cmd_queue, &cmd, 1);

    if(rcvd == pdTRUE){
        // Process New Command
        switch(cmd.type){
            case JR_CMD_TYPE_SHUTDOWN:

                runner->state = JOB_RUNNER_SHUT_DOWN;
                
                if(cmd.cmd_data != NULL){
                    // A special case where the caller can provide a queue to be notified on when the shutdown is complete
                    runner->shutdown_resp_channel = cmd.cmd_data;
                    cmd.cmd_data = NULL;
                }

                break;
            
            default:
                err = JR_INVALID_CMD;
                break;
        }

        if(cmd.cmd_data != NULL){
            if(cmd.cmd_dtor != NULL){
                cmd.cmd_dtor(cmd.cmd_data);
            }
            else {
                err = JR_INVALID_DESTRUCTOR;
            }
        }

    }

    return err;
}

static void __job_runner_task( void* params ) {

    struct job_runner* runner = (struct job_runner*) params;
    if(runner == NULL){
        ESP_LOGE("__job_runner_task","Runner NULL, Abrting!!!");
        vTaskDelete(NULL);
    }

    jrerr_t err = JR_SUCCESS;

    struct job_runner_job* previous = NULL;
    struct job_runner_job* current = runner->jobs;

    while(runner->jobs != NULL){

        err = __job_runner_process_current(runner, &current, &previous);
        if(err != JR_SUCCESS){
            ESP_LOGE("__job_runner_task","Error Processing Current, killing runner!!!");
            break;
        }

        err = __job_runner_process_command(runner);
        if(err != JR_SUCCESS){
            ESP_LOGE("__job_runner_task","Error Processing Command, killing runner!!!");
            break;
        }

        vTaskDelay(runner->loop_delay);

    }

    ESP_LOGI("__job_runner_task","No More Jobs, Shutting Down Runner!!!");

    if(runner->shutdown_resp_channel != NULL){
        const int resp = JOB_RUNNER_SHUTDOWN_COMPLETE;
        xQueueSend(runner->shutdown_resp_channel, &resp, portMAX_DELAY);
    }

    __job_runner_free_runner(runner);

    vTaskDelete(NULL);

}

jrerr_t job_runner_add_job(struct job_runner* runner, void* job_callback, uint32_t repeat_delay) {

    jrerr_t err = JR_SUCCESS;

    struct job_runner_job* new_job = NULL;

    if( runner == NULL || job_callback == NULL ){
        err = JR_NULL_POINTER;
    }

    if(err == JR_SUCCESS){

        new_job = malloc( sizeof(struct job_runner_job) );
        if(new_job == NULL){
            err = JR_MEMORY_ALLOC_FAIL;
        }

    }

    if(err == JR_SUCCESS){

        new_job->job_callback = job_callback;
        new_job->last_run = 0;
        new_job->repeat_delay = repeat_delay;

        new_job->next = runner->jobs;
        runner->jobs = new_job;

    }

    if(err != JR_SUCCESS){

        SAFE_FREE(new_job);

    }

    return err;

}

jrerr_t job_runner_execute(struct job_runner* runner, const char* runner_name, uint32_t runner_stack, unsigned int priority){

    jrerr_t err = JR_SUCCESS;

    BaseType_t crerr = xTaskCreate(__job_runner_task, runner_name, runner_stack, runner, priority, &runner->task_hnd);
    if(crerr != pdPASS){
        err = JR_FAIL;
    }
    
    return err;

}

jrerr_t job_runner_shutdown(struct job_runner* runner){

    jrerr_t err = JR_SUCCESS;

    if(runner == NULL){
        err = JR_NULL_POINTER;
    }

    if(err == JR_SUCCESS){

        if(runner->cmd_queue == NULL){
            err = JR_NULL_POINTER;
        }

    }

    if(err == JR_SUCCESS){

        struct job_cmd cmd = { .type = JR_CMD_TYPE_SHUTDOWN, .cmd_data = NULL, .cmd_dtor = NULL };
        BaseType_t sderr = xQueueSend(runner->cmd_queue, &cmd, portMAX_DELAY);
        if(sderr != pdTRUE){
            err = JR_QUEUE_FULL;
        }
    
    }

    return err;

}

jrerr_t job_runner_shutdown_async(struct job_runner* runner, job_runner_shutdown_response_handle_t* shutdown_resp_channel){

    jrerr_t err = JR_SUCCESS;
    xQueueHandle* sd_resp_hnd = NULL;

    if(runner == NULL){
        err = JR_NULL_POINTER;
    }

    if(err == JR_SUCCESS){

        if(runner->cmd_queue == NULL){
            err = JR_NULL_POINTER;
        }

    }

    if(err == JR_SUCCESS){

        sd_resp_hnd = xQueueCreate(1, sizeof(int));
        if(sd_resp_hnd == NULL){
            err = JR_NULL_POINTER;
        }

    }

    if(err == JR_SUCCESS){

        struct job_cmd cmd = { .type = JR_CMD_TYPE_SHUTDOWN, .cmd_data = sd_resp_hnd, .cmd_dtor = NULL };
        BaseType_t sderr = xQueueSend(runner->cmd_queue, &cmd, portMAX_DELAY);
        if(sderr != pdTRUE){
            err = JR_QUEUE_FULL;
        }
    
    }

    if(err != JR_SUCCESS){
        if(sd_resp_hnd){
            vQueueDelete(sd_resp_hnd);
        }
    } 
    else {
        *shutdown_resp_channel = sd_resp_hnd;
    }

    return err;

}

jrerr_t job_runner_await_shutdown(job_runner_shutdown_response_handle_t shutdown_resp_channel, uint32_t ticks_to_wait){

    jrerr_t err = JR_SUCCESS;

    int resp = 0;

    if(shutdown_resp_channel == NULL){
        err = JR_NULL_POINTER;
    }

    if(err == JR_SUCCESS){

        BaseType_t resp_err = xQueueReceive(shutdown_resp_channel, &resp, ticks_to_wait);
        if(resp_err != pdPASS){
            err = JR_TIMEOUT;
        }

    }

    if(err == JR_SUCCESS){

        if(resp != JOB_RUNNER_SHUTDOWN_COMPLETE){
            err = JR_INVALID_SHUTDOWN_CODE;
        }
    }

    if(shutdown_resp_channel != NULL){
        vQueueDelete(shutdown_resp_channel);
    }

    return err;

}

jrerr_t job_runner_get_shutdown_response_handle(struct job_runner* runner, job_runner_shutdown_response_handle_t* shutdown_resp_channel){

    jrerr_t err = JR_SUCCESS;
    xQueueHandle* sd_resp_hnd = NULL;

    if(runner == NULL){
        err = JR_NULL_POINTER;
    }

    if(err == JR_SUCCESS){

        sd_resp_hnd = xQueueCreate(1, sizeof(int));
        if(sd_resp_hnd == NULL){
            err = JR_NULL_POINTER;
        }

    } 

    if(err == JR_SUCCESS){

        runner->shutdown_resp_channel = sd_resp_hnd;
        *shutdown_resp_channel = sd_resp_hnd;
        
    }

    return err;

}

jrerr_t job_runner_create( struct job_runner** new_runner, uint32_t loop_delay ){
    
    jrerr_t err = JR_SUCCESS;

    struct job_runner* runner = NULL;

    runner = malloc( sizeof(struct job_runner) );
    if(runner == NULL){
        err = JR_MEMORY_ALLOC_FAIL;
    }

    if(err == JR_SUCCESS){

        runner->task_hnd = NULL;
        runner->jobs = NULL;
        runner->loop_delay = loop_delay;
        runner->state = JOB_RUNNER_OK;

        runner->cmd_queue = xQueueCreate(5, sizeof(struct job_cmd));
        runner->shutdown_resp_channel = NULL;

    }

    if(err == JR_SUCCESS){

        *new_runner = runner;

    }

    if(err != JR_SUCCESS){

        SAFE_FREE(runner);

    }    

    return err;

}

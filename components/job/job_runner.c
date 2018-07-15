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

    job_runner_state_t (*job_callback) (job_runner_state_t state, void* data);
    TickType_t last_run;
    TickType_t repeat_delay;
    int16_t job_id;
    
    int8_t notif;
    void* notif_data;
    void (*notif_dtor)(void* notif_data);

    struct job_runner_job* next;

};

struct job_runner {

    TaskHandle_t task_hnd;
    struct job_runner_job* jobs;
    TickType_t loop_delay;
    job_runner_state_t state;
    xQueueHandle* cmd_queue;
    xQueueHandle* shutdown_resp_channel;
    int8_t started;

};

enum cmd_type {

    JR_CMD_TYPE_SHUTDOWN,
    JR_CMD_TYPE_NOTIFY

};

struct job_cmd {

    enum cmd_type type;
    int16_t job_id;
    void* cmd_data;
    void (*cmd_dtor)(void* cmd_data);

};

static void __job_runner_free_job(struct job_runner_job* job){

    if(job->notif_data){

        if(job->notif_dtor){

            job->notif_dtor(job->notif_data);
            job->notif_dtor = NULL;
        
        }
        
        job->notif_data = NULL;

    }

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

static bool __job_runner_contains_job(struct job_runner* runner, int16_t job_id){

    if(runner == NULL){
        return false;
    }

    struct job_runner_job* start = runner->jobs;
    
    while( start != NULL ){

        if(start->job_id == job_id){
            return true;
        }

        start = start->next;

    }

    return false;

}

static jrerr_t __job_runner_find_job(struct job_runner* runner, struct job_runner_job** job, int16_t job_id){
    
    jrerr_t err = JR_SUCCESS;

    if(runner == NULL || job == NULL){
        err = JR_NULL_POINTER;
    }

    if(err == JR_SUCCESS){
    
        struct job_runner_job* start = runner->jobs;
        
        while( start != NULL ){

            *job = NULL;

            if(start->job_id == job_id){
                *job = start;
                break;
            }

            start = start->next;

        }

        if(job == NULL){
            err = JR_JOB_NOT_EXIST;
        }
    
    }

    return err;

}

static jrerr_t __job_runner_assign_job_id(struct job_runner* runner, struct job_runner_job* job){

    jrerr_t err = JR_SUCCESS;

    if(runner == NULL || job == NULL){

        err = JR_NULL_POINTER;

    }

    if(err == JR_SUCCESS){

        int16_t id = 0;

        while( __job_runner_contains_job(runner, id) ){

            id++;

        }

        job->job_id = id;

    }

    return err;

}

static jrerr_t __job_runner_process_notification(struct job_runner* runner, struct job_cmd* cmd){

    jrerr_t err = JR_SUCCESS;

    struct job_runner_job* job = NULL;

    if(runner == NULL || cmd == NULL){
        err = JR_NULL_POINTER;
    }

    if(err == JR_SUCCESS){

        err = __job_runner_find_job(runner, &job, cmd->job_id);
    
    }

    if(err == JR_SUCCESS){

        job->notif = 1;
        job->notif_data = cmd->cmd_data;
        job->notif_dtor = cmd->cmd_dtor;

        // Pass ownership of this data onto the notification system. 
        cmd->cmd_data = NULL;
        cmd->cmd_dtor = NULL;

    }

    return err;

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

    if( (elapsed >= current->repeat_delay) || current->notif || (runner->state == JOB_RUNNER_SHUT_DOWN) ){

        job_runner_state_t job_state = current->job_callback(runner->state, current->notif_data);
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

            current->notif = 0;
            if(current->notif_data){

                if(current->notif_dtor){

                    current->notif_dtor(current->notif_data);
                    current->notif_dtor = NULL;
                
                }
                
                current->notif_data = NULL;

            }

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
            
            case JR_CMD_TYPE_NOTIFY:

                err = __job_runner_process_notification(runner, &cmd);
                if(err == JR_JOB_NOT_EXIST){
                    ESP_LOGE("Job Runner","Job Not Exist");

                    // Return Success so job runner does not get shut down. 
                    err = JR_SUCCESS;
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

jrerr_t job_runner_notify_job(struct job_runner* runner, int16_t job_id, void* notif_data, void (*notif_dtor)(void* nd) ){

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

        struct job_cmd cmd = { .type = JR_CMD_TYPE_NOTIFY, .job_id = job_id, .cmd_data = notif_data, .cmd_dtor = notif_dtor };
        BaseType_t sderr = xQueueSend(runner->cmd_queue, &cmd, portMAX_DELAY);
        if(sderr != pdTRUE){
            err = JR_QUEUE_FULL;
        }
    
    }

    return err;

}

jrerr_t job_runner_add_job(struct job_runner* runner, void* job_callback, uint32_t repeat_delay, int16_t* job_id) {

    jrerr_t err = JR_SUCCESS;

    struct job_runner_job* new_job = NULL;

    if( runner == NULL || job_callback == NULL ){
        err = JR_NULL_POINTER;
    }

    if( err == JR_SUCCESS ){

        if(runner->started){
            err = JR_ALREADY_STARTED;
        }

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
        new_job->notif = 0;
        new_job->notif_data = NULL;
        new_job->notif_dtor = NULL;

        err = __job_runner_assign_job_id(runner, new_job);

    }

    if(err == JR_SUCCESS){

        if(job_id != NULL){
            *job_id = new_job->job_id;
        }

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

    if(runner == NULL || runner_name == NULL){

        err = JR_NULL_POINTER;

    }

    if(err == JR_SUCCESS){

        if(runner->started){
            err = JR_ALREADY_STARTED;
        }

    }

    if(err == JR_SUCCESS){

        runner->started = 1;

        BaseType_t crerr = xTaskCreate(&__job_runner_task, runner_name, runner_stack, runner, priority, &runner->task_hnd);
        if(crerr != pdPASS){
            runner->started = 0;
            err = JR_FAIL;
        }

    }

    return err;

}

jrerr_t job_runner_shutdown(struct job_runner* runner){

    jrerr_t err = JR_SUCCESS;

    if(runner == NULL){
        err = JR_NULL_POINTER;
    }

    if(err == JR_SUCCESS){

        if( ! runner->started ){
            err = JR_NOT_STARTED;
        }
    
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

    if( err == JR_SUCCESS ){

        runner->started = 0;

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

        if( ! runner->started ){
            err = JR_NOT_STARTED;
        }
    
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
        runner->started = 0;

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
        runner->started = 0;

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

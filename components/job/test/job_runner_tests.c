#include "job_runner_tests.h"

#ifdef JOB_RUNNER_TESTING_ENABLE

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "job_runner.h"

#define PRINT_FREE_HEAP()    // ESP_LOGI("Free Heap", "%d", esp_get_free_heap_size());


#ifdef JOB_RUNNER_TEST_RAND_SHUTDOWN
job_runner_state_t job_runner_test_rand_job1(job_runner_state_t state){

    if(state == JOB_RUNNER_SHUT_DOWN){
        // Cleanup
        ESP_LOGI("job_runner_test_job1","Cheaning up for pending shutdown!!!");
        return JOB_RUNNER_IM_DONE;
    }

    PRINT_FREE_HEAP();

    static TickType_t last_run = 0;

    ESP_LOGI("job_runner_test_job1","Running Job 1 after %d milliseconds", (xTaskGetTickCount() - last_run) * portTICK_PERIOD_MS ) ;

    last_run = xTaskGetTickCount();

    uint32_t rand = ( esp_random() % 10 );

    if (rand == 1){
        ESP_LOGW("job_runner_test_job1","Got 1, shutting down..");
        return JOB_RUNNER_IM_DONE;
    } else {
        return JOB_RUNNER_KEEP_ALIVE;
    }

}

job_runner_state_t job_runner_test_rand_job2(job_runner_state_t state){

    if(state == JOB_RUNNER_SHUT_DOWN){
        // Cleanup
        ESP_LOGI("job_runner_test_job2","Cheaning up for pending shutdown!!!");
        return JOB_RUNNER_IM_DONE;
    }

    PRINT_FREE_HEAP();

    static TickType_t last_run = 0;

    ESP_LOGI("job_runner_test_job2","Running Job 2 after %d milliseconds", (xTaskGetTickCount() - last_run) * portTICK_PERIOD_MS);

    last_run = xTaskGetTickCount();

    uint32_t rand = ( esp_random() % 10 );

    if (rand == 1){
            ESP_LOGW("job_runner_test_job2","Got 1, shutting down..");
        return JOB_RUNNER_IM_DONE;
    } else {
        return JOB_RUNNER_KEEP_ALIVE;
    }

}

job_runner_state_t job_runner_test_rand_job3(job_runner_state_t state){

    if(state == JOB_RUNNER_SHUT_DOWN){
        // Cleanup
        ESP_LOGI("job_runner_test_job3","Cheaning up for pending shutdown!!!");
        return JOB_RUNNER_IM_DONE;
    }

    PRINT_FREE_HEAP();

    static TickType_t last_run = 0;

    ESP_LOGI("job_runner_test_job3","Running Job 3 after %d milliseconds", (xTaskGetTickCount() - last_run) * portTICK_PERIOD_MS);

    last_run = xTaskGetTickCount();

    uint32_t rand = ( esp_random() % 10 );

    if (rand == 1){
        ESP_LOGW("job_runner_test_job3","Got 1, shutting down..");
        return JOB_RUNNER_IM_DONE;
    } else {
        return JOB_RUNNER_KEEP_ALIVE;
    }

}

job_runner_state_t job_runner_test_rand_job4(job_runner_state_t state){

    if(state == JOB_RUNNER_SHUT_DOWN){
        // Cleanup
        ESP_LOGI("job_runner_test_job4","Cheaning up for pending shutdown!!!");
        return JOB_RUNNER_IM_DONE;
    }

    PRINT_FREE_HEAP();

    static TickType_t last_run = 0;

    ESP_LOGI("job_runner_test_job4","Running Job 4 after %d milliseconds", (xTaskGetTickCount() - last_run) * portTICK_PERIOD_MS);

    last_run = xTaskGetTickCount();

    uint32_t rand = ( esp_random() % 10 );

    if (rand == 1){
        ESP_LOGW("job_runner_test_job4","Got 1, shutting down..");
        return JOB_RUNNER_IM_DONE;
    } else {
        return JOB_RUNNER_KEEP_ALIVE;
    }

}

job_runner_state_t job_runner_test_rand_job5(job_runner_state_t state){

    if(state == JOB_RUNNER_SHUT_DOWN){
        // Cleanup
        ESP_LOGI("job_runner_test_job5","Cheaning up for pending shutdown!!!");
        return JOB_RUNNER_IM_DONE;
    }

    PRINT_FREE_HEAP();

    static TickType_t last_run = 0;

    ESP_LOGI("job_runner_test_job5","Running Job 5 after %d milliseconds", (xTaskGetTickCount() - last_run) * portTICK_PERIOD_MS);

    last_run = xTaskGetTickCount();

    uint32_t rand = ( esp_random() % 10 );

    if (rand == 1){
        ESP_LOGW("job_runner_test_job5","Got 1, shutting down..");
        return JOB_RUNNER_IM_DONE;
    } else {
        return JOB_RUNNER_KEEP_ALIVE;
    }

}

void job_runner_test_rand_shutdown(){

    ESP_LOGI("job_runner_test", "Job Runner Test.");

    PRINT_FREE_HEAP();

    jrerr_t err = JR_SUCCESS;

    struct job_runner* runner = NULL;
    job_runner_shutdown_response_handle_t hnd = NULL;
    err = job_runner_create(&runner, 1);
    
    if(err == JR_SUCCESS){

        err = job_runner_get_shutdown_response_handle(runner, &hnd);

    }

    if(err == JR_SUCCESS){

        err = job_runner_add_job(runner, job_runner_test_rand_job1, 5000 / portTICK_PERIOD_MS);

    }

    if(err == JR_SUCCESS){

        err = job_runner_add_job(runner, job_runner_test_rand_job2, 5000 / portTICK_PERIOD_MS);

    }

    if(err == JR_SUCCESS){

        err = job_runner_add_job(runner, job_runner_test_rand_job3, 5000 / portTICK_PERIOD_MS);

    }

    if(err == JR_SUCCESS){

        err = job_runner_add_job(runner, job_runner_test_rand_job4, 5000 / portTICK_PERIOD_MS);

    }

    if(err == JR_SUCCESS){

        err = job_runner_add_job(runner, job_runner_test_rand_job5, 5000 / portTICK_PERIOD_MS);

    }

    if(err == JR_SUCCESS){

        ESP_LOGI("job_runner_test","All Jobs Added, Starting Runner...");

        err = job_runner_execute(runner, "test_run", 4096, 5);

    }

    if(err == JR_SUCCESS){

        ESP_LOGI("job_runner_test","Runner Successfully Started!!!");

    }

    ESP_LOGI("job_runner_test","Waiting for jobs to finish");
    err = job_runner_await_shutdown(hnd, portMAX_DELAY);
    if(err != JR_SUCCESS){
        ESP_LOGE("job_runner_test","Failed to wait for shutdown!!!");
    } else {
        ESP_LOGI("job_runner_test","Job Runner has shutdown!!!");
    }

    PRINT_FREE_HEAP();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();
    
}
#endif //JOB_RUNNER_TEST_RAND_SHUTDOWN

#ifdef JOB_RUNNER_TEST_ASYNC_SHUTDOWN
job_runner_state_t job_runner_test_job1(job_runner_state_t state){

    if(state == JOB_RUNNER_SHUT_DOWN){
        // Cleanup
        ESP_LOGI("job_runner_test_job1","Cheaning up for pending shutdown!!!");
        return JOB_RUNNER_IM_DONE;
    }

    static TickType_t last_run = 0;

    ESP_LOGI("job_runner_test_job1","Running Job 1 after %d milliseconds", (xTaskGetTickCount() - last_run) * portTICK_PERIOD_MS ) ;

    last_run = xTaskGetTickCount();

    return JOB_RUNNER_KEEP_ALIVE;

}

job_runner_state_t job_runner_test_job2(job_runner_state_t state){

    if(state == JOB_RUNNER_SHUT_DOWN){
        // Cleanup
        ESP_LOGI("job_runner_test_job2","Cheaning up for pending shutdown!!!");
        return JOB_RUNNER_IM_DONE;
    }

    static TickType_t last_run = 0;

    ESP_LOGI("job_runner_test_job2","Running Job 2 after %d milliseconds", (xTaskGetTickCount() - last_run) * portTICK_PERIOD_MS);

    last_run = xTaskGetTickCount();

    return JOB_RUNNER_KEEP_ALIVE;

}

job_runner_state_t job_runner_test_job3(job_runner_state_t state){

    if(state == JOB_RUNNER_SHUT_DOWN){
        // Cleanup
        ESP_LOGI("job_runner_test_job3","Cheaning up for pending shutdown!!!");
        return JOB_RUNNER_IM_DONE;
    }

    static TickType_t last_run = 0;

    ESP_LOGI("job_runner_test_job3","Running Job 3 after %d milliseconds", (xTaskGetTickCount() - last_run) * portTICK_PERIOD_MS);

    last_run = xTaskGetTickCount();

    return JOB_RUNNER_KEEP_ALIVE;

}

job_runner_state_t job_runner_test_job4(job_runner_state_t state){

    if(state == JOB_RUNNER_SHUT_DOWN){
        // Cleanup
        ESP_LOGI("job_runner_test_job4","Cheaning up for pending shutdown!!!");
        return JOB_RUNNER_IM_DONE;
    }

    static TickType_t last_run = 0;

    ESP_LOGI("job_runner_test_job4","Running Job 4 at %d", (xTaskGetTickCount() - last_run) * portTICK_PERIOD_MS);

    last_run = xTaskGetTickCount();

    return JOB_RUNNER_KEEP_ALIVE;

}

job_runner_state_t job_runner_test_job5(job_runner_state_t state){

    if(state == JOB_RUNNER_SHUT_DOWN){
        // Cleanup
        ESP_LOGI("job_runner_test_job5","Cheaning up for pending shutdown!!!");
        return JOB_RUNNER_IM_DONE;
    }

    static TickType_t last_run = 0;

    ESP_LOGI("job_runner_test_job5","Running Job 5 after %d milliseconds", (xTaskGetTickCount() - last_run) * portTICK_PERIOD_MS);

    last_run = xTaskGetTickCount();

    return JOB_RUNNER_KEEP_ALIVE;

}

void job_runner_test_async_shutdown(){

    ESP_LOGI("job_runner_test","Job Runner Test.");

    jrerr_t err = JR_SUCCESS;

    struct job_runner* runner = NULL;

    err = job_runner_create(&runner, 1);
    
    if(err == JR_SUCCESS){

        err = job_runner_add_job(runner, job_runner_test_job1, 10000 / portTICK_PERIOD_MS);

    }

    if(err == JR_SUCCESS){

        err = job_runner_add_job(runner, job_runner_test_job2, 15000 / portTICK_PERIOD_MS);

    }

    if(err == JR_SUCCESS){

        err = job_runner_add_job(runner, job_runner_test_job3, 20000 / portTICK_PERIOD_MS);

    }

    if(err == JR_SUCCESS){

        err = job_runner_add_job(runner, job_runner_test_job4, 60000 / portTICK_PERIOD_MS);

    }

    if(err == JR_SUCCESS){

        err = job_runner_add_job(runner, job_runner_test_job5, 5000 / portTICK_PERIOD_MS);

    }

    if(err == JR_SUCCESS){

        ESP_LOGI("job_runner_test","All Jobs Added, Starting Runner...");

        err = job_runner_execute(runner, "test_run", 4096, 5);

    }

    if(err == JR_SUCCESS){

        ESP_LOGI("job_runner_test","Runner Successfully Started!!!");

    }

    vTaskDelay(120000 / portTICK_PERIOD_MS);
    ESP_LOGW("job_runner_test", "Sending shutdown signal...");

    job_runner_shutdown_response_handle_t hnd = NULL;
    err = job_runner_shutdown_async(runner, &hnd);
    if(err == JR_SUCCESS){
        ESP_LOGW("job_runner_test", "Waiting for shutdown confirmation...");
        err = job_runner_await_shutdown(hnd, 10000 / portTICK_PERIOD_MS);
        if(err != JR_SUCCESS){
            ESP_LOGE("job_runner_test", "Failed to await shutdown!!!");
        }
        else {
            ESP_LOGI("job_runner_test", "Job Runner Shutdown Success!!!");
        }
    }
    else {
        ESP_LOGE("job_runner_test", "Failed to send shutdown signal!!!");
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();
}
#endif //JOB_RUNNER_TEST_ASYNC_SHUTDOWN

#endif // JOB_RUNNER_TESTING_ENABLE
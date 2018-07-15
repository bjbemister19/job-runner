#ifndef __JOB_RUNNER_TESTS__
#define __JOB_RUNNER_TESTS__

#define JOB_RUNNER_TESTING_ENABLE
#define JOB_RUNNER_TEST_NOTIFICATIONS

#ifdef JOB_RUNNER_TESTING_ENABLE

#ifdef JOB_RUNNER_TEST_ASYNC_SHUTDOWN
void job_runner_test_async_shutdown();
#endif

#ifdef JOB_RUNNER_TEST_RAND_SHUTDOWN
void job_runner_test_rand_shutdown();
#endif

#ifdef JOB_RUNNER_TEST_NOTIFICATIONS
void job_runner_test_notifications();
#endif


#endif //JOB_RUNNER_TESTING_ENABLE

#endif //__JOB_RUNNER_TESTS__
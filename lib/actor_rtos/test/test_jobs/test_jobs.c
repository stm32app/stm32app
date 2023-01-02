#include <actor/actor.h>
#include <actor/job.h>
#include <actor/thread.h>
#include <unity.h>
#include <stdio.h>


static actor_signal_t async_job_example(actor_job_t *job, actor_signal_t signal, actor_t *caller) {
    return ACTOR_SIGNAL_OK;
}

static void test_async_job(void) {
    actor_t doer = {};
    actor_t caller = {};
    actor_job_t job = {
        .actor = &doer,
        .handler = async_job_example,
    };
    actor_job_execute(&job, ACTOR_SIGNAL_OK, &caller);
}



int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_async_job);
    return UNITY_END();
}
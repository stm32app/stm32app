#include <actor_job.h>
#include <actor_thread.h>
#include <unity.h>
#include <stdio.h>


 static void test_job_creation(void) {
}

 static void test_job_creation2(void) {
}


int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_job_creation);
    RUN_TEST(test_job_creation2);
    return UNITY_END();
}

#define ACTOR_ACTOR_GET_INPUT_THREAD(actor) actor_input

#include <actor/thread.h>
#include <stdio.h>
#include <unity.h>

struct actor {};

actor_thread_t *thread_with_listeners;
actor_thread_t *thread_without_listeners;

actor_t actor_root;
actor_t actor_listener;
actor_t actor_nonlistener;

static actor_signal_t test_listener_callback(void *object, actor_event_t *event, actor_worker_t *worker, actor_thread_t *thread) {
    return 0;
}

uint32_t actor_thread_iterate_workers(actor_thread_t *thread, actor_worker_t *destination) {
    if (thread != thread_without_listeners) {
        if (destination != NULL) {
            destination[0] = (actor_worker_t){.callback = test_listener_callback, .actor = &actor_listener, .thread = thread};
        }
        return 1;
    } else {
        return 0;
    }
}

void setUp(void) {
    actor_thread_create(&thread_with_listeners, &actor_root, (actor_procedure_t)actor_thread_execute, "LISTEND", 1024, 0, 10, 0);
}

void tearDown(void) {
    actor_thread_destroy(thread_with_listeners);
    actor_thread_destroy(thread_without_listeners);
}

static void test_thread_with_listeners(void) {
    TEST_ASSERT_NOT_NULL(thread_with_listeners);
    TEST_ASSERT_EQUAL_PTR(thread_with_listeners->actor, &actor_root);
}

static void test_thread_with_no_listeners(void) {
    actor_thread_create(&thread_without_listeners, &actor_root, (actor_procedure_t)actor_thread_execute, "NOLISTEN", 1024, 0, 10, 0);
    TEST_ASSERT_NULL(thread_without_listeners);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_thread_with_listeners);
    RUN_TEST(test_thread_with_no_listeners);
    return UNITY_END();
}
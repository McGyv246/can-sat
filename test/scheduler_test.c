// compilazione: gcc -I include -I mocks -I test/unity -Wall test/unity/unity.c test/scheduler_test.c mocks/hal_time.c src/service/scheduler.c -o build/scheduler_runner

#include <stdint.h>

#include "unity.h"
#include "types.h"
#include "scheduler.h"

task_t test_tasks_empty[] = {
    {
        NULL,
        NULL,
        NULL,
        IDLE,
        "task_1",
        30,
        0,
        1,
    },
    {
        NULL,
        NULL,
        NULL,
        IDLE,
        "task_2",
        50,
        0,
        3,
    },
};

uint8_t test_enabled_tasks[] = {0, 1};

void are_tasks_equal(task_t *expected_tasks, task_t *tasks, size_t n)
{
    TEST_ASSERT_EQUAL_PTR_ARRAY(expected_tasks, tasks, n);

    int i;
    for (i = 0; i < n; i++)
    {
        TEST_ASSERT_EQUAL_PTR(expected_tasks->run, tasks->run);
        TEST_ASSERT_EQUAL_PTR(expected_tasks->on_enable, tasks->on_enable);
        TEST_ASSERT_EQUAL_PTR(expected_tasks->on_disable, tasks->on_disable);
        // Missing last_status
        TEST_ASSERT_EQUAL_CHAR_ARRAY(expected_tasks->name, tasks->name, 6);
        TEST_ASSERT_EQUAL_UINT16(expected_tasks->period, tasks->period);
        TEST_ASSERT_EQUAL_UINT64(expected_tasks->due, tasks->due);
        TEST_ASSERT_EQUAL_UINT8(expected_tasks->priority, tasks->priority);
    }
}

void setUp() {}

void tearDown() {}

void when_initializingSchedulerEmpty_expect_correctValues()
{
    scheduler_t sched;

    sched_init(&sched, NULL, 0, NULL);

    TEST_ASSERT_EQUAL_PTR(NULL, sched.tasks);
    TEST_ASSERT_EQUAL_size_t(0, sched.tasks_number);
    TEST_ASSERT_EQUAL_PTR(NULL, sched.enabled_tasks);
    TEST_ASSERT_EQUAL_UINT16(0, sched.max_exec_time);
    TEST_ASSERT_EQUAL_UINT64(0, sched.run_count);
}

void when_initializingSchedulerTasks_expect_correctValues()
{
    scheduler_t sched;

    sched_init(&sched, test_tasks_empty, 2, test_enabled_tasks);

    TEST_ASSERT_EQUAL_PTR(test_tasks_empty, sched.tasks);
    TEST_ASSERT_EQUAL_size_t(2, sched.tasks_number);
    TEST_ASSERT_EQUAL_PTR(test_enabled_tasks, sched.enabled_tasks);
    TEST_ASSERT_EQUAL_UINT16(0, sched.max_exec_time);
    TEST_ASSERT_EQUAL_UINT64(0, sched.run_count);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(when_initializingSchedulerEmpty_expect_correctValues);
    RUN_TEST(when_initializingSchedulerTasks_expect_correctValues);
    return UNITY_END();
}
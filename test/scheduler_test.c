// compilazione: gcc -fsanitize=address -I include -I mocks -I test/unity -Wall test/unity/unity.c test/scheduler_test.c mocks/hal_time.c src/service/scheduler.c -o build/scheduler_runner

#include <stdint.h>
#include <stdbool.h>

#include "unity.h"
#include "types.h"
#include "scheduler.h"
#include "hal_time.h"

#pragma region setup

void assert_tasks_equal(task_t *expected_tasks, task_t *tasks, size_t n);
uint32_t run_task_1(task_t *self, uint32_t now_millis);
uint32_t run_task_2(task_t *self, uint32_t now_millis);
uint32_t run_task_slow(task_t *self, uint32_t now_millis);

void on_enable_task_1(task_t *self);
void on_disable_task_2(task_t *self);

void sleep(uint32_t delay);

int run_check, enabled_check;

const uint32_t MILLIS_TOLERANCE = 50;

const size_t TASKS_NUMBER = 2;

const uint16_t period_task_1 = 100;
const uint16_t period_task_2 = 1000;
const uint16_t period_task_slow = 1000;

task_t test_tasks_empty[] = {
    {
        NULL,
        NULL,
        NULL,
        SENS_NOT_READY,
        "task_1",
        period_task_1,
        0,
        1,
    },
    {
        NULL,
        NULL,
        NULL,
        SENS_NOT_READY,
        "task_2",
        period_task_2,
        0,
        3,
    },
};

// Task 1 higher priority
task_t test_tasks_1[] = {
    {
        &run_task_1,
        &on_enable_task_1,
        NULL,
        SENS_NOT_READY,
        "task_1",
        period_task_1,
        0,
        0,
    },
    {
        &run_task_2,
        NULL,
        &on_disable_task_2,
        SENS_NOT_READY,
        "task_2",
        period_task_2,
        0,
        10,
    },
};

// Task 2 higher priority
task_t test_tasks_2[] = {
    {
        &run_task_1,
        &on_enable_task_1,
        NULL,
        SENS_NOT_READY,
        "task_1",
        period_task_1,
        0,
        10,
    },
    {
        &run_task_2,
        NULL,
        &on_disable_task_2,
        SENS_NOT_READY,
        "task_2",
        period_task_2,
        0,
        0,
    },
};

task_t test_tasks_slow[] = {
    {
        &run_task_slow,
        NULL,
        NULL,
        SENS_NOT_READY,
        "task_slow",
        period_task_slow,
        0,
        0,
    },
    {
        &run_task_1,
        NULL,
        NULL,
        SENS_NOT_READY,
        "task_1",
        period_task_1,
        0,
        10,
    },
};

bool tasks_disabled_enabled[] = {false, true};
bool tasks_all_enabled[] = {true, true};
bool tasks_all_disabled[] = {false, false};

uint32_t run_task_1(task_t *self, uint32_t now_millis)
{
    printf("run_task_1\n");
    run_check += 10;
    return period_task_1;
}

uint32_t run_task_2(task_t *self, uint32_t now_millis)
{
    printf("run_task_2\n");
    run_check += 20;
    return period_task_2;
}

uint32_t run_task_slow(task_t *self, uint32_t now_millis)
{
    printf("begin_run_task_slow\n");
    sleep(1000);
    run_check -= 50;
    printf("finish_run_task_slow\n");
    return period_task_slow;
}

void on_enable_task_1(task_t *self)
{
    printf("on_enable_task_1\n");
    enabled_check += 50;
}

void on_disable_task_2(task_t *self)
{
    printf("on_disable_task_1\n");
    enabled_check += 20;
}

#pragma endregion

void assert_tasks_equal(task_t *expected_tasks, task_t *tasks, size_t n)
{
    TEST_ASSERT_EQUAL_PTR_ARRAY(expected_tasks, tasks, n);

    int i;
    for (i = 0; i < n; i++)
    {
        TEST_ASSERT_EQUAL_PTR(expected_tasks->run, tasks->run);
        TEST_ASSERT_EQUAL_PTR(expected_tasks->on_enable, tasks->on_enable);
        TEST_ASSERT_EQUAL_PTR(expected_tasks->on_disable, tasks->on_disable);
        TEST_ASSERT_EQUAL(expected_tasks->last_status, tasks->last_status);
        TEST_ASSERT_EQUAL_CHAR_ARRAY(expected_tasks->name, tasks->name, 6);
        TEST_ASSERT_EQUAL_UINT16(expected_tasks->period, tasks->period);
        TEST_ASSERT_EQUAL_UINT32(expected_tasks->due, tasks->due);
        TEST_ASSERT_EQUAL_UINT8(expected_tasks->priority, tasks->priority);
    }
}

void sleep(uint32_t delay)
{
    uint32_t delay_start = hal_millis();
    while (hal_millis() < delay_start + delay)
        ;
}

void setUp()
{
    run_check = 0;
    enabled_check = 0;

    for (size_t i = 0; i < TASKS_NUMBER; i++)
    {
        test_tasks_empty[i].due = 0;
        test_tasks_1[i].due = 0;
        test_tasks_2[i].due = 0;
        test_tasks_slow[i].due = 0;
    }
}

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

    sched_init(&sched, test_tasks_empty, TASKS_NUMBER, tasks_disabled_enabled);

    assert_tasks_equal(test_tasks_empty, sched.tasks, TASKS_NUMBER);
    TEST_ASSERT_EQUAL_size_t(TASKS_NUMBER, sched.tasks_number);
    TEST_ASSERT_EQUAL_PTR(tasks_disabled_enabled, sched.enabled_tasks);
    TEST_ASSERT_EQUAL_UINT16(0, sched.max_exec_time);
    TEST_ASSERT_EQUAL_UINT64(0, sched.run_count);
}

void when_executeOneTick_expect_correctTaskExecuted()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_1, TASKS_NUMBER, tasks_all_enabled);

    sched_tick(&sched);

    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(10, run_check);
}

void when_executeOneTickFirstTaskDisabled_expect_secondTaskToRun()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_1, TASKS_NUMBER, tasks_disabled_enabled);

    sched_tick(&sched);

    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(20, run_check);
}

void when_executeOneTick_expect_newDueTimeCorrect()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_1, TASKS_NUMBER, tasks_all_enabled);

    sched_tick(&sched);

    task_t *first_task = sched.tasks;
    uint32_t now = hal_millis();

    printf("Next due: %u\n", first_task->due);
    printf("Current time millis: %u\n", now);
    printf("Threshold: %u\n", MILLIS_TOLERANCE);
    TEST_ASSERT_GREATER_THAN_INT32(now + period_task_1 - MILLIS_TOLERANCE, first_task->due);
    TEST_ASSERT_LESS_THAN_INT32(now + period_task_1 + MILLIS_TOLERANCE, first_task->due);
}

void when_executeOneTickWithSecondTaskHigherPriority_expect_secondTaskExecuted()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_2, TASKS_NUMBER, tasks_all_enabled);

    sched_tick(&sched);

    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(20, run_check);
}

void when_executeTwoTicksWithSecondTaskHigherPriority_expect_secondAndFirstTaskExecuted()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_2, TASKS_NUMBER, tasks_all_enabled);

    sched_tick(&sched);

    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(20, run_check);

    sched_tick(&sched);

    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(30, run_check);
}

void when_executeTwoTasks_expect_runCountTwo()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_2, TASKS_NUMBER, tasks_all_enabled);

    sched_tick(&sched);
    sched_tick(&sched);

    printf("run_count: %u\n", sched.run_count);
    TEST_ASSERT_EQUAL_UINT32(2, sched.run_count);
}

void when_disablingTaskWithoutOnDisable_expect_notCrashAndTaskSkipped()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_1, TASKS_NUMBER, tasks_all_enabled);

    bool new_enabled[] = {false, true};
    sched_change_enabled_tasks(&sched, new_enabled);

    printf("enabled_check: %d\n", enabled_check);
    TEST_ASSERT_EQUAL(0, enabled_check);

    sched_tick(&sched);

    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(20, run_check);
}

void when_disablingTask_expect_onDisableExecuteAndTaskSkipped()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_2, TASKS_NUMBER, tasks_all_enabled);

    bool new_enabled[] = {true, false};
    sched_change_enabled_tasks(&sched, new_enabled);

    printf("enabled_check: %d\n", enabled_check);
    TEST_ASSERT_EQUAL(20, enabled_check);

    sched_tick(&sched);

    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(10, run_check);
}

void when_enablingTaskWithoutOnEnable_expect_notCrashAndTaskSkipped()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_2, TASKS_NUMBER, tasks_all_disabled);

    bool new_enabled[] = {false, true};
    sched_change_enabled_tasks(&sched, new_enabled);

    printf("enabled_check: %d\n", enabled_check);
    TEST_ASSERT_EQUAL(0, enabled_check);

    sched_tick(&sched);

    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(20, run_check);
}

void when_enablingTask_expect_onEnableExecuteAndTaskSkipped()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_2, TASKS_NUMBER, tasks_all_disabled);

    bool new_enabled[] = {true, false};
    sched_change_enabled_tasks(&sched, new_enabled);

    printf("enabled_check: %d\n", enabled_check);
    TEST_ASSERT_EQUAL(50, enabled_check);

    sched_tick(&sched);

    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(10, run_check);
}

void when_runningTickMultipleTimes_expect_taskExecutionOrderRespectsPeriod()
{
    scheduler_t sched;
    sched_init(&sched, test_tasks_2, TASKS_NUMBER, tasks_all_enabled);

    sched_tick(&sched);
    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(20, run_check);

    sched_tick(&sched);
    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(30, run_check);

    // too few time passed and no task is due
    sched_tick(&sched);
    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(30, run_check);

    sleep(1100);

    sched_tick(&sched);
    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(50, run_check);

    sleep(1100);

    sched_tick(&sched);
    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(70, run_check);

    sleep(1100);

    sched_tick(&sched);
    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(90, run_check);

    sched_tick(&sched);
    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(100, run_check);

    sleep(200);

    sched_tick(&sched);
    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(110, run_check);
}

void when_runningSlowAndFastTasks_expect_maxExecTimeToBeTheSlowOne()
{
    test_tasks_slow[0].due = hal_millis() + 300;
    test_tasks_slow[1].due = hal_millis() + 200;

    scheduler_t sched;
    sched_init(&sched, test_tasks_slow, TASKS_NUMBER, tasks_all_enabled);

    // Executes one task when ready (task_1 not task_slow)
    while (run_check == 0)
    {
        sched_tick(&sched);
    }

    printf("run_check: %d\n", run_check);
    TEST_ASSERT_EQUAL(10, run_check);

    printf("max_exec_time: %u\n", sched.max_exec_time);
    TEST_ASSERT_LESS_THAN_UINT16(500, sched.max_exec_time);

    // Continue executing tasks until task_slow gets executed
    int prev_run_check = run_check;
    while (prev_run_check <= run_check)
    {
        sched_tick(&sched);
    }

    printf("prev_run_check: %d\n", prev_run_check);
    printf("run_check: %d\n", run_check);
    printf("max_exec_time: %u\n", sched.max_exec_time);
    TEST_ASSERT_GREATER_THAN_UINT16(1000 - MILLIS_TOLERANCE, sched.max_exec_time);
    TEST_ASSERT_LESS_THAN_UINT16(1000 + MILLIS_TOLERANCE, sched.max_exec_time);
}

/*
TEST CASES:
- check max exec time and run count set correctly
(to check this I might need to initialize time in a certain way in the structs,
so that I don't get crazy timings. I could even create another run function that
takes a while to execute.
Don't forget to reset it in the setup function)
*/

int main()
{
    UNITY_BEGIN();
    RUN_TEST(when_initializingSchedulerEmpty_expect_correctValues);
    RUN_TEST(when_initializingSchedulerTasks_expect_correctValues);
    RUN_TEST(when_executeOneTick_expect_correctTaskExecuted);
    RUN_TEST(when_executeOneTickFirstTaskDisabled_expect_secondTaskToRun);
    RUN_TEST(when_executeOneTick_expect_newDueTimeCorrect);
    RUN_TEST(when_executeOneTickWithSecondTaskHigherPriority_expect_secondTaskExecuted);
    RUN_TEST(when_executeTwoTicksWithSecondTaskHigherPriority_expect_secondAndFirstTaskExecuted);
    RUN_TEST(when_executeTwoTasks_expect_runCountTwo);
    RUN_TEST(when_disablingTaskWithoutOnDisable_expect_notCrashAndTaskSkipped);
    RUN_TEST(when_disablingTask_expect_onDisableExecuteAndTaskSkipped);
    RUN_TEST(when_enablingTaskWithoutOnEnable_expect_notCrashAndTaskSkipped);
    RUN_TEST(when_enablingTask_expect_onEnableExecuteAndTaskSkipped);
    RUN_TEST(when_runningTickMultipleTimes_expect_taskExecutionOrderRespectsPeriod);
    RUN_TEST(when_runningSlowAndFastTasks_expect_maxExecTimeToBeTheSlowOne);
    return UNITY_END();
}
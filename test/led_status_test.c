// compile: gcc -fsanitize=address -I include -I mocks -I test/unity -Wall test/unity/unity.c test/led_status_test.c src/service/led_status.c -o build/led_status_runner

#include <stdint.h>
#include <stdbool.h>

#include "unity.h"
#include "config.h"
#include "types.h"
#include "led_status.h"

#pragma region helpers

// Inputs of a satellite on the ground with everything ok and the GPS off
static led_inputs_t ok_inputs(flight_state_t state)
{
    return (led_inputs_t){.state = state, .gps = LED_GPS_OFF};
}

static bool is_on(led_status_t status, uint32_t elapsed_ms)
{
    return led_pattern_is_on(&led_patterns[status], elapsed_ms);
}

#pragma endregion

void setUp(void) {}
void tearDown(void) {}

#pragma region choose

void when_noProblems_expect_statusOfEachFlightState(void)
{
    led_inputs_t in = ok_inputs(FLIGHT_STATE_BOOT);
    TEST_ASSERT_EQUAL(LED_STATUS_BOOT, led_status_choose(&in));

    in.state = FLIGHT_STATE_SELF_TEST;
    TEST_ASSERT_EQUAL(LED_STATUS_BOOT, led_status_choose(&in));

    in.state = FLIGHT_STATE_IDLE;
    TEST_ASSERT_EQUAL(LED_STATUS_READY, led_status_choose(&in));

    in.state = FLIGHT_STATE_ASCENT;
    TEST_ASSERT_EQUAL(LED_STATUS_OFF, led_status_choose(&in));

    in.state = FLIGHT_STATE_DESCENT;
    TEST_ASSERT_EQUAL(LED_STATUS_OFF, led_status_choose(&in));

    in.state = FLIGHT_STATE_LANDED;
    TEST_ASSERT_EQUAL(LED_STATUS_LANDED, led_status_choose(&in));
}

void when_manyProblemsOnTheGround_expect_highestPriorityShown(void)
{
    led_inputs_t in = ok_inputs(FLIGHT_STATE_IDLE);
    in.gps = LED_GPS_LOCKED;
    in.sensor_warning = true;
    TEST_ASSERT_EQUAL(LED_STATUS_SENSOR_WARNING, led_status_choose(&in));

    in.battery_low = true;
    TEST_ASSERT_EQUAL(LED_STATUS_BATTERY_LOW, led_status_choose(&in));

    in.error = true;
    TEST_ASSERT_EQUAL(LED_STATUS_ERROR, led_status_choose(&in));

    in.state = FLIGHT_STATE_BOOT;
    TEST_ASSERT_EQUAL(LED_STATUS_ERROR, led_status_choose(&in));
}

void when_problemsInFlightOrLanded_expect_problemsHidden(void)
{
    led_inputs_t in = {.error = true, .battery_low = true, .sensor_warning = true, .gps = LED_GPS_SEARCHING};

    in.state = FLIGHT_STATE_ASCENT;
    TEST_ASSERT_EQUAL(LED_STATUS_OFF, led_status_choose(&in));

    in.state = FLIGHT_STATE_DESCENT;
    TEST_ASSERT_EQUAL(LED_STATUS_OFF, led_status_choose(&in));

    in.state = FLIGHT_STATE_LANDED;
    TEST_ASSERT_EQUAL(LED_STATUS_LANDED, led_status_choose(&in));
}

void when_gpsOn_expect_gpsShownOnlyInIdle(void)
{
    led_inputs_t in = ok_inputs(FLIGHT_STATE_IDLE);
    in.gps = LED_GPS_SEARCHING;
    TEST_ASSERT_EQUAL(LED_STATUS_GPS_SEARCHING, led_status_choose(&in));

    in.gps = LED_GPS_LOCKED;
    TEST_ASSERT_EQUAL(LED_STATUS_GPS_LOCKED, led_status_choose(&in));

    in.state = FLIGHT_STATE_SELF_TEST;
    TEST_ASSERT_EQUAL(LED_STATUS_BOOT, led_status_choose(&in));
}

void when_invalidFlightState_expect_error(void)
{
    led_inputs_t in = ok_inputs(FLIGHT_STATE_COUNT);
    TEST_ASSERT_EQUAL(LED_STATUS_ERROR, led_status_choose(&in));
}

#pragma endregion

#pragma region patterns

void when_patternOff_expect_neverOn(void)
{
    for (uint32_t t = 0; t < 5000; t += 10)
        TEST_ASSERT_FALSE(is_on(LED_STATUS_OFF, t));
}

void when_patternBoot_expect_alwaysOn(void)
{
    for (uint32_t t = 0; t < 5000; t += 10)
        TEST_ASSERT_TRUE(is_on(LED_STATUS_BOOT, t));
}

void when_patternReady_expect_singleFlashEvery2s(void)
{
    TEST_ASSERT_TRUE(is_on(LED_STATUS_READY, 0));
    TEST_ASSERT_TRUE(is_on(LED_STATUS_READY, 99));
    TEST_ASSERT_FALSE(is_on(LED_STATUS_READY, 100));
    TEST_ASSERT_FALSE(is_on(LED_STATUS_READY, 1999));
    TEST_ASSERT_TRUE(is_on(LED_STATUS_READY, 2000));
}

void when_patternGpsLocked_expect_doubleFlashEvery2s(void)
{
    TEST_ASSERT_TRUE(is_on(LED_STATUS_GPS_LOCKED, 0));
    TEST_ASSERT_FALSE(is_on(LED_STATUS_GPS_LOCKED, 100));
    TEST_ASSERT_TRUE(is_on(LED_STATUS_GPS_LOCKED, 250));
    TEST_ASSERT_FALSE(is_on(LED_STATUS_GPS_LOCKED, 350));
    TEST_ASSERT_FALSE(is_on(LED_STATUS_GPS_LOCKED, 500));
    TEST_ASSERT_FALSE(is_on(LED_STATUS_GPS_LOCKED, 1999));
    TEST_ASSERT_TRUE(is_on(LED_STATUS_GPS_LOCKED, 2000));
}

void when_patternError_expect_fastBlink(void)
{
    TEST_ASSERT_TRUE(is_on(LED_STATUS_ERROR, 0));
    TEST_ASSERT_FALSE(is_on(LED_STATUS_ERROR, 100));
    TEST_ASSERT_TRUE(is_on(LED_STATUS_ERROR, 200));
    TEST_ASSERT_FALSE(is_on(LED_STATUS_ERROR, 300));
}

// The light sensors run in IDLE: the led must never stay on long enough to look like the ejection
void when_patternShownInIdle_expect_neverOnLongEnoughToFakeEjection(void)
{
    const led_status_t idle_statuses[] = {
        LED_STATUS_ERROR,
        LED_STATUS_BATTERY_LOW,
        LED_STATUS_SENSOR_WARNING,
        LED_STATUS_GPS_SEARCHING,
        LED_STATUS_GPS_LOCKED,
        LED_STATUS_READY,
    };

    for (size_t i = 0; i < sizeof(idle_statuses) / sizeof(idle_statuses[0]); i++)
    {
        uint32_t lit_ms = 0;
        for (uint32_t t = 0; t < 10000; t++)
        {
            lit_ms = is_on(idle_statuses[i], t) ? lit_ms + 1 : 0;
            TEST_ASSERT_LESS_THAN_UINT32(EJECTION_LIGHT_HOLD_MS / 2, lit_ms);
        }
    }
}

#pragma endregion

#pragma region inputs

void when_taskFailsMoreThanMax_expect_taskDead(void)
{
    task_t tasks[2] = {0};
    tasks[1].fail_count = SCHED_MAX_FAILS;
    TEST_ASSERT_FALSE(led_any_task_dead(tasks, 2));

    tasks[1].fail_count = SCHED_MAX_FAILS + 1;
    TEST_ASSERT_TRUE(led_any_task_dead(tasks, 2));
}

void when_taskFailed_expect_sensorWarningOnlyIfEnabled(void)
{
    task_t tasks[2] = {{.last_state = TASK_STATE_ERR}, {.last_state = TASK_STATE_BUSY}};
    bool enabled[2] = {false, true};
    TEST_ASSERT_FALSE(led_sensor_warning(tasks, enabled, 2));

    enabled[0] = true;
    TEST_ASSERT_TRUE(led_sensor_warning(tasks, enabled, 2));
}

void when_batteryAlertOrBelowThreshold_expect_batteryLow(void)
{
    battery_t battery = {.s.valid = false, .percentage = 1.0f};
    TEST_ASSERT_FALSE(led_battery_low(&battery));

    battery = (battery_t){.s.valid = true, .percentage = 80.0f};
    TEST_ASSERT_FALSE(led_battery_low(&battery));

    battery.is_alert_active = true;
    TEST_ASSERT_TRUE(led_battery_low(&battery));

    battery = (battery_t){.s.valid = true, .percentage = LED_BATTERY_LOW_PCT - 0.5f};
    TEST_ASSERT_TRUE(led_battery_low(&battery));
}

void when_gpsTaskAndWarm_expect_offSearchingOrLocked(void)
{
    TEST_ASSERT_EQUAL(LED_GPS_OFF, led_gps_status(false, false));
    TEST_ASSERT_EQUAL(LED_GPS_OFF, led_gps_status(false, true));
    TEST_ASSERT_EQUAL(LED_GPS_SEARCHING, led_gps_status(true, false));
    TEST_ASSERT_EQUAL(LED_GPS_LOCKED, led_gps_status(true, true));
}

#pragma endregion

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(when_noProblems_expect_statusOfEachFlightState);
    RUN_TEST(when_manyProblemsOnTheGround_expect_highestPriorityShown);
    RUN_TEST(when_problemsInFlightOrLanded_expect_problemsHidden);
    RUN_TEST(when_gpsOn_expect_gpsShownOnlyInIdle);
    RUN_TEST(when_invalidFlightState_expect_error);

    RUN_TEST(when_patternOff_expect_neverOn);
    RUN_TEST(when_patternBoot_expect_alwaysOn);
    RUN_TEST(when_patternReady_expect_singleFlashEvery2s);
    RUN_TEST(when_patternGpsLocked_expect_doubleFlashEvery2s);
    RUN_TEST(when_patternError_expect_fastBlink);
    RUN_TEST(when_patternShownInIdle_expect_neverOnLongEnoughToFakeEjection);

    RUN_TEST(when_taskFailsMoreThanMax_expect_taskDead);
    RUN_TEST(when_taskFailed_expect_sensorWarningOnlyIfEnabled);
    RUN_TEST(when_batteryAlertOrBelowThreshold_expect_batteryLow);
    RUN_TEST(when_gpsTaskAndWarm_expect_offSearchingOrLocked);

    return UNITY_END();
}

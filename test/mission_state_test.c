// compile: gcc -fsanitize=address -I include -I mocks -I test/unity -Wall test/unity/unity.c test/mission_state_test.c src/service/tasks/change_mission_state_task.c src/service/datastore.c -lm -o build/mission_state_runner

#include <stdint.h>
#include <stdbool.h>

#include "unity.h"
#include "config.h"
#include "types.h"
#include "tasks.h"
#include "datastore.h"

const uint16_t MISSION_STATE_PERIOD = 50;

task_t mission_task = {
    .name = "MISSION_STATE",
    .run = mission_state_task_run,
    .period = MISSION_STATE_PERIOD,
};

datastore_t ds;

#pragma region helpers

// Puts the datastore in the given state with no light, invalid sensors
static void set_state(flight_state_t state)
{
    ds = (datastore_t){.state = state};
    _ds_set_full(&ds);
}

static void set_light(bool fotores_light, bool uv_light)
{
    ds.state = ds_read_state();
    ds.fotores.s.valid = true;
    ds.fotores.is_there_light = fotores_light;
    ds.uv.s.valid = true;
    ds.uv.is_there_light = uv_light;
    _ds_set_full(&ds);
}

static void set_landing_data(float altitude_m, float accel_mag)
{
    ds.state = ds_read_state();
    ds.env.s.valid = true;
    ds.env.altitude_m = altitude_m;
    ds.imu.s.valid = true;
    ds.imu.accel_mag = accel_mag;
    _ds_set_full(&ds);
}

static void set_altitude(float altitude_m)
{
    ds.state = ds_read_state();
    ds.env.s.valid = true;
    ds.env.altitude_m = altitude_m;
    _ds_set_full(&ds);
}

// Runs the task every MISSION_STATE_PERIOD ms from start_ms to end_ms included
static void run_between(uint32_t start_ms, uint32_t end_ms)
{
    for (uint32_t t = start_ms; t <= end_ms; t += MISSION_STATE_PERIOD)
        mission_state_task_run(&mission_task, t);
}

// IDLE: keeps it dark until the light check is armed, light tests start at T_ARMED
#define T_ARMED (LIGHT_ARM_DARK_HOLD_MS + MISSION_STATE_PERIOD)
static void arm_light(void)
{
    set_light(false, false);
    run_between(0, LIGHT_ARM_DARK_HOLD_MS);
}

#pragma endregion

void setUp(void)
{
    set_state(FLIGHT_STATE_BOOT);
    mission_state_task_on_enable(&mission_task);
}

void tearDown(void) {}

#pragma region tests

void test_boot_goes_to_idle(void)
{
    mission_state_task_run(&mission_task, 0);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_SELF_TEST, ds_read_state());

    mission_state_task_run(&mission_task, 50);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());
}

void test_idle_without_light_stays_idle(void)
{
    set_state(FLIGHT_STATE_IDLE);
    set_light(false, false);

    run_between(0, 10000);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());
}

void test_idle_short_light_stays_idle(void)
{
    set_state(FLIGHT_STATE_IDLE);
    arm_light();
    set_light(true, false);

    run_between(T_ARMED, T_ARMED + EJECTION_LIGHT_HOLD_MS - MISSION_STATE_PERIOD);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());
}

void test_idle_held_light_goes_to_descent(void)
{
    set_state(FLIGHT_STATE_IDLE);
    arm_light();
    set_light(true, false);

    run_between(T_ARMED, T_ARMED + EJECTION_LIGHT_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_DESCENT, ds_read_state());
}

void test_uv_light_alone_is_enough(void)
{
    set_state(FLIGHT_STATE_IDLE);
    arm_light();
    set_light(false, true);

    run_between(T_ARMED, T_ARMED + EJECTION_LIGHT_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_DESCENT, ds_read_state());
}

void test_idle_light_before_arming_stays_idle(void)
{
    // Turned on with the rocket open: light from the start
    set_state(FLIGHT_STATE_IDLE);
    set_light(true, true);

    run_between(0, 30000);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());
}

void test_idle_short_dark_does_not_arm(void)
{
    set_state(FLIGHT_STATE_IDLE);

    // A shadow shorter than the arming window, then light again
    set_light(false, false);
    run_between(0, LIGHT_ARM_DARK_HOLD_MS - MISSION_STATE_PERIOD);

    set_light(true, false);
    run_between(LIGHT_ARM_DARK_HOLD_MS, LIGHT_ARM_DARK_HOLD_MS + 10000);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());
}

void test_idle_invalid_light_does_not_arm(void)
{
    set_state(FLIGHT_STATE_IDLE);
    set_light(false, false);
    ds.fotores.s.valid = false;
    ds.uv.s.valid = false;
    _ds_set_full(&ds);
    run_between(0, 2 * LIGHT_ARM_DARK_HOLD_MS);

    // Sensors come up in daylight: must not be armed
    set_light(true, true);
    run_between(2 * LIGHT_ARM_DARK_HOLD_MS + MISSION_STATE_PERIOD, 3 * LIGHT_ARM_DARK_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());
}

void test_full_flight_with_open_rocket_at_power_on(void)
{
    set_state(FLIGHT_STATE_IDLE);

    // Rocket open, then closed, then the ascent is missed: the light still triggers the descent
    set_light(true, true);
    run_between(0, 5000);
    set_light(false, false);
    run_between(5050, 5050 + LIGHT_ARM_DARK_HOLD_MS + 20000);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());

    set_light(true, false);
    run_between(40000, 40000 + EJECTION_LIGHT_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_DESCENT, ds_read_state());
}

void test_ascent_held_light_goes_to_descent(void)
{
    set_state(FLIGHT_STATE_ASCENT);
    set_light(true, true);

    run_between(0, EJECTION_LIGHT_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_DESCENT, ds_read_state());
}

void test_interrupted_light_restarts_window(void)
{
    set_state(FLIGHT_STATE_IDLE);
    arm_light();

    set_light(true, false);
    run_between(T_ARMED, T_ARMED + EJECTION_LIGHT_HOLD_MS - 500);

    set_light(false, false);
    mission_state_task_run(&mission_task, T_ARMED + EJECTION_LIGHT_HOLD_MS - 450);

    set_light(true, false);
    run_between(T_ARMED + EJECTION_LIGHT_HOLD_MS - 400, T_ARMED + EJECTION_LIGHT_HOLD_MS + 500);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());

    run_between(T_ARMED + EJECTION_LIGHT_HOLD_MS + 550, T_ARMED + 2 * EJECTION_LIGHT_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_DESCENT, ds_read_state());
}

void test_invalid_light_is_ignored(void)
{
    set_state(FLIGHT_STATE_IDLE);
    set_light(true, true);
    ds.fotores.s.valid = false;
    ds.uv.s.valid = false;
    _ds_set_full(&ds);

    run_between(0, 10000);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());
}

void test_idle_ground_altitude_stays_idle(void)
{
    set_state(FLIGHT_STATE_IDLE);

    // Baro noise around the ground altitude
    for (uint32_t t = 0; t <= 20000; t += MISSION_STATE_PERIOD)
    {
        set_altitude(100.0f + ((t / MISSION_STATE_PERIOD) % 2 ? 1.0f : -1.0f));
        mission_state_task_run(&mission_task, t);
    }

    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());
}

void test_idle_held_climb_goes_to_ascent(void)
{
    set_state(FLIGHT_STATE_IDLE);

    set_altitude(100.0f);
    run_between(0, 1000);

    set_altitude(100.0f + ASCENT_ALT_DELTA_M + 1.0f);
    run_between(1050, 1050 + ASCENT_HOLD_MS - MISSION_STATE_PERIOD);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());

    mission_state_task_run(&mission_task, 1050 + ASCENT_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_ASCENT, ds_read_state());
}

void test_idle_altitude_spike_stays_idle(void)
{
    set_state(FLIGHT_STATE_IDLE);

    set_altitude(100.0f);
    run_between(0, 1000);

    set_altitude(100.0f + ASCENT_ALT_DELTA_M + 1.0f);
    run_between(1050, 1050 + ASCENT_HOLD_MS - MISSION_STATE_PERIOD);

    set_altitude(100.0f);
    run_between(1050 + ASCENT_HOLD_MS, 10000);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());
}

void test_idle_ground_reference_is_lowest_altitude(void)
{
    set_state(FLIGHT_STATE_IDLE);

    // First sample is high, the reference must follow the ground once it's seen
    set_altitude(100.0f + ASCENT_ALT_DELTA_M + 1.0f);
    run_between(0, 500);
    set_altitude(100.0f);
    run_between(550, 1000);

    set_altitude(100.0f + ASCENT_ALT_DELTA_M + 1.0f);
    run_between(1050, 1050 + ASCENT_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_ASCENT, ds_read_state());
}

void test_idle_invalid_env_stays_idle(void)
{
    set_state(FLIGHT_STATE_IDLE);
    set_altitude(100.0f);
    run_between(0, 1000);

    set_altitude(500.0f);
    ds.env.s.valid = false;
    _ds_set_full(&ds);

    run_between(1050, 10000);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_IDLE, ds_read_state());
}

void test_ascent_without_light_stays_ascent(void)
{
    set_state(FLIGHT_STATE_ASCENT);
    set_light(false, false);
    set_altitude(100.0f);

    run_between(0, 10000);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_ASCENT, ds_read_state());
}

void test_full_flight_goes_through_ascent(void)
{
    set_state(FLIGHT_STATE_IDLE);
    set_light(false, false);

    set_altitude(100.0f);
    run_between(0, 1000);

    set_altitude(100.0f + ASCENT_ALT_DELTA_M + 1.0f);
    run_between(1050, 1050 + ASCENT_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_ASCENT, ds_read_state());

    set_altitude(800.0f);
    set_light(true, false);
    run_between(3000, 3000 + EJECTION_LIGHT_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_DESCENT, ds_read_state());
}

void test_descent_still_goes_to_landed(void)
{
    set_state(FLIGHT_STATE_DESCENT);
    set_landing_data(12.0f, 1.02f);

    run_between(0, LANDED_HOLD_MS - MISSION_STATE_PERIOD);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_DESCENT, ds_read_state());

    mission_state_task_run(&mission_task, LANDED_HOLD_MS);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_LANDED, ds_read_state());
}

void test_descent_falling_stays_descent(void)
{
    set_state(FLIGHT_STATE_DESCENT);

    // Falls at 5 m/s with ~1 g on the accelerometer (terminal velocity)
    for (uint32_t t = 0; t <= 20000; t += MISSION_STATE_PERIOD)
    {
        set_landing_data(500.0f - 5.0f * (t / 1000.0f), 1.0f);
        mission_state_task_run(&mission_task, t);
    }

    TEST_ASSERT_EQUAL(FLIGHT_STATE_DESCENT, ds_read_state());
}

void test_descent_shaking_stays_descent(void)
{
    set_state(FLIGHT_STATE_DESCENT);

    // Still altitude but accelerations far from 1 g
    for (uint32_t t = 0; t <= 20000; t += MISSION_STATE_PERIOD)
    {
        set_landing_data(12.0f, (t / MISSION_STATE_PERIOD) % 2 ? 1.5f : 0.5f);
        mission_state_task_run(&mission_task, t);
    }

    TEST_ASSERT_EQUAL(FLIGHT_STATE_DESCENT, ds_read_state());
}

void test_descent_invalid_env_stays_descent(void)
{
    set_state(FLIGHT_STATE_DESCENT);
    set_landing_data(12.0f, 1.0f);
    ds.env.s.valid = false;
    _ds_set_full(&ds);

    run_between(0, 20000);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_DESCENT, ds_read_state());
}

void test_landed_stays_landed(void)
{
    set_state(FLIGHT_STATE_LANDED);
    set_light(true, true);

    run_between(0, 10000);
    TEST_ASSERT_EQUAL(FLIGHT_STATE_LANDED, ds_read_state());
}

void test_run_returns_period(void)
{
    TEST_ASSERT_EQUAL_UINT32(MISSION_STATE_PERIOD, mission_state_task_run(&mission_task, 0));
    TEST_ASSERT_EQUAL(TASK_STATE_OK, mission_task.last_state);
}

#pragma endregion

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_boot_goes_to_idle);
    RUN_TEST(test_idle_without_light_stays_idle);
    RUN_TEST(test_idle_short_light_stays_idle);
    RUN_TEST(test_idle_held_light_goes_to_descent);
    RUN_TEST(test_uv_light_alone_is_enough);
    RUN_TEST(test_idle_light_before_arming_stays_idle);
    RUN_TEST(test_idle_short_dark_does_not_arm);
    RUN_TEST(test_idle_invalid_light_does_not_arm);
    RUN_TEST(test_full_flight_with_open_rocket_at_power_on);
    RUN_TEST(test_ascent_held_light_goes_to_descent);
    RUN_TEST(test_interrupted_light_restarts_window);
    RUN_TEST(test_invalid_light_is_ignored);
    RUN_TEST(test_idle_ground_altitude_stays_idle);
    RUN_TEST(test_idle_held_climb_goes_to_ascent);
    RUN_TEST(test_idle_altitude_spike_stays_idle);
    RUN_TEST(test_idle_ground_reference_is_lowest_altitude);
    RUN_TEST(test_idle_invalid_env_stays_idle);
    RUN_TEST(test_ascent_without_light_stays_ascent);
    RUN_TEST(test_full_flight_goes_through_ascent);
    RUN_TEST(test_descent_still_goes_to_landed);
    RUN_TEST(test_descent_falling_stays_descent);
    RUN_TEST(test_descent_shaking_stays_descent);
    RUN_TEST(test_descent_invalid_env_stays_descent);
    RUN_TEST(test_landed_stays_landed);
    RUN_TEST(test_run_returns_period);

    return UNITY_END();
}

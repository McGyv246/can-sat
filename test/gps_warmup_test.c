// compile: gcc -fsanitize=address -I include -I mocks -I test/unity -Wall test/unity/unity.c test/gps_warmup_test.c src/service/gps_warmup.c -o build/gps_warmup_runner

#include <stdint.h>
#include <stdbool.h>

#include "unity.h"
#include "config.h"
#include "types.h"
#include "gps_warmup.h"

#define T0 1000 /* first update in IDLE */

static gps_warmup_t w;

#pragma region helpers

// Brings w to SEARCHING (GPS just powered on) at T0 + GPS_POWER_ON_DELAY_MS, returns that time
static uint32_t reach_searching(void)
{
    gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, T0);
    uint32_t t = T0 + GPS_POWER_ON_DELAY_MS;
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, t));
    return t;
}

// Brings w to ASLEEP (just locked), returns the time of the lock
static uint32_t reach_asleep(void)
{
    uint32_t t = reach_searching();
    gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, t);
    t += GPS_LOCK_HOLD_MS;
    TEST_ASSERT_FALSE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, t));
    return t;
}

#pragma endregion

void setUp(void) { gps_warmup_reset(&w); }
void tearDown(void) {}

void when_notIdle_expect_alwaysOn(void)
{
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_BOOT, false, T0));
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_DESCENT, false, T0));
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_LANDED, true, T0));
}

void when_idleStarts_expect_offUntilPowerOnDelay(void)
{
    TEST_ASSERT_FALSE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, T0));
    TEST_ASSERT_FALSE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, T0 + GPS_POWER_ON_DELAY_MS - 1));
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, T0 + GPS_POWER_ON_DELAY_MS));
    TEST_ASSERT_EQUAL(GPS_WARMUP_SEARCHING, w.phase);
}

void when_noFix_expect_keepsSearching(void)
{
    uint32_t t = reach_searching();
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, t + 30u * 60u * 1000u));
    TEST_ASSERT_FALSE(gps_warmup_is_warm(&w, t + 30u * 60u * 1000u));
}

void when_fixHeldForLockHold_expect_offAndWarm(void)
{
    uint32_t t = reach_searching();
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, t));
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, t + GPS_LOCK_HOLD_MS - 1));
    TEST_ASSERT_FALSE(gps_warmup_is_warm(&w, t + GPS_LOCK_HOLD_MS - 1));

    TEST_ASSERT_FALSE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, t + GPS_LOCK_HOLD_MS));
    TEST_ASSERT_EQUAL(GPS_WARMUP_ASLEEP, w.phase);
    TEST_ASSERT_TRUE(gps_warmup_is_warm(&w, t + GPS_LOCK_HOLD_MS));
}

void when_fixLostDuringHold_expect_holdRestarts(void)
{
    uint32_t t = reach_searching();
    gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, t);
    gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, t + GPS_LOCK_HOLD_MS / 2);
    gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, t + GPS_LOCK_HOLD_MS / 2 + 1);

    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, t + GPS_LOCK_HOLD_MS));
    TEST_ASSERT_FALSE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, t + GPS_LOCK_HOLD_MS / 2 + 1 + GPS_LOCK_HOLD_MS));
}

void when_asleepForRefreshTime_expect_onAgainAndStillWarm(void)
{
    uint32_t t = reach_asleep();
    TEST_ASSERT_FALSE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, t + GPS_REFRESH_MS - 1));
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, t + GPS_REFRESH_MS));
    TEST_ASSERT_TRUE(gps_warmup_is_warm(&w, t + GPS_REFRESH_MS));

    // a stale valid fix from before the sleep doesn't count: the hold starts again
    uint32_t r = t + GPS_REFRESH_MS + 1000;
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, r));
    TEST_ASSERT_FALSE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, true, r + GPS_LOCK_HOLD_MS));
    TEST_ASSERT_EQUAL_UINT32(r + GPS_LOCK_HOLD_MS, w.last_lock_ms);
}

void when_refreshNeverLocks_expect_warmExpires(void)
{
    uint32_t t = reach_asleep();
    gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, t + GPS_REFRESH_MS);
    TEST_ASSERT_TRUE(gps_warmup_is_warm(&w, t + GPS_WARM_VALID_MS));
    TEST_ASSERT_FALSE(gps_warmup_is_warm(&w, t + GPS_WARM_VALID_MS + 1));
}

void when_idleEndsWhileAsleep_expect_onAgain(void)
{
    uint32_t t = reach_asleep();
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_ASCENT, false, t + 1000));
}

void when_millisWrapsAround_expect_delaysStillWork(void)
{
    uint32_t start = UINT32_MAX - GPS_POWER_ON_DELAY_MS / 2;
    TEST_ASSERT_FALSE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, start));
    TEST_ASSERT_FALSE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, start + GPS_POWER_ON_DELAY_MS - 1));
    TEST_ASSERT_TRUE(gps_warmup_update(&w, FLIGHT_STATE_IDLE, false, start + GPS_POWER_ON_DELAY_MS));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(when_notIdle_expect_alwaysOn);
    RUN_TEST(when_idleStarts_expect_offUntilPowerOnDelay);
    RUN_TEST(when_noFix_expect_keepsSearching);
    RUN_TEST(when_fixHeldForLockHold_expect_offAndWarm);
    RUN_TEST(when_fixLostDuringHold_expect_holdRestarts);
    RUN_TEST(when_asleepForRefreshTime_expect_onAgainAndStillWarm);
    RUN_TEST(when_refreshNeverLocks_expect_warmExpires);
    RUN_TEST(when_idleEndsWhileAsleep_expect_onAgain);
    RUN_TEST(when_millisWrapsAround_expect_delaysStillWork);

    return UNITY_END();
}

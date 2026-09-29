// compile: gcc -fsanitize=address -I include -I mocks -I test/unity -Wall test/unity/unity.c test/datastore_test.c mocks/hal_time.c src/service/datastore.c -o build/datastore_runner

#include "unity.h"
#include "types.h"
#include "datastore.h"

datastore_t ds_empty = {.state = FLIGHT_STATE_BOOT};
uint32_t mock_now_millis = 100;

#pragma region helper_functions

void print_ts(stamp_t *stamp, char *name)
{
    printf("%s.s.t_ms: %u\n", name, stamp->t_ms);
    printf("%s.s.valid: %d\n", name, stamp->valid);
}

void print_float_array(float *arr, size_t n, char *name)
{
    for (size_t i = 0; i < n; i++)
    {
        printf("%s[%lu]: %f\n", name, i, arr[i]);
    }
}

void print_ds(datastore_t *ds)
{
    printf("t_ms: %u\n", ds->t_ms);
    printf("flight_state: %d\n", ds->state);
    printf("alt_peak_env: %f\n", ds->alt_peak_env);
    printf("alt_peak_gps: %f\n", ds->alt_peak_gps);
    printf("records_dropped: %u\n", ds->records_dropped);

    printf("\n");

    printf("env.temp_c: %f\n", ds->env.temp_c);
    printf("env.pressure_pa: %f\n", ds->env.pressure_pa);
    printf("env.humidity_pct: %f\n", ds->env.humidity_pct);
    printf("env.altitude_m: %f\n", ds->env.altitude_m);
    print_ts(&(ds->env.s), "env");

    printf("\n");

    print_float_array(ds->imu.accel, 3, "imu.accel");
    print_float_array(ds->imu.gyro, 3, "imu.gyro");
    printf("imu.accel_mag: %f\n", ds->imu.accel_mag);
    print_ts(&(ds->imu.s), "imu");

    printf("\n");

    printf("gps.lat: %lf\n", ds->gps.lat);
    printf("gps.lon: %lf\n", ds->gps.lon);
    printf("gps.alt_m: %f\n", ds->gps.alt_m);
    printf("gps.speed_ms: %f\n", ds->gps.speed_ms);
    printf("gps.course_deg: %f\n", ds->gps.course_deg);
    printf("gps.hdop: %f\n", ds->gps.hdop);
    printf("gps.sats: %u\n", ds->gps.sats);
    printf("gps.fix_type: %u\n", ds->gps.fix_type);
    printf("gps.utc_s: %u\n", ds->gps.utc_s);
    print_ts(&(ds->gps.s), "gps");

    printf("\n");

    printf("uv.is_there_light: %d\n", ds->uv.is_there_light);
    printf("uv.uv_raw: %u\n", ds->uv.uv_raw);
    printf("uv.uv_index: %f\n", ds->uv.uv_index);
    print_ts(&(ds->uv.s), "uv");

    printf("\n");

    printf("fotores.is_there_light: %d\n", ds->fotores.is_there_light);
    print_ts(&(ds->fotores.s), "fotores");

    printf("\n");

    printf("gas.raw: %u\n", ds->gas.raw);
    printf("gas.heater_on_s: %u\n", ds->gas.heater_on_s);
    printf("gas.heater_enabled: %d\n", ds->gas.heater_enabled);
    print_ts(&(ds->gas.s), "gas");

    printf("\n");

    printf("battery.voltage_raw: %u\n", ds->battery.voltage_raw);
    printf("battery.voltage: %f\n", ds->battery.voltage);
    printf("battery.percentage: %f\n", ds->battery.percentage);
    printf("battery.is_alert_active: %u\n", ds->battery.is_alert_active);
    print_ts(&(ds->battery.s), "battery");

    printf("\n\n");
}

void assert_equal_stamp(stamp_t *expected, stamp_t *actual)
{
    TEST_ASSERT_EQUAL_UINT32(expected->t_ms, actual->t_ms);
    TEST_ASSERT_EQUAL(expected->valid, actual->valid);
}

void assert_equal_ds_env_no_ts(datastore_t *expected, datastore_t *actual)
{
    TEST_ASSERT_EQUAL_FLOAT(expected->env.temp_c, actual->env.temp_c);
    TEST_ASSERT_EQUAL_FLOAT(expected->env.pressure_pa, actual->env.pressure_pa);
    TEST_ASSERT_EQUAL_FLOAT(expected->env.humidity_pct, actual->env.humidity_pct);
    TEST_ASSERT_EQUAL_FLOAT(expected->env.altitude_m, actual->env.altitude_m);
}

void assert_equal_ds_imu_no_ts(datastore_t *expected, datastore_t *actual)
{
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected->imu.accel, actual->imu.accel, 3);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected->imu.gyro, actual->imu.gyro, 3);
    TEST_ASSERT_EQUAL_FLOAT(expected->imu.accel_mag, actual->imu.accel_mag);
}

void assert_equal_ds_gps_no_ts(datastore_t *expected, datastore_t *actual)
{
    // Type cast from double to float needed because testing library doesn't support doubles
    TEST_ASSERT_EQUAL_FLOAT((float)expected->gps.lat, (float)actual->gps.lat);
    TEST_ASSERT_EQUAL_FLOAT((float)expected->gps.lon, (float)actual->gps.lon);
    TEST_ASSERT_EQUAL_FLOAT(expected->gps.alt_m, actual->gps.alt_m);
    TEST_ASSERT_EQUAL_FLOAT(expected->gps.speed_ms, actual->gps.speed_ms);
    TEST_ASSERT_EQUAL_FLOAT(expected->gps.course_deg, actual->gps.course_deg);
    TEST_ASSERT_EQUAL_FLOAT(expected->gps.hdop, actual->gps.hdop);
    TEST_ASSERT_EQUAL_UINT8(expected->gps.sats, actual->gps.sats);
    TEST_ASSERT_EQUAL_UINT8(expected->gps.fix_type, actual->gps.fix_type);
    TEST_ASSERT_EQUAL_UINT32(expected->gps.utc_s, actual->gps.utc_s);
}

void assert_equal_ds_uv_no_ts(datastore_t *expected, datastore_t *actual)
{
    TEST_ASSERT_EQUAL(expected->uv.is_there_light, actual->uv.is_there_light);
    TEST_ASSERT_EQUAL_UINT16(expected->uv.uv_raw, actual->uv.uv_raw);
    TEST_ASSERT_EQUAL_FLOAT(expected->uv.uv_index, actual->uv.uv_index);
}

void assert_equal_ds_fotores_no_ts(datastore_t *expected, datastore_t *actual)
{
    TEST_ASSERT_EQUAL(expected->fotores.is_there_light, actual->fotores.is_there_light);
}

void assert_equal_ds_gas_no_ts(datastore_t *expected, datastore_t *actual)
{
    TEST_ASSERT_EQUAL_UINT16(expected->gas.raw, actual->gas.raw);
    TEST_ASSERT_EQUAL_UINT16(expected->gas.heater_on_s, actual->gas.heater_on_s);
    TEST_ASSERT_EQUAL(expected->gas.heater_enabled, actual->gas.heater_enabled);
}

void assert_equal_ds_battery_no_ts(datastore_t *expected, datastore_t *actual)
{
    TEST_ASSERT_EQUAL_UINT16(expected->battery.voltage_raw, actual->battery.voltage_raw);
    TEST_ASSERT_EQUAL_FLOAT(expected->battery.voltage, actual->battery.voltage);
    TEST_ASSERT_EQUAL_FLOAT(expected->battery.percentage, actual->battery.percentage);
    TEST_ASSERT_EQUAL_UINT8(expected->battery.is_alert_active, actual->battery.is_alert_active);
}

void assert_equal_ds_stats(datastore_t *expected, datastore_t *actual)
{
    TEST_ASSERT_EQUAL_UINT32(expected->t_ms, actual->t_ms);
    TEST_ASSERT_EQUAL(expected->state, actual->state); /* enum -> int-based compare */
    TEST_ASSERT_EQUAL_FLOAT(expected->alt_peak_env, actual->alt_peak_env);
    TEST_ASSERT_EQUAL_FLOAT(expected->alt_peak_gps, actual->alt_peak_gps);
    TEST_ASSERT_EQUAL_UINT16(expected->records_dropped, actual->records_dropped);
}

void assert_equal_ds(datastore_t *expected, datastore_t *actual)
{
    assert_equal_ds_stats(expected, actual);

    /* env */
    assert_equal_ds_env_no_ts(expected, actual);
    assert_equal_stamp(&expected->env.s, &actual->env.s);

    /* imu */
    assert_equal_ds_imu_no_ts(expected, actual);
    assert_equal_stamp(&expected->imu.s, &actual->imu.s);

    /* gps */
    assert_equal_ds_gps_no_ts(expected, actual);
    assert_equal_stamp(&expected->gps.s, &actual->gps.s);

    /* uv */
    assert_equal_ds_uv_no_ts(expected, actual);
    assert_equal_stamp(&expected->uv.s, &actual->uv.s);

    /* fotores */
    assert_equal_ds_fotores_no_ts(expected, actual);
    assert_equal_stamp(&expected->fotores.s, &actual->fotores.s);

    /* gas */
    assert_equal_ds_gas_no_ts(expected, actual);
    assert_equal_stamp(&expected->gas.s, &actual->gas.s);

    /* battery */
    assert_equal_ds_battery_no_ts(expected, actual);
    assert_equal_stamp(&expected->battery.s, &actual->battery.s);
}

#pragma endregion

void setUp() {}

void tearDown()
{
    _ds_set_full(&ds_empty);
}

void when_startingProgram_expect_datastoreInitializedToBootAndZero()
{
    datastore_t ds_copy;

    ds_read_all(&ds_copy);

    print_ds(&ds_copy);

    datastore_t ds_expected = {.state = FLIGHT_STATE_BOOT};
    assert_equal_ds(&ds_expected, &ds_copy);
}

void when_gettingValuesAndEditingEnv_expect_copiedValuesNotToChange()
{
    datastore_t ds_copy;

    ds_read_all(&ds_copy);

    printf("BEFORE WRITE\n");
    print_ds(&ds_copy);

    env_t env = {
        {
            mock_now_millis,
            false,
        },
        100.,
        18000.,
        30.,
        200.,
    };

    ds_write_env(&env, mock_now_millis);

    printf("AFTER WRITE\n");
    print_ds(&ds_copy);

    assert_equal_ds(&ds_empty, &ds_copy);
}

void when_gettingValuesAndEditingImu_expect_copiedValuesNotToChange()
{
    datastore_t ds_copy;

    ds_read_all(&ds_copy);

    printf("BEFORE WRITE\n");
    print_ds(&ds_copy);

    imu_t imu = {
        {
            mock_now_millis,
            false,
        },
        {
            10.,
            10.,
            10.,
        },
        {
            20.,
            20.,
            20.,
        },
        100.,
    };

    ds_write_imu(&imu, mock_now_millis);

    printf("AFTER WRITE\n");
    print_ds(&ds_copy);

    assert_equal_ds(&ds_empty, &ds_copy);
}

void when_editingImuAndGettingValues_expect_copiedValuesUpdatedCorrectly()
{
    datastore_t ds_copy;

    ds_read_all(&ds_copy);

    printf("BEFORE WRITE\n");
    print_ds(&ds_copy);

    imu_t imu = {
        {
            mock_now_millis,
            false,
        },
        {
            10.,
            10.,
            10.,
        },
        {
            20.,
            20.,
            20.,
        },
        100.,
    };

    ds_write_imu(&imu, mock_now_millis);

    ds_read_all(&ds_copy);

    printf("AFTER WRITE\n");
    print_ds(&ds_copy);

    datastore_t expected = ds_empty;

    expected.imu = imu;
    expected.t_ms = mock_now_millis;

    printf("EXPECTED\n");
    print_ds(&expected);

    assert_equal_ds(&expected, &ds_copy);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(when_startingProgram_expect_datastoreInitializedToBootAndZero);
    RUN_TEST(when_gettingValuesAndEditingEnv_expect_copiedValuesNotToChange);
    RUN_TEST(when_gettingValuesAndEditingImu_expect_copiedValuesNotToChange);
    RUN_TEST(when_editingImuAndGettingValues_expect_copiedValuesUpdatedCorrectly);
    return UNITY_END();
}
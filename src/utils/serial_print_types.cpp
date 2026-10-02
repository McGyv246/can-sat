#include "debug_print.h"

// Compiled only when debug prints are enabled, so flight builds don't carry this code
#ifdef SERIAL_DEBUG_ANY

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

#include "serial_print_types.h"

#define FLOAT_DIGITS 3
#define LOG_BUF_SIZE 160
#define LINE_BUF_SIZE 128
#define LAT_LON_DIGITS 6

/*
NOTE: fields of packed structs are always read by value (never through a pointer to the
field itself) because the RP2040 (Cortex-M0+) faults on unaligned multi-byte accesses.
*/

#pragma region line_buffer

// Writes "[<tag> <millis>ms c<core>] " in buf and returns its length
static size_t line_prefix(char *buf, size_t size, const char *tag)
{
    int n = snprintf(buf, size, "[%s %lums c%u] ", tag, (unsigned long)millis(), (unsigned)rp2040.cpuid());
    return (n < 0) ? 0 : ((size_t)n < size ? (size_t)n : size - 1);
}

/*
Both cores print on the same Serial. A single Serial.write call is never mixed with the
other core's output (the USB serial is locked internally), but a line made of many print
calls can be. The struct dumps are then collected here one line at a time and sent with a
single Serial.write: lines of the two cores can alternate, but each line stays whole.
*/
class LinePrint : public Print
{
public:
    size_t write(uint8_t c) override
    {
        if (len == 0)
            len = line_prefix(buf, sizeof(buf), "DAT");

        buf[len++] = (char)c;

        // Lines longer than the buffer are split, not lost
        if (c == '\n' || len == sizeof(buf))
        {
            Serial.write((const uint8_t *)buf, len);
            len = 0;
        }
        return 1;
    }

private:
    char buf[LINE_BUF_SIZE];
    size_t len = 0;
};

// One buffer per core, so the cores never write in the same one
static LinePrint lines[2];

static LinePrint &out()
{
    return lines[rp2040.cpuid()];
}

// Builds "[<tag> <millis>ms c<core>] <message>\r\n" and sends it with a single write
static void log_line(const char *tag, const char *fmt, va_list args)
{
    char buf[LOG_BUF_SIZE];
    size_t len = line_prefix(buf, sizeof(buf) - 2, tag);

    int n = vsnprintf(buf + len, sizeof(buf) - 2 - len, fmt, args); // longer messages are truncated
    if (n > 0)
        len += ((size_t)n < sizeof(buf) - 2 - len) ? (size_t)n : sizeof(buf) - 3 - len;

    buf[len++] = '\r';
    buf[len++] = '\n';
    Serial.write((const uint8_t *)buf, len);
}

#pragma endregion

#pragma region helpers

static void indent(uint8_t ind)
{
    for (uint8_t i = 0; i < ind; i++)
        out().print("  ");
}

static void key(uint8_t ind, const char *k)
{
    indent(ind);
    out().print(k);
    out().print(": ");
}

/* Prints the title of a (sub)struct. Returns false if the pointer is NULL. */
static bool section(uint8_t ind, const char *name, const void *p)
{
    indent(ind);
    out().print(name);
    if (p == NULL)
    {
        out().println(": (null)");
        return false;
    }
    out().println(":");
    return true;
}

static void p_u(uint8_t ind, const char *k, uint32_t v)
{
    key(ind, k);
    out().println((unsigned long)v);
}

static void p_i(uint8_t ind, const char *k, int32_t v)
{
    key(ind, k);
    out().println((long)v);
}

static void p_f(uint8_t ind, const char *k, double v, int digits = FLOAT_DIGITS)
{
    key(ind, k);
    out().println(v, digits);
}

static void p_b(uint8_t ind, const char *k, bool v)
{
    key(ind, k);
    out().println(v ? "true" : "false");
}

static void p_hex(uint8_t ind, const char *k, uint32_t v)
{
    key(ind, k);
    out().print("0x");
    out().println((unsigned long)v, HEX);
}

static void p_str(uint8_t ind, const char *k, const char *s)
{
    key(ind, k);
    out().println(s != NULL ? s : "(null)");
}

static void p_f3(uint8_t ind, const char *k, float x, float y, float z)
{
    key(ind, k);
    out().print("[");
    out().print(x, FLOAT_DIGITS);
    out().print(", ");
    out().print(y, FLOAT_DIGITS);
    out().print(", ");
    out().print(z, FLOAT_DIGITS);
    out().println("]");
}

static void p_i3(uint8_t ind, const char *k, int16_t x, int16_t y, int16_t z)
{
    key(ind, k);
    out().print("[");
    out().print((int)x);
    out().print(", ");
    out().print((int)y);
    out().print(", ");
    out().print((int)z);
    out().println("]");
}

/* Prints "NAME (value)", or "UNKNOWN (value)" if the value is out of range */
static void p_enum(uint8_t ind, const char *k, const char *const *names, size_t count, int32_t v)
{
    key(ind, k);
    out().print((v >= 0 && (size_t)v < count) ? names[v] : "UNKNOWN");
    out().print(" (");
    out().print((long)v);
    out().println(")");
}

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static const char *const flight_state_names[] = {
    "FLIGHT_STATE_BOOT",
    "FLIGHT_STATE_SELF_TEST",
    "FLIGHT_STATE_IDLE",
    "FLIGHT_STATE_ASCENT",
    "FLIGHT_STATE_DESCENT",
    "FLIGHT_STATE_LANDED",
};
static_assert(ARRAY_LEN(flight_state_names) == FLIGHT_STATE_COUNT, "flight_state_names out of sync with flight_state_t");

static const char *const data_type_names[] = {
    "DATA_TYPE_ENV",
    "DATA_TYPE_IMU",
    "DATA_TYPE_GPS",
    "DATA_TYPE_UV",
    "DATA_TYPE_FOTORES",
    "DATA_TYPE_GAS",
    "DATA_TYPE_BATTERY",
    "DATA_TYPE_STATS",
};

static const char *const sens_state_names[] = {
    "SENS_STATE_IDLE",
    "SENS_STATE_NOT_READY",
    "SENS_STATE_READY",
    "SENS_STATE_ERR",
};

static const char *const task_state_names[] = {
    "TASK_STATE_OK",
    "TASK_STATE_BUSY",
    "TASK_STATE_ERR",
};

static void p_flight_state(uint8_t ind, const char *k, int32_t v)
{
    p_enum(ind, k, flight_state_names, ARRAY_LEN(flight_state_names), v);
}

static void p_data_type(uint8_t ind, const char *k, int32_t v)
{
    p_enum(ind, k, data_type_names, ARRAY_LEN(data_type_names), v);
}

static void p_task_state(uint8_t ind, const char *k, int32_t v)
{
    p_enum(ind, k, task_state_names, ARRAY_LEN(task_state_names), v);
}

#pragma endregion

#pragma region main_datastore_impl

static void stamp_(uint8_t ind, const char *name, const stamp_t *x)
{
    if (!section(ind, name, x))
        return;
    p_u(ind + 1, "t_ms", x->t_ms);
    p_b(ind + 1, "valid", x->valid);
}

static void env_(uint8_t ind, const char *name, const env_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_(ind + 1, "s", &x->s);
    p_f(ind + 1, "temp_c", x->temp_c);
    p_f(ind + 1, "pressure_pa", x->pressure_pa);
    p_f(ind + 1, "humidity_pct", x->humidity_pct);
    p_f(ind + 1, "altitude_m", x->altitude_m);
}

static void imu_(uint8_t ind, const char *name, const imu_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_(ind + 1, "s", &x->s);
    p_f3(ind + 1, "accel", x->accel[0], x->accel[1], x->accel[2]);
    p_f3(ind + 1, "gyro", x->gyro[0], x->gyro[1], x->gyro[2]);
    p_f(ind + 1, "accel_mag", x->accel_mag);
}

static void gps_(uint8_t ind, const char *name, const gps_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_(ind + 1, "s", &x->s);
    p_f(ind + 1, "lat", x->lat, LAT_LON_DIGITS);
    p_f(ind + 1, "lon", x->lon, LAT_LON_DIGITS);
    p_f(ind + 1, "alt_m", x->alt_m);
    p_f(ind + 1, "speed_ms", x->speed_ms);
    p_f(ind + 1, "course_deg", x->course_deg);
    p_f(ind + 1, "hdop", x->hdop);
    p_u(ind + 1, "sats", x->sats);
    p_u(ind + 1, "fix_type", x->fix_type);
    p_u(ind + 1, "utc_s", x->utc_s);
}

static void uv_(uint8_t ind, const char *name, const uv_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_(ind + 1, "s", &x->s);
    p_b(ind + 1, "is_there_light", x->is_there_light);
    p_u(ind + 1, "uv_raw", x->uv_raw);
    p_f(ind + 1, "uv_index", x->uv_index);
}

static void fotores_(uint8_t ind, const char *name, const fotores_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_(ind + 1, "s", &x->s);
    p_b(ind + 1, "is_there_light", x->is_there_light);
}

static void gas_(uint8_t ind, const char *name, const gas_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_(ind + 1, "s", &x->s);
    p_u(ind + 1, "raw", x->raw);
    p_u(ind + 1, "heater_on_s", x->heater_on_s);
    p_b(ind + 1, "heater_enabled", x->heater_enabled);
}

static void battery_(uint8_t ind, const char *name, const battery_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_(ind + 1, "s", &x->s);
    p_u(ind + 1, "voltage_raw", x->voltage_raw);
    p_f(ind + 1, "voltage", x->voltage);
    p_f(ind + 1, "percentage", x->percentage);
    p_b(ind + 1, "is_alert_active", x->is_alert_active);
}

static void datastore_(uint8_t ind, const char *name, const datastore_t *x)
{
    if (!section(ind, name, x))
        return;
    p_u(ind + 1, "t_ms", x->t_ms);
    p_flight_state(ind + 1, "state", x->state);
    p_f(ind + 1, "alt_peak_env", x->alt_peak_env);
    p_f(ind + 1, "alt_peak_gps", x->alt_peak_gps);
    p_u(ind + 1, "records_dropped", x->records_dropped);
    env_(ind + 1, "env", &x->env);
    imu_(ind + 1, "imu", &x->imu);
    gps_(ind + 1, "gps", &x->gps);
    uv_(ind + 1, "uv", &x->uv);
    fotores_(ind + 1, "fotores", &x->fotores);
    gas_(ind + 1, "gas", &x->gas);
    battery_(ind + 1, "battery", &x->battery);
}

#pragma endregion

#pragma region lora_structs_impl

static void data_to_lora_(uint8_t ind, const char *name, const data_to_lora_t *x)
{
    if (!section(ind, name, x))
        return;
    p_hex(ind + 1, "sync", x->sync);
    p_u(ind + 1, "version", x->version);
    p_u(ind + 1, "seq", x->seq);
    p_u(ind + 1, "ms_since_boot", x->ms_since_boot);
    p_flight_state(ind + 1, "flight_state", x->flight_state);
    p_u(ind + 1, "records_dropped", x->records_dropped);
    p_i(ind + 1, "temp_c", x->temp_c);
    p_u(ind + 1, "pressure", x->pressure);
    p_u(ind + 1, "humidity_pct", x->humidity_pct);
    p_i(ind + 1, "altitude_baro_m", x->altitude_baro_m);
    p_i3(ind + 1, "accel", x->accel[0], x->accel[1], x->accel[2]);
    p_i3(ind + 1, "gyro", x->gyro[0], x->gyro[1], x->gyro[2]);
    p_i(ind + 1, "accel_mag", x->accel_mag);
    p_u(ind + 1, "is_there_light_uv", x->is_there_light_uv);
    p_u(ind + 1, "is_there_light_fotores", x->is_there_light_fotores);
    p_u(ind + 1, "uv", x->uv);
    p_u(ind + 1, "gas_raw", x->gas_raw);
    p_u(ind + 1, "heater_on_since", x->heater_on_since);
    p_i(ind + 1, "lat", x->lat);
    p_i(ind + 1, "lon", x->lon);
    p_i(ind + 1, "altitude_gps_m", x->altitude_gps_m);
    p_u(ind + 1, "satfix", x->satfix);
    p_u(ind + 1, "vbatt", x->vbatt);
    p_hex(ind + 1, "status", x->status);
    p_hex(ind + 1, "crc16", x->crc16);
}

#pragma endregion

#pragma region sd_structs_impl

static void sd_header_(uint8_t ind, const char *name, const sd_header_t *x)
{
    if (!section(ind, name, x))
        return;
    p_hex(ind + 1, "sync", x->sync);
    p_data_type(ind + 1, "type", x->type);
    p_u(ind + 1, "len", x->len);
    p_u(ind + 1, "t_ms", x->t_ms);
}

static void stamp_packed_(uint8_t ind, const char *name, const stamp_packed_t *x)
{
    if (!section(ind, name, x))
        return;
    p_u(ind + 1, "t_ms", x->t_ms);
    p_u(ind + 1, "valid", x->valid);
}

static void env_packed_(uint8_t ind, const char *name, const env_packed_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_packed_(ind + 1, "s", &x->s);
    p_f(ind + 1, "temp_c", x->temp_c);
    p_f(ind + 1, "pressure_pa", x->pressure_pa);
    p_f(ind + 1, "humidity_pct", x->humidity_pct);
    p_f(ind + 1, "altitude_m", x->altitude_m);
}

static void imu_packed_(uint8_t ind, const char *name, const imu_packed_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_packed_(ind + 1, "s", &x->s);
    p_f3(ind + 1, "accel", x->accel[0], x->accel[1], x->accel[2]);
    p_f3(ind + 1, "gyro", x->gyro[0], x->gyro[1], x->gyro[2]);
    p_f(ind + 1, "accel_mag", x->accel_mag);
}

static void gps_packed_(uint8_t ind, const char *name, const gps_packed_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_packed_(ind + 1, "s", &x->s);
    p_f(ind + 1, "lat", x->lat, LAT_LON_DIGITS);
    p_f(ind + 1, "lon", x->lon, LAT_LON_DIGITS);
    p_f(ind + 1, "alt_m", x->alt_m);
    p_f(ind + 1, "speed_ms", x->speed_ms);
    p_f(ind + 1, "course_deg", x->course_deg);
    p_f(ind + 1, "hdop", x->hdop);
    p_u(ind + 1, "sats", x->sats);
    p_u(ind + 1, "fix_type", x->fix_type);
    p_u(ind + 1, "utc_s", x->utc_s);
}

static void uv_packed_(uint8_t ind, const char *name, const uv_packed_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_packed_(ind + 1, "s", &x->s);
    p_u(ind + 1, "is_there_light", x->is_there_light);
    p_u(ind + 1, "uv_raw", x->uv_raw);
    p_f(ind + 1, "uv_index", x->uv_index);
}

static void fotores_packed_(uint8_t ind, const char *name, const fotores_packed_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_packed_(ind + 1, "s", &x->s);
    p_u(ind + 1, "is_there_light", x->is_there_light);
}

static void gas_packed_(uint8_t ind, const char *name, const gas_packed_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_packed_(ind + 1, "s", &x->s);
    p_u(ind + 1, "raw", x->raw);
    p_u(ind + 1, "heater_on_s", x->heater_on_s);
    p_u(ind + 1, "heater_enabled", x->heater_enabled);
}

static void battery_packed_(uint8_t ind, const char *name, const battery_packed_t *x)
{
    if (!section(ind, name, x))
        return;
    stamp_packed_(ind + 1, "s", &x->s);
    p_u(ind + 1, "voltage_raw", x->voltage_raw);
    p_f(ind + 1, "voltage", x->voltage);
    p_f(ind + 1, "percentage", x->percentage);
    p_u(ind + 1, "is_alert_active", x->is_alert_active);
}

static void stats_packed_(uint8_t ind, const char *name, const stats_packed_t *x)
{
    if (!section(ind, name, x))
        return;
    p_u(ind + 1, "t_ms", x->t_ms);
    p_flight_state(ind + 1, "state", x->state);
    p_f(ind + 1, "alt_peak_env", x->alt_peak_env);
    p_f(ind + 1, "alt_peak_gps", x->alt_peak_gps);
    p_u(ind + 1, "records_dropped", x->records_dropped);
}

/* All the *_sd_t structs share the same layout: header, data, crc */
#define DEFINE_SD_PRINTER(fn, type_name, sd_t, data_printer) \
    void fn(const sd_t *x)                                   \
    {                                                        \
        if (!section(0, type_name, x))                       \
            return;                                          \
        sd_header_(1, "header", &x->header);                 \
        data_printer(1, "data", &x->data);                   \
        p_hex(1, "crc", x->crc);                             \
    }

#pragma endregion

#pragma region scheduler_impl

static void task_(uint8_t ind, const char *name, const task_t *x)
{
    if (!section(ind, name, x))
        return;
    p_str(ind + 1, "name", x->name);
    p_hex(ind + 1, "run", (uint32_t)(uintptr_t)x->run);
    p_hex(ind + 1, "on_enable", (uint32_t)(uintptr_t)x->on_enable);
    p_hex(ind + 1, "on_disable", (uint32_t)(uintptr_t)x->on_disable);
    p_task_state(ind + 1, "last_state", x->last_state);
    p_u(ind + 1, "period", x->period);
    p_u(ind + 1, "due", x->due);
    p_u(ind + 1, "priority", x->priority);
    p_u(ind + 1, "fail_count", x->fail_count);
}

static void scheduler_(uint8_t ind, const char *name, const scheduler_t *x)
{
    if (!section(ind, name, x))
        return;
    p_u(ind + 1, "tasks_number", (uint32_t)x->tasks_number);
    p_u(ind + 1, "max_exec_time", x->max_exec_time);
    p_u(ind + 1, "run_count", x->run_count);

    if (x->tasks == NULL)
    {
        section(ind + 1, "tasks", NULL);
        return;
    }

    for (size_t i = 0; i < x->tasks_number; i++)
    {
        indent(ind + 1);
        out().print("tasks[");
        out().print((unsigned long)i);
        out().println("]:");
        if (x->enabled_tasks != NULL)
            p_b(ind + 2, "enabled", x->enabled_tasks[i]);
        task_(ind + 2, "task", &x->tasks[i]);
    }
}

#pragma endregion

/* ---------------------------------- Public API ---------------------------------- */

extern "C"
{

    void debug_log_error(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log_line("ERR", fmt, args);
        va_end(args);
    }

    void debug_log_info(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        log_line("INF", fmt, args);
        va_end(args);
    }

    const char *debug_flight_state_name(flight_state_t x)
    {
        // Skips the "FLIGHT_STATE_" prefix
        static const size_t prefix_len = sizeof("FLIGHT_STATE_") - 1;
        return ((size_t)x < FLIGHT_STATE_COUNT) ? flight_state_names[x] + prefix_len : "UNKNOWN";
    }

#pragma region main_datastore

    void print_stamp(const stamp_t *x) { stamp_(0, "stamp_t", x); }
    void print_env(const env_t *x) { env_(0, "env_t", x); }
    void print_imu(const imu_t *x) { imu_(0, "imu_t", x); }
    void print_gps(const gps_t *x) { gps_(0, "gps_t", x); }
    void print_uv(const uv_t *x) { uv_(0, "uv_t", x); }
    void print_fotores(const fotores_t *x) { fotores_(0, "fotores_t", x); }
    void print_gas(const gas_t *x) { gas_(0, "gas_t", x); }
    void print_battery(const battery_t *x) { battery_(0, "battery_t", x); }
    void print_flight_state(flight_state_t x) { p_flight_state(0, "flight_state_t", x); }
    void print_datastore(const datastore_t *x) { datastore_(0, "datastore_t", x); }

#pragma endregion

#pragma region lora_structs

    void print_data_to_lora(const data_to_lora_t *x) { data_to_lora_(0, "data_to_lora_t", x); }

#pragma endregion

#pragma region sd_structs

    void print_data_type(data_type_t x) { p_data_type(0, "data_type_t", x); }
    void print_sd_header(const sd_header_t *x) { sd_header_(0, "sd_header_t", x); }

    void print_stamp_packed(const stamp_packed_t *x) { stamp_packed_(0, "stamp_packed_t", x); }
    void print_env_packed(const env_packed_t *x) { env_packed_(0, "env_packed_t", x); }
    void print_imu_packed(const imu_packed_t *x) { imu_packed_(0, "imu_packed_t", x); }
    void print_gps_packed(const gps_packed_t *x) { gps_packed_(0, "gps_packed_t", x); }
    void print_uv_packed(const uv_packed_t *x) { uv_packed_(0, "uv_packed_t", x); }
    void print_fotores_packed(const fotores_packed_t *x) { fotores_packed_(0, "fotores_packed_t", x); }
    void print_gas_packed(const gas_packed_t *x) { gas_packed_(0, "gas_packed_t", x); }
    void print_battery_packed(const battery_packed_t *x) { battery_packed_(0, "battery_packed_t", x); }
    void print_stats_packed(const stats_packed_t *x) { stats_packed_(0, "stats_packed_t", x); }

    DEFINE_SD_PRINTER(print_env_sd, "env_sd_t", env_sd_t, env_packed_)
    DEFINE_SD_PRINTER(print_imu_sd, "imu_sd_t", imu_sd_t, imu_packed_)
    DEFINE_SD_PRINTER(print_gps_sd, "gps_sd_t", gps_sd_t, gps_packed_)
    DEFINE_SD_PRINTER(print_uv_sd, "uv_sd_t", uv_sd_t, uv_packed_)
    DEFINE_SD_PRINTER(print_fotores_sd, "fotores_sd_t", fotores_sd_t, fotores_packed_)
    DEFINE_SD_PRINTER(print_gas_sd, "gas_sd_t", gas_sd_t, gas_packed_)
    DEFINE_SD_PRINTER(print_battery_sd, "battery_sd_t", battery_sd_t, battery_packed_)
    DEFINE_SD_PRINTER(print_stats_sd, "stats_sd_t", stats_sd_t, stats_packed_)

#pragma endregion

#pragma region sensor_states

    void print_sens_state(sens_state_t x)
    {
        p_enum(0, "sens_state_t", sens_state_names, ARRAY_LEN(sens_state_names), x);
    }

    void print_task_state(task_state_t x) { p_task_state(0, "task_state_t", x); }

#pragma endregion

#pragma region scheduler

    void print_task(const task_t *x) { task_(0, "task_t", x); }
    void print_scheduler(const scheduler_t *x) { scheduler_(0, "scheduler_t", x); }

#pragma endregion

#pragma region colors

    void print_rgb_colors(const rgb_colors_t *x)
    {
        if (!section(0, "rgb_colors_t", x))
            return;
        p_u(1, "red", x->red);
        p_u(1, "green", x->green);
        p_u(1, "blue", x->blue);
    }

#pragma endregion
}

#endif

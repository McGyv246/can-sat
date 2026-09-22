#include <stdint.h>
#include "types.h"
#include "hal_time.h"

void build_header(sd_header_t *out, sensor_type_t type, uint16_t len, uint32_t now_millis)
{
    out->sync = 0xA5A5;
    out->type = type;
    out->len = len;
    out->t_ms = now_millis;
}

void pack_stamp(stamp_packed_t *out, stamp_t *data)
{
    out->t_ms = data->t_ms;
    out->valid = data->valid;
}

void env_to_sd(env_sd_t *out, env_t *data)
{
    stamp_packed_t stamp;
    pack_stamp(&stamp, &(data->s));

    out->data.s = stamp;

    out->data.temp_c = data->temp_c;
    out->data.pressure_pa = data->pressure_pa;
    out->data.humidity_pct = data->humidity_pct;
    out->data.altitude_m = data->altitude_m;

    sd_header_t header;
    build_header(&header, ENV, sizeof(out->data), hal_millis());

    out->header = header;

    // out->crc = crc16_compute(...);
}
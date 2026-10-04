#include <Arduino.h>
#include "types.h"
#include "debug_print.h"
#include "lora_drivers.h"
#include "pack_data_lora.h"

/*
Ground station firmware (env:ground in platformio.ini): receives the LoRa packets sent by the satellite.
Only core 0 is used (no setup1/loop1, so core 1 is never started).

Everything goes to Serial as text, one line per record, comma separated. The first field says what the line is,
the program on the PC must skip every other line (debug prints start with "[", e.g. "[INF 123ms c0] ..."):
- HDR,<kind>,<column names...>  printed at boot, names of the columns of each kind of line
- RAW,<rx_ms>,<hex bytes>        every packet received, as is: keep it in the log to decode it again later
- PKT,<rx_ms>,<rssi>,...         a packet that passed every check, in physical units (see PKT_COLUMNS)
- BAD,<rx_ms>,<rssi>,<snr>,<why> a packet that was rejected
- STAT,<rx_ms>,<counters...>     totals since boot, every STAT_PERIOD_MS
*/

#define GROUND_SERIAL_WAIT_MS 5000
#define STAT_PERIOD_MS 10000
#define RX_BUFFER_SIZE 255 /* biggest LoRa payload: longer packets can't exist */

static const char PKT_COLUMNS[] =
    "rx_ms,rssi_dbm,snr_db,seq,lost,version,ms_since_boot,flight_state,records_dropped,"
    "temp_c,pressure_pa,humidity_pct,altitude_baro_m,"
    "accel_x_g,accel_y_g,accel_z_g,gyro_x_dps,gyro_y_dps,gyro_z_dps,accel_mag_g,"
    "light_uv,light_fotores,uv_index,gas_raw,heater_on_s,"
    "lat,lon,altitude_gps_m,sats,fix_type,vbatt_v,status";
static const char BAD_COLUMNS[] = "rx_ms,rssi_dbm,snr_db,reason";
static const char STAT_COLUMNS[] = "rx_ms,ok,radio_err,bad_length,bad_crc,bad_sync,bad_version,lost";

typedef struct
{
    uint32_t ok;
    uint32_t radio_err; /* the radio's own CRC or header check failed */
    uint32_t bad_length, bad_crc, bad_sync, bad_version;
    uint32_t lost; /* packets missing from the seq numbers */
} ground_stats_t;

static ground_stats_t stats;
static uint32_t last_stat_ms = 0;

// Last good packet, to find the lost ones
static bool have_last = false;
static uint16_t last_seq;
static uint32_t last_ms_since_boot;

#pragma region printing

static void print_field(uint32_t x)
{
  Serial.print(',');
  Serial.print(x);
}

static void print_field(double x, int digits)
{
  Serial.print(',');
  Serial.print(x, digits);
}

static void print_header()
{
  Serial.print("HDR,PKT,");
  Serial.println(PKT_COLUMNS);
  Serial.print("HDR,BAD,");
  Serial.println(BAD_COLUMNS);
  Serial.print("HDR,STAT,");
  Serial.println(STAT_COLUMNS);
}

static void print_raw(uint32_t rx_ms, const uint8_t *buf, size_t len)
{
  static const char HEX_DIGITS[] = "0123456789ABCDEF";

  Serial.print("RAW");
  print_field(rx_ms);
  Serial.print(',');
  for (size_t i = 0; i < len; i++)
  {
    Serial.print(HEX_DIGITS[buf[i] >> 4]);
    Serial.print(HEX_DIGITS[buf[i] & 0x0F]);
  }
  Serial.println();
}

static void print_bad(uint32_t rx_ms, float rssi, float snr, const char *reason)
{
  Serial.print("BAD");
  print_field(rx_ms);
  print_field(rssi, 1);
  print_field(snr, 2);
  Serial.print(',');
  Serial.println(reason);
}

// Digits = resolution of each field in the packet (see pack_data_lora.c), so nothing is lost or made up
static void print_pkt(uint32_t rx_ms, float rssi, float snr, uint16_t lost, const lora_decoded_t *p)
{
  Serial.print("PKT");
  print_field(rx_ms);
  print_field(rssi, 1);
  print_field(snr, 2);
  print_field((uint32_t)p->seq);
  print_field((uint32_t)lost);
  print_field((uint32_t)p->version);
  print_field(p->ms_since_boot);
  print_field((uint32_t)p->flight_state);
  print_field((uint32_t)p->records_dropped);

  print_field(p->temp_c, 2);
  print_field(p->pressure_pa, 0);
  print_field(p->humidity_pct, 1);
  print_field(p->altitude_baro_m, 0);

  for (int i = 0; i < 3; i++)
    print_field(p->accel[i], 3);
  for (int i = 0; i < 3; i++)
    print_field(p->gyro[i], 1);
  print_field(p->accel_mag, 3);

  print_field((uint32_t)p->is_there_light_uv);
  print_field((uint32_t)p->is_there_light_fotores);
  print_field(p->uv_index, 1);
  print_field((uint32_t)p->gas_raw);
  print_field((uint32_t)p->heater_on_s);

  print_field(p->lat, 7);
  print_field(p->lon, 7);
  print_field(p->altitude_gps_m, 0);
  print_field((uint32_t)p->sats);
  print_field((uint32_t)p->fix_type);
  print_field(p->vbatt, 2);
  print_field((uint32_t)p->status);
  Serial.println();
}

static void print_stats(uint32_t now)
{
  Serial.print("STAT");
  print_field(now);
  print_field(stats.ok);
  print_field(stats.radio_err);
  print_field(stats.bad_length);
  print_field(stats.bad_crc);
  print_field(stats.bad_sync);
  print_field(stats.bad_version);
  print_field(stats.lost);
  Serial.println();
}

#pragma endregion

/* Packets missing between the last good packet and this one */
static uint16_t count_lost(const lora_decoded_t *p)
{
  uint16_t lost = 0;

  // seq restarts from 0 when the satellite reboots: that's not a loss
  if (have_last && p->ms_since_boot < last_ms_since_boot)
    DBG_INFO("satellite rebooted (seq %u -> %u)", (unsigned)last_seq, (unsigned)p->seq);
  else if (have_last)
    lost = (uint16_t)(p->seq - last_seq - 1); // uint16_t arithmetic, so it works across the 65535 -> 0 wrap

  have_last = true;
  last_seq = p->seq;
  last_ms_since_boot = p->ms_since_boot;
  return lost;
}

static void handle_packet(uint32_t rx_ms, const uint8_t *buf, size_t len, float rssi, float snr)
{
  lora_decoded_t p;

  print_raw(rx_ms, buf, len);

  switch (lora_decode(&p, buf, len))
  {
  case LORA_DECODE_OK:
  {
    uint16_t lost = count_lost(&p);
    stats.ok++;
    stats.lost += lost;
    print_pkt(rx_ms, rssi, snr, lost, &p);
    break;
  }
  case LORA_DECODE_BAD_LENGTH:
    stats.bad_length++;
    print_bad(rx_ms, rssi, snr, "length");
    break;
  case LORA_DECODE_BAD_CRC:
    stats.bad_crc++;
    print_bad(rx_ms, rssi, snr, "crc");
    break;
  case LORA_DECODE_BAD_SYNC:
    stats.bad_sync++;
    print_bad(rx_ms, rssi, snr, "sync");
    break;
  case LORA_DECODE_BAD_VERSION:
    stats.bad_version++;
    print_bad(rx_ms, rssi, snr, "version");
    break;
  }
}

void setup()
{
  Serial.begin(115200);

  // USB serial drops what's printed before the monitor opens: wait a bit so the init messages are seen
  while (!Serial && millis() < GROUND_SERIAL_WAIT_MS)
    ;

  // SX1276 on SPI (CS D9, DIO0 D8, RESET D10), same wiring as the satellite. As receiver it starts listening right away
  if (lora_init(true) == SENS_STATE_READY)
    DBG_INFO("LORA receiver ready");
  else
    DBG_ERR("LORA init failed");

  print_header();
}

void loop()
{
  static uint8_t buf[RX_BUFFER_SIZE];
  size_t len;
  float rssi = 0, snr = 0;
  uint32_t now = millis();

  switch (lora_read_packet(buf, sizeof(buf), &len, &rssi, &snr, nullptr))
  {
  case SENS_STATE_READY:
    handle_packet(now, buf, len, rssi, snr);
    break;
  case SENS_STATE_ERR:
    stats.radio_err++;
    print_bad(now, rssi, snr, "radio");
    break;
  default: // nothing received
    break;
  }

  if (now - last_stat_ms >= STAT_PERIOD_MS)
  {
    last_stat_ms = now;
    print_stats(now);
  }
}

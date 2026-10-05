#include "bench_setup.h"

#ifdef BENCH_TEST

#include "tasks.h"
#include "scheduler.h"
#include "debug_print.h"
#include "fotores_drivers.h"
#include "uv_drivers.h"
#include "gybmep_drivers.h"
#include "gps_drivers.h"
#include "fuel_gauge_drivers.h"
#include "mq_7_drivers.h"
#include "lora_drivers.h"
#include "sd_drivers.h"

/*
To add a sensor call its init and enable its task here, comment out the ones you don't want to run.
The t/T commands still work to turn tasks on/off at runtime.
*/
void bench_setup(scheduler_t *sched_core0)
{
  fotores_init();
  sched_change_enabled_i(sched_core0, true, TASK_CORE0_FOTORES_SLOW);

  // GUVA-S12SD on A0 (D14), powered from 3.3 V so its output stays inside the ADC range
  uv_init();
  sched_change_enabled_i(sched_core0, true, TASK_CORE0_UV_SLOW);

  // BME280 on I2C (A4 SDA / A5 SCL). init leaves it asleep, enabling the task wakes it up
  if (gybmep_init() == SENS_STATE_READY)
    sched_change_enabled_i(sched_core0, true, TASK_CORE0_ENV);
  else
    DBG_ERR("GYBMEP init failed, task not enabled");

  // LSM6DSOX on the Nano board itself (I2C 0x6A), no init: enabling the task starts it
  sched_change_enabled_i(sched_core0, true, TASK_CORE0_IMU);

  // NEO-6M on Serial1 (GPS TX -> D0/RX, GPS RX -> D1/TX), 9600 baud. init powers it on and sets the
  // command pin, it returns NOT_READY until there's a fix, that's expected here
  // Disabled: turning the GPS on at boot browns out the board. Use t0 to turn it on at runtime
  // gps_init();
  // sched_change_enabled_i(sched_core0, true, TASK_CORE0_GPS);

  // MAX1704X on I2C (0x36), powered by the battery cell: it only answers with the battery connected.
  // init leaves it asleep, enabling the task wakes it up
  if (fuel_gauge_init() != SENS_STATE_ERR)
    sched_change_enabled_i(sched_core0, true, TASK_CORE0_BATTERY);
  else
    DBG_ERR("FUEL_GAUGE init failed, task not enabled");

  // MQ-7 on A1 (D15) through the voltage divider, heater switched by D3. init leaves the heater off.
  // The task (which turns the heater on, ~150 mA at 5 V) is not enabled at boot because of the brownouts
  // seen with the GPS: use t3 to turn it on/off at runtime
  mq_7_init();

  // MLX90640 on I2C (0x33), no init: enabling the task boots the camera (it has no sleep mode).
  // Reading a frame blocks core 0 for up to ~500 ms: use t10 to turn it off if it gets in the way
  sched_change_enabled_i(sched_core0, true, TASK_CORE0_THERMAL);

  // RGB led on D6 (R) / D5 (G) / D4 (B), enabling the task sets the pins. The state stays BOOT on the bench
  // (steady blue if all is ok): use the s command to see the patterns of the other flight states
  sched_change_enabled_i(sched_core0, true, TASK_CORE0_LED);
}

void bench_setup1(scheduler_t *sched_core1)
{
  // SD module on SPI (CS D7), shared with LoRa. The file is opened in append mode, so data from previous
  // runs is kept. The task writes what the sensor tasks put in the ring buffer (pack_data_sd format).
  // FAT 8.3 names only (max 8 chars + 3 of extension)
  if (sd_init("/DATA", "/DATA/LOG.BIN") == SENS_STATE_READY)
    sched_change_enabled_i(sched_core1, true, TASK_CORE1_SD);
  else
    DBG_ERR("SD init failed, task not enabled");

  // SX1276 on SPI (CS D9, DIO0 D8, RESET D10) as sender. init leaves it asleep and the task stays
  // disabled at boot: use T1 to turn it on at runtime. The task sends the whole datastore packed by
  // ds_to_lora every period
  if (lora_init(false) != SENS_STATE_READY)
    DBG_ERR("LORA init failed");
}

#endif

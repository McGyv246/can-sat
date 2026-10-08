#include <stddef.h>
#include <stdio.h>
#include <SPI.h>
#include <SD.h>
#include "types.h"
#include "sd_drivers.h"
#include "config_arduino.h"
#include "debug_print.h"

static bool flushed = true;
static const char *current_path = nullptr;
static File current_file;
static bool file_open = false;
static char current_filepath[32];

#define SD_MAX_LOG_FILES 1000

#pragma region internal_utils

static bool sd_mkdir(const char *path)
{
    if (!SD.mkdir(path))
        return false;
    return true;
}

static bool sd_chng_path(const char *path)
{
    if (SD.exists(path))
    {
        current_path = path;
        return true;
    }
    return false;
}

// Finds the first <path>/<prefix>_<n>.BIN that does not exist yet, so every session gets its own file.
// FAT 8.3 names: prefix + "_" + up to 3 digits must stay within 8 chars, so prefix is at most 4 chars
static bool sd_next_log_file(const char *path, const char *prefix, char *out, size_t out_len)
{
    for (unsigned i = 0; i < SD_MAX_LOG_FILES; i++)
    {
        int n = snprintf(out, out_len, "%s/%s_%u.BIN", path, prefix, i);
        if (n < 0 || (size_t)n >= out_len)
            return false;
        if (!SD.exists(out))
            return true;
    }
    return false;
}

static bool sd_open_file(const char *filepath)
{
    current_file = SD.open(filepath, FILE_WRITE);
    if (!current_file)
        return false; // for some reason this object can be evaluated in a boolean way
    file_open = true;
    return true;
}

// The LoRa radio shares the SPI bus: its CS is left floating (pulled low by the RP2040 pads at reset)
// until lora_init, so the radio would answer on MISO together with the card
static void deselect_other_spi_devices(void)
{
    pinMode(CHIPSELECT_LORA, OUTPUT);
    digitalWrite(CHIPSELECT_LORA, HIGH);
}

#ifdef SERIAL_DEBUG_ERRORS
// SD.begin only says true/false: redo its steps by hand to print where it stops (blocks up to ~2 s more).
// Card error codes are in utility/Sd2Card.h: 0x01 CMD0 = no answer at all (wiring, bus, power),
// 0x08 ACMD41 = answers but never gets ready (power). Card ok but volume failed = not FAT16/FAT32 (e.g. exFAT)
static void sd_print_begin_error(void)
{
    Sd2Card card;
    if (!card.init(SPI_HALF_SPEED, CHIPSELECT_SD))
    {
        DBG_ERR("SD card init failed, error 0x%02x data 0x%02x", (unsigned)card.errorCode(), (unsigned)card.errorData());
        return;
    }
    SdVolume volume;
    if (!volume.init(card))
    {
        DBG_ERR("SD card ok but no FAT16/FAT32 volume found");
        return;
    }
    DBG_ERR("SD card and volume ok on the second try");
}
#endif

#pragma endregion

#pragma region public_interface

extern "C"
{
    sens_state_t sd_init(const char *path, const char *prefix)
    {
        deselect_other_spi_devices();
        if (!SD.begin(CHIPSELECT_SD))
        {
#ifdef SERIAL_DEBUG_ERRORS
            sd_print_begin_error();
#endif
            return SENS_STATE_ERR;
        }
        if (!sd_mkdir(path))
        {
            DBG_ERR("SD mkdir %s failed", path);
            return SENS_STATE_ERR;
        }
        if (!sd_chng_path(path))
        {
            DBG_ERR("SD %s not found after mkdir", path);
            return SENS_STATE_ERR;
        }
        if (!sd_next_log_file(path, prefix, current_filepath, sizeof(current_filepath)))
        {
            DBG_ERR("SD no free %s/%s_<n>.BIN name", path, prefix);
            return SENS_STATE_ERR;
        }
        if (!sd_open_file(current_filepath))
        {
            DBG_ERR("SD open %s failed", current_filepath);
            return SENS_STATE_ERR;
        }
        DBG_INFO("SD logging to %s", current_filepath);
        return SENS_STATE_READY;
    }

    sens_state_t sd_write_uint8_buf(uint8_t *buf, size_t len)
    {
        if (!file_open)
            return SENS_STATE_ERR;
        if (!flushed)
            return SENS_STATE_NOT_READY;
        current_file.write(buf, len);
        flushed = false;
        return SENS_STATE_NOT_READY;
    }

    sens_state_t sd_flush_buf(void)
    {
        if (!file_open)
            return SENS_STATE_ERR;
        current_file.flush();
        flushed = true;
        return SENS_STATE_READY;
    }

    void sd_close()
    {
        if (file_open)
            current_file.close();
        file_open = false;
        SD.end();
        return;
    }
}

#pragma endregion
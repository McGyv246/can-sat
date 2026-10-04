#include <stddef.h>
#include <SPI.h>
#include <SD.h>
#include "types.h"
#include "sd_drivers.h"
#include "config_arduino.h"

static bool flushed = true;
static const char *current_path = nullptr;
static File current_file;
static bool file_open = false;

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

static bool sd_open_file(const char *filepath)
{
    current_file = SD.open(filepath, FILE_WRITE);
    if (!current_file)
        return false; // for some reason this object can be evaluated in a boolean way
    file_open = true;
    return true;
}

#pragma endregion

#pragma region public_interface

extern "C"
{
    sens_state_t sd_init(const char *path, const char *filepath)
    {
        if (!SD.begin(CHIPSELECT_SD))
            return SENS_STATE_ERR;
        if (!sd_mkdir(path))
            return SENS_STATE_ERR;
        if (!sd_chng_path(path))
            return SENS_STATE_ERR;
        if (!sd_open_file(filepath))
            return SENS_STATE_ERR;
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
#include "bmp_388.h"
#include "driver/bmp3.h"
#include "driver/bmp3_selftest.h"
#include "hardware/i2c.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "pico/stdlib.h"


BMP388::BMP388(uint8_t addr, int seaLevelArg) {
    int8_t res = 0;
    
    _bmp388_address = addr;
    seaLevel = seaLevelArg * 100;

    // Set the BMP388 struct to use I2C and self-defined read/write/delay funcs
    bmp388.intf = BMP3_I2C_INTF;
    bmp388.delay_us = (bmp3_delay_us_fptr_t)&bmpDelay;
    bmp388.read = (bmp3_read_fptr_t)&bmpRead;
    bmp388.write = (bmp3_write_fptr_t)&bmpWrite;

    // Perform soft reset of sensor before initialization
    if (bmp3_soft_reset(&bmp388) != BMP3_OK) {
        printf("ERROR: Unable to soft-reset the BMP388 sensor\n");
        fflush(stdout);
        return;
    }

    if (bmp3_init(&bmp388) != BMP3_OK) {
        printf("ERROR: Unable to initialize the BMP388 sensor\n");
        fflush(stdout);
        return;
    }

    // Configure the optimal settings for payload
    if ((res = set_sensor_settings()) != BMP3_OK) {
        printf("ERROR: Unable to set the BMP388 sensor settings \n");
        fflush(stdout);
        return;
    }

    // Set sensor to forced mode to perform reading when queried
    if (set_power_mode_forced() != BMP3_OK) {
        printf("ERROR: Unable to set the BMP388 power mode\n");
        fflush(stdout);
        return;
    }

    sensor_valid = true;
}

int8_t BMP388::perform_reading() {
    int8_t res = 0;

    // Variable used to select the sensor component
    uint8_t sensor_comp;

    // Use Pressure and Temperature readings    
    sensor_comp = BMP3_PRESS | BMP3_TEMP;

    // Set the BMP to forced mode before querying for data
    set_power_mode_forced();

    if ((res = bmp3_get_sensor_data(sensor_comp, &sensorData, &bmp388)) != BMP3_OK) {
        printf("\nERROR: Unable to get BMP388 sensor data\n");
        this->sensor_valid = false;
        return res;
    }

    // Perform Altitude calculation
    altitude = calculate_altitude();

    return 0;
}

double BMP388::calculate_altitude() {
    // Returns the altitude height in meters as a double
    double alt = 0.0;

    // Apply international barometric formula
    alt = ALTITUDE_COEFF * (1 - pow((sensorData.pressure / seaLevel), (1/ALTITUDE_DIV)) );

    return alt > 0 ? alt : 0.0;
}

int8_t BMP388::set_power_mode_forced() {
    bmp388.settings.op_mode = BMP3_MODE_FORCED;
    return bmp3_set_op_mode(&bmp388);
}

int8_t BMP388::set_sensor_settings() {
    uint16_t settings_sel;
    
    bmp388.settings.press_en = BMP3_ENABLE;
    bmp388.settings.temp_en = BMP3_ENABLE;
    bmp388.settings.odr_filter.press_os = BMP3_OVERSAMPLING_8X;
    bmp388.settings.odr_filter.temp_os = BMP3_OVERSAMPLING_2X;
    bmp388.settings.odr_filter.iir_filter = BMP3_IIR_FILTER_COEFF_15;
    bmp388.settings.odr_filter.odr = BMP3_ODR_25_HZ;

    settings_sel = BMP3_SEL_PRESS_EN | BMP3_SEL_TEMP_EN | BMP3_SEL_PRESS_OS | BMP3_SEL_TEMP_OS | BMP3_SEL_ODR;

    return bmp3_set_sensor_settings(settings_sel, &bmp388);
}

BMP3_INTF_RET_TYPE bmpWrite(uint8_t reg_addr, const uint8_t *write_data, uint32_t len, void *intf_ptr) {
    // Writes to the BMP at the register address specified via I2C
    // Combine the reg_addr into one concatenated buffer with the write data for I2C comm.
    uint8_t *write_buff = (uint8_t *)malloc(sizeof(uint8_t) + sizeof(write_data));
    memcpy(write_buff, &reg_addr, sizeof(uint8_t));
    memcpy(&write_buff[1], write_data, len);
    int8_t write_res = i2c_write_blocking(i2c0, _bmp388_address, write_buff, 2, false);

    // free the allocated memory
    free(write_buff);

    return write_res > 0 ? BMP3_INTF_RET_SUCCESS : BMP3_E_COMM_FAIL;
}

BMP3_INTF_RET_TYPE bmpRead(uint8_t reg_addr, uint8_t *read_data, uint32_t len, void *intf_ptr) {
    // To read, first write the register address wanting to be read
    i2c_write_blocking(i2c0, _bmp388_address, &reg_addr, sizeof(uint8_t), true);
    int8_t write_result = i2c_read_blocking(i2c0, _bmp388_address, read_data, len, false);

    return write_result > 0 ? BMP3_INTF_RET_SUCCESS : BMP3_E_COMM_FAIL;
}

BMP3_INTF_RET_TYPE bmpDelay(uint32_t amt, void *intf_ptr) {
    sleep_us(amt);
    return BMP3_INTF_RET_SUCCESS;
}
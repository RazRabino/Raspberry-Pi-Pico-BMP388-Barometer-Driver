#ifndef BMP_388_H
#define BMP_388_H

#include "pico/stdlib.h"
#include "driver/bmp3.h"

#define ALTITUDE_COEFF 44330
#define ALTITUDE_DIV 5.255
#define ALTITUDE_ERROR -1

static uint8_t _bmp388_address;
static int seaLevel;

class BMP388 {
    private:
        int8_t set_sensor_settings();
        int8_t set_power_mode_forced();
        double calculate_altitude();

    public:
        struct bmp3_dev bmp388;
        struct bmp3_data sensorData;
        double altitude;
        bool sensor_valid;

        BMP388(uint8_t addr, int seaLevelArg);

        double get_altitude() { return altitude; }

        double get_temperature() { return sensorData.temperature; }

        double get_pressure() { return sensorData.pressure; }

        int8_t perform_reading();
};

BMP3_INTF_RET_TYPE bmpWrite(uint8_t reg_addr, const uint8_t *read_data, uint32_t len, void *intf_ptr);
BMP3_INTF_RET_TYPE bmpRead(uint8_t reg_addr, uint8_t *read_data, uint32_t len, void *intf_ptr);
BMP3_INTF_RET_TYPE bmpDelay(uint32_t amt, void *intf_ptr);


#endif
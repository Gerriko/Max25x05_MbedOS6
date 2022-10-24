/*
*
*
*/

#ifndef __MAX25X05_I2C_H__
#define __MAX25X05_I2C_H__

#include "MAX25x05.h"
//#include <cstdio>

class MAX25x05_I2C: public MAX25x05_BusInterface {
public:
    MAX25x05_I2C(I2C &i2c, int hz, PinName csbPin, uint8_t csbPinState):
        _i2c(i2c)
        {
            begin(hz);
            DigitalOut csb(csbPin, csbPinState);
            if (csbPinState == 1) _i2c_device_addr = 0xA0;
            else if (csbPinState == 1) _i2c_device_addr = 0x9E;

        };

    ~MAX25x05_I2C();

    void begin(int hz) {
        _i2c.frequency(hz);
    }

    int reg_write(const uint8_t reg_addr, const uint8_t reg_val) {
        char data[2];
        data[0] = reg_addr;
        data[1] = reg_val;
        int result = -1;
        result = _i2c.write(_i2c_device_addr, data, 2);
        return result;
    }

    int reg_read(const uint8_t reg_addr, const uint8_t num_bytes, uint8_t reg_vals[])
    {
        char read_addr = (char)reg_addr;
        _i2c.write(_i2c_device_addr, &read_addr, 1);
        _i2c.read(_i2c_device_addr, (char*)reg_vals, (int)num_bytes);
        return 0;
    }

private:
    I2C         &_i2c;
    uint32_t    _i2c_device_addr;

};


#endif  // __MAX25X05_I2C_H__

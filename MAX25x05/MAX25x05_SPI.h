
#ifndef __MAX25X05_SPI_H__
#define __MAX25X05_SPI_H__

#include "MAX25x05.h"

class MAX25x05_SPI: public MAX25x05_BusInterface {
public:
    MAX25x05_SPI(SPI &spi, int hz, PinName cselPin):
        _spi(spi), _csel(cselPin)
        {
            begin(hz);
        };

    ~MAX25x05_SPI();
    
    void begin(int hz) {          // set default at 3MHz (3e6)
        _spi.frequency(hz);
    }

    int reg_write(const uint8_t reg_addr, const uint8_t reg_val) {
        int result = -1;
        _csel = 0;

        _spi.write(reg_addr);    // byte1: register address
        _spi.write(0x00);        // byte2: write command 0x00
        result = _spi.write(reg_val);     // byte3: write byte
        _csel = 1;

        return result;
    }

    int reg_read(const uint8_t reg_addr, const uint8_t num_bytes, uint8_t reg_vals[])
    {
        _csel = 0;
        _spi.write(reg_addr);                // byte 1: register address
        _spi.write(0x80);                    // byte 2: read command 0x80
        for(int i=0; i<num_bytes; i++) {
            reg_vals[i] = _spi.write(0x00);  // byte3: read byte
        }
        _csel = 1;
    return 0;
    }



private:
    SPI         &_spi;
    DigitalOut  _csel;

};

#endif      // __MAX25X05_SPI_H__
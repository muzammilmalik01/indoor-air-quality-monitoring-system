#include <zephyr/drivers/i2c.h>
#include "sensirion_i2c.h"

void sensirion_sleep_usec(uint32_t usesconds){
    int32_t remaining = usesconds;
    while(remaining > 0){
        remaining = k_usleep(remaining);
    }
}

int8_t sensirion_i2c_read(const struct i2c_dt_spec *dev_bus, uint8_t *data, uint16_t count){
    return i2c_read_dt(dev_bus, data, count);
}

int8_t sensirion_i2c_write(const struct i2c_dt_spec *dev_bus, uint8_t *data, uint16_t count){
    return i2c_write_dt(dev_bus, data, count);
}


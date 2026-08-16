#ifndef MPU6050_H_
#define MPU6050_H_


// 7 bit 
#include "stm32g4xx_hal_i2c.h"
#include <stdbool.h>
#define MPU6050_BASE_ADDRESS ((uint8_t)0b1101000)
// Registers' addressess
// Configuration Registers
#define MPU6050_WHO_AM_I_BASE 0x75
#define MPU6050_PWR_MGMT_BASE 0x6B
#define MPU6050_SMPRT_DIV_BASE 0x19
#define MPU6050_GYRO_CONFIG_BASE 0x1B
#define MPU6050_ACCEL_CONFIG_BASE 0x1C
// Measurement registers
#define MPU6050_XOUT_BASE 0x3B
#define MPU6050_YOUT_BASE 0x3D
#define MPU6050_ZOUT_BASE 0x3F
// Default timeout
#define MPU6050_TIMEOUT 1000

#define MPU6050_ADDR(handle) ((MPU6050_BASE_ADDRESS | (handle).address_lsb) << 1)


typedef struct {
    I2C_HandleTypeDef* i2c; 
    bool address_lsb; 
} MPU6050_Handle; 
bool MPU6050_init(MPU6050_Handle handle); 

typedef struct {
    uint16_t x, y, z;
} MPU6050_Raw;
bool MPU6050_read_u16(MPU6050_Handle handle, uint16_t reg_base, uint16_t* res) ; 
bool MPU6050_read_raw(MPU6050_Handle handle, MPU6050_Raw* result); 
void MPU6050_print_raw(MPU6050_Raw data); 


bool MPU6050_write_byte(MPU6050_Handle handle, uint16_t reg, uint8_t byte); 




#ifdef MPU6050_IMPL

bool MPU6050_init(MPU6050_Handle handle) {
    uint8_t id = 0; 
    if (HAL_I2C_Mem_Read(handle.i2c,
                         MPU6050_ADDR(handle),
                         MPU6050_WHO_AM_I_BASE, 1, &id, 1,
                         MPU6050_TIMEOUT)
        != HAL_OK)
        return false;
    printf("[DBG] id = %u, addr = %u\r\n", id, MPU6050_BASE_ADDRESS);
    if (id != MPU6050_BASE_ADDRESS) {
        return false; 
    }
    // Wake up the sensor.
    if (!MPU6050_write_byte(handle, MPU6050_PWR_MGMT_BASE, 0)) return false; 
    // Set the Sample rate
    // According to the formula: Sample Rate = Output Rate / ( 1 + SMPLRT_DIV )
    // Here, we set SMPLRT_DIV to 7.
    if (!MPU6050_write_byte(handle, MPU6050_SMPRT_DIV_BASE, 0x7)) return false; 

    // Configure accelerometer and gyroscope
    if(!MPU6050_write_byte(handle, MPU6050_ACCEL_CONFIG_BASE, 0)) return false;
    if(!MPU6050_write_byte(handle, MPU6050_GYRO_CONFIG_BASE, 0)) return false;
    return true; 
}

bool MPU6050_read_u16(MPU6050_Handle handle, uint16_t reg_base, uint16_t* res)  {
    return HAL_I2C_Mem_Read(handle.i2c, MPU6050_ADDR(handle), reg_base, 1, (uint8_t*)res, 2, MPU6050_TIMEOUT) == HAL_OK;
}

bool MPU6050_read_raw(MPU6050_Handle handle, MPU6050_Raw* result) {
    if (!MPU6050_read_u16(handle, MPU6050_XOUT_BASE, &result->x)) return false; 
    if (!MPU6050_read_u16(handle, MPU6050_YOUT_BASE, &result->y)) return false;
    if (!MPU6050_read_u16(handle, MPU6050_ZOUT_BASE, &result->z)) return false;
    return true;
}

bool MPU6050_write_byte(MPU6050_Handle handle, uint16_t reg, uint8_t byte) {
    return HAL_I2C_Mem_Write(handle.i2c, MPU6050_ADDR(handle), reg, 1, &byte, 1, MPU6050_TIMEOUT) == HAL_OK;
}


void MPU6050_print_raw(MPU6050_Raw data) {
    printf(
        "Measurements = {\r\n"
        "  x = %u,\r\n"
        "  y = %u,\r\n"
        "  z = %u.\r\n"
        "}\r\n",
        data.x, data.y, data.z
    );
}




#endif // MPU6050_IMPL


#endif // MPU6050_H_

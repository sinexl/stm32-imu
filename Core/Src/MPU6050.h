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
#define MPU6050_ACCEL_MEASUREMENT_BASE 0x3B
#define MPU6050_GYRO_MEASUREMENT_BASE 0x43
// Default timeout
#define MPU6050_TIMEOUT 1000
// INT16_MAX / 2g = 16384 LSB/g
#define MPU6050_ACCEL_LSB_SENSITIVITY 16384.0f
// INT16_MAX / 250 approx 131 LSB/deg/s
#define MPU6050_GYRO_LSB_SENSITIVITY 131.0f

#define MPU6050_ADDR(handle) ((MPU6050_BASE_ADDRESS | (handle).address_lsb) << 1)


typedef struct {
    I2C_HandleTypeDef* i2c; 
    bool address_lsb; 
} MPU6050_Handle; 
bool MPU6050_init(MPU6050_Handle handle); 

typedef struct {
    float x, y, z;
} MPU6050_Measurements;

// Measures axial acceleration in g 
bool MPU6050_read_accelerometer(MPU6050_Handle handle, MPU6050_Measurements* result); 
// Measures rotation rate against IMU's axes in deg/s
bool MPU6050_read_gyro(MPU6050_Handle handle, MPU6050_Measurements* result);

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

bool MPU6050_read_accelerometer(MPU6050_Handle handle, MPU6050_Measurements* result) {
    uint8_t data[6] = {0};
    if (!HAL_I2C_Mem_Read(handle.i2c, MPU6050_ADDR(handle), MPU6050_ACCEL_MEASUREMENT_BASE, 1, data, 6, MPU6050_TIMEOUT) == HAL_OK)
        return false;

    // convert big endian to little endian

    int16_t raw_x = ((data[0] << 8) | data[1]);
    int16_t raw_y = ((data[2] << 8) | data[3]);
    int16_t raw_z = ((data[4] << 8) | data[5]);

    *result = (MPU6050_Measurements) {
        .x = raw_x / MPU6050_ACCEL_LSB_SENSITIVITY,
        .y = raw_y / MPU6050_ACCEL_LSB_SENSITIVITY,
        .z = raw_z / MPU6050_ACCEL_LSB_SENSITIVITY,
    };

    return true;
}
bool MPU6050_read_gyro(MPU6050_Handle handle, MPU6050_Measurements* result) {
    uint8_t data[6] = {0};
    if (!HAL_I2C_Mem_Read(handle.i2c, MPU6050_ADDR(handle), MPU6050_GYRO_MEASUREMENT_BASE, 1, data, 6, MPU6050_TIMEOUT) == HAL_OK)
        return false;

    // convert big endian to little endian

    int16_t raw_x = ((data[0] << 8) | data[1]);
    int16_t raw_y = ((data[2] << 8) | data[3]);
    int16_t raw_z = ((data[4] << 8) | data[5]);

    *result = (MPU6050_Measurements) {
        .x = raw_x / MPU6050_GYRO_LSB_SENSITIVITY,
        .y = raw_y / MPU6050_GYRO_LSB_SENSITIVITY,
        .z = raw_z / MPU6050_GYRO_LSB_SENSITIVITY,
    };

    return true;
}

bool MPU6050_write_byte(MPU6050_Handle handle, uint16_t reg, uint8_t byte) {
    return HAL_I2C_Mem_Write(handle.i2c, MPU6050_ADDR(handle), reg, 1, &byte, 1, MPU6050_TIMEOUT) == HAL_OK;
}




#endif // MPU6050_IMPL


#endif // MPU6050_H_

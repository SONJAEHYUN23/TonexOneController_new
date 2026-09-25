/************************************************
      Header File: SX1509.H
************************************************/

#ifndef _SX1509E_H
#define _SX1509E_H
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


esp_err_t SX1509E_Init(i2c_master_bus_handle_t bus_handle, SemaphoreHandle_t I2CMutex);
esp_err_t SX1509E_refresh(void);
esp_err_t SX1509E_gpioMode(uint8_t pin, uint8_t mode);
esp_err_t SX1509E_digitalWrite(uint8_t pin, uint8_t value);
esp_err_t SX1509E_digitalRead(uint8_t pin, uint8_t* value); 
esp_err_t SX1509E_getPinValues(uint16_t* values);


#endif      //_SX1509E_H

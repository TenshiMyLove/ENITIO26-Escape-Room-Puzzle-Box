// Includes
#include "74HC595.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "driver/gpio.h"						// GPIO
#include "sdkconfig.h"
#include "esp_rom_sys.h"


// Functions
void LED_RegInit() 
{ gpio_config_t led_conf = 
    { .pin_bit_mask = (1ULL << latch_pin) | // Select GPIOs 
                      (1ULL << clock_pin) |
                      (1ULL << data_pin), 
      .mode = GPIO_MODE_OUTPUT, // Set as output 
      .pull_up_en = GPIO_PULLUP_DISABLE, // Disable pull-up 
      .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down 
      .intr_type = GPIO_INTR_DISABLE // Disable interrupts 
    };

    gpio_config(&led_conf);

    gpio_set_level(latch_pin, 0); 
    gpio_set_level(clock_pin, 0); 
    gpio_set_level(data_pin, 0); 

    return; 
} 


void clockInData() 
{ 
    gpio_set_level(clock_pin, 1); 
    esp_rom_delay_us(PulseLen);   

    gpio_set_level(clock_pin, 0); 
    esp_rom_delay_us(PulseLen);    

    return; 
} 


void storeData() 
{ 
    gpio_set_level(latch_pin, 1); 
    esp_rom_delay_us(PulseLen);   

    gpio_set_level(latch_pin, 0); 
    esp_rom_delay_us(PulseLen); 

    return; 
} 


void updateReg(uint8_t regData) 
{ 
    for (int i = 0; i < regLen; i++) 
    { 
        gpio_set_level(data_pin, (regData >> i) & 0x01); 
        clockInData(); 
    } 

    storeData();

    return; 
} 


void resetReg() 
{ 
    updateReg(0x00); 
    return; 
}
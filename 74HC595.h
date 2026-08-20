// Includes
#include <stdint.h>

// Definitions
#define latch_pin   (GPIO_NUM_25)
#define clock_pin   (GPIO_NUM_26)
#define data_pin    (GPIO_NUM_27)

#define PulseLen    (5)                // In us (microseconds)
#define regLen      (8)


// Function Declarations
void LED_RegInit();
void clockInData();
void storeData();
void updateReg(uint8_t regData);
void resetReg();
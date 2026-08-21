// ENITIO 2026 Escape Room Puzzle Box Station


// Libraries
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"						// For non-blocking delay
#include "driver/gpio.h"						// GPIO
#include "sdkconfig.h"
#include "esp_adc/adc_oneshot.h"				// ADC Oneshot (P.S. to include go to CMakeLists.txt)
#include <esp_log.h>							// Logs
#include "QAPASS_LCD.h"							// LCD screen
#include "74HC595.h"							// Shift register


// Definitions	
#define numofBits		(8)						// Number of bits
#define passLen			(8)						// Password Length
#define BIT0_PIN 		(GPIO_NUM_12)			// LSB
#define BIT1_PIN 		(GPIO_NUM_13)
#define BIT2_PIN 		(GPIO_NUM_14)
#define BIT3_PIN 		(GPIO_NUM_15)
#define BIT4_PIN 		(GPIO_NUM_16)
#define BIT5_PIN 		(GPIO_NUM_17)
#define BIT6_PIN 		(GPIO_NUM_18)
#define BIT7_PIN 		(GPIO_NUM_19)			// MSB

#define BUTTON_PIN 		(GPIO_NUM_23)

#define LED0_PIN        (GPIO_NUM_4)
#define LED1_PIN        (GPIO_NUM_5)
#define LED2_PIN        (GPIO_NUM_32)
#define LED3_PIN        (GPIO_NUM_33)

#define ADC_PIN       	(ADC_CHANNEL_7)    		// Channel 7 - Check ESP32 Pinout for the GPIO Number
#define ADC_UNIT      	(ADC_UNIT_1)        	// ADC1
#define ADC_BITWIDTH  	(ADC_BITWIDTH_12)   	// 12-bit resolution (0-4095)
#define ADC_ATTEN     	(ADC_ATTEN_DB_12)    	// ~3.3V full-scale voltage

#define nextGS			(0x0A)
#define readGS			(0x0B)
#define resetGS			(0x0C)
#define resetFlagGS		(0x0D)
#define errorGS			(0xFF)
#define successGS		(0x00)
#define GS1				(0xA0)
#define GS2				(0xA1)
#define GS3				(0xA2)
#define GS4 			(0xA3)
#define storeBitsPCmd	(0x1A)
#define checkPCmd		(0x1B)
#define resetPCmd		(0x1C)

// #define debug


// Structs
typedef struct {
	uint16_t checkedBit;
	uint16_t toggles; 
} ToggleValues;


// Global Variables
static volatile TickType_t lastInterruptTick = 0;
uint8_t bitBuffer = 0;
uint8_t inputPass[passLen];
uint8_t isr_debounce = 0;
	

// Function Prototypes
void readBits();
static void IRAM_ATTR gpio_isr_handler(void *arg);
uint8_t gamestateCmd(uint8_t cmd);
void stationInit();
void ledUpdate(uint16_t state);
void passCmd(uint8_t cmd);
void ENI26_GPIO_Init();
void ENI26_ADC_Init(adc_oneshot_unit_handle_t *adc_handle);
void HC_Charge(int adc_value);
int checkPass();
uint8_t gamestate2Encode();
void gamestate1Check(int adc_val);


// Main
void app_main(void)
{
	// Variables
	int adc_value;
    adc_oneshot_unit_handle_t adc_handle;
	uint8_t adc_Enabled = 1;
	uint8_t firstRun = 1;


	// Initialization
	ENI26_GPIO_Init();
	ENI26_ADC_Init(&adc_handle);
	
	gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, (void *)BUTTON_PIN);
	
	stationInit();

	gpio_intr_disable(BUTTON_PIN);

	lcd_clear();
	lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
    lcd_send_string("Stage 1: Use the");        	// Display the string "Hello world!!!" on the LCD
    lcd_put_cursor(1, 0);                   // Set the cursor position to the second row, first column
    lcd_send_string("crank!  Level: 1");     	// Display the string "from @voidlooop" on the LCD
	

	// Main loop
    while (true) 
	{
		#ifdef debug
			for (int i = 0; i < passLen; i++)
			{
				printf("%i ", inputPass[i]);
			}
			printf("%i\n", bitBuffer);
			if(0)
			{
				gpio_set_level(GPIO_NUM_2, 0);
				ledUpdate(0x0000);
			}

			// Read ADC value with Oneshot
        	ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_PIN, &adc_value));
        	// Print ADC value
        	ESP_LOGI("ADC Value", "%d", adc_value);
        	// Delay 0.2 second
        	vTaskDelay(pdMS_TO_TICKS(200)); 
		#endif


		switch(gamestateCmd(readGS))
		{
			case (GS1):						// First gamestate: hand crank
				// Read ADC value with Oneshot
				ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_PIN, &adc_value));

				// #ifdef debug
				// Print ADC value
				ESP_LOGI("ADC Value", "%d", adc_value);
				// #endif

				// Check ADC value
				gamestate1Check(adc_value);

				// Delay 20 miliseconds
				vTaskDelay(pdMS_TO_TICKS(20)); 
				break;
				

			case (GS2):						// Second gamestate: LED toggle
				if(adc_Enabled)
				{
					ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));
					adc_Enabled = 0;
					gamestateCmd(resetFlagGS);
				}
				readBits();
				if((gamestate2Encode()) == 0xFF)
				{
					vTaskDelay(pdMS_TO_TICKS(1000));
					gamestateCmd(nextGS);
				}

				vTaskDelay(pdMS_TO_TICKS(50));

				// NOTE: [tk] if possible add a time limit for this section, display on LCD

				break;

			case (GS3):						// Third gamestate: Input the password
				char charBuffer;
				if (firstRun)
				{
					lcd_clear();
					lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
					lcd_send_string("Stage Cleared!");        
					lcd_put_cursor(1, 0);                   // Set the cursor position to the second row, first column
					lcd_send_string("Loading...");     

					passCmd(resetPCmd);
					gamestateCmd(resetFlagGS);
					ledUpdate(0x0000);
					firstRun = 0;
					gpio_intr_enable(BUTTON_PIN);
					vTaskDelay(pdMS_TO_TICKS(3000));

					lcd_clear();
					lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
					lcd_send_string("Stage 3: Input");        	// Display the string "Hello world!!!" on the LCD
					lcd_put_cursor(1, 0);                   // Set the cursor position to the second row, first column
					lcd_send_string("the Passcode!");     	// Display the string "from @voidlooop" on the LCD
					vTaskDelay(pdMS_TO_TICKS(3000));
					lcd_clear();
					lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
					lcd_send_string("Selected:");    
				}

				passCmd(checkPCmd);
				ledUpdate(0x0000);
				readBits();
				charBuffer = (char)bitBuffer;
				lcd_put_cursor(1, 0);                   // Set the cursor position to the second row, first column
				lcd_send_string(&charBuffer);
				
				if (isr_debounce)
				{
					passCmd(storeBitsPCmd);
					isr_debounce = 0;
				}

				vTaskDelay(pdMS_TO_TICKS(100));
				break;

				case (GS4):
					lcd_clear();
					lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
					lcd_send_string("Solved!");        		
					vTaskDelay(pdMS_TO_TICKS(3000));
					while(1)
					{
						printf("done\n");
						vTaskDelay(pdMS_TO_TICKS(5000));
					}
		}
    }
}


// Functions
void readBits()
{
	// Pins to read
	uint8_t bitPins[numofBits] = {BIT0_PIN, BIT1_PIN, BIT2_PIN, BIT3_PIN, 
								  BIT4_PIN, BIT5_PIN, BIT6_PIN, BIT7_PIN};

	// Read pins 
	bitBuffer = 0;
	for (int i = 0; i < 8; i++) 
	{
    	bitBuffer += (gpio_get_level(bitPins[i]) << i);
	}
	
	return;
}

void passCmd(uint8_t cmd)
{
	static uint8_t index = 0;

	switch(cmd)
	{
		case (checkPCmd):
			printf("(%i) %i %i\n", index, isr_debounce, inputPass[index]);
			if(index == (passLen))
			{
				if((checkPass()) != 1)
				{
					index = 0;
					return;
				}
				gamestateCmd(nextGS);
			}
			break;

		case (storeBitsPCmd):
            if (bitBuffer == 0x08)
            {
                index--;
                lcd_put_cursor(1, (8 + index));
				lcd_send_data(' ');
            }
            else if (bitBuffer <= 0x20)
            {
                inputPass[index] = bitBuffer;
				lcd_put_cursor(1, (8 + index));
				lcd_send_data(' ');
				if ((index) == passLen)
				{
					return;
				}
				index++;
            }
            else
            {
                inputPass[index] = bitBuffer;
				lcd_put_cursor(1, (8 + index));
				lcd_send_data((char)bitBuffer);
				if ((index) == passLen)
				{
					return;
				}
				index++;
            }
				
			break;

		case (resetPCmd):
			index = 0;
			break;

		default:
			return;
	}

	
	return;
}

uint8_t gamestateCmd(uint8_t cmd)
{
	static uint8_t gamestateFlag = 1;
	uint8_t gamestates[] = {GS1, GS2, GS3, GS4};
	static uint8_t current_GS = 0;
	
	switch(cmd)
	{
		case (resetGS):
			current_GS = 0; 
			break;
			
		case (nextGS):
			if(gamestateFlag == 1)
			{
				current_GS += 1;
				printf("nextGS");	
				gamestateFlag = 0;
			}
			printf("Repeated Call!");
			break;
			
		case (readGS):
			return gamestates[current_GS];
			break;

		case (resetFlagGS):
			gamestateFlag = 1;
			break;
			
		default:
		return errorGS;
	}
	
	return successGS;
}

void gamestate1Check(int adc_val)
{
	static uint8_t stage = 0;
	static uint16_t timePassed = 0;

	HC_Charge(adc_val);

	switch (stage)
	{
		case (0):
			if ((adc_val >= 2000) && (adc_val < 3000))
			{
				timePassed += 1;
			}
			else
			{
				if(timePassed > 0)
				{
					timePassed -= 1;
				}
			}
			break;

		case (1):
			if ((adc_val >= 1000) && (adc_val < 2000))
			{
				timePassed += 1;
			}
			else
			{
				if(timePassed > 0)
				{
					timePassed -= 1;
				}
			}
			break;

		case (2):
			if ((adc_val >= 2000) && (adc_val < 3000))
			{
				timePassed += 1;
			}
			else
			{
				if(timePassed > 0)
				{
					timePassed -= 1;
				}
			}
			break;

		case (3):
			if ((adc_val >= 1500) && (adc_val < 2500))
			{
				timePassed += 1;
			}
			else
			{
				if(timePassed > 0)
				{
					timePassed -= 1;
				}
			}
			break;

		case (4):
			if ((adc_val >= 500) && (adc_val < 1500))
			{
				timePassed += 1;
			}
			else
			{
				if(timePassed > 0)
				{
					timePassed -= 1;
				}
			}
			break;

		default:
			break;
	}

	
	if (timePassed > 140)
	{
		if (stage == 4)
		{
			lcd_clear();
			lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
			lcd_send_string("Stage Clear!");        
			lcd_put_cursor(1, 0);                   // Set the cursor position to the second row, first column
			lcd_send_string("Loading next...");     
			vTaskDelay(pdMS_TO_TICKS(3000));


			if(gamestateCmd(readGS) == GS1)
			{
				gamestateCmd(nextGS);
			}

			lcd_clear();
			lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
			lcd_send_string("Stage 2: Toggle");        
			lcd_put_cursor(1, 0);                   // Set the cursor position to the second row, first column
			lcd_send_string("the Switches!");     
			vTaskDelay(pdMS_TO_TICKS(3000));

			return;
		}
			

		printf("Next\n");
		lcd_clear();
		lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
		lcd_send_string("Clear!");         
		vTaskDelay(pdMS_TO_TICKS(1500));

		lcd_clear();
		lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
		lcd_send_string("Stage 1: Use the");        	// Display the string "Hello world!!!" on the LCD
		lcd_put_cursor(1, 0);                   // Set the cursor position to the second row, first column
		lcd_send_string("crank!  Level: ");     	// Display the string "from @voidlooop" on the LCD
        lcd_send_data((char)(stage + 0x32));

		stage += 1;
		timePassed = 0;
	}
		

	return;
}

uint8_t gamestate2Encode()
{
	ToggleValues tVal[numofBits];
	uint16_t toToggle = 0;
	static uint8_t stage = 0;
	uint16_t toggleBits1[] = {0b0000000010010011, 
							  0b0000000000101100, 
							  0b0000000000100010, 
							  0b0000000010101000,
							  0b0000000001000101, 
							  0b0000000010100011, 
							  0b0000000001100101, 
							  0b0000000011001000}; // Stage 1 LED toggles [tk]
	uint16_t toggleBits2[] = {0b0000000000011001, 
							  0b0000000001001100, 
							  0b0000000001010101, 
							  0b0000000011010000,
							  0b0000000000100101, 
							  0b0000000010100100, 
							  0b0000000001001101, 
							  0b0000000010010110}; // Stage 2 LED toggles [tk]
	uint16_t toggleBits3[] = {0b0000000000010101, 
							  0b0000000000101010, 
							  0b0000000001010100, 
							  0b0000000010101000,
							  0b0000000001010001, 
							  0b0000000010100010, 
							  0b0000000001000101, 
							  0b0000000010001010}; // Stage 3 LED toggles [tk]

	for (int i = 0; i < numofBits; i++)
	{
		tVal[i].checkedBit = (0b0000000000000001 << i);
		switch(stage)
		{
			case (0):
				tVal[i].toggles = toggleBits1[i];
				break;

			case (1):
				tVal[i].toggles = toggleBits2[i];
				break;

			case (2):
				tVal[i].toggles = toggleBits3[i];
				break;

			default:
				break;
		}

		if (tVal[i].checkedBit & bitBuffer)
		{
			toToggle = toToggle ^ tVal[i].toggles;
		}
	}

	ledUpdate(toToggle);
	if(toToggle == 0x00FF)
	{
		stage++;

		if(stage == 3)
		{
			return 0xFF;
		}

		lcd_clear();
		lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
		lcd_send_string("Clear!");        
		lcd_put_cursor(1, 0);                   // Set the cursor position to the second row, first column
		lcd_send_string("Next level.");     
		vTaskDelay(pdMS_TO_TICKS(3000));

		lcd_clear();
		lcd_put_cursor(0, 0);                   // Set the cursor position to the first row, first column
		lcd_send_string("Stage 2: Toggle");        
		lcd_put_cursor(1, 0);                   // Set the cursor position to the second row, first column
		lcd_send_string("the Switches!");     
	}

	return 0;
}

void stationInit()
{
	gamestateCmd(resetGS);
	ledUpdate(0x0000);
	lcd_init();
	LED_RegInit();
	resetReg();

	return;
}

void ledUpdate(uint16_t state)
{
    // For the upper 4 LEDs
    gpio_set_level(LED0_PIN, (state && 0b0000000100000000));
    gpio_set_level(LED1_PIN, (state && 0b0000001000000000));
    gpio_set_level(LED2_PIN, (state && 0b0000010000000000));
    gpio_set_level(LED3_PIN, (state && 0b0000100000000000));


	// Update 8-bit SIPO Shift Register
    uint8_t buffer = (state % 256);
	updateReg(~buffer);

	return;
}

int checkPass()
{
	static uint8_t passNum = 0;					// Stage number
	uint8_t Pass1[] = {0x45, 0x6d, 0x70, 0x65,
					   0x72, 0x6f, 0x72, 0x00};	// First Pass: Emperor
	uint8_t Pass2[] = {0x4E, 0x65, 0x77, 0x59,
                       0x65, 0x61, 0x72, 0x73};	// Second Pass: NewYears
	uint8_t Pass3[] = {0x48, 0x65, 0x72, 0x69,
                       0x74, 0x61, 0x67, 0x65};	// Third Pass: Heritage
	uint8_t Pass4[] = {0x45, 0x4e, 0x49, 0x54,
					   0x49, 0x4f, 0x32, 0x36};	// Fourth Pass: ENITIO26

	switch(passNum)
	{
		case (0):
			for(int i = 0; i < passLen; i++)
			{
				if(inputPass[i] != Pass1[i])
				{
					printf("%i", i);
					lcd_put_cursor(1, 8);
					lcd_send_string("        ");
					return 0;
				}
			}
			printf("first pass completed");
            lcd_put_cursor(0, (12 + passNum));
            lcd_send_data((char)(0xff));
			ledUpdate(0x0FFF);
			passNum++;
			break;

		case (1):
			for(int i = 0; i < passLen; i++)
			{
				if(inputPass[i] != Pass2[i])
				{
					lcd_put_cursor(1, 8);
					lcd_send_string("        ");
					return 0;
				}
			}
			printf("second pass completed");
            lcd_put_cursor(0, (12 + passNum));
            lcd_send_data((char)(0xff));
			ledUpdate(0x0FFF);
			passNum++;
			break;

		case (2):
			for(int i = 0; i < passLen; i++)
			{
				if(inputPass[i] != Pass3[i])
				{
					lcd_put_cursor(1, 8);
					lcd_send_string("        ");
					return 0;
				}
			}
			printf("third pass completed");
            lcd_put_cursor(0, (12 + passNum));
            lcd_send_data((char)(0xff));
			ledUpdate(0x0FFF);
			passNum++;
			break;

		case (3):
			for(int i = 0; i < passLen; i++)
			{
				if(inputPass[i] != Pass4[i])
				{
					lcd_put_cursor(1, 8);
					lcd_send_string("        ");
					return 0;
				}
			}
			printf("last pass completed");
            lcd_put_cursor(0, (12 + passNum));
            lcd_send_data((char)(0xff));
			ledUpdate(0x0FFF);
			passNum++;
			break;

		default:
			return 0x00;
			break;
	}

	if(passNum == 4)
	{
		return 1;
	}

	lcd_put_cursor(1, 8);
	lcd_send_string("        ");
	
	return 0;
}

void HC_Charge(int adc_value)
{
	uint16_t reading = adc_value;
    
	switch((reading) / 500)
	{
		case (1):
			ledUpdate(0x0001);
			break;

		case (2):
			ledUpdate(0x0003);
			break;
;
		case (3):
			ledUpdate(0x0007);
			break;

		case (4):
			ledUpdate(0x000F);
			break;

		case (5):
			ledUpdate(0x001F);
			break;

		case (6):
			ledUpdate(0x003F);
			break;

		case (7):
			ledUpdate(0x007F);
			break;

		case (8):
			ledUpdate(0x00FF);
			break;

		default:
			ledUpdate(0x0000);
	}
}

static void gpio_isr_handler(void *arg)
{
	#ifdef debug
 	// static uint8_t test = 0;
	#endif

    int pin = (int)arg; 		// INTR pin trigger, if more button/exti added
    
    // IRQ Handler based on pin
    if(pin == BUTTON_PIN)
    {
		gpio_intr_disable(BUTTON_PIN);
		
		TickType_t now = xTaskGetTickCountFromISR();

		// Ignore interrupts within 30 ms of the previous one
		if ((now - lastInterruptTick) < pdMS_TO_TICKS(200))
		{
			gpio_intr_enable(BUTTON_PIN);
			return;
		}

		lastInterruptTick = now;

		isr_debounce = 1;
		
		#ifdef debug
		// gpio_set_level(GPIO_NUM_2, 1);
		#endif

		
		gpio_intr_enable(BUTTON_PIN);
	}
}

void ENI26_GPIO_Init()
{
	gpio_config_t bit_conf = 
		{
			.pin_bit_mask = (1ULL << BIT0_PIN) |		// Select GPIOs
							(1ULL << BIT1_PIN) |
							(1ULL << BIT2_PIN) |
							(1ULL << BIT3_PIN) |
							(1ULL << BIT4_PIN) |
							(1ULL << BIT5_PIN) |
							(1ULL << BIT6_PIN) |
							(1ULL << BIT7_PIN),      
			.mode = GPIO_MODE_INPUT,            		// Set as input
			.pull_up_en = GPIO_PULLUP_ENABLE,  			// Enable pull-up
			.pull_down_en = GPIO_PULLDOWN_DISABLE,  	// Disable pull-down
			.intr_type = GPIO_INTR_DISABLE             	// Disable interrupts
		};
		
		gpio_config_t button_conf = 
		{
			.pin_bit_mask = (1ULL << BUTTON_PIN),		// Select GPIOs
			.mode = GPIO_MODE_INPUT,            		// Set as input
			.pull_up_en = GPIO_PULLUP_ENABLE,  			// Enable pull-up
			.pull_down_en = GPIO_PULLDOWN_DISABLE,  	// Disable pull-down
			.intr_type = GPIO_INTR_NEGEDGE             	// Enable interrupts (falling edge trigger)
		};
		
		gpio_config_t led_config = 
		{
			.pin_bit_mask = (1ULL << LED0_PIN) |		// Select GPIOs
							(1ULL << LED1_PIN) |
							(1ULL << LED2_PIN) |
							(1ULL << LED3_PIN),                
			.mode = GPIO_MODE_OUTPUT,            		// Set as output
			.pull_up_en = GPIO_PULLUP_DISABLE,  		// Disable pull-up
			.pull_down_en = GPIO_PULLDOWN_DISABLE,  	// Disable pull-down
			.intr_type = GPIO_INTR_DISABLE             	// Disable interrupts
		};

		#ifdef debug
		gpio_config_t debug_conf = 
		{
			.pin_bit_mask = (1ULL << GPIO_NUM_2),		// Select GPIOs
			.mode = GPIO_MODE_OUTPUT,            		// Set as output
			.pull_up_en = GPIO_PULLUP_DISABLE,  		// Disable pull-up
			.pull_down_en = GPIO_PULLDOWN_DISABLE,  	// Disable pull-down
			.intr_type = GPIO_INTR_DISABLE             	// Disable interrupts
		};
		
		ESP_ERROR_CHECK(gpio_config(&debug_conf));
		#endif


		ESP_ERROR_CHECK(gpio_config(&bit_conf));
		ESP_ERROR_CHECK(gpio_config(&button_conf));
		ESP_ERROR_CHECK(gpio_config(&led_config));

		return;
}

void ENI26_ADC_Init(adc_oneshot_unit_handle_t *adc_handle)
{
	// Initialize ADC Oneshot Mode Driver on the ADC Unit
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, adc_handle));

    // Configure ADC channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_handle, ADC_PIN, &config));

	return;
}
	


// Last Updated on 21th Aug 2026 by TenshiMyLove

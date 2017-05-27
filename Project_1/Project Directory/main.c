#include "stm32l476xx.h"
#include "UART.h"
#include "SysClock.h"

#include <string.h>
#include <stdio.h>

#define POST_MAX_TIME 100000
#define DEFAULT_LOW_LIMIT 950
#define UPPER_LIMIT_RANGE 100
#define NUMBER_OF_SAMPLES 1000

int POST(void);
int ChangeLimit(void);
void GPIO_Conf(void);
void TimerConf(void);
void PrintData(int lowLimit, int data[]);
int AskForNewTest(void);


/*


*************************************  32L476GDISCOVERY ***************************************************************************
// STM32L4:  STM32L476VGT6 MCU = ARM Cortex-M4 + FPU + DSP, 
//           LQFP100, 1 MB of Flash, 128 KB of SRAM
//           Instruction cache = 32 lines of 4x64 bits (1KB)
//           Data cache = 8 lines of 4x64 bits (256 B)
//
// Joystick (MT-008A): 
//   Right = PA2        Up   = PA3         Center = PA0
//   Left  = PA1        Down = PA5
//
// User LEDs: 
//   LD4 Red   = PB2    LD5 Green = PE8
//   
// CS43L22 Audio DAC Stereo (I2C address 0x94):  
//   SAI1_MCK = PE2     SAI1_SD  = PE6    I2C1_SDA = PB7    Audio_RST = PE3    
//   SAI1_SCK = PE5     SAI1_FS  = PE4    I2C1_SCL = PB6                                           
//
// MP34DT01 Digital MEMS microphone 
//    Audio_CLK = PE9   Audio_DIN = PE7
//
// LSM303C eCompass (a 3D accelerometer and 3D magnetometer module): 
//   MEMS_SCK  = PD1    MAG_DRDY = PC2    XL_CS  = PE0             
//   MEMS_MOSI = PD4    MAG_CS  = PC0     XL_INT = PE1       
//                      MAG_INT = PC1 
//
// L3GD20 Gyro (three-axis digital output): 
//   MEMS_SCK  = PD1    GYRO_CS   = PD7
//   MEMS_MOSI = PD4    GYRO_INT1 = PD2
//   MEMS_MISO = PD3    GYRO_INT2 = PB8
//
// ST-Link V2 (Virtual com port, Mass Storage, Debug port): 
//   USART_TX = PD5     SWCLK = PA14      MFX_USART3_TX   MCO
//   USART_RX = PD6     SWDIO = PA13      MFX_USART3_RX   NRST
//   PB3 = 3V3_REG_ON   SWO   = PB3      
//
// Quad SPI Flash Memory (128 Mbit)
//   QSPI_CS  = PE11    QSPI_D0 = PE12    QSPI_D2 = PE14
//   QSPI_CLK = PE10    QSPI_D1 = PE13    QSPI_D3 = PE15
//
// LCD (24 segments, 4 commons)
//   VLCD = PC3
//   COM0 = PA8     COM1  = PA9      COM2  = PA10    COM3  = PB9
//   SEG0 = PA7     SEG6  = PD11     SEG12 = PB5     SEG18 = PD8
//   SEG1 = PC5     SEG7  = PD13     SEG13 = PC8     SEG19 = PB14
//   SEG2 = PB1     SEG8  = PD15     SEG14 = PC6     SEG20 = PB12
//   SEG3 = PB13    SEG9  = PC7      SEG15 = PD14    SEG21 = PB0
//   SEG4 = PB15    SEG10 = PA15     SEG16 = PD12    SEG22 = PC4
//   SEG5 = PD9     SEG11 = PB4      SEG17 = PD10    SEG23 = PA6
// 
// USB OTG
//   OTG_FS_PowerSwitchOn = PC9    OTG_FS_VBUS = PC11     OTG_FS_DM = PA11  
//   OTG_FS_OverCurrent   = PC10   OTG_FS_ID   = PC12    OTG_FS_DP = PA12  
//
// PC14 = OSC32_IN      PC15 = OSC32_OUT
// PH0  = OSC_IN        PH1  = OSC_OUT 
// 
// PA4  = DAC1_OUT1 (NLMFX0 WAKEUP)   PA5 = DAC1_OUT2 (Joy Down)
// PA3  = OPAMP1_VOUT (Joy Up)        PB0 = OPAMP2_VOUT (LCD SEG21)
//
****************************************************************************************************************
*/


int main(void){
	
	uint8_t buff[100];
	int n = 0, i = 0;
	int lowLimite = DEFAULT_LOW_LIMIT;
	int data[UPPER_LIMIT_RANGE + 1];
	long prevAns = 0;
	long currentValue = 0;
	int newTest = 1;
	
	
	// Copied from Zhu's lab_01_LEDS pushbutton C main.c code
	//
	// Enable High Speed Internal Clock (HSI = 16 MHz)
	RCC->CR |= ((uint32_t)RCC_CR_HSION);
	
	// wait until HSI is ready
	while( (RCC->CR & (uint32_t) RCC_CR_HSIRDY) == 0 ) {;}
	
	// Select HSI as system clock source 
	RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
	RCC->CFGR |= (uint32_t)RCC_CFGR_SW_HSI;     // 01: HSI16 oscillator used as system clock

	// Wait till HSI is used as system clock source 
	while((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) == 0 ) {;}
	
	/* INITIALIZATION */
	System_Clock_Init();
	UART2_Init();
	GPIO_Conf();
	TimerConf();
	
	/* POST */
	n = sprintf((char*)buff, "\r\n\nPOST...\r\n");
	USART_Write(USART2, buff, n);	
	while(POST()) {
		n = sprintf((char*)buff, "POST failed, press Enter to try again");
		USART_Write(USART2, buff, n);	
		while(USART_Read(USART2) != 13);	// Wait for the Enter Key to be pressed
		n = sprintf((char*)buff, "\r\n");
		USART_Write(USART2, buff, n);
	}
	n = sprintf((char*)buff, "POST successful !\r\n");
	USART_Write(USART2, buff, n);	

	while(newTest) {
	
		/* LIMITE CHANGE */
		lowLimite = ChangeLimit();

		/* TEST BEGIN */
		for(i=0; i<=UPPER_LIMIT_RANGE; i++) {data[i]=0;}
		n = sprintf((char*)buff, "Press Enter to start the Test");
		USART_Write(USART2, buff, n);
		while(USART_Read(USART2) != 13);	// Wait for the Enter Key to be pressed
		n = sprintf((char*)buff, "\r\n");
		USART_Write(USART2, buff, n);

		/* TEST*/
		while(!(TIM2->SR & 0x2));
		prevAns = TIM2->CCR1;
		for(i=0; i<NUMBER_OF_SAMPLES; i++) {
			while(!(TIM2->SR & 0x2));
			currentValue = TIM2->CCR1 - prevAns;
			if(currentValue>=lowLimite && currentValue<=lowLimite+UPPER_LIMIT_RANGE) {
				data[currentValue-lowLimite]++;
			}
			prevAns = TIM2->CCR1;
		}
		
		/* PRINT DATA */
		PrintData(lowLimite, data);
		
		/* NEW TEST */
		newTest = AskForNewTest();
	}

	while(1);
}


/*
Function in charge of operating the Power On Self Test
Return: 0 if successful, -1 if not
*/
int POST(void) {
	RCC->APB1RSTR1 &= ~(0x1);				// Reset the timer value
	TIM2->CCR1 = 0;							// Clear the Timer Capture Register
	TIM2->SR &= ~(0x2);						// Clear the Timer Capture Flag
	while(!(TIM2->SR & 0x2) && TIM2->CNT < POST_MAX_TIME);
	if(TIM2->CCR1 != 0) return 0;
	else return -1;
}

/*
Communicate with the user and return the new value of the low limit
Return: the new value of the lower limite of the time range
*/
int ChangeLimit(void) {
	int n;
	uint8_t buff[100], ans;
	int correctAnswer = 0;
	
	n = sprintf((char*)buff, "Would you like to change the lower limit (default=950us) ? (y/n)");
	USART_Write(USART2, buff, n);

	while(!correctAnswer) {
		ans = USART_Read(USART2);
		if (ans == 'n' || ans == 'N' || ans == 'y' || ans == 'Y') { 
			correctAnswer = 1;
		}
	}
	
	if(ans == 'n' || ans == 'N') {
		n = sprintf((char*)buff, " NO\r\n");
		USART_Write(USART2, buff, n);	
		
		return DEFAULT_LOW_LIMIT;
	}
	else {
		int newLimit = DEFAULT_LOW_LIMIT;
		n = sprintf((char*)buff, " YES\r\n");
		USART_Write(USART2, buff, n);
		n = sprintf((char*)buff, "Low limit (50 - 9950) : ");
		USART_Write(USART2, buff, n);
		
		correctAnswer = 0;
		while(!correctAnswer){
			int value = 0;
			do {
				ans = USART_Read(USART2);
				if(ans>='0' && ans<='9') {
					value = value*10 + ans - 48; 		//ASCII offset for '0' is 48
					USART_Write(USART2, &ans, 1);
				}
			}while(ans != 13);							// While the Enter Key is not pressed
			if(value>=50 && value <=9950) {
				correctAnswer = 1;
				newLimit = value;
			}
			else {
				n = sprintf((char*)buff, "\r\nInvalid value, try again : ");
				USART_Write(USART2, buff, n);
			}
		}
		
		n = sprintf((char*)buff, "\r\nThe new limits are %d and %dus\r\n", newLimit, newLimit+UPPER_LIMIT_RANGE);
		USART_Write(USART2, buff, n);
		return newLimit;
	}
}

/*
Function in charge of configuring the pin 0 of the GPIO port A in order to work in Alternate Function mode.
*/
void GPIO_Conf(void) {
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;				// Enable the clock to GPIO Port A
	GPIOA->MODER = (GPIOA->MODER & ~0x3) | 0x2;			// Set PA0 as AF
	GPIOA->AFR[0] = (GPIOA->AFR[0] & ~0xF) | 0x1;		// Select the right AF	
	GPIOA->PUPDR = (GPIOA->PUPDR & ~0x3) | 0x2;			// Configure PA0 as pull-down
}

/*
Function in charge of configuring the timer in Capture mode and with the correct prescaler
*/
void TimerConf(void) {
	//Timer Configuration
	RCC->APB1ENR1 |= 0x1;				// Enable Clock for TIM2
	TIM2->PSC = 80;						// Change the prescaler value
	TIM2->EGR |= 0x0001;				// Set update flag
	
	// Timer Capture Configuration
	TIM2->CCER &= ~(0x1 << 0);						// Disable the capt/compare input
	TIM2->CCMR1 = (TIM2->CCMR1 & ~0xFF) | 0x1;		// Set the filter to 0 and set the timer in capture mod
	TIM2->CCER |= (0x1 << 0);						// Enable the capt/compare input
	
	TIM2->CR1 |= 0x1;					// Start counting
}

/*
Print the histogram through the Serial Terminal
Only values different to 0 are printed
Inputs:
	int lowLimit: 	the low limit in time range (in milliseconds)
	int data[]:		array of data to print
*/
void PrintData(int lowLimit, int data[]) {
	uint8_t buff[800];
	int n = 0, i;
	int total = 0;
	
	n = sprintf((char *)buff , "\r\nResults :\r\n");
	USART_Write(USART2, buff, n);
	n=0;
	
	for(i=0; i<=UPPER_LIMIT_RANGE; i++) {
		if(data[i] != 0) {n += sprintf((char *)buff + n , "\t%5d : %d\r\n", lowLimit+i, data[i]);}
		total += data[i];
	}
	USART_Write(USART2, buff, n);

	n = sprintf((char *)buff , "%d in range out of %d\r\n", total, NUMBER_OF_SAMPLES);
	USART_Write(USART2, buff, n);
}

/*
Ask the user if he want to renew the test
Return: 0 if "no", 1 if "yes"
*/
int AskForNewTest(void) {
	int n;
	uint8_t buff[100], ans;
	int correctAnswer = 0;
	
	n = sprintf((char*)buff, "Would you like to restart the test ? (y/n) ");
	USART_Write(USART2, buff, n);

	while(!correctAnswer) {
		ans = USART_Read(USART2);
		if (ans == 'n' || ans == 'N' || ans == 'y' || ans == 'Y') { 
			correctAnswer = 1;
		}
	}
	
	if(ans == 'n' || ans == 'N') {
		n = sprintf((char*)buff, " NO\r\n\n");
		USART_Write(USART2, buff, n);	
		return 0;
	}
	else {
		n = sprintf((char*)buff, " YES\r\n\n");
		USART_Write(USART2, buff, n);
		return 1;
	}
}

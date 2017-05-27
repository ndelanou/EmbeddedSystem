#include <stm32l476xx.h>
#include "UART.h"
#include "SysClock.h"

#include "recipe.h"

#include <string.h>
#include <stdio.h>

#define MAX_CMD_VALUE 2

void GPIO_Init(void);
void Timer_Interrupt_Init(void);
void PWM_Init(void);
void User_Interaction(uint8_t car);
void Cmd_Management(uint8_t cmd[]);
void LED_Management(void);
void Sync_Management(void);

struct Engine eng1, eng2;

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
	int n=0;
	
	
	/* INITIALIZATION */
	System_Clock_Init();
	UART2_Init();
	GPIO_Init();
	Timer_Interrupt_Init();
	PWM_Init();
	
	/* ENGINE CONFIGURATION */
	Engine_Init(&eng1, CHANNEL_1_id);
	Fill_Test_Recipe1(&eng1);
	Engine_Init(&eng2, CHANNEL_2_id);
	Fill_Test_Recipe2(&eng2);	
		
	n = sprintf((char*)buff, "\n\n\rNew Test...\r\n>");
	USART_Write(USART2, buff, n);
		
	/* IDLE FUNCTION */
	while(1) {
		User_Interaction(USART_Read(USART2));
	}
}

/*
	Interruption function called every 100ms by the timer5
*/
void TIM5_IRQHandler (void) {
	TIM5->SR &= ~0x3;		// Reset IT flags

	Engine_Management(&eng1);
	Engine_Management(&eng2);
	
	LED_Management();
	Sync_Management();
}

/*
	Function in charge of configuring the pin 0 of the GPIO port A in order to work in Alternate Function mode.
*/
void GPIO_Init(void) {
	// Enable clocks on GPIO Ports A, B and E
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
	
	GPIOA->MODER = (GPIOA->MODER & ~0xF) | 0xA;				// Set PA0 and PA1 as AF
	GPIOA->AFR[0] = (GPIOA->AFR[0] & ~0xFF) | 0x11;		// Select the right AF for PA0 and PA1
	GPIOA->OTYPER &= ~0x3;		// Configure PA0 and PA1 as push-pull outputs
	GPIOA->PUPDR &= ~0xF;			// Configure PA0 and PA1 as no pull-up, pull-down
		
	// Set PB2 & PE8 as output
	GPIOB->MODER &= ~(0x03<<(2*2));		 
  GPIOB->MODER |= (1 << (2*2));
	GPIOE->MODER &= ~(0x03<<(2*8));		 
  GPIOE->MODER |= (1 << (2*8));
}

/*
	Configuration of the timer5 in order to generate an interrupt every 100ms
*/
void Timer_Interrupt_Init(void) {
	//Timer Configuration
	RCC->APB1ENR1 |= 0x8;				// Enable Clock for TIM5
	TIM5->PSC = 8000;						// Change the prescaler value
	TIM5->ARR = 1000;						// Auto-reset value
	
	// Interrupt
	TIM5->DIER |= TIM_DIER_UIE;			// Enable interrupt creation on CCI1 and update the register
	
	TIM5->EGR |= 0x1;								// Set update flag
	TIM5->CR1 = TIM_CR1_CEN;   			// Enable timer
	
	NVIC_EnableIRQ(TIM5_IRQn);			// Enable Timer 5 Interrupt
}

/*
	Configuration of the timer2 in order to generate two PWM signals with a frequency of 20ms
	Initial position : POSITION_0
*/
void PWM_Init(void) {
	RCC->APB1ENR1 |= 0x1;				// Enable Clock for TIM2
	
	TIM2->PSC = 0;							// Change the prescaler value
	TIM2->EGR |= 0x0001;				// Set update flag
	
	// Timer Capture Configuration
	TIM2->CCER &= ~0x11;																	// Disable the capt/compare input
	TIM2->CCMR1 = (TIM2->CCMR1 & ~0x0101FFFF) | 0x6060;		// Set channel 1 and 2 with filter to 0 and set the timer in compare mode
	TIM2->CCER |=  0x11;																	// Enable the capt/compare input
	
	TIM2->ARR = AUTORESET_20MS_80MHZ;				// Set the AutoReset value for a 20ms period with 80MHz
	TIM2->CCR1 = TIM2->CCR2 = POSITION_0;		// Set to initial position
	
	TIM2->CR1 |= 0x1;												// Start counting
}

/*
	Function in charge of the interaction with the user by the Serial Terminal
*/
void User_Interaction(uint8_t car) {
	static uint8_t cmd[MAX_CMD_VALUE];
	static int index_cmd;
	
	if(car == 127 && index_cmd > 0) {												// Backspace
		USART_Write(USART2, (uint8_t *) 26, 2);
		USART_Write(USART2, (uint8_t *) " ", 2);
		USART_Write(USART2, (uint8_t *) 26, 2);
		index_cmd--;
		car = 0;
	} 
	else if (car == 13 && index_cmd == MAX_CMD_VALUE) {			// Enter
		index_cmd = 0;
		USART_Write(USART2, (uint8_t *) "\n\r>", 3);
		Cmd_Management(cmd);
	}
	else if(index_cmd < MAX_CMD_VALUE && ((car >= 65 && car <= 90) || (car >= 97 && car <= 122)) ){
		USART_Write(USART2,  &car, 2);
		cmd[index_cmd] = car;
		index_cmd++;
	}
}

/*
	Function in charge of calling the right function after the user pressed enter with a new command
*/
void Cmd_Management(uint8_t cmd[]) {
	if(cmd[0]>90) {cmd[0] -= 32;}
	if(cmd[1]>90) {cmd[1] -= 32;}
	
	switch (cmd[0]) {
		case 'C' : eng1.in_Pause = 0;
			break;
		
		case 'P' : eng1.in_Pause = 1;
			break;

		case 'L' : goLeft(&eng1);
			break;

		case 'R' : goRight(&eng1);
			break;

		case 'B' : Restart_Recipe(&eng1);
			break;

		case 'N' : 
			break;
			
		default:
			break;
	}
	
	switch (cmd[1]) {
		case 'C' : eng2.in_Pause = 0;
			break;
		
		case 'P' : eng2.in_Pause = 1;
			break;

		case 'L' : goLeft(&eng2);
			break;

		case 'R' : goRight(&eng2);
			break;

		case 'B' : Restart_Recipe(&eng2);
			break;

		case 'N' : 
			break;
			
		default:
			break;
	}
}

/*
	Function giving the state of the program by switching LEDs on or off
*/
void LED_Management(void) {
	if(!eng1.recipe_ended || !eng2.recipe_ended) {
		// Normal
		if((!eng1.in_Pause && !eng1.recipe_ended)||(!eng2.in_Pause && !eng2.recipe_ended)){
			GPIOB->ODR &= ~GPIO_ODR_ODR_2;
			GPIOE->ODR |= GPIO_ODR_ODR_8;
		}
		else {	// Pause
			GPIOB->ODR |= GPIO_ODR_ODR_2;
			GPIOE->ODR &= ~GPIO_ODR_ODR_8;
		}
	}
	else {		// Recipe End
		GPIOB->ODR &= ~GPIO_ODR_ODR_2;
		GPIOE->ODR &= ~GPIO_ODR_ODR_8;
	}
}

/*
	Function checking if the two engine are on Sync mode
*/
void Sync_Management(void) {
	if(eng1.in_Sync && eng2.in_Sync) {
		eng1.in_Sync = eng2.in_Sync = 0;
	}
}

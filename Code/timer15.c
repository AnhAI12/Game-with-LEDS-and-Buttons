#include "stm32f4xx.h"                  // Device header
#include <stdlib.h>

volatile int num_rand;
volatile uint16_t counter_1s, duty, duty2, counter_redled, i, time_led, counter_hold_2s;
volatile char flag_red_led, flag_btn3_2s;

void rand_led()
{
	num_rand = 20 + rand()%81;
}

///////////////////////RESET SYSTEM ////////
void reset_system()
{
	TIM9->CCER &= ~1;											// Disable CC1E PWM CH1
	TIM9->CCER &= ~(1<<4);								//Disable CC2E PWM CH2
	TIM9->CR1 &= ~(1<<0); 								//enable timer
	GPIOA->ODR &= ~1;
	TIM9->CNT=0;
	flag_red_led = 0;
	counter_redled=0;
	counter_1s=0;
	num_rand=0;
	counter_hold_2s=0;
	time_led=0;
}

//////////////////// check button ///////////
void check_btn()
{
		if( GPIOB->IDR & 1 )
		{
			//for(int k =0 ; k<50000; k++);
			while( GPIOB->IDR & 1);
			if( duty <100)
			{
				TIM9->CCER |= 1;									//enable PWM CH1
				duty = 50;
				TIM9->CCR1 =duty;
			}
			i++;																	//check debug counter press button
		}
}

void check_btn2()
{
		if( GPIOB->IDR & (1<<1) )
		{
			//for(int k =0 ; k<50000; k++);
			while( GPIOB->IDR & (1<<1));					//check button pressed
			TIM9->CCER |= 1;											//enable PWM CH1
			TIM9->CCER |= (1<<4);								//enable CC2E PWM CH2
			TIM9->DIER |= (1<<0); 								//enable interrupt	
			TIM9->CR1 |= (1<<0);									//enable TIM9
		}
}

void check_btn3()
{
	if( GPIOB->IDR & (1<<2) )
		{
			while( GPIOB->IDR & (1<<2));					//check button pressed
			//reset system//
			reset_system();
		}
}

void check_btn3_2()
{
	if( GPIOB->IDR & (1<<2) )
		{
			flag_btn3_2s=1;
		}
		else flag_btn3_2s=0;
}

/////////////////////////////// TIM9 INTERRUPT ////////////////////////////
void TIM1_BRK_TIM9_IRQHandler(void)
{
	TIM9->SR = 0;					// Flag = 0
	time_led++;
	
	if(time_led==200)			//2s random
	{
		if(flag_red_led ==0) rand_led();
		time_led=0;
	}
	
	/////////// green led /////
	if(num_rand <=80 && counter_1s <90)
	{
		TIM9->CCER |= 1;											// Disable CC1E PWM CH1
		duty=10 + counter_1s;
		TIM9->CCR1 =duty;
		counter_1s++;
	}
	if(counter_1s >= 90) 
	{
		counter_1s=1;
		duty=10;
	}
	
	////////////// red led //////////////
	if(num_rand >80 && ((time_led%25)==0) && counter_redled <=400)
	{
		TIM9->CCER ^= 1<<4; 	//enable CC2E
		flag_red_led=1;
	}
	
	/////////////check button 3  - reset system////
	if( flag_red_led ==1 )
	{
		counter_redled++;			//counter check 4s
		
		//check button hold 2s
		if(flag_btn3_2s==1)
		{
			counter_hold_2s++;
			if(counter_hold_2s>=200)
			{
				reset_system();
				flag_btn3_2s=0;
				flag_red_led=0;
			}
		}
		
		//
		if( counter_redled <= 400 )
		{
			check_btn3();
		}
		else {
			// Red led off
			TIM9->CCER &= ~(1<<4);								//Disable CC2E PWM CH2
			GPIOA->ODR |= 1;											//blue led ON
			check_btn3_2();												//check_btn3 pressed 2s	
		}
	}
}

int main(void)
{
	//HCLK 16MHz Default
	//1. Enable RCC, PA2, TIMER15
	RCC->AHB1ENR |= 1; //GPIOA
	RCC->AHB1ENR |= 1<<1;
	RCC->APB2ENR |= (1<<16); 							//timer 9
	//2. Config PA2 alternate function
	GPIOA->MODER |= (1<<5);
	GPIOA->AFR[0] |= (3<<8);
	//C. Config PA3 AF3 TIMER9_CH2
	GPIOA->MODER |= (1<<7); 							//alternate 
	GPIOA->AFR[0] |= (3<<12);							//AF3
	//d. Config PA0 output general
	GPIOA->MODER |= 1;
	//b. config PB0 PB1 input
	GPIOB->MODER &= ~(15<<0);
	GPIOB->MODER &= ~(3<<4);							//PB2 
	
	//4. config timer 9
	TIM9->PSC=1600; //100us
	TIM9->CR1 &= ~(1<<3); 								//NOT STOPPED UPDATE COUNTER
	//TIM9->DIER |= (1<<0); 							//enable interrupt

	TIM9->CNT=0;
	//6. config pwm mode TIM9_CH1
	TIM9->CCMR1 &= ~(3<<0); 							//cc1 output
	TIM9->CCER &=~ (1<<1); 								//oc1 high active
	TIM9->CCMR1 |= (3<<5); 								//pwm mode 1
	//SET ARR (period), CCR1,
	duty = 50;
	TIM9->ARR =100; 											//10ms interrupt
	TIM9->CCR1 =duty;
	TIM9->CCMR1 |= (1<<3); 								//Preload register_counter enable + cho phep thay doi gia tri CCR1 
	//TIM9->ARPE = 0 TRONG CR1. LA cho phep cap nhat gia trij ARR moiws ngay lap trinh
	TIM9->CR1 |= 1<<7;										//enable auto reload

	//6. enable
	TIM9->CCER |= 1; //enable CC1E
	TIM9->CR1 |= 1<<0; //enable timer
	
		//3.config NVIC timer 9
	__enable_irq();
	NVIC_SetPriority(TIM1_BRK_TIM9_IRQn,0);
	NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);
	
	//8. Config TIM9_CH2
	TIM9->CCMR1 &= ~(3<<8); 			//cc2s output
	TIM9->CCER &=~ (1<<5); 				//oc2 high active
	TIM9->CCMR1 |= (3<<13); 			//pwm CH2 mode 1
	//SET ARR (period), CCR1,
	duty2 = 80; 									//80%
	TIM9->ARR =100; //10ms interrupt
	TIM9->CCR2 =duty2;
	TIM9->CCMR1 |= (1<<11); 			//Preload register_counter enable + cho phep thay doi gia tri CCR1 
	//TIM9->ARPE = 0 TRONG CR1. LA cho phep cap nhat gia trij ARR moiws ngay lap tuc
	//TIM9->CR1 |= 1<<7;						//enable auto reload, config ben tren

	//9. enable
	//TIM9->CCER |= 1<<4; //enable CC2E
	TIM9->CR1 |= 1<<0; //enable timer
	
		//10.config NVIC timer 9
	__enable_irq();
	NVIC_SetPriority(TIM1_BRK_TIM9_IRQn,0);
	NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);
	
	//7. Init
	i=0;
	time_led=0;
	counter_1s =1;
	counter_redled=0;
	GPIOA->ODR &=~(1);
	while(1)
	{
		check_btn();
		check_btn2();

	}
}
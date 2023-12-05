#include "main.h"

#define DMA_Stream DMA2_Stream5

#define SIZE 10 // Размер буфера для передачи данных
uint8_t counter = 0;
volatile uint32_t  SysTick_Count = 0;


uint16_t buf[SIZE] = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020};



void  SysTick_Handler(void)
 	     {
 	    	if (SysTick_Count > 0){
 	    		SysTick_Count --;
 	    	}
 	    	}

 void delay_ms(int ms){

 	 SysTick_Count = ms;
 	 while (SysTick_Count){}

 	 }





void GPIO_Init(){

		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; //RCC on for GPIO A
		GPIOA->MODER &= ~0x00000C00; /* clear pin mode */
		GPIOA->MODER |= GPIO_MODER_MODER5_0; /* set pin to output mode */
		GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED5_0 | GPIO_OSPEEDR_OSPEED5_1 ); // very high
		GPIOA->BSRR = GPIO_BSRR_BS5; // установили в высокое состояние


		}


void TIM1_UP_TIM10_IRQHandler(void){
	if(READ_BIT(TIM2->SR, TIM_SR_UIF)){


		TIM2->SR &= ~TIM_SR_UIF;

	}

}


void TIM1_Init(void)
{
    /* Enable clock for TIM2 */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* Initialize TIM2 */
    TIM1->PSC = 0; // Prescaler
    TIM1->ARR = 1666 - 1; // Auto-reload value

    /* Force update generation to update registers */
    TIM1->EGR |= TIM_EGR_UG;

    /* Clear update interrupt flag */
   TIM1->SR &= ~TIM_SR_UIF;

   TIM1->DIER |= TIM_DIER_TDE;

    TIM1->DIER |= TIM_DIER_UIE;

    //TIM1->DIER |= TIM_DIER_UDE;// Update dma // Включаем запрос DMA по обновлению таймера

    /* Enable TIM2 global interrupt */

   NVIC_EnableIRQ(TIM2_IRQn);


}


void DMA2_Stream5_IRQHandler(void){


	if (READ_BIT(DMA2->HISR, DMA_HISR_HTIF5)){


	}

	if ((READ_BIT(DMA2->HISR, DMA_HISR_TCIF5))){ // передача по 2 каналу завершена полностью


		}

	}




void DMA_Init(void){

	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
	DMA_Stream->CR = (DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_2)  ; // 6 канал 110
	DMA_Stream->CR |= DMA_SxCR_MSIZE_0;// MSIZE 16 bit
	DMA_Stream->CR |= DMA_SxCR_PSIZE_0; // (0x0<<11); // PSIZE 16 bit
	DMA_Stream->CR |= DMA_SxCR_MINC; // (0x1<<10); // увеличиваем память

	DMA_Stream->CR |= DMA_SxCR_DIR_0;  //  сбрасываю 7 бит 01 Из памяти в периф



	DMA_Stream->CR |= DMA_SxCR_PL_1; // Приоритет потока: высокий
	DMA_Stream->CR |= DMA_SxCR_CIRC ;//(0x1<<8); Circular mode
	DMA_Stream->CR |= DMA_SxCR_TCIE;
	DMA_Stream->CR |= DMA_SxCR_HTIE;
	DMA_Stream->CR |= DMA_SxCR_TCIE;
	DMA_Stream->CR |= DMA_SxCR_TEIE;



}

void DMA_Config(uint32_t perih_address, uint32_t mem_address , uint16_t data_amount ){

	DMA_Stream->NDTR = data_amount;

	DMA_Stream->PAR = perih_address;

	DMA_Stream->M0AR = mem_address;

	DMA_Stream->CR |= DMA_SxCR_EN;


}

int main(void)
{

	GPIO_Init();
	DMA_Init();
	DMA_Config( (uint32_t) &GPIOA->ODR , (uint32_t) buf, SIZE);
	TIM1_Init();
	SysTick_Config(SystemCoreClock/1000);
	TIM1->DIER |= TIM_DIER_UDE;// Update dma // Включаем запрос DMA по обновлению таймера
	//TIM1->CR1 |= TIM_CR1_CEN; // включаем таймер TIM2
	DMA2->HISR |= DMA_HISR_TCIF5;
	uint8_t Flag = 0x00;

	//NVIC_EnableIRQ(DMA1_Stream7_IRQn);


	char *a = "Hello";
	char *c = a;





	while(1){

		//if(DMA2->HISR &= DMA_HISR_TCIF5){
			//TIM1->CR1 &= ~TIM_CR1_CEN; // включаем таймер TIM2

			while(*a){
			uint8_t b = (uint8_t) *a++;
			//uint16_t buf[8] = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000}; // исходный массив

			uint16_t mask = 0x0020; // маска для установки бита
			for(int i = 0; i < 8; i++) { // цикл по 8 элементам массива
				  buf[i+1] = buf[i+1] & ~mask; // сбрасываем 6-й бит значения i-го элемента массива
				  buf[i+1] = buf[i+1] | ((b >> i) & 0x01) << 5; // помещаем i-й бит из c в 6-й бит значения i-го элемента массива
			}


			TIM1->CR1 |= TIM_CR1_CEN; // включаем таймер TIM2
			delay_ms(1000);

			}
		//}
			a = c;

		//delay_ms(1);



		}
}





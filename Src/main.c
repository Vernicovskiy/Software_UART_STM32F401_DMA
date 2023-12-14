#include "main.h"
#include "string.h"

#define DMA_Stream DMA2_Stream5


#define SIZE 10 // Размер буфера для передачи данных

uint16_t buf[SIZE] = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020};

uint16_t buf1[SIZE] = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020};

char a[] = "Hello\n";


#define rx_size (sizeof(a)-1)

char rx_buffer[rx_size];

uint8_t i = 1;
uint8_t rx_index = 0;

void Data_Put(char *a, uint16_t *buf);



void GPIO_Init(){

		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; //Тактирование на порт A

		GPIOA->MODER &= ~GPIO_MODER_MODER5; // очистили режим для нужного пина

		GPIOA->MODER |= GPIO_MODER_MODER5_0; // пин на выход

		GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED5_0 | GPIO_OSPEEDR_OSPEED5_1 ); // скорость very high

		GPIOA->BSRR = GPIO_BSRR_BS5; // установили в высокое состояние (мы передатчик)

		// UART
		GPIOA->MODER |= GPIO_MODER_MODER3_1; //режим альтернативной функции

		GPIOA->AFR[0] |= (GPIO_AFRL_AFRL3_0 | GPIO_AFRL_AFRL3_1 | GPIO_AFRL_AFRL3_2); // Альтернативная функция для приемника


		}


void TIM1_Init(void)
{

		RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; // тактирование на таймер


		TIM1->PSC = 0; // Prescaler

		TIM1->ARR = 1667 - 1; // Auto-reload value для 9600 Baud rate

		TIM1->EGR |= TIM_EGR_UG; // Очистили теневые регистры

		TIM1->SR &= ~TIM_SR_UIF; // Очиста вызывает update поэтому очищаем флаг

		TIM1->DIER |= TIM_DIER_UDE; // Включаем запрос DMA по обновлению таймера

}



void USART_init() { // включаем USART2 PA3

		RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // Вкл тактирование

		USART2->BRR = 0x683; //Задали частоту работы

		USART2->CR1 |= (USART_CR1_RE | USART_CR1_RXNEIE); // Настроили на чтение

		USART2->CR1 |= USART_CR1_UE; //Вкл USART

		NVIC_EnableIRQ(USART2_IRQn); // глобальные прерывания

}

void USART2_IRQHandler (void)
{
	 if (USART2->SR & USART_SR_RXNE)
	 {

		 a[rx_index++] = (char) USART2->DR; // заменяем символ в передаваемой строке по порядку

		 if(rx_index == rx_size-1) rx_index = 0; // если дошли до конца строки то переходим вначало
	 }
 }

void FillBuf(char* data, uint16_t *buf) // функция для заполнения чередующихся буферов
{
	Data_Put(data + i, buf);
	i++;
	if (i > (strlen(data) - 1)) i = 0;

}


void DMA2_Stream5_IRQHandler(void){


	if (READ_BIT(DMA2->HISR, DMA_HISR_HTIF5)){ // половина буфера передалось

		if(!(DMA_Stream->CR & DMA_SxCR_CT)) // первый источник памяти
		{
			FillBuf(a,buf1); // передали половину первого буфера , заполнили второй

		}
		else if ((DMA_Stream->CR & DMA_SxCR_CT)) // второй источник памяти
		{
			FillBuf(a,buf); // передали половину второго буфера , заполнили первый

		}

		DMA2->HIFCR |= DMA_HIFCR_CHTIF5; // очистили флаг


	}

	if ((READ_BIT(DMA2->HISR, DMA_HISR_TCIF5))){ // передача по 5 потоку завершена полностью

		DMA2->HIFCR |= DMA_HIFCR_CTCIF5; // очистили флаг

		TIM1->CNT = 0;



		}

	}


void DMA_Init(void){

		RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

		DMA_Stream->CR = (DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_2)  ; // 6 канал 110

		DMA_Stream->CR |= DMA_SxCR_MSIZE_0;// MSIZE 16 bit

		DMA_Stream->CR |= DMA_SxCR_PSIZE_0;// PSIZE 16 bit

		DMA_Stream->CR |= DMA_SxCR_MINC; // инкремент памяти памяти

		DMA_Stream->CR |= DMA_SxCR_DIR_0;  //  сбрасываю 7 бит 01 Из памяти в периф

		DMA_Stream->CR |= DMA_SxCR_PL_1; // Приоритет потока: высокий

		DMA_Stream->CR |= DMA_SxCR_HTIE; // Прерывание по половине

		DMA_Stream->CR |= DMA_SxCR_TCIE; // Прерывание полная передача

		DMA_Stream->CR |= DMA_SxCR_TEIE; // Ошибка передачи (если неверный адрес и тд)

		DMA_Stream->CR |= DMA_SxCR_DBM; //  Режим двойной буферизации.

		NVIC_EnableIRQ(DMA2_Stream5_IRQn); // глоб прерывания для DMA2_Stream5

}

void DMA_Config(uint32_t perih_address, uint32_t mem_address , uint32_t mem_address1, uint16_t data_amount ){

		DMA_Stream->NDTR = data_amount;

		DMA_Stream->PAR = perih_address;

		DMA_Stream->M0AR = mem_address; // сначала передаем первый буфер

		DMA_Stream->M1AR = mem_address1; // после передачи первого передаем второй

		DMA_Stream->CR |= DMA_SxCR_EN;


}


void Data_Put(char *a , uint16_t *buf){


	uint8_t b = (uint8_t) *a;

	uint16_t mask = 0x0020; // маска для бита

	for(int i = 0; i < 8; i++) // цикл по 8 элементам массива
	{
		  buf[i+1] = buf[i+1] & ~mask; // сбрасываем 6-й бит значения i-го элемента массива
		  buf[i+1] = buf[i+1] | ((b >> i) & 0x01) << 5; // помещаем i-й бит из а в 6-й бит значения i-го элемента массива
	}



}


int main(void)
{
	USART_init();
	Data_Put(a, buf); // первый раз заполняем первый буфер
	GPIO_Init();
	TIM1_Init();
	DMA_Init();
	DMA_Config( (uint32_t) &GPIOA->ODR , (uint32_t) buf, (uint32_t) buf1, SIZE);

	TIM1->CR1 |= TIM_CR1_CEN; // включаем таймер TIM1

	while(1){

	}
}





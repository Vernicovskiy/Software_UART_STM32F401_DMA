# Программный UART c использованием DMA

![Static Badge](https://img.shields.io/badge/Unic_Lab-green)
![Static Badge](https://img.shields.io/badge/STM32-red)
![GitHub last commit (by committer)](https://img.shields.io/github/last-commit/Vernicovskiy/STM32_TIM)
![GitHub Repo stars](https://img.shields.io/github/stars/Vernicovskiy/STM32_TIM)
![GitHub watchers](https://img.shields.io/github/watchers/Vernicovskiy/STM32_TIM)
![GitHub top language](https://img.shields.io/github/languages/top/Vernicovskiy/STM32_TIM)

* NUCLEO-F401RE
* STM32F401RET6U
* ARM Cortex M4
* CMSIS
* STM32 CubeIDE v1.13.2

>Реализация передачи данных через программный UART используя DMA.

Мы можем передавать данные по UART с помощью GPIO, потому что UART - это протокол асинхронной последовательной связи, который определяет формат и скорость передачи данных. Для реализации UART нам нужно уметь управлять уровнем сигнала на проводе в соответствии с битами данных, старт-битом и стоп-битами. Это можно сделать с помощью GPIO, способными устанавливать или считывать логический уровень на пине. Для того, чтобы GPIO работали как UART, нам также нужно использовать таймер, который будет генерировать запросы DMA с заданной частотой, определяющей скорость передачи.

<p align="center">
<img src="PNG/image.png" alt="Diagram of System Timer (SysTick)" width="500"/>
</<p align="center">


Разработка программы была разделена на блоки: 

1) Разработа четкого алгоритма ±

2) Проверить смогу ли я управлять светодиодом с определенным периодом заданным в регистре ARR таймера ✓

3) Реализовать передачу одного символа на терминал ✓
   
4) Реализовать передачу строки в терминал ✓
   
5) Реализовать корректную передачу заданной нам строки в терминал ✖


Алгоритм работы:

Передача одного кадра 

1) Первым делом надо настроить порт на выход и с подтяжкой (**PA5**) , чтобы установить высокий уровень , так как мы передатчик.

2) Передача данных из памяти на второе UART устройство выполняется с помощью DMA. Таймер посылает Dma запрос , чтобы установить нашу ножку в нулевое состояние и начать передачу .

3) После этого таймер начинает считать промежуток времени равный определенного нами Baudrate.

4) Когда промежуток времени по передаче одного бита истек , таймер шлет запрос dma установить следующий бит из памяти . 
   

<p align="center">
<img src="PNG/image-1.png" alt="Diagram of System Timer (SysTick)" width="500"/>
</<p align="center">




```c
#define SIZE 10 // Размер буфера для передачи данных

uint8_t counter = 0;

uint16_t buf[SIZE] = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020};
```

```c
void GPIO_Init(){

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; //RCC on for GPIO A
    GPIOA->MODER &= ~0x00000C00; /* clear pin mode */
    GPIOA->MODER |= GPIO_MODER_MODER5_0; /* set pin to output mode */
    GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED5_0 | GPIO_OSPEEDR_OSPEED5_1 ); // very high
    GPIOA->BSRR = GPIO_BSRR_BS5; // установили в высокое состояние

}
```
Настройка таймера как как измерителя интервалов времени. Запрос к блоку DMA будет происходить во время переполнения — TIM1_UP
```c
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
    TIM1->DIER |= TIM_DIER_UIE; // прерывание по Update по таймеру

    TIM1->DIER |= TIM_DIER_UDE; // Update dma // Включаем запрос DMA по обновлению таймера

    NVIC_EnableIRQ(TIM2_IRQn);


}
```
Регистр ODR 32-ух битный, но используется в нём только первые 16 бит, мы будем передавать полслова

<p align="center">
<img src="PNG/image-2.png" alt="Diagram of System Timer (SysTick)" width="500"/>
</<p align="center">

```c
void DMA_Init(void){

	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
	DMA_Stream->CR = (DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_2)  ; // 6 канал 110
	DMA_Stream->CR |= DMA_SxCR_MSIZE_0;// MSIZE 16 bit
	DMA_Stream->CR |= DMA_SxCR_PSIZE_0;// PSIZE 16 bit
	DMA_Stream->CR |= DMA_SxCR_MINC;   // Увеличиваем память

	DMA_Stream->CR |= DMA_SxCR_DIR_0;  // Из памяти в периф

	DMA_Stream->CR |= DMA_SxCR_CIRC ; // Circular mode
	
	//DMA_Stream->CR |= DMA_SxCR_TCIE;
	DMA_Stream->CR |= DMA_SxCR_TEIE;

}
```
```c
void DMA_Config(uint32_t perih_address, uint32_t mem_address , uint16_t data_amount ){

	DMA_Stream->NDTR = data_amount;

	DMA_Stream->PAR = perih_address;

	DMA_Stream->M0AR = mem_address;

	DMA_Stream->CR |= DMA_SxCR_EN;


}
```
## Функция main()
```c
    GPIO_Init();
	DMA_Init();
	DMA_Config( (uint32_t) &GPIOA->ODR , (uint32_t) buf, SIZE);
	TIM1_Init();

    char *a = "Hello";
	char *c = a;

```

Главная логика программы
Определяет переменную mask со значением 0x0020. В двоичной системе счисления как 0000000000100000. Это число будет использоваться как маска для установки бита, то есть для изменения 6го бита в значении (на самом деле для нас 5го так как счет с нуля).

Помещает i-й бит из b в 6-й бит значения i-го элемента массива buf. Это означает, что он копирует один бит из переменной b, которая содержит символ из строки a, в один бит в массиве buf. Для этого он использует операцию побитового сдвига вправо (>>) для получения i-го бита из b, операцию побитового И (&) с единицей для отбрасывания лишних битов, операцию побитового сдвига влево (<<) для перемещения бита на 6-ю позицию, и операцию побитового ИЛИ (|) для объединения битов с предыдущим значением элемента массива. Побитовое ИЛИ возвращает единицу, если хотя бы один из операндов равен единице, поэтому все биты, кроме 6-го, остаются неизменными, а 6-й бит становится равным i-му биту из b.
```c
while(1){

while(*a){
			uint8_t b = (uint8_t) *a++; 
			
			uint16_t mask = 0x0020; // маска для установки бита
			for(int i = 0; i < 8; i++) { // цикл по 8 элементам массива
				  buf[i+1] = buf[i+1] & ~mask; // сбрасываем 6-й бит значения i-го элемента массива
				  buf[i+1] = buf[i+1] | ((b >> i) & 0x01) << 5; // помещаем i-й бит из b в 6-й бит значения i-го элемента массива
			}


			TIM1->CR1 |= TIM_CR1_CEN; // включаем таймер TIM2
			delay_ms(1000);

			}
	
			a = c;
}
```

## Сборка проекта
Собрать программу можно с помощью утилиты `make` для этого надо иметь `GNU Arm Embedded Toolchain` 

Если вы используете **STM32CubeIDE** с дефолтным расположением на диске C при установке, то вы можете прописать в системной переменной среды `Path` следующую команду 

`C:\ST\STM32CubeIDE_1.13.2\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.11.3.rel1.win32_1.1.1.202309131626\tools\bin`

Также понадобится минимальный набор утилит для процессинга **Make** файлов , он также расположен в:

`C:\ST\STM32CubeIDE_1.13.2\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_2.1.0.202305091550\tools\bin`

Открываем консоль `cmd` в папке склонированного репозитория и вводим следующие команды

```c
cd Debug
make all
```
На выходе вы получаете файл с расширением `.hex` в папке Debug

Для запуска программы понадобиться `STM32 ST-LINK Utility.exе` c помощью которой вы сможете зашить **.hex** файл в МК

Если вы используете **STM32 CubeIDE v1.13.2** то вы можете также добавить свой проект в свой WORKSPACE кликнув на файл `.project`

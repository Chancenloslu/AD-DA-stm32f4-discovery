/**
  ******************************************************************************
  * @file    Template/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    20-September-2013
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "display.h"

/** @addtogroup Template
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

	
	#define DAC_DHR12R2_ADDRESS    0x40007414
	#define DAC_DHR8R1_ADDRESS     0x40007410
	//#define ADC3_DR_ADDRESS     ((uint32_t)0x4001224C)
	#define ADCBufferSize		 	21 
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint16_t uhADC1ConvertedValue = 0;
__IO uint32_t uwADC1ConvertedVoltage = 0;
__IO uint32_t uwADC2ConvertedVoltage = 0;
__IO uint16_t uhADC2ConvertedValue = 0;

uint32_t ADCVoltageBuffer[ADCBufferSize];
uint16_t filterIndex;

DAC_InitTypeDef  	DAC_InitStructure;

const uint16_t Sine12bit[32] = {
                      2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056, 4095, 4056,
                      3939, 3750, 3495, 3185, 2831, 2447, 2047, 1647, 1263, 909, 
                      599, 344, 155, 38, 0, 38, 155, 344, 599, 909, 1263, 1647};
const uint8_t Escalator8bit[6] = {0x0, 0x33, 0x66, 0x99, 0xCC, 0xFF};

uint8_t 					flagExti0 = 0;

/* Private function prototypes -----------------------------------------------*/

void TIM6_Config(void);
void TIM2_Config(void);
void DAC_Ch2_SineWaveConfig(void);
void DAC_Ch2_TriangleConfig(void);
static void ADC2_CH13_DMA_Config(void);
uint32_t MidFilter(uint32_t arr[ADCBufferSize]);
/* Private functions ---------------------------------------------------------*/

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       files (startup_stm32f429_439xx.s) before to branch to application main. 
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f4xx.c file
     */  
//	GPIO_InitTypeDef GPIO_InitStructure;
//  	/*DAC 2 (DAC_OUT2 = PA.5) configuration */
//	GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_5;
//	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AN;
//	GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_NOPULL;
//	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Add your application code here */
	STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4);
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI); 

	Display_Init();

	TIM2_Config();
	ADC2_CH13_DMA_Config();
	ADC_SoftwareStartConv(ADC2);

	TIM6_Config();
	//DAC_Ch2_SineWaveConfig();
	DAC_Ch2_TriangleConfig();
	
	filterIndex = 0;
	int index = 0;
	indexPrint = 0;
/* Infinite loop */
	uint8_t indexPrintOutPut[50];
	
	while (1)
	{
		//Display(uwADC2ConvertedVoltage);
		sprintf((char *)indexPrintOutPut, "indexPrint = %d", indexPrint);
		LCD_DisplayStringLine(LCD_LINE_0, (uint8_t*)indexPrintOutPut);
		if(indexPrint >= 2)
		{
			indexPrint = 0;
			uwADC2ConvertedVoltage = uhADC2ConvertedValue *3000/0xFFF;
			Display(uwADC2ConvertedVoltage);
		}
		
//		if(filterIndex < ADCBufferSize) {
//			ADCVoltageBuffer[filterIndex] = uwADC1ConvertedVoltage;
//			filterIndex++;
//		}else {
//			filterIndex = 0;
//			Display(MidFilter(ADCVoltageBuffer));
//		}
	}
}

uint32_t MidFilter(uint32_t arr[ADCBufferSize]){
	short isSorted;
	uint32_t temp;
	uint16_t length = sizeof(arr[ADCBufferSize]) / sizeof(uint32_t);
	for(int i=0; i<length-1; i++){
        isSorted = 1;  			//assume the remaining elements are sorted
        for(int j=0; j<length-1-i; j++){
            if(arr[j] > arr[j+1]){
                temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
                isSorted = 0;  			//if exchange happens => the array is unordered
            }
        }
        if(isSorted) break; 			//if no change => array is ordered
    }
    return arr[length/2];
}


void TIM2_Config(void)
{
	TIM_TimeBaseInitTypeDef    TIM_TimeBaseStructure; 	//Tout = (ARR+1)(PSC+1)/f_clk
	/* TIM2 Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period 		= 4999;			//ARR
	TIM_TimeBaseStructure.TIM_Prescaler		= 4199;			//PSC	Tout = (4999+1)(4199+1)/42M = 0.5 but in fact the timedaly is likely to be 0.25
	TIM_TimeBaseStructure.TIM_CounterMode 	= TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	//configure interrupt source
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	//enable interrupt
	TIM_Cmd(TIM2, ENABLE);
	
	/* configure the interrupt priority*/
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel 						= TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority 	= 0x01;  //PreemptionPriority
	NVIC_InitStructure.NVIC_IRQChannelSubPriority 			= 0x03;		    
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
	NVIC_Init(&NVIC_InitStructure);

}
/**
  * @brief  TIM6 Configuration
  * @note   TIM6 configuration is based on CPU @180MHz and APB1 @45MHz
  * @note   TIM6 Update event occurs each TIM6CLK(MHz)/256    
  * @param  None
  * @retval None
  */
void TIM6_Config(void)
{
  /* TIM6CLK = HCLK / 4 = SystemCoreClock /4 */
  
  TIM_TimeBaseInitTypeDef    TIM_TimeBaseStructure;
  /* TIM6 Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

  /* Time base configuration */
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Period = 255;					// period = 256
  TIM_TimeBaseStructure.TIM_Prescaler = 0;					//
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; 
  TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

  /* TIM6 TRGO selection */
  TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
  
  /* TIM6 enable counter */
  TIM_Cmd(TIM6, ENABLE);
}

/**
  * @brief  DAC Channel2 Triangle Configuration
  * @param  None
  * @retval None
  */
void DAC_Ch2_TriangleConfig(void)
{
  /* DAC Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
  /* DMA1 clock and GPIOA clock enable (to be used with DAC) */
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE);
	
  /* DAC channel2 Configuration */
  DAC_InitStructure.DAC_Trigger = DAC_Trigger_T6_TRGO;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_Triangle;
  DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_TriangleAmplitude_1023;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_2, &DAC_InitStructure);

  /* Enable DAC Channel2 */
  DAC_Cmd(DAC_Channel_2, ENABLE);

  /* Set DAC channel2 DHR12R2 register */
  DAC_SetChannel2Data(DAC_Align_12b_R, 0x100);
}

/**
  * @brief  DAC Channel2 Sine Wave Configuration
  * @param  None
  * @retval None
  */
void DAC_Ch2_SineWaveConfig(void)
{
  DMA_InitTypeDef DMA_InitStructure;
	
  /* DAC channel2 Configuration */
  DAC_InitStructure.DAC_Trigger 				= DAC_Trigger_T6_TRGO;
  DAC_InitStructure.DAC_WaveGeneration 	= DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_OutputBuffer 		= DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_2, &DAC_InitStructure);
	
	/* DAC Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	/* DMA1 clock and GPIOA clock enable (to be used with DAC) */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1 | RCC_AHB1Periph_GPIOA, ENABLE);
	
  /* 	channel7 configuration **************************************/  
  DMA_DeInit(DMA1_Stream6);
  DMA_InitStructure.DMA_Channel 						= DMA_Channel_7;  
  DMA_InitStructure.DMA_PeripheralBaseAddr	= (uint32_t)DAC->DHR12R2;
  DMA_InitStructure.DMA_Memory0BaseAddr 		= (uint32_t)&Sine12bit;
  DMA_InitStructure.DMA_BufferSize 					= 32;
  DMA_InitStructure.DMA_PeripheralDataSize 	= DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize 			= DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_DIR 								= DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_PeripheralInc 			= DMA_PeripheralInc_Disable;				//Peripheral data address fixed
  DMA_InitStructure.DMA_MemoryInc 					= DMA_MemoryInc_Enable;							//Memory data address auto-increment
  DMA_InitStructure.DMA_Mode 								= DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority 						= DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode 						= DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold 			= DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst 				= DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst 		= DMA_PeripheralBurst_Single;
  DMA_Init(DMA1_Stream6, &DMA_InitStructure);

  /* Enable DMA1_Stream6 */
  DMA_Cmd(DMA1_Stream6, ENABLE);
  
  /* Enable DAC Channel2 */
  DAC_Cmd(DAC_Channel_2, ENABLE);

  /* Enable DMA for DAC Channel2 */
  DAC_DMACmd(DAC_Channel_2, ENABLE);
}

/**
  * @brief  ADC1 channel0 with DMA configuration port PA0
  * @param  None
  * @retval None
  */
static void ADC2_CH13_DMA_Config(void)
{
  ADC_InitTypeDef       ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  DMA_InitTypeDef       DMA_InitStructure;
  GPIO_InitTypeDef      GPIO_InitStructure;

  /* Enable ADC2, DMA2 and GPIO clocks ****************************************/
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

  /* DMA2 Stream2 channel1 configuration **************************************/
	DMA_DeInit(DMA2_Stream2);
	DMA_InitStructure.DMA_Channel 				= DMA_Channel_1;
  DMA_InitStructure.DMA_PeripheralBaseAddr 		= (uint32_t)&(ADC2->DR);
  DMA_InitStructure.DMA_Memory0BaseAddr 		= (uint32_t)&uhADC2ConvertedValue;
  DMA_InitStructure.DMA_DIR 					= DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize 				= 1;
  DMA_InitStructure.DMA_PeripheralInc 			= DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc 				= DMA_MemoryInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize 		= DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize 			= DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode 					= DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority 				= DMA_Priority_High;
  DMA_InitStructure.DMA_FIFOMode 				= DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold 			= DMA_FIFOThreshold_HalfFull;
  DMA_InitStructure.DMA_MemoryBurst 			= DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst 		= DMA_PeripheralBurst_Single;
  DMA_Init(DMA2_Stream2, &DMA_InitStructure);
  /* Enable DMA2 Stream2 */
	DMA_Cmd(DMA2_Stream2, ENABLE);

//  /* Configure ADC1 Channel0 pin as analog input ******************************/
//  GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_0;
//  GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AN;
//  GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_NOPULL ;
//  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* Configure ADC2 Channel13 pin as analog input ******************************/
  GPIO_InitStructure.GPIO_Pin 	= GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd 	= GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* ADC Common Init **********************************************************/
  ADC_CommonInitStructure.ADC_Mode 				= ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler 		= ADC_Prescaler_Div2;
  ADC_CommonInitStructure.ADC_DMAAccessMode 	= ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay 	= ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);

  /* ADC2 Init ****************************************************************/
  ADC_InitStructure.ADC_Resolution 				= ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode 			= DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode 		= ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge 	= ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_ExternalTrigConv 		= ADC_ExternalTrigConv_T1_CC1;	
  ADC_InitStructure.ADC_DataAlign				= ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion 		= 1;
  ADC_Init(ADC2, &ADC_InitStructure);

  /* ADC2 regular channel13 configuration *************************************/
  ADC_RegularChannelConfig(ADC2, ADC_Channel_13, 1, ADC_SampleTime_3Cycles);

 /* Enable DMA request after last transfer (Single-ADC mode) */
  ADC_DMARequestAfterLastTransferCmd(ADC2, ENABLE);

  /* Enable ADC2 DMA */
  ADC_DMACmd(ADC2, ENABLE);

  /* Enable ADC2 */
  ADC_Cmd(ADC2, ENABLE);
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

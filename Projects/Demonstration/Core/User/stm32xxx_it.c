/**
  ******************************************************************************
  * @file    stm32xxx_it.c
  * @author  MCD Application Team
  * @version V1.0.3
  * @date    05-March-2021
  * @brief   Exceptions Handlers
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
#include "global_includes.h"
#include "usb_hcd_int.h"
#include "usbh_core.h"

#include "test_program.h"
#include "usb_core.h"
#include "usbd_core.h"
#include "usbd_hid_core.h"
#include "usb_conf.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern __IO uint32_t TS_Pressed;
extern uint32_t demo_mode, Test_mode;
extern USB_OTG_CORE_HANDLE           USB_OTG_Core;
extern USB_OTG_CORE_HANDLE           USB_OTG_dev;
extern uint32_t USBD_OTG_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
__IO uint32_t ButtonPressed = 0x00;
extern uint32_t USBD_OTG_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
extern __IO uint32_t PeriodValue;
extern __IO uint32_t CaptureNumber;
uint16_t tmpCC4[2] = {0, 0};
uint8_t pBuffer;
/* Private function prototypes -----------------------------------------------*/
extern void USB_OTG_BSP_TimerIRQ (void);
extern void xPortSysTickHandler( void );
extern void LTDC_ISR_Handler(void);
extern void DMA2D_ISR_Handler(void);
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*             Cortex-M Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{}

/**
  * @brief  This function handles PendSV_Handler exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
if(demo_mode == 0)
  xPortSysTickHandler();
else  
  TimingDelay_Decrement();
}

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
void EXTI0_IRQHandler(void)
{ 
  ButtonPressed = 0x01;
  
  EXTI_ClearITPendingBit(USER_BUTTON_EXTI_LINE);
}
/**
  * @brief  TIM2_IRQHandler
  *         This function handles Timer2 Handler.
  * @param  None
  * @retval None
  */

void TIM2_IRQHandler(void)
{
  USB_OTG_BSP_TimerIRQ();
}

/**
  * @brief  This function handles OTG_HS Handler.
  * @param  None
  * @retval None
  */
void OTG_HS_IRQHandler(void)
{
  if(demo_mode == 0)
  {
    USBH_OTG_ISR_Handler (&USB_OTG_Core);
  }
  else
  {
    USBD_OTG_ISR_Handler (&USB_OTG_dev);
  }
}

/**
  * @brief  This function handles TIM5 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM5_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM5, TIM_IT_CC4) != RESET)
  {    
    /* Get the Input Capture value */
    tmpCC4[CaptureNumber++] = TIM_GetCapture4(TIM5);
   
    /* Clear CC4 Interrupt pending bit */
    TIM_ClearITPendingBit(TIM5, TIM_IT_CC4);

    if (CaptureNumber >= 2)
    {
      /* Compute the period length */
      PeriodValue = (uint16_t)(0xFFFF - tmpCC4[0] + tmpCC4[1] + 1);
    }
  }
}

/**
  * @brief  This function handles DMA2 Stream1 global interrupt request.
  * @param  None
  * @retval None
  */

void DMA2_Stream1_IRQHandler(void)
{
}

/**
  * @brief  This function handles LTDC global interrupt request.
  * @param  None
  * @retval None
  */
void LTDC_IRQHandler(void)
{
  LTDC_ISR_Handler();
}

/**
  * @brief  This function handles DMA2D global interrupt request.
  * @param  None
  * @retval None
  */
void DMA2D_IRQHandler(void)
{
  DMA2D_ISR_Handler();
}

/**
  * @brief  This function handles External line 2 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI15_10_IRQHandler(void)
{
  if(EXTI_GetITStatus(IOE_IT_EXTI_LINE) != RESET)
  {
    /* Specify mesage below to be displayed only on TP_Test2*/ 
    if (Test_mode == 1)
    {
      /* LCD display message */ 
      LCD_Clear(LCD_COLOR_GREEN);
      LCD_SetTextColor(LCD_COLOR_BLACK); 
      LCD_DisplayStringLine(LCD_LINE_1,(uint8_t*)"   TP TEST2 OK ");
      LCD_DisplayStringLine(LCD_LINE_2,(uint8_t*)" INTERRUPT MODE");
      LCD_DisplayStringLine(LCD_LINE_5,(uint8_t*)"      PUSH     ");
      LCD_DisplayStringLine(LCD_LINE_7,(uint8_t*)"  USER BUTTON  ");
      Test_mode = 0; 
    }
    /* Clear the interrupt pending bits */    
    IOE_ClearGITPending(IOE_TP_IT);      
  }
  EXTI_ClearITPendingBit(IOE_IT_EXTI_LINE);
}

/**
  * @brief  This function handles External line 1 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI1_IRQHandler(void)
{  
  if(EXTI_GetITStatus(EXTI_Line1) != RESET)
  {
    /* Specify mesage below to be displayed only on MEMS_Test2*/  
    if (Test_mode == 2)
    {
      /* LCD display message */ 
      LCD_Clear(LCD_COLOR_GREEN);
      LCD_SetTextColor(LCD_COLOR_BLACK); 
      LCD_DisplayStringLine(LCD_LINE_1,(uint8_t*)" MEMS TEST2 OK ");
      LCD_DisplayStringLine(LCD_LINE_2,(uint8_t*)" INTERRUPT MODE");
      LCD_DisplayStringLine(LCD_LINE_4,(uint8_t*)"      PUSH     ");
      LCD_DisplayStringLine(LCD_LINE_6,(uint8_t*)"  USER BUTTON  ");
      LCD_DisplayStringLine(LCD_LINE_8,(uint8_t*)"    TO GO TO   ");
      LCD_DisplayStringLine(LCD_LINE_10,(uint8_t*)"   NEXT TEST   ");    
      /*  LED3 On */   
      STM_EVAL_LEDOff(LED4);
      STM_EVAL_LEDOn(LED3);
      Test_mode = 0;
    }
  }
  EXTI_ClearITPendingBit(EXTI_Line1);   
  L3GD20_Read(&pBuffer, L3GD20_INT1_SRC_ADDR, 1); 
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

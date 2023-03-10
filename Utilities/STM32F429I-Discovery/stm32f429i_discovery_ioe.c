/**
  ******************************************************************************
  * @file    stm32f429i_discovery_ioe.c
  * @author  MCD Application Team
  * @version V1.0.3
  * @date    05-March-2021
  * @brief   This file provides a set of functions needed to manage the STMPE811
  *          IO Expander device mounted on STM32F429I-DISCO Kit.
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

  /* File Info : ---------------------------------------------------------------

    Note:
    -----
    - This driver uses the DMA method for sending and receiving data on I2C bus
      which allow higher efficiency and reliability of the communication.

    SUPPORTED FEATURES:
      - Touch Panel Features: Single point mode (Polling/Interrupt)
  ----------------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f429i_discovery_ioe.h"

/** @addtogroup Utilities
  * @{
  */

/** @addtogroup STM32F4_DISCOVERY
  * @{
  */

/** @addtogroup STM32F429I_DISCOVERY
  * @{
  */

/** @defgroup STM32F429I_DISCOVERY_IOE
  * @brief  This file includes the IO Expander driver for STMPE811 IO Expander
  *         devices.
  * @{
  */


/** @defgroup STM32F429I_DISCOVERY_IOE_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup STM32F429I_DISCOVERY_IOE_Private_Defines
  * @{
  */
#define TIMEOUT_MAX    0x3000 /*<! The value of the maximal timeout for I2C waiting loops */
/**
  * @}
  */


/** @defgroup STM32F429I_DISCOVERY_IOE_Private_Macros
  * @{
  */
/**
  * @}
  */

/** @defgroup STM32F429I_DISCOVERY_IOE_Private_Variables
  * @{
  */
TP_STATE TP_State;              /* The global structure holding the TS state */

uint32_t IOE_TimeOut = TIMEOUT_MAX; /* Value of Timeout when I2C communication fails */

uint16_t Device_ID = 0; /* Device ID: SX8651_ID or STMPE811_ID */
/**
  * @}
  */


/** @defgroup STM32F429I_DISCOVERY_IOE_Private_FunctionPrototypes
  * @{
  */
static void IOE_TP_Read_sx8651(uint16_t *X, uint16_t *Y);
static uint16_t IOE_TP_Read_X_stmpe811(void);
static uint16_t IOE_TP_Read_Y_stmpe811(void);
static uint16_t IOE_TP_Read_Z_stmpe811(void);
static void     IOE_GPIO_Config(void);
static void     IOE_I2C_Config(void);
static void     IOE_DMA_Config(IOE_DMADirection_TypeDef Direction, uint8_t* buffer);
static void     IOE_EXTI_Config(void);
#ifndef USE_Delay
static void delay(__IO uint32_t nCount);
#endif /* USE_Delay */
/**
  * @}
  */


/** @defgroup STM32F429I_DISCOVERY_IOE_Private_Functions
  * @{
  */


/**
  * @brief  Initializes and Configures the IO_Expander Touch Panel Functionality
  *         and configures all STM32F429I_DISCO necessary
  *         hardware (GPIOs, APB clocks ..).
  * @param  None
  * @retval IOE_OK if all initializations done correctly. Other value if error.
  */
uint8_t IOE_Config(void)
{
  /* Configure the needed pins */
  IOE_GPIO_Config();

  /* I2C initialization */
  IOE_I2C_Config();

  /* Read IO Expander ID  */
  if(IOE_IsOperational())
  {
    return IOE_NOT_OPERATIONAL;
  }

  /* Generate IO Expander Software reset */
  IOE_Reset();

  /* IO Expander configuration */
  /* Touch Panel controller and ADC configuration */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    IOE_FnctCmd(IOE_ADC_FCT, ENABLE);
  }
  IOE_TP_Config();

  /* Configuration is OK */
  return IOE_OK;
}

/**
  * @brief  Enables the touch Panel interrupt.
  * @param  None
  * @retval IOE_OK: if all initializations are OK. Other value if error.
  */
uint8_t IOE_TPITConfig(void)
{
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    /* Enable the Global interrupt */
    IOE_GITCmd(ENABLE);

    /* Enable the Global GPIO Interrupt */
    IOE_GITConfig((uint8_t)(IOE_GIT_TOUCH | IOE_GIT_FTH | IOE_GIT_FOV), ENABLE);

    /* Read the GPIO_IT_STA to clear all pending bits if any */
    I2C_ReadDeviceRegister(IOE_REG_GPIO_INT_STA);
  }
  else
  {
    uint8_t  data[4];
    I2C_ReadDeviceRegister(0x45);
    I2C_ReadDeviceData4(data);
  }
  /* Configure the Interrupt line as EXTI source */
  IOE_EXTI_Config();

  /* If all OK return IOE_OK */
  return IOE_OK;
}

/**
  * @brief  Returns Status and positions of the Touch Panel.
  * @param  None
  * @retval Pointer to TP_STATE structure holding Touch Panel information.
  */
TP_STATE* IOE_TP_GetState(void)
{
  uint16_t xDiff, yDiff , x , y;
  static uint32_t _x = 0, _y = 0;

  /* Check if the Touch detect event happened */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    TP_State.TouchDetected = (I2C_ReadDeviceRegister(IOE_REG_TP_CTRL) & 0x80);
  }
  else
  {
    TP_State.TouchDetected = (I2C_ReadDeviceRegister(0x45) & 0x80);
  }

  if(TP_State.TouchDetected)
  {
    if ( Device_ID == (uint16_t)STMPE811_ID )
    {
      x = IOE_TP_Read_X_stmpe811();
      y = IOE_TP_Read_Y_stmpe811();
    }
    else
    {
      IOE_TP_Read_sx8651(&x, &y);
    }
    xDiff = x > _x? (x - _x): (_x - x);
    yDiff = y > _y? (y - _y): (_y - y);
    if (xDiff + yDiff > 5)
    {
      _x = x;
      _y = y;
    }
  }
  /* Update the X position */
  TP_State.X = _x;

  /* Update the Y position */
  TP_State.Y = _y;

  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    /* Update the Z Pression index */
    TP_State.Z = IOE_TP_Read_Z_stmpe811();

    /* Clear the interrupt pending bit and enable the FIFO again */
    I2C_WriteDeviceRegister(IOE_REG_FIFO_STA, 0x01);
    I2C_WriteDeviceRegister(IOE_REG_FIFO_STA, 0x00);
  }

  /* Return pointer to the updated structure */
  return &TP_State;
}

/**
  * @brief  Checks the selected Global interrupt source pending bit
  * @param  Global_IT: the Global interrupt source to be checked, could be:
  *   @arg  Global_IT_ADC : ADC interrupt
  *   @arg  Global_IT_FE : Touch Panel Controller FIFO Error interrupt
  *   @arg  Global_IT_FF : Touch Panel Controller FIFO Full interrupt
  *   @arg  Global_IT_FOV : Touch Panel Controller FIFO Overrun interrupt
  *   @arg  Global_IT_FTH : Touch Panel Controller FIFO Threshold interrupt
  *   @arg  Global_IT_TOUCH : Touch Panel Controller Touch Detected interrupt
  * @retval Status of the checked flag. Could be SET or RESET.
  */
FlagStatus IOE_GetGITStatus(uint8_t Global_IT)
{
  __IO uint8_t tmp = 0;

  /* Get the Interrupt status */
  tmp = I2C_ReadDeviceRegister(IOE_REG_INT_STA);

  if ((tmp & (uint8_t)Global_IT) != 0)
  {
    return SET;
  }
  else
  {
    return RESET;
  }
}

/**
  * @brief  Clears the selected Global interrupt pending bit(s)
  * @param  Global_IT: the Global interrupt to be cleared, could be any combination
  *         of the following values:
  *   @arg  Global_IT_ADC : ADC interrupt
  *   @arg  Global_IT_FE : Touch Panel Controller FIFO Error interrupt
  *   @arg  Global_IT_FF : Touch Panel Controller FIFO Full interrupt
  *   @arg  Global_IT_FOV : Touch Panel Controller FIFO Overrun interrupt
  *   @arg  Global_IT_FTH : Touch Panel Controller FIFO Threshold interrupt
  *   @arg  Global_IT_TOUCH : Touch Panel Controller Touch Detected interrupt
  * @retval IOE_OK: if all initializations are OK. Other value if error.
  */
uint8_t IOE_ClearGITPending(uint8_t Global_IT)
{
  uint8_t Buffer[4];
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    /* Write 1 to the bits that have to be cleared */
    I2C_WriteDeviceRegister(IOE_REG_INT_STA, Global_IT);
  }
  else
  {
    I2C_ReadDeviceRegister(0x45);
    I2C_ReadDeviceData4(Buffer);
  }

  /* If all OK return IOE_OK */
  return IOE_OK;
}


/**
  * @brief  Checks if the IOE device is correctly configured and
  *         communicates correctly ont the I2C bus.
  * @param  None
  * @retval IOE_OK if IOE is operational. Other value if failure.
  */
uint8_t IOE_IsOperational(void)
{
  /* Return Error if the ID is not correct */
  Device_ID = IOE_ReadID();
  if( (Device_ID != (uint16_t)STMPE811_ID) && (Device_ID != (uint16_t)SX8651_ID))
  {
    /* Check if a Timeout occurred */
    if (IOE_TimeOut == 0)
    {
      return(IOE_TimeoutUserCallback());
    }
    else
    {
      return IOE_FAILURE; /* ID is not Correct */
    }
  }
  else
  {
    return IOE_OK; /* ID is correct */
  }
}

/**
  * @brief  Resets the IO Expander by Software (SYS_CTRL1, RESET bit).
  * @param  None
  * @retval IOE_OK: if all initializations are OK. Other value if error.
  */
uint8_t IOE_Reset(void)
{
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    /* Power Down the IO_Expander */
    I2C_WriteDeviceRegister(IOE_REG_SYS_CTRL1, 0x02);

    /* wait for a delay to insure registers erasing */
    _delay_(2);

    /* Power On the Codec after the power off => all registers are reinitialized*/
    I2C_WriteDeviceRegister(IOE_REG_SYS_CTRL1, 0x00);
  }
  else
  {
    /* Reset IO_Expander for SX8651 */
    I2C_WriteDeviceRegister(0x1f, 0xDE);
      /* wait for a delay to insure registers erasing */
  _delay_(2);
  }

  /* If all OK return IOE_OK */
  return IOE_OK;
}

/**
  * @brief  Reads the IOE device ID.
  * @param  None
  * @retval The Device ID (two bytes).
  */
uint16_t IOE_ReadID(void)
{
  uint16_t tmp = 0;

  /* Read device ID  */
  tmp = I2C_ReadDeviceID_stmpe811(0);
  tmp = (uint32_t)(tmp << 8);
  tmp |= (uint32_t)I2C_ReadDeviceID_stmpe811(1);
  if (tmp != (uint16_t)STMPE811_ID)
  {
    tmp = I2C_ReadDeviceID_sx8651(0x44);
  }
  /* Return the ID */
  return (uint16_t)tmp;
}

/**
  * @brief  Configures the selected IO Expander functionalities.
  * @param  Fct: the functions to be configured. could be any
  *         combination of the following values:
  *   @arg  IOE_IO_FCT : IO function
  *   @arg  IOE_TP_FCT : Touch Panel function
  *   @arg  IOE_ADC_FCT : ADC function
  * @param  NewState: can be ENABLE pr DISABLE
  * @retval IOE_OK: if all initializations are OK. Other value if error.
  */
uint8_t IOE_FnctCmd(uint8_t Fct, FunctionalState NewState)
{
  uint8_t tmp = 0;

  /* Get the register value */
  tmp = I2C_ReadDeviceRegister(IOE_REG_SYS_CTRL2);

  if (NewState != DISABLE)
  {
    /* Set the Functionalities to be Enabled */
    tmp &= ~(uint8_t)Fct;
  }
  else
  {
    /* Set the Functionalities to be Disabled */
    tmp |= (uint8_t)Fct;
  }

  /* Set the register value */
  I2C_WriteDeviceRegister(IOE_REG_SYS_CTRL2, tmp);

  /* If all OK return IOE_OK */
  return IOE_OK;
}


/**
  * @brief  Enables or disables the Global interrupt.
  * @param  NewState: could be ENABLE or DISABLE.
  * @retval IOE_OK: if all initializations are OK. Other value if error.
  */
uint8_t IOE_GITCmd(FunctionalState NewState)
{
  uint8_t tmp = 0;

  /* Read the Interrupt Control register  */
  tmp = I2C_ReadDeviceRegister(IOE_REG_INT_CTRL);

  if (NewState != DISABLE)
  {
    /* Set the global interrupts to be Enabled */
    tmp |= (uint8_t)IOE_GIT_EN;
  }
  else
  {
    /* Set the global interrupts to be Disabled */
    tmp &= ~(uint8_t)IOE_GIT_EN;
  }

  /* Write Back the Interrupt Control register */
  I2C_WriteDeviceRegister(IOE_REG_INT_CTRL, tmp);

  /* If all OK return IOE_OK */
  return IOE_OK;
}

/**
  * @brief  Configures the selected source to generate or not a global interrupt
  * @param Global_IT: the interrupt source to be configured, could be:
  *   @arg  Global_IT_ADC : ADC interrupt
  *   @arg  Global_IT_FE : Touch Panel Controller FIFO Error interrupt
  *   @arg  Global_IT_FF : Touch Panel Controller FIFO Full interrupt
  *   @arg  Global_IT_FOV : Touch Panel Controller FIFO Overrun interrupt
  *   @arg  Global_IT_FTH : Touch Panel Controller FIFO Threshold interrupt
  *   @arg  Global_IT_TOUCH : Touch Panel Controller Touch Detected interrupt
  * @param  NewState: can be ENABLE pr DISABLE
  * @retval IOE_OK: if all initializations are OK. Other value if error.
  */
uint8_t IOE_GITConfig(uint8_t Global_IT, FunctionalState NewState)
{
  uint8_t tmp = 0;

  /* Get the current value of the INT_EN register */
  tmp = I2C_ReadDeviceRegister(IOE_REG_INT_EN);

  if (NewState != DISABLE)
  {
    /* Set the interrupts to be Enabled */
    tmp |= (uint8_t)Global_IT;
  }
  else
  {
    /* Set the interrupts to be Disabled */
    tmp &= ~(uint8_t)Global_IT;
  }
  /* Set the register */
  I2C_WriteDeviceRegister(IOE_REG_INT_EN, tmp);

  /* If all OK return IOE_OK */
  return IOE_OK;
}

/**
  * @brief  Configures the touch Panel Controller (Single point detection)
  * @param  None
  * @retval IOE_OK if all initializations are OK. Other value if error.
  */
uint8_t IOE_TP_Config(void)
{
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    /* Enable touch Panel functionality */
    IOE_FnctCmd(IOE_TP_FCT, ENABLE);

    /* Select Sample Time, bit number and ADC Reference */
    I2C_WriteDeviceRegister(IOE_REG_ADC_CTRL1, 0x49);

    /* Wait for ~20 ms */
    _delay_(2);

    /* Select the ADC clock speed: 3.25 MHz */
    I2C_WriteDeviceRegister(IOE_REG_ADC_CTRL2, 0x01);

    /* Select TSC pins in non default mode */
    IOE_IOAFConfig((uint8_t)TOUCH_IO_ALL, DISABLE);

    /* Select 2 nF filter capacitor */
    I2C_WriteDeviceRegister(IOE_REG_TP_CFG, 0x9A);

    /* Select single point reading  */
    I2C_WriteDeviceRegister(IOE_REG_FIFO_TH, 0x01);

    /* Write 0x01 to clear the FIFO memory content. */
    I2C_WriteDeviceRegister(IOE_REG_FIFO_STA, 0x01);

    /* Write 0x00 to put the FIFO back into operation mode  */
    I2C_WriteDeviceRegister(IOE_REG_FIFO_STA, 0x00);

    /* set the data format for Z value: 7 fractional part and 1 whole part */
    I2C_WriteDeviceRegister(IOE_REG_TP_FRACT_XYZ, 0x01);

    /* set the driving capability of the device for TSC pins: 50mA */
    I2C_WriteDeviceRegister(IOE_REG_TP_I_DRIVE, 0x01);

    /* Use no tracking index, touch-panel controller operation mode (XYZ) and
       enable the TSC */
    I2C_WriteDeviceRegister(IOE_REG_TP_CTRL, 0x03);

    /*  Clear all the status pending bits */
    I2C_WriteDeviceRegister(IOE_REG_INT_STA, 0xFF);

    /* Initialize the TS structure to their default values */
    TP_State.TouchDetected = TP_State.X = TP_State.Y = TP_State.Z = 0;
  }
  else if ( Device_ID == (uint16_t)SX8651_ID )
  {
    /* Configuration:
    - RATE                     : 300 cps value: 0x80
    - Settling time of POWDLY  : 1140 uS  value: 0x0B
    */
    if (I2C_WriteDeviceRegister(0x00,0x8B) != IOE_OK)
    {
      return IOE_FAILURE;
    }
    /* check if the value is wrote with success */
    if (I2C_ReadDeviceRegister(0x40) != 0x8B)
    {
      return IOE_FAILURE;
    }

    /* Configuration:
    - Touch average control    : 3 samples
    - Pen Detect Resistor      : 25k ohm
    - conditional interrupts   : interrupt generated when pen detect is successful
    */
    if (I2C_WriteDeviceRegister(0x01,0x2D) != IOE_OK)
    {
      return IOE_FAILURE;
    }
    /* check if the value is wrote with success */
    if (I2C_ReadDeviceRegister(0x41) != 0x2D)
    {
      return IOE_FAILURE;
    }

    /* Configuration:
    - Settling time while filtering   : 140 us
    */
    if (I2C_WriteDeviceRegister(0x02,0x08) != IOE_OK)
    {
      return IOE_FAILURE;
    }
    /* check if the value is wrote with success */
    if (I2C_ReadDeviceRegister(0x42) != 0x08)
    {
      return IOE_FAILURE;
    }

    /* Configuration:
    - Rm : 542 ohm
    */
    if (I2C_WriteDeviceRegister(0x03,0x12) != IOE_OK)
    {
      return IOE_FAILURE;
    }
    /* check if the value is wrote with success */
    if (I2C_ReadDeviceRegister(0x43) != 0x12)
    {
      return IOE_FAILURE;
    }

    /* Set the Functionalities to be Enabled
    XCONV & YCONV to support single-touch by default
    */
    if (I2C_WriteDeviceRegister(0x04,0xC0) != IOE_OK)
    {
      return IOE_FAILURE;
    }
    /* check if the value is wrote with success */
    if (I2C_ReadDeviceRegister(0x44) != 0xC0)
    {
      return IOE_FAILURE;
    }

    /* Start Touch Screen function, in AUTO mode (RATE > 0) */
    I2C_WriteDeviceCommand(0xB0);

    /* Wait for ~10 ms */
    _delay_(1);

    /* Initialize the TS structure to their default values */
    TP_State.TouchDetected = TP_State.X = TP_State.Y = TP_State.Z = 0;
  }
  else
  {
    return IOE_FAILURE;
  }

  /* All configuration done */
  return IOE_OK;
}

/**
  * @brief  Configures the selected pin to be in Alternate function or not.
  * @param  IO_Pin: IO_Pin_x, Where x can be from 0 to 7.
  * @param  NewState: State of the AF for the selected pin, could be
  *         ENABLE or DISABLE.
  * @retval IOE_OK: if all initializations are OK. Other value if error.
  */
uint8_t IOE_IOAFConfig(uint8_t IO_Pin, FunctionalState NewState)
{
  uint8_t tmp = 0;

  /* Get the current state of the GPIO_AF register */
  tmp = I2C_ReadDeviceRegister(IOE_REG_GPIO_AF);

  if (NewState != DISABLE)
  {
    /* Enable the selected pins alternate function */
    tmp |= (uint8_t)IO_Pin;
  }
  else
  {
    /* Disable the selected pins alternate function */
    tmp &= ~(uint8_t)IO_Pin;
  }

  /* Write back the new value in GPIO_AF register */
  I2C_WriteDeviceRegister(IOE_REG_GPIO_AF, tmp);

  /* If all OK return IOE_OK */
  return IOE_OK;
}

/**
  * @brief  Writes a value in a register of the device through I2C.
  * @param  RegisterAddr: The target register address
  * @param  RegisterValue: The target register value to be written
  * @retval IOE_OK: if all operations are OK. Other value if error.
  */
uint8_t I2C_DMA_WriteDeviceRegister(uint8_t RegisterAddr, uint8_t RegisterValue)
{
  uint32_t read_verif = 0;
  uint8_t IOE_BufferTX = 0;

  /* Get Value to be written */
  IOE_BufferTX = RegisterValue;

  /* Configure DMA Peripheral */
  IOE_DMA_Config(IOE_DMA_TX, (uint8_t*)(&IOE_BufferTX));

  /* Enable the I2C peripheral */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on SB Flag */
  IOE_TimeOut = TIMEOUT_MAX;
  while (I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_SB) == RESET)
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the slave address and enable writing operation */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Transmitter);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Transmitter);
  }

  /* Test on ADDR Flag */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_CheckEvent(IOE_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the first address for r/w operations */
  I2C_SendData(IOE_I2C, RegisterAddr);

  /* Test on TXE FLag (data dent) */
  IOE_TimeOut = TIMEOUT_MAX;
  while ((!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_TXE)) && (!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_BTF)))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Enable I2C DMA request */
  I2C_DMACmd(IOE_I2C,ENABLE);

  /* Enable DMA TX Channel */
  DMA_Cmd(IOE_DMA_TX_STREAM, ENABLE);

  /* Wait until DMA Transfer Complete */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!DMA_GetFlagStatus(IOE_DMA_TX_STREAM,IOE_DMA_TX_TCFLAG))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Wait until BTF Flag is set before generating STOP */
  IOE_TimeOut = 0xFF * TIMEOUT_MAX;
  while ((!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_BTF)))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send STOP Condition */
  I2C_GenerateSTOP(IOE_I2C, ENABLE);

  /* Disable DMA TX Channel */
  DMA_Cmd(IOE_DMA_TX_STREAM, DISABLE);

  /* Disable I2C DMA request */
  I2C_DMACmd(IOE_I2C,DISABLE);

  /* Clear DMA TX Transfer Complete Flag */
  DMA_ClearFlag(IOE_DMA_TX_STREAM,IOE_DMA_TX_TCFLAG);

#ifdef VERIFY_WRITTENDATA
  /* Verify (if needed) that the loaded data is correct  */

  /* Read the just written register*/
  read_verif = I2C_ReadDeviceRegister(RegisterAddr);
  /* Load the register and verify its value  */
  if (read_verif != RegisterValue)
  {
    /* Control data wrongly transferred */
    read_verif = IOE_FAILURE;
  }
  else
  {
    /* Control data correctly transferred */
    read_verif = 0;
  }
#endif

  /* Return the verifying value: 0 (Passed) or 1 (Failed) */
  return read_verif;
}

/**
  * @brief  Reads a register of the device through I2C.
  * @param  RegisterAddr: The target register address (between 00x and 0x24)
  * @retval The value of the read register (0xAA if Timeout occurred)
  */
uint8_t I2C_DMA_ReadDeviceRegister(uint8_t RegisterAddr)
{
  uint8_t IOE_BufferRX[2] = {0x00, 0x00};

  /* Configure DMA Peripheral */
  IOE_DMA_Config(IOE_DMA_RX, (uint8_t*)IOE_BufferRX);

  /* Enable DMA NACK automatic generation */
  I2C_DMALastTransferCmd(IOE_I2C, ENABLE);

  /* Enable the I2C peripheral */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on SB Flag */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send device address for write */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Transmitter);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Transmitter);
  }

  /* Test on ADDR Flag */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_CheckEvent(IOE_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send the device's internal address to write to */
  I2C_SendData(IOE_I2C, RegisterAddr);

  /* Test on TXE FLag (data dent) */
  IOE_TimeOut = TIMEOUT_MAX;
  while ((!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_TXE)) && (!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_BTF)))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send START condition a second time */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on SB Flag */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send IOExpander address for read */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Receiver);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Receiver);
  }

  /* Test on ADDR Flag */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_CheckEvent(IOE_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Enable I2C DMA request */
  I2C_DMACmd(IOE_I2C,ENABLE);

  /* Enable DMA RX Channel */
  DMA_Cmd(IOE_DMA_RX_STREAM, ENABLE);

  /* Wait until DMA Transfer Complete */
  IOE_TimeOut = 2 * TIMEOUT_MAX;
  while (!DMA_GetFlagStatus(IOE_DMA_RX_STREAM,IOE_DMA_RX_TCFLAG))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send STOP Condition */
  I2C_GenerateSTOP(IOE_I2C, ENABLE);

  /* Disable DMA RX Channel */
  DMA_Cmd(IOE_DMA_RX_STREAM, DISABLE);

  /* Disable I2C DMA request */
  I2C_DMACmd(IOE_I2C,DISABLE);

  /* Clear DMA RX Transfer Complete Flag */
 DMA_ClearFlag(IOE_DMA_RX_STREAM,IOE_DMA_RX_TCFLAG);

  /* return a pointer to the IOE_Buffer */
  return (uint8_t)IOE_BufferRX[0];
}


/**
  * @brief  Reads a buffer of 2 bytes from the device registers.
  * @param  RegisterAddr: The target register address (between 00x and 0x24)
  * @retval A pointer to the buffer containing the two returned bytes (in halfword).
  */
uint16_t I2C_DMA_ReadDataBuffer(uint32_t RegisterAddr)
{
  uint8_t tmp= 0;
  uint8_t IOE_BufferRX[2] = {0x00, 0x00};

  /* Configure DMA Peripheral */
  IOE_DMA_Config(IOE_DMA_RX, (uint8_t*)IOE_BufferRX);

  /* Enable DMA NACK automatic generation */
  I2C_DMALastTransferCmd(IOE_I2C, ENABLE);

  /* Enable the I2C peripheral */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on SB Flag */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send device address for write */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Transmitter);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Transmitter);
  }

  /* Test on ADDR Flag */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_CheckEvent(IOE_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send the device's internal address to write to */
  I2C_SendData(IOE_I2C, RegisterAddr);

  /* Test on TXE FLag (data dent) */
  IOE_TimeOut = TIMEOUT_MAX;
  while ((!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_TXE)) && (!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_BTF)))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send START condition a second time */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on SB Flag */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C,I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send IO Expander address for read */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Receiver);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Receiver);
  }

  /* Test on ADDR Flag */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_CheckEvent(IOE_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Enable I2C DMA request */
  I2C_DMACmd(IOE_I2C,ENABLE);

  /* Enable DMA RX Channel */
  DMA_Cmd(IOE_DMA_RX_STREAM, ENABLE);

  /* Wait until DMA Transfer Complete */
  IOE_TimeOut = 2 * TIMEOUT_MAX;
  while (!DMA_GetFlagStatus(IOE_DMA_RX_STREAM, IOE_DMA_RX_TCFLAG))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send STOP Condition */
  I2C_GenerateSTOP(IOE_I2C, ENABLE);

  /* Disable DMA RX Channel */
  DMA_Cmd(IOE_DMA_RX_STREAM, DISABLE);

  /* Disable I2C DMA request */
  I2C_DMACmd(IOE_I2C,DISABLE);

  /* Clear DMA RX Transfer Complete Flag */
  DMA_ClearFlag(IOE_DMA_RX_STREAM,IOE_DMA_RX_TCFLAG);

  /* Reorganize received data */
  tmp = IOE_BufferRX[0];
  IOE_BufferRX[0] = IOE_BufferRX[1];
  IOE_BufferRX[1] = tmp;

  /* return the data */
  return (uint16_t) ((uint16_t)IOE_BufferRX[0] | (uint16_t)IOE_BufferRX[1]<< 8);
}

/**
  * @brief  Get Touch Panel X and Y position value
  * @param  None
  * @retval None
  */
static void IOE_TP_Read_sx8651(uint16_t *X, uint16_t *Y)
{
  uint8_t  dataXYZ[4];
  uint32_t uldataXYZ;
  int16_t x,y;
  I2C_ReadDeviceData4(dataXYZ);

  /* Calculate positions values */
  uldataXYZ = (dataXYZ[0] << 8)|(dataXYZ[1] << 0);
  if(uldataXYZ & DATA_CHANNEL_INVALID)
  {
    *X = 0;
    *Y = 0;
    return;
  }
  else if((uldataXYZ & DATA_CHANNEL_MASK) == DATA_CHANNEL_MASK_X)
    *X = uldataXYZ & DATA_CHANNEL_DATA;
  else if((uldataXYZ & DATA_CHANNEL_MASK) == DATA_CHANNEL_MASK_Y)
    *Y = uldataXYZ & DATA_CHANNEL_DATA;
  else if((uldataXYZ & DATA_CHANNEL_MASK) == DATA_CHANNEL_MASK_SEQ)
    *X = uldataXYZ & DATA_CHANNEL_DATA;

  uldataXYZ = (dataXYZ[2] << 8)|(dataXYZ[3] << 0);
  if(uldataXYZ & DATA_CHANNEL_INVALID)
  {
    *X = 0;
    *Y = 0;
    return;
  }
  else if((uldataXYZ & DATA_CHANNEL_MASK) == DATA_CHANNEL_MASK_X)
    *X = uldataXYZ & DATA_CHANNEL_DATA;
  else if((uldataXYZ & DATA_CHANNEL_MASK) == DATA_CHANNEL_MASK_Y)
    *Y = uldataXYZ & DATA_CHANNEL_DATA;
  else if((uldataXYZ & DATA_CHANNEL_MASK) == DATA_CHANNEL_MASK_SEQ)
    *Y = uldataXYZ & DATA_CHANNEL_DATA;

  x =  251 - (*X/15);
  y =  336 - (*Y/11);

    if(y <= 0)
  {
    y = 0;
  }
  else if (y >= 320)
  {
    y = 319;
  }

    if(x <= 0)
  {
    x = 0;
  }
  else if (x >= 240)
  {
    x = 239;
  }


  *X = x;
  *Y = y;

  /* Reset FIFO, not available for SX8651 */
}


 /**
  * @brief  Return Touch Panel X position value
  * @param  None
  * @retval X position.
  */
static uint16_t IOE_TP_Read_X_stmpe811(void)
{
  int32_t x, xr;

  /* Read x value from DATA_X register */
  x = I2C_ReadDataBuffer(IOE_REG_TP_DATA_X);

  /* x value first correction */
  if(x <= 3000)
  {
  x = 3870 - x;
  }
  else
  {
   x = 3800 - x;
  }

  /* x value second correction */
  xr = x / 15;

  /* return x position value */
  if(xr <= 0)
  {
    xr = 0;
  }
  else if (xr > 240)
  {
    xr = 239;
  }
  else
  {}
  return (uint16_t)(xr);
}

/**
  * @brief  Return Touch Panel Y position value
  * @param  None
  * @retval Y position.
  */
static uint16_t IOE_TP_Read_Y_stmpe811(void)
{
  int32_t y, yr;

  /* Read y value from DATA_Y register */
  y = I2C_ReadDataBuffer(IOE_REG_TP_DATA_Y);

  /* y value first correction */
#if defined (USE_STM32F429I_DISCO_REVC)
  y -= 360;
#else /* USE_STM32F429I_DISCO_REVD */
  /* y axis is swapped in STM32F429I_Disco RevD and RevE boards */
  y = 3650 - y;
#endif /* USE_STM32F429I_DISCO_REVC */
  
  /* y value second correction */
  yr = y / 11;

  /* return y position value */
  if(yr <= 0)
  {
    yr = 0;
  }
  else if (yr > 320)
  {
    yr = 319;
  }
  else
  {}
  return (uint16_t)(yr);
}

/**
  * @brief  Return Touch Panel Z position value
  * @param  None
  * @retval Z position.
  */
static uint16_t IOE_TP_Read_Z_stmpe811(void)
{
  uint32_t z;

  /* Read z value from DATA_Z register */
  z = I2C_ReadDataBuffer(IOE_REG_TP_DATA_Z);

  /* return z position value */
  if(z <= 0)
    z = 0;
  return (uint16_t)(z);
}

/**
  * @brief  Initializes the GPIO pins used by the IO expander.
  * @param  None
  * @retval None
  */
static void IOE_GPIO_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable IOE_I2C and IOE_I2C_GPIO_PORT & Alternate Function clocks */
  RCC_APB1PeriphClockCmd(IOE_I2C_CLK, ENABLE);
  RCC_AHB1PeriphClockCmd(IOE_I2C_SCL_GPIO_CLK | IOE_I2C_SDA_GPIO_CLK |
                         IOE_IT_GPIO_CLK, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  /* Reset IOE_I2C IP */
  RCC_APB1PeriphResetCmd(IOE_I2C_CLK, ENABLE);
  /* Release reset signal of IOE_I2C IP */
  RCC_APB1PeriphResetCmd(IOE_I2C_CLK, DISABLE);

  /* Connect PXx to I2C_SCL*/
  GPIO_PinAFConfig(IOE_I2C_SCL_GPIO_PORT, IOE_I2C_SCL_SOURCE, IOE_I2C_SCL_AF);
  /* Connect PXx to I2C_SDA*/
  GPIO_PinAFConfig(IOE_I2C_SDA_GPIO_PORT, IOE_I2C_SDA_SOURCE, IOE_I2C_SDA_AF);

  /* IOE_I2C SCL and SDA pins configuration */
  GPIO_InitStructure.GPIO_Pin = IOE_I2C_SCL_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
  GPIO_Init(IOE_I2C_SCL_GPIO_PORT, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = IOE_I2C_SDA_PIN;
  GPIO_Init(IOE_I2C_SDA_GPIO_PORT, &GPIO_InitStructure);

}

/**
  * @brief  Configure the I2C Peripheral used to communicate with IO_Expanders.
  * @param  None
  * @retval None
  */
static void IOE_I2C_Config(void)
{
  I2C_InitTypeDef I2C_InitStructure;

  /* If the I2C peripheral is already enabled, don't reconfigure it */
  if ((IOE_I2C->CR1 & I2C_CR1_PE) == 0)
  {
    /* IOE_I2C configuration */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;

    /* Initialize the I2C peripheral */
    I2C_Init(IOE_I2C, &I2C_InitStructure);

    /* Enable the I2C peripheral */
    I2C_Cmd(IOE_I2C, ENABLE);
  }
}

/**
  * @brief  Configure the DMA Peripheral used to handle communication via I2C.
  * @param  None
  * @retval None
  */
static void IOE_DMA_Config(IOE_DMADirection_TypeDef Direction, uint8_t* buffer)
{
  DMA_InitTypeDef DMA_InitStructure;

  RCC_AHB1PeriphClockCmd(IOE_DMA_CLK, ENABLE);

  /* Initialize the DMA_Channel member */
  DMA_InitStructure.DMA_Channel = IOE_DMA_CHANNEL;

  /* Initialize the DMA_PeripheralBaseAddr member */
  DMA_InitStructure.DMA_PeripheralBaseAddr = IOE_I2C_DR;

  /* Initialize the DMA_Memory0BaseAddr member */
  DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)buffer;

  /* Initialize the DMA_PeripheralInc member */
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;

  /* Initialize the DMA_MemoryInc member */
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;

  /* Initialize the DMA_PeripheralDataSize member */
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;

  /* Initialize the DMA_MemoryDataSize member */
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;

  /* Initialize the DMA_Mode member */
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;

  /* Initialize the DMA_Priority member */
  DMA_InitStructure.DMA_Priority = DMA_Priority_Low;

  /* Initialize the DMA_FIFOMode member */
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;

  /* Initialize the DMA_FIFOThreshold member */
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;

  /* Initialize the DMA_MemoryBurst member */
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;

  /* Initialize the DMA_PeripheralBurst member */
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

  /* If using DMA for Reception */
  if (Direction == IOE_DMA_RX)
  {
    /* Initialize the DMA_DIR member */
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;

    /* Initialize the DMA_BufferSize member */
    DMA_InitStructure.DMA_BufferSize = 2;

    DMA_DeInit(IOE_DMA_RX_STREAM);

    DMA_Init(IOE_DMA_RX_STREAM, &DMA_InitStructure);
  }
  /* If using DMA for Transmission */
  else if (Direction == IOE_DMA_TX)
  {
    /* Initialize the DMA_DIR member */
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;

    /* Initialize the DMA_BufferSize member */
    DMA_InitStructure.DMA_BufferSize = 1;

    DMA_DeInit(IOE_DMA_TX_STREAM);

    DMA_Init(IOE_DMA_TX_STREAM, &DMA_InitStructure);
  }
}

/**
  * @brief  Writes a value in a register of the device through I2C.
  * @param  RegisterAddr: The target register address
  * @param  RegisterValue: The target register value to be written
  * @retval IOE_OK: if all operations are OK. Other value if error.
  */
uint8_t I2C_WriteDeviceRegister(uint8_t RegisterAddr, uint8_t RegisterValue)
{
  uint32_t read_verif = 0;

  /* Begin the configuration sequence */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the slave address and enable writing operation */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Transmitter);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Transmitter);
  }

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV8_1 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the first address for r/w operations */
  I2C_SendData(IOE_I2C, RegisterAddr);

  /* Test on EV8 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Prepare the register value to be sent */
  I2C_SendData(IOE_I2C, RegisterValue);

  /* Test on EV8_2 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while ((!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_BTF)))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* End the configuration sequence */
  I2C_GenerateSTOP(IOE_I2C, ENABLE);

#ifdef VERIFY_WRITTENDATA
  /* Verify (if needed) that the loaded data is correct  */

  /* Read the just written register*/
  read_verif = IOE_I2C_ReadDeviceRegister(RegisterAddr);

  /* Load the register and verify its value  */
  if (read_verif != RegisterValue)
  {
    /* Control data wrongly transferred */
    read_verif = IOE_FAILURE;
  }
  else
  {
    /* Control data correctly transferred */
    read_verif = 0;
  }
#endif

  /* Return the verifying value: 0 (Passed) or 1 (Failed) */
  return read_verif;

}

/**
  * @brief  Send command.
  * @param  Command: The command to be sent
  * @retval IOE_OK: if all operations are OK. Other value if error.
  */
uint8_t  I2C_WriteDeviceCommand(uint8_t Command)
{
  uint32_t read_verif = 0;

  /* Begin the configuration sequence */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the slave address and enable writing operation */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Transmitter);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Transmitter);
  }

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV8_1 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the first address for r/w operations */
  I2C_SendData(IOE_I2C, Command);

  /* Test on EV8 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* End the configuration sequence */
  I2C_GenerateSTOP(IOE_I2C, ENABLE);

  /* Return the verifying value: 0 (Passed) or 1 (Failed) */
  return read_verif;
}

/**
  * @brief  Reads a register of the device through I2C without DMA.
  * @param  RegisterAddr: The target register address (between 00x and 0x24)
  * @retval The value of the read register (0xAA if Timeout occurred)
  */
uint8_t I2C_ReadDeviceRegister(uint8_t RegisterAddr)
{
  uint8_t tmp = 0;

  /* Enable the I2C peripheral */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

    /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }
  /* Disable Acknowledgement */
  I2C_AcknowledgeConfig(IOE_I2C, DISABLE);

  /* Transmit the slave address and enable writing operation */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Transmitter);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Transmitter);
  }

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV8 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the first address for r/w operations */
  I2C_SendData(IOE_I2C, RegisterAddr);

  /* Test on EV8 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while ((!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_BTF)))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Regenerate a start condition */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the slave address and enable writing operation */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Receiver);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Receiver);
  }

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

    /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV7 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_RXNE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* End the configuration sequence */
  I2C_GenerateSTOP(IOE_I2C, ENABLE);

  /* Load the register value */
  tmp = I2C_ReceiveData(IOE_I2C);

  /* Enable Acknowledgement */
  I2C_AcknowledgeConfig(IOE_I2C, ENABLE);

  /* Return the read value */
  return tmp;

}

/**
  * @brief  Reads a register of the stmpe811 device through I2C without DMA.
  * @param  RegisterAddr: The target register address (between 00x and 0x24)
  * @retval The value of the read register (0xAA if Timeout occurred)
  */
uint8_t I2C_ReadDeviceID_stmpe811(uint8_t RegisterAddr)
{
  uint8_t tmp = 0;

  /* Enable the I2C peripheral */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

    /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }
  /* Disable Acknowledgement */
  I2C_AcknowledgeConfig(IOE_I2C, DISABLE);

  /* Transmit the slave address and enable writing operation */
  I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Transmitter);

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV8 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the first address for r/w operations */
  I2C_SendData(IOE_I2C, RegisterAddr);

  /* Test on EV8 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while ((!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_BTF)))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Regenerate a start condition */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the slave address and enable writing operation */
  I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Receiver);

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

    /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV7 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_RXNE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* End the configuration sequence */
  I2C_GenerateSTOP(IOE_I2C, ENABLE);

  /* Load the register value */
  tmp = I2C_ReceiveData(IOE_I2C);

  /* Enable Acknowledgement */
  I2C_AcknowledgeConfig(IOE_I2C, ENABLE);

  /* Return the read value */
  return tmp;
}

/**
  * @brief  Reads a register of the sx8651 device through I2C without DMA.
  * @param  RegisterAddr: The target register address (between 00x and 0x24)
  * @retval The value of the read register (0xAA if Timeout occurred)
  */
uint8_t I2C_ReadDeviceID_sx8651(uint8_t RegisterAddr)
{
  uint8_t tmp = 0;

  /* Enable the I2C peripheral */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

    /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }
  /* Disable Acknowledgement */
  I2C_AcknowledgeConfig(IOE_I2C, DISABLE);

  /* Transmit the slave address and enable writing operation */
  I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Transmitter);

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV8 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the first address for r/w operations */
  I2C_SendData(IOE_I2C, RegisterAddr);

  /* Test on EV8 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while ((!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE)) || (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_BTF)))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Regenerate a start condition */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Transmit the slave address and enable writing operation */
  I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Receiver);

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

    /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV7 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_RXNE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* End the configuration sequence */
  I2C_GenerateSTOP(IOE_I2C, ENABLE);

  /* Load the register value */
  tmp = I2C_ReceiveData(IOE_I2C);

  /* Enable Acknowledgement */
  I2C_AcknowledgeConfig(IOE_I2C, ENABLE);

  /* Return the read value */
  return tmp;
}

/**
  *@brief  Reads a buffer of 4 bytes from the device registers.
  */
uint8_t I2C_ReadDeviceData4(uint8_t *Buffer)
{
  /* Enable the I2C peripheral */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send IO Expander address for read */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Receiver);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Receiver);
  }

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV7 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_RXNE ))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Read the first byte from the IO Expander */
  Buffer[0] = I2C_ReceiveData(IOE_I2C);

   while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_RXNE ))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Read the second byte from the IO Expander */
  Buffer[1] = I2C_ReceiveData(IOE_I2C);
   while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_RXNE ))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }
  Buffer[2] = I2C_ReceiveData(IOE_I2C);
    while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_RXNE ))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }
  /* Read the second byte from the IO Expander */
  Buffer[3] = I2C_ReceiveData(IOE_I2C);

  /* Enable Acknowledgement and reset POS bit to be ready for another reception */
  I2C_AcknowledgeConfig(IOE_I2C, ENABLE);
  I2C_NACKPositionConfig(IOE_I2C, I2C_NACKPosition_Current);
   I2C_GenerateSTOP(IOE_I2C, ENABLE);
  /* return the data */
  return IOE_OK;
}

/**
  * @brief  Reads a buffer of 2 bytes from the device registers.
  * @param  RegisterAddr: The target register adress (between 00x and 0x24)
  * @retval The data in the buffer containing the two returned bytes (in halfword).
  */
uint16_t I2C_ReadDataBuffer(uint32_t RegisterAddr)
{
  uint8_t IOE_BufferRX[2] = {0x00, 0x00};

  /* Enable the I2C peripheral */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send device address for write */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Transmitter);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Transmitter);
  }

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV8 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_TXE))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send the device's internal address to write to */
  I2C_SendData(IOE_I2C, RegisterAddr);

  /* Send START condition a second time */
  I2C_GenerateSTART(IOE_I2C, ENABLE);

  /* Test on EV5 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_SB))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send IO Expander address for read */
  if ( Device_ID == (uint16_t)STMPE811_ID )
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_STMPE811, I2C_Direction_Receiver);
  }
  else
  {
    I2C_Send7bitAddress(IOE_I2C, IOE_ADDR_SX8651, I2C_Direction_Receiver);
  }

  /* Test on EV6 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_ADDR))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Disable Acknowledgement and set Pos bit */
  I2C_AcknowledgeConfig(IOE_I2C, DISABLE);
  I2C_NACKPositionConfig(IOE_I2C, I2C_NACKPosition_Next);

  /* Read status register 2 to clear ADDR flag */
  IOE_I2C->SR2;

  /* Test on EV7 and clear it */
  IOE_TimeOut = TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(IOE_I2C, I2C_FLAG_BTF))
  {
    if (IOE_TimeOut-- == 0) return(IOE_TimeoutUserCallback());
  }

  /* Send STOP Condition */
  I2C_GenerateSTOP(IOE_I2C, ENABLE);

  /* Read the first byte from the IO Expander */
  IOE_BufferRX[1] = I2C_ReceiveData(IOE_I2C);

  /* Read the second byte from the IO Expander */
  IOE_BufferRX[0] = I2C_ReceiveData(IOE_I2C);

  /* Enable Acknowledgement and reset POS bit to be ready for another reception */
  I2C_AcknowledgeConfig(IOE_I2C, ENABLE);
  I2C_NACKPositionConfig(IOE_I2C, I2C_NACKPosition_Current);

  /* return the data */
  return ((uint16_t) IOE_BufferRX[0] | ((uint16_t)IOE_BufferRX[1]<< 8));
}

/**
  * @brief  Configures the IO expander Interrupt line and GPIO in EXTI mode.
  * @param  None
  * @retval None
  */
static void IOE_EXTI_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;

  /* Enable GPIO clock */
  RCC_AHB1PeriphClockCmd(IOE_IT_GPIO_CLK, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  /* Configure Button pin as input floating */
  GPIO_InitStructure.GPIO_Pin = IOE_IT_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(IOE_IT_GPIO_PORT, &GPIO_InitStructure);

  /* Connect Button EXTI Line to Button GPIO Pin */
  SYSCFG_EXTILineConfig(IOE_IT_EXTI_PORT_SOURCE, IOE_IT_EXTI_PIN_SOURCE);

  /* Configure Button EXTI line */
  EXTI_InitStructure.EXTI_Line = IOE_IT_EXTI_LINE;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set Button EXTI Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = IOE_IT_EXTI_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

#ifndef USE_TIMEOUT_USER_CALLBACK
/**
  * @brief  IOE_TIMEOUT_UserCallback
  * @param  None
  * @retval 0
  */
uint8_t IOE_TimeoutUserCallback(void)
{
  I2C_InitTypeDef I2C_InitStructure;

  I2C_GenerateSTOP(IOE_I2C, ENABLE);
  I2C_SoftwareResetCmd(IOE_I2C, ENABLE);
  I2C_SoftwareResetCmd(IOE_I2C, DISABLE);

  IOE_GPIO_Config();

  /* CODEC_I2C peripheral configuration */
  I2C_DeInit(IOE_I2C);
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x00;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;

  /* Enable the I2C peripheral */
  I2C_Cmd(IOE_I2C, ENABLE);
  I2C_Init(IOE_I2C, &I2C_InitStructure);

  return 0;
}
#endif /* !USE_TIMEOUT_USER_CALLBACK */

#ifndef USE_Delay
/**
  * @brief  Inserts a delay time.
  * @param  nCount: specifies the delay time length.
  * @retval None
  */
static void delay(__IO uint32_t nCount)
{
  __IO uint32_t index = 0;
  for(index = (100000 * nCount); index != 0; index--)
  {
  }
}
#endif /* USE_Delay */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

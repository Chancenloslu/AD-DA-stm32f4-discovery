#include "display.h"

/* Private define ------------------------------------------------------------*/
	#define TIME_AXI	 "----------------------------->"
//	#define VOLTAGE_AXIS_1 "^3.0"
//	#define VOLTAGE_AXIS_2 '|'
//	#define VOLTAGE_AXI_COLUMN 3
	#define MESSAGE1   "  ADC conversion w/DMA          "
	#define MESSAGE1_1 "  continuouslyTransfer          " 
	#define MESSAGE2   " ADC Ch0 Conv    " 
	#define MESSAGE2_1 "    2.4Msps      "
	#define MESSAGE3   "Connect voltage "
	#define MESSAGE4   "to ADC Ch0   "
	#define MESSAGE5   " ADC1 = %d,%d V"
	#define MESSAGE6   " ADC2 = %d,%d V"
	#define MESSAGE7   " ADC3 = %d,%d V"
	#define LINENUM            	0x15
	#define FONTSIZE         	Font8x12
	#define VOLTAGE_AXI_COLUMN 	3
/* Private variables ---------------------------------------------------------*/
	Point Origin_point;
	Point lastPoint;
	Point newPoint;
	Point polyLineVertex[25];
	uint16_t indexVertex = 0;
	uint16_t indexRow;
void Axis_init(void) {
	Origin_point.X = 20;
	Origin_point.Y = 10;

	newPoint = Origin_point;
	lastPoint = Origin_point;
	indexRow = 0;
}
/**
  * @brief  Display Init (LCD)
  * @param  None
  * @retval None
  */
void Display_Init(void)
{
  /* Initialize the LCD */
  LCD_Init();
  LCD_LayerInit();
  
  /* Eable the LTDC */
  LTDC_Cmd(ENABLE);
  
  /* Set LCD Background Layer  */
  LCD_SetLayer(LCD_BACKGROUND_LAYER);
  
  /* Clear the Background Layer */ 
  LCD_Clear(LCD_COLOR_WHITE);
  
  /* Configure the transparency for background */
  LCD_SetTransparency(0);
  
  /* Set LCD Foreground Layer  */
  LCD_SetLayer(LCD_FOREGROUND_LAYER);

  /* Configure the transparency for foreground */
  LCD_SetTransparency(200);
  
  /* Clear the Foreground Layer */ 
  LCD_Clear(LCD_COLOR_WHITE);
  
  /* Set the LCD Back Color and Text Color*/
  LCD_SetBackColor(LCD_COLOR_WHITE);
  LCD_SetTextColor(LCD_COLOR_BLUE); 
  
    /* Set the LCD Text size */
  LCD_SetFont(&FONTSIZE);
	
	Axis_init();
//  LCD_DisplayStringLine(LCD_LINE_25, (uint8_t*)TIME_AXI);
//  
//	LCD_DisplayStringLine(LINE(1), (uint8_t*)VOLTAGE_AXIS_1);
//	for(int i = 2; i <= 25; i++) {
//		LCD_DisplayChar(LINE(i), VOLTAGE_AXI_COLUMN, VOLTAGE_AXIS_2);
//	}
//  LCD_DisplayStringLine(LINE(LINENUM), (uint8_t*)MESSAGE1);
//  LCD_DisplayStringLine(LINE(LINENUM + 1), (uint8_t*)MESSAGE1_1);
//  LCD_DisplayStringLine(LINE(0x17), (uint8_t*)"                               ");
  
  /* Set the LCD Text size */
//  LCD_SetFont(&Font16x24);
  
//  LCD_DisplayStringLine(LCD_LINE_0, (uint8_t*)MESSAGE2);
//  LCD_DisplayStringLine(LCD_LINE_1, (uint8_t*)MESSAGE2_1);
  
  /* Set the LCD Back Color and Text Color*/
  LCD_SetBackColor(LCD_COLOR_WHITE);
  LCD_SetTextColor(LCD_COLOR_BLUE); 
  
//  LCD_DisplayStringLine(LCD_LINE_2, (uint8_t*)MESSAGE3);
//  LCD_DisplayStringLine(LCD_LINE_3, (uint8_t*)MESSAGE4);
}


void Vertex_record(uint32_t ADCVoltageConvertedValue){
	
	uint32_t uwVoltage =0, uwMVoltage=0, uwMMVoltage;
	uint8_t aTextBuffer[50];
	
	uwVoltage = (ADCVoltageConvertedValue)/1000;
	uwMVoltage = (ADCVoltageConvertedValue%1000)/100;
	uwMMVoltage = (ADCVoltageConvertedValue%100)/10;
	
	newPoint.X = lastPoint.X + 10;
	newPoint.Y = Origin_point.Y + uwVoltage*100 + uwMVoltage*10 + uwMMVoltage;
	
	if(newPoint.X > 220){
		newPoint.X = Origin_point.X;
		LCD_Clear(LCD_COLOR_WHITE);
	}else {
		LCD_DrawUniLine(lastPoint.X, lastPoint.Y , newPoint.X, newPoint.Y);
	}
	lastPoint = newPoint;
}
/**
  * @brief  Display ADCs converted values on LCD
  * @param  None
  * @retval None
  */
void Display(uint32_t uwADCConvertedVoltage)
{
	uint32_t uwVoltage =0, uwMVoltage=0, uwMMVoltage;
	uint8_t aTextBuffer[50];
	
	uwVoltage = (uwADCConvertedVoltage)/1000;
	uwMVoltage = (uwADCConvertedVoltage%1000)/100;
	uwMMVoltage = (uwADCConvertedVoltage%100)/10;
	
//	newPoint.X = newPoint.X + 1;
//	newPoint.Y = Origin_point.Y + uwVoltage*100 + uwMVoltage*10 + uwMMVoltage;
//	
//	if(newPoint.X > 220){
//		newPoint.X = Origin_point.X;
//		LCD_Clear(LCD_COLOR_WHITE);
//		LCD_PolyLineRelative(polyLineVertex, indexVertex);
//		indexVertex = 0;
//	}else {
//		
//	}
//	polyLineVertex[indexVertex] = newPoint;
//	indexVertex++;
	
	//lastPoint = newPoint;
	
	sprintf((char*)aTextBuffer, MESSAGE5, uwVoltage, uwMVoltage);
	if(indexRow < 26) {
		LCD_DisplayStringLine(LINE(indexRow), (uint8_t*)aTextBuffer);
		indexRow++;
	}else{
		indexRow = 1;
		LCD_Clear(LCD_COLOR_WHITE);
	}
}
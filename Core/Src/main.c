/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_customhid.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceHS;

static uint8_t reporteHID[8] =
{
    0x00, 0x80,   // X  = 32768
    0x00, 0x80,   // Y  = 32768
    0x00, 0x80,   // Z  = 32768
    0x00, 0x80    // Rz = 32768
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  uint32_t ultimoEnvioUSB = 0;
  static uint8_t mensajeUSB[512] = {0};

  uint8_t tramaSBUS[25] = {0};
  uint8_t byteSBUS = 0;
  uint8_t i_trama = 0;
  uint32_t tramasValidas = 0;
  uint16_t canales[16] = {0};
  uint8_t failsafe_flag = 0;
  uint8_t lost_flag = 0;
  uint32_t ultimaTrama = 0;
  uint32_t now = 0;
  uint32_t ultimoByte = 0;
  uint8_t existeByteAnterior = 0;
  uint8_t inicioPermitido = 0;


  uint32_t pausasDetectadas = 0;
  uint32_t tramasRechazadas = 0;

  uint32_t erroresUART = 0;
  uint32_t overrunsUART = 0;
  uint8_t tramaRaw[25] = {0};

  uint32_t ultimoHID = 0;


  // Señal de arranque: cambiar el LED dos veces
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
  HAL_Delay(500);

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
  HAL_Delay(500);

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
      HAL_StatusTypeDef estado;
	  estado = HAL_UART_Receive(
	  	          &huart1,
	  	          &byteSBUS,
	  	          1,
	  	          20
	  	      );

	  if (estado == HAL_ERROR)
	  {
	      erroresUART++;

	      if ((HAL_UART_GetError(&huart1) & HAL_UART_ERROR_ORE) != 0U)
	      {
	          overrunsUART++;
	      }
	  }

	  now = HAL_GetTick();

	  if (estado == HAL_OK){

		  uint8_t pausaDetectada =
		          existeByteAnterior &&
		          ((uint32_t)(now - ultimoByte) >= 3U);

		  ultimoByte = now;
		      existeByteAnterior = 1;


		  if (pausaDetectada){
			  i_trama = 0;
			  inicioPermitido = 1;

			  pausasDetectadas++;

		  }

		  if(i_trama == 0){
			  //Posible encabezado
			  if(inicioPermitido && byteSBUS == 0x0F){
				  tramaSBUS[0] = byteSBUS;
				  i_trama += 1;
			  }
			  inicioPermitido = 0;

		  }
		  else{
			  tramaSBUS[i_trama] = byteSBUS;
			  i_trama++;
			  if(i_trama == 25){
				  if(tramaSBUS[24] == 0x00){
					  ultimaTrama = now;
					  tramasValidas++;

					  for (uint8_t j = 0; j < 25U; j++)
					  {
					      tramaRaw[j] = tramaSBUS[j];
					  }

					  //Trama completada
					  // decodificar canalaes

					  failsafe_flag = ((tramaSBUS[23] & (1U << 3)) != 0U);
					  lost_flag = ((tramaSBUS[23] & (1U << 2)) != 0U);

					  for (uint8_t i = 0; i < 16; i++)
					  {
					      uint16_t bitInicial = i * 11U;
					      uint8_t byteInicial = 1U + bitInicial / 8U;
					      uint8_t desplazamiento = bitInicial % 8U;

					      uint32_t ventana;

					      ventana = (uint32_t)tramaSBUS[byteInicial] |
					    		  ((uint32_t)tramaSBUS[byteInicial+1U] << 8U);
					      if (byteInicial + 2U <= 22U)
					      {
					          // Añadir el tercer byte a ventana
					    	  ventana |= ((uint32_t)tramaSBUS[byteInicial+2U] << 16U);
					      }
					      canales[i] = (ventana >> desplazamiento) & 0x7FF;
					  }


				  }
				  else{
					  tramasRechazadas++;
				  }
				  inicioPermitido = 0;
				  i_trama = 0;
			  }

		  }
	  }
	  else{
		  i_trama = 0;
		  inicioPermitido = 0;

	  }

	  uint8_t comunicacionReciente =
	      (tramasValidas > 0U) &&
	      ((uint32_t)(now - ultimaTrama) < 100U);

	  uint8_t datosConfiables =
	      comunicacionReciente &&
	      !failsafe_flag &&
	      !lost_flag;

	  if (datosConfiables)
	  {
	      // Podemos utilizar canales[0], canales[1], etc.
		  if (canales[3] > 1000U){
			  HAL_GPIO_WritePin(GPIOE, LED_Pin, GPIO_PIN_SET);
		  }
		  else{
			  HAL_GPIO_WritePin(GPIOE, LED_Pin, GPIO_PIN_RESET);
		  }
	  }
	  else{
		  HAL_GPIO_WritePin(GPIOE, LED_Pin, GPIO_PIN_RESET);

	  }

    
uint32_t tiempoHID = HAL_GetTick();

if ((uint32_t)(tiempoHID - ultimoHID) >= 20U)
{
    ultimoHID = tiempoHID;

    if (hUsbDeviceHS.dev_state == USBD_STATE_CONFIGURED)
    {
    	if (((tiempoHID / 1000U) % 2U) == 0U)
    	{
    	    reporteHID[0] = 0x00;
    	    reporteHID[1] = 0x00;
    	}
    	else
    	{
    	    reporteHID[0] = 0xFF;
    	    reporteHID[1] = 0xFF;
    	}
        USBD_CUSTOM_HID_SendReport(
            &hUsbDeviceHS,
            reporteHID,
            sizeof(reporteHID)
        );
    }
}




	  
  }



  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = 64;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 100000;
  huart1.Init.WordLength = UART_WORDLENGTH_9B;
  huart1.Init.StopBits = UART_STOPBITS_2;
  huart1.Init.Parity = UART_PARITY_EVEN;
  huart1.Init.Mode = UART_MODE_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXINVERT_INIT;
  huart1.AdvancedInit.RxPinLevelInvert = UART_ADVFEATURE_RXINV_ENABLE;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

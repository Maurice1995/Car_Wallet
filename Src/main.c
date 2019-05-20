/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BOARD1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

SPI_HandleTypeDef hspi2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
CAN_FilterTypeDef sFilterConfig;
CAN_TxHeaderTypeDef TxMessage;
CAN_RxHeaderTypeDef RxMessage;
uint8_t               TxData[8];
uint8_t               RxData[8];
uint32_t              TxMailbox;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
void Can_Ayarla();
int spiReadStatus(uint8_t *readBuffer);
int spiWriteStatus(uint32_t status);
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
  MX_CAN1_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */
  Can_Ayarla();

     if (HAL_CAN_Start(&hcan1) != HAL_OK)
      {
        Error_Handler();
      }

     TxData[0] = 0xCA;
     TxData[1] = 0xFE;

  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);


  while (1)
  {

    #ifdef BOARD1
    if(spiTest() < 0)
    {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
    }
    HAL_Delay(1000);
    #endif

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

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 21;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PE4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

}

/* USER CODE BEGIN 4 */

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{


	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &RxMessage, RxData);


		        if(((RxData[0]<<8 | RxData[1]) != 0xCAFE))
		        {
		          /* Rx message Error */
		        	GPIOD->ODR = 0x00 << 12;

		        }
		        else
		        {
		        	GPIOD->ODR = RxData[2] << 12;

		        }


}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  static uint8_t ledNum;

	if (GPIO_Pin == GPIO_PIN_0)
	{
		/* Request transmission */
    TxData[2] = ++ledNum;

    if(HAL_CAN_AddTxMessage(&hcan1, &TxMessage, TxData, &TxMailbox) != HAL_OK)
    {
      /* Transmission request Error */
      Error_Handler();
    }
    /* Wait transmission complete */
    while(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) != 3) {}

	}

}
int spiTest()
{
  const uint32_t num = 0xBEEFCAFE;
  uint8_t buffer[4];
  if(spiWriteStatus(num) != HAL_OK)
    {
      /* Transmission request Error */
      return -1;
    }
    for(int i = 0; i< 1000000 ; i++);
    if( spiReadStatus(&buffer) != HAL_OK)
    {
      /* Transmission request Error */
      return -1;
    }
    if(num != (uint32_t)(buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24) )) // Test Code to check reflected SPI message
    {
      return -1;
    }
    return 0;
}
int spiWriteStatus(uint32_t status)
{
    const uint8_t c = 0x01;
    static uint8_t statusBuffer[4];

    int rv = HAL_ERROR;

    for(int i=0 ;i<4; i++)
    {
      statusBuffer[i] = ((status >> (i*8)) & 0xFF);
    }

    HAL_GPIO_WritePin(GPIOE,GPIO_PIN_4,GPIO_PIN_RESET);

    rv = HAL_SPI_Transmit_IT(&hspi2,&c,1);

    if (rv != HAL_OK)
    {
      return rv;
    }
    while(HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
    {}

    for(int i=0 ;i<4; i++)
    {
      rv = HAL_SPI_Transmit_IT(&hspi2,statusBuffer+i,1);

      if (rv != HAL_OK)
      {
        return rv;
      }

      while(HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
      {}


    }
    HAL_GPIO_WritePin(GPIOE,GPIO_PIN_4,GPIO_PIN_SET);

    return HAL_OK;


}
int spiReadStatus(uint8_t *readBuffer)
{
      uint8_t c = 0x04;
      int rv = HAL_ERROR;

      memset(readBuffer, 0, sizeof(uint32_t));

      HAL_GPIO_WritePin(GPIOE,GPIO_PIN_4,GPIO_PIN_RESET);

      rv = HAL_SPI_Transmit_IT(&hspi2,&c,1);

      if (rv != HAL_OK)
      {
        return rv;
      }

      while(HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
      {}

      rv = HAL_SPI_Receive_IT(&hspi2,readBuffer,4);

      if (rv != HAL_OK)
      {
        return rv;
      }

      while(HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
      {}

      HAL_GPIO_WritePin(GPIOE,GPIO_PIN_4,GPIO_PIN_SET);
      return HAL_OK;

}


void Can_Ayarla(){

	sFilterConfig.FilterBank= 0;
	sFilterConfig.FilterMode= CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale= CAN_FILTERSCALE_32BIT;
  #ifdef BOARD1
	sFilterConfig.FilterIdHigh= 0xEF<<5;  //On second node change this to 0xBE
  #elif defined(BOARD2)
  sFilterConfig.FilterIdHigh= 0xBE<<5;
  #endif
	sFilterConfig.FilterIdLow= 0x0000;
	sFilterConfig.FilterMaskIdHigh= 0xFF<<5;  // All four bytes must match to accept message
	sFilterConfig.FilterMaskIdLow= 0x0000;
	sFilterConfig.FilterFIFOAssignment= CAN_RX_FIFO0;
	sFilterConfig.FilterActivation= ENABLE;
	sFilterConfig.SlaveStartFilterBank = 14;

	HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
  #ifdef BOARD1
	TxMessage.StdId= 0xBE;  // On second node change this ID to 0xEF
  #elif defined(BOARD2)
  TxMessage.StdId= 0xEF;
  #endif
	TxMessage.RTR= CAN_RTR_DATA;
	TxMessage.IDE= CAN_ID_STD;
	TxMessage.DLC= 3;
	TxMessage.TransmitGlobalTime = DISABLE;

}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
    HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_13);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

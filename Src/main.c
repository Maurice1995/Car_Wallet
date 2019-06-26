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
#include "smComSCI2C.h"
#include <stdint.h>
#include "a71ch_ex.h"
#include "sci2c.h"
#include "dwt_delay.h"
#include "string.h"
#include "transaction.h"
#include "stm32f4xx_hal_tim.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define KEYSET (0x55)
#define LOCK_SLOT ((uint16_t)0)
#define KEY_SLOT ((uint16_t)2)
#define NONCE_SLOT ((uint16_t)40)
#define NONCE_SIZE_SLOT ((uint16_t)104)
#define CONNECTED (0b00001000)
#define TX_BUSY (0b00010000)
#define SSID_READY (0b00000001)
#define PSWD_READY (0b00000010)
#define MAS_MULTIPLIER 0.3017485143159276406820527 


// EVAN DETAILS

#define EVAN_NUM_CONTRACT_PARAMETERS 5
uint8_t EVAN_DAIMLER_SET_INFO_METHOD_ID[] = {0x17, 0x56, 0xcd, 0x8d};

#ifdef EVAN_TESTNET
#define EVAN_CHAIN_ID (uint32_t)0x6ec0511e
uint32_t EVAN_GAS_PRICE = 0x04a817c8;
uint32_t EVAN_GAS_LIMIT = 0x0186a0;
uint8_t EVAN_CONTRACT[] = {0x3d, 0xca, 0xb9, 0x7c, 0x38, 0x1f, 0xa3, 0xe8, 0xcb, 0xec, 0xcd, 0xad, 0x6f, 0xee, 0x59, 0x38, 0xbc, 0x51, 0x2c, 0xd7};
#else // EVAN_MAINNET // TODO: find what is the gas price/limit/contract for mainnet
#define EVAN_CHAIN_ID (uint32_t)0xC06E
uint32_t EVAN_GAS_PRICE = 0x04a817c8;
uint32_t EVAN_GAS_LIMIT = 0x0186a0;
uint8_t EVAN_CONTRACT[] = {0x3d, 0xca, 0xb9, 0x7c, 0x38, 0x1f, 0xa3, 0xe8, 0xcb, 0xec, 0xcd, 0xad, 0x6f, 0xee, 0x59, 0x38, 0xbc, 0x51, 0x2c, 0xd7};
#endif

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi2;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

CAN_FilterTypeDef sFilterConfig;
CAN_TxHeaderTypeDef TxMessage;
CAN_RxHeaderTypeDef RxMessage;
TIM_HandleTypeDef htim2;

uint8_t TxData[8];
uint8_t RxData[8];
uint32_t TxMailbox;

uint8_t tempVIN[] = {0x86, 0xae, 0x28, 0x43, 0x70, 0xe0, 0x7b, 0xe9, 0x2d, 0xa1, 0xa3, 0xb3, 0x41, 0x5a, 0x6f, 0x2f, 0x41, 0x7c, 0x3c, 0x68, 0xdb, 0x10, 0x0b, 0xb2, 0xf3, 0x87, 0x29, 0xab, 0x21, 0x3f, 0x7b, 0x29};

uint8_t rlp_tx[512];
uint8_t rlp_hex_tx[512];
ETH_TX tx;
const uint8_t SSID[128] = {"comay"};
const uint8_t header[3][3] = {"ID", "PW", "TX"};
const uint8_t PSWD[128] = {"111comay989"};
uint8_t status[4];
ETH_FIELD storedNonce;
const uint8_t testTx[] = {"0xf88d128504a817c800830186a094983530eb2c4ab3694f66c537bb8c83af80a7248b80a418935e80000000000000000000000000000000000000000000000000000000000005468e843ca380ffa09f973654a0fa726ab1ae16ff9fc971082627a954554df84b8bf4c3b017bae8b1a06732049603e0637274dd72c6e30cc060f77f04220720ba650e7fe7ea4e8214f2"};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_SPI2_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
void Can_Setup();
int spiReadStatus(uint8_t *readBuffer);
int spiWriteStatus(uint32_t status);
int spiWrite(uint8_t *data, uint16_t dataLen, uint8_t *header, uint8_t headerLen);
int spiWriteData(uint8_t *data);
int A71CHSignTest();
int A71CHStoreTest();
int connectWifi();
int sendTx(uint8_t const *tx, uint16_t txLen);
void generateIdentity();
void InitializeTimer();
int readNonce(ETH_FIELD *nonce);
int updateNonce(ETH_FIELD *nonce);
int getEpochOverNtp(ETH_FIELD *nonce);
void   increaseNonce();
static int processing_time = 0;
static volatile int start_processing = false;
static volatile bool isSynced = false;
typedef struct
{
 uint32_t latitude;
 uint32_t longitude;
}GPS;

static struct measurements_t
{
  uint8_t receivedFlags;
  uint16_t velocity;
  uint16_t displayed_velocity;
  GPS gps ;
  uint32_t odo;
} g_measurements = {0};

enum flags_e
{
  RECEIVED_RESET = 0x00,
  RECEIVED_VELOCITY = 0x01,
  RECEIVED_GPS = 0x02,
  RECEIVED_ODO = 0x04,
  RECEIVED_ALL = 0x07
};

/* USER CODE END PFP */

void sm_sleep(uint32_t msec)
{
  HAL_Delay(msec);
}
void sm_usleep(uint32_t microsec)
{
  // HAL_Delay(microsec);
  DWT_Delay(microsec);
}

void get_transaction(uint16_t speed, uint32_t mileage, uint32_t latitude, uint32_t longitude, uint8_t vin[32], ETH_FIELD *nonce, uint8_t *serialized_tx, uint32_t *tx_max_size)
{
  ETH_TX tx;

  memset(serialized_tx, 0, 512);
  memset(&tx, 0, sizeof(ETH_TX));

  // Build transaction details
  tx.chain_id = EVAN_CHAIN_ID;
  memcpy(&tx.nonce, nonce, sizeof(ETH_FIELD));
  tx.nonce.size = nonce->size;

  memcpy(&tx.gas_price.bytes, (uint8_t*)&EVAN_GAS_PRICE, sizeof(EVAN_GAS_PRICE));
  tx.gas_price.size = sizeof(EVAN_GAS_PRICE);

  memcpy(&tx.gas_limit.bytes, (uint8_t*)&EVAN_GAS_LIMIT, sizeof(EVAN_GAS_LIMIT));
  tx.gas_limit.size = sizeof(EVAN_GAS_LIMIT);

  memcpy(&tx.to.bytes, (uint8_t*)&EVAN_CONTRACT, sizeof(EVAN_CONTRACT));
  tx.to.size = sizeof(EVAN_CONTRACT);

  // Build contract method parameters
  uint8_t parameters[EVAN_NUM_CONTRACT_PARAMETERS][32];
  uint32_t parameter_sizes[EVAN_NUM_CONTRACT_PARAMETERS];
  uint8_t index = 0;

  memcpy(parameters[index], (uint8_t*) &speed, sizeof(speed));
  parameter_sizes[index++] = sizeof(speed);

  memcpy(parameters[index], (uint8_t*) &mileage, sizeof(mileage));
  parameter_sizes[index++] = sizeof(mileage) ;

  memcpy(parameters[index], (uint8_t*) &latitude, sizeof(latitude));
  parameter_sizes[index++] = sizeof(latitude) ;

  memcpy(parameters[index], (uint8_t*) &longitude, sizeof(longitude));
  parameter_sizes[index++] = sizeof(longitude) ;

  memcpy(parameters[index], (uint8_t*) &vin, sizeof(vin));
  parameter_sizes[index++] = 32;

  // Build data field fo tx
  build_raw_data_input(
    EVAN_DAIMLER_SET_INFO_METHOD_ID, 
    parameters, 
    parameter_sizes, 
    EVAN_NUM_CONTRACT_PARAMETERS,
    tx.data.bytes,
    sizeof(tx.data.bytes)
  );

  // Build, sign and serialize transaction
  get_ethereum_tx(&tx, serialized_tx, tx_max_size);
  increaseNonce();

}

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
  MX_I2C1_Init();
  InitializeTimer();

  /* USER CODE BEGIN 2 */
  Can_Setup();

  if (HAL_CAN_Start(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

  uint8_t Atr[64];
  uint16_t AtrLen = sizeof(Atr);
  int sw = smComSCI2C_Open(ESTABLISH_SCI2C, 0x00, Atr, &AtrLen);


  generateIdentity(); // Generates a priv key if it is not already set, it locks the first the byte of eeprom memory of A71CH after saving to its memory. After calling this function , call A71_GetGpData(KEY_SLOT,pKey,pKeyLen) to get the prik key stored

  uint32_t rnd = 0;
  uint8_t rndLen = 4;
  const uint8_t test1[] = {0xa9 ,0x00 ,0xff ,0x0f ,0x32 ,0xff ,0xcf ,0x7f};
  const uint8_t test2[] = {0x3c ,0xb4 ,0x96 ,0xff ,0x00 ,0x00 ,0xf4 ,0x01 };
  const uint8_t test3[] = {0xf7, 0x17, 0xa3, 0x62, 0xd6 ,0x0b, 0x67 ,0x86 };

  connectWifi();
  getEpochOverNtp(&storedNonce);

  while (1)
  {
    uint32_t odo;
    uint32_t disp_vel;
    uint32_t vel;
    float latitude;
    float longitude;
    uint16_t serialized_tx_size=0;
    g_measurements.velocity = ( (test1[3] >> 4) | (test1[4] << 4)) & 0xFFF;
    g_measurements.displayed_velocity =( (test2[7] << 8) | test2[6]) & 0xFFF;
    g_measurements.odo = ( (test2[2] << 16) | (test2[1] << 8 ) | test2[0]) & 0xFFFFFF;
    g_measurements.gps.latitude = ( test3[0] | (test3[1]<<8) | (test3[2]<<16) | ( test3[3]<<24) );
    g_measurements.gps.longitude = ( test3[4] | (test3[5]<<8) | (test3[6]<<16) | ( test3[7]<<24) );

    if (start_processing)
    {
      start_processing = false;

      /* Important function 2 */
      uint32_t prim;

      /* Do some stuff here which can be interrupted */

      /* Read PRIMASK register, check interrupt status before you disable them */
      /* Returns 0 if they are enabled, or non-zero if disabled */
      prim = __get_PRIMASK();

      /* Disable interrupts */
      __disable_irq();
      if (isSynced)
      {
        isSynced = false;

        odo      =  g_measurements.odo*0.1;
        disp_vel =  g_measurements.displayed_velocity*0.1;
        vel      =  g_measurements.velocity*0.1;
        latitude  = ((g_measurements.gps.latitude)*MAS_MULTIPLIER - 324000000)/3600;
        longitude = ((g_measurements.gps.longitude)*MAS_MULTIPLIER - 648000000)/3600;
        g_measurements.receivedFlags = RECEIVED_RESET;
        // construct testTx from GPS & ODO & VEL

        /* Do some stuff here which can not be interrupted */
        if (!prim)
        {
          __enable_irq();
        }
        get_transaction(disp_vel,odo,latitude,longitude,tempVIN,&storedNonce,rlp_tx,&serialized_tx_size);
        toHex(rlp_tx,rlp_hex_tx,serialized_tx_size);
        sendTx(rlp_hex_tx, serialized_tx_size*2+2);
        //sendTx(testTx, sizeof(testTx));

      }

      /* Enable interrupts back only if they were enabled before we disable it here in this function */

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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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
  hcan1.Init.Prescaler = 6;  //500 kBits
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_11TQ;
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */
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
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PE4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 1);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void generateIdentity()
{
  int rsp = ERR_COMM_ERROR;
  uint8_t lock[2] = {0};
  uint8_t keyIsSet[2] = {0x55, 0x66};
  uint8_t pKey[32] = {0};
  uint16_t pKeyLen = sizeof(pKey);
  rsp = A71_GetGpData(LOCK_SLOT, lock, sizeof(lock));
  if (rsp != SW_OK)
    return -1;
  if (memcmp(lock, keyIsSet, 2) != 0)
  {
    uint8_t rnd[64] = {0};
    uint8_t rndLen = 64;
    rsp = A71_GetRandom(&rnd, rndLen);
    if (rsp != SW_OK)
      return -1;
    uint8_t hash[64] = {0};
    for (int i = 0; i < 64; i++)
    {
      hash[i] = rnd[i] & tempVIN[i];
    }
    rsp = A71_GetSha256(hash, sizeof(hash), pKey, &pKeyLen);
    if (rsp != SW_OK)
      return -1;
    rsp = A71_SetGpData(KEY_SLOT, pKey, pKeyLen);
    if (rsp != SW_OK)
      return -1;
    rsp = A71_SetGpData(LOCK_SLOT, &keyIsSet, sizeof(keyIsSet));
    if (rsp != SW_OK)
      return -1;
    memset(pKey, 0, pKeyLen);
  }
}
void increaseNonce()
{
  uint32_t buffer = storedNonce.bytes[0] | (storedNonce.bytes[1]<<8) | (storedNonce.bytes[2]<<16) | (storedNonce.bytes[3]<<24);
  buffer ++ ;
  memcpy(storedNonce.bytes,&buffer,4);
}

void toHex(uint8_t *inBuf,uint8_t *outBuf,uint16_t bufLen)
{
    /* target buffer should be large enough */
    unsigned char * pin = inBuf;
    const char * hex = "0123456789abcdef";
    char * pout = outBuf;
    *pout++ = '0';
    *pout++ = 'x';
    int i = 0;
    for(; i < (bufLen)-1; ++i){
        *pout++ = hex[(*pin>>4)&0xF];
        *pout++ = hex[(*pin++)&0xF];
    }
    *pout++ = hex[(*pin>>4)&0xF];
    *pout++ = hex[(*pin)&0xF];
    *pout = 0;

}
int readNonce(ETH_FIELD *nonce)
{
  int rsp = ERR_COMM_ERROR;
  rsp = A71_GetGpData(NONCE_SIZE_SLOT, &(nonce->size), sizeof(nonce->size));
  if (rsp != SW_OK)
    return -1;
  if(nonce->size)
  {
    rsp = A71_GetGpData(NONCE_SLOT, nonce->bytes, nonce->size);
    if (rsp != SW_OK)
      return -1;
  }
  return 0;
}

int updateNonce(ETH_FIELD *nonce)
{
  int rsp = ERR_COMM_ERROR;
  rsp = A71_SetGpData(NONCE_SLOT, nonce->bytes, nonce->size);
  if (rsp != SW_OK)
    return -1;
  rsp = A71_SetGpData(NONCE_SIZE_SLOT, &(nonce->size), sizeof(nonce->size));
  if (rsp != SW_OK)
    return -1;
  return 0;

}
int sendTx(uint8_t const *tx, uint16_t txLen)
{
  int rv = HAL_ERROR;
  rv = spiReadStatus(status);
  if (rv != HAL_OK)
  {
    return rv;
  }
  HAL_Delay(100);
  if ((status[0] & TX_BUSY))
  {
    return 0; // processing the previous TX
  }
  else if (!(status[0] & CONNECTED))
  {
    connectWifi(); //wifi connection dropped , reconnect
  }
  rv = spiWrite(tx, txLen, header[2], strlen(header[2]));
  if (rv != HAL_OK)
  {
    return rv;
  }
}
int connectWifi()
{
  int rv = HAL_ERROR;
  rv = spiReadStatus(status);
  if (rv != HAL_OK)
  {
    return rv;
  }
  HAL_Delay(100);
  if (status[0] & CONNECTED != 0)
  {
    return 0; // Already connected to wifi
  }
  rv = spiWrite(SSID, strlen(SSID), header[0], strlen(header[0]));
  if (rv != HAL_OK)
  {
    return rv;
  }
  HAL_Delay(100);
  rv = spiWrite(PSWD, strlen(PSWD), header[1], strlen(header[1]));
  if (rv != HAL_OK)
  {
    return rv;
  }

  HAL_Delay(100);
  while (!(status[0] & CONNECTED))
  {
    rv = spiReadStatus(status);
    if (rv != HAL_OK)
    {
      return rv;
    }
    HAL_Delay(100);

    if (!(status[0] & SSID_READY))
    {
      rv = spiWrite(SSID, strlen(SSID), header[0], strlen(header[0]));
      if (rv != HAL_OK)
      {
        return rv;
      }
    }

    HAL_Delay(100);
    if (!(status[0] & PSWD_READY))
    {
      rv = spiWrite(PSWD, strlen(PSWD), header[1], strlen(header[1]));
      if (rv != HAL_OK)
      {
        return rv;
      }
    }
    HAL_Delay(100);
  }
}


int getEpochOverNtp(ETH_FIELD *nonce)
{
  int rv = HAL_ERROR;
  rv = spiReadStatus(status);
  if (rv != HAL_OK)
  {
    return rv;
  }
  HAL_Delay(100);

  while ((status[3] == 0))
  {
    rv = spiReadStatus(status);
    if (rv != HAL_OK)
    {
      return rv;
    }
    HAL_Delay(100);

  }
  memcpy(nonce->bytes,status,4);
  nonce->size = 4;
  return 0;
}


int A71CHSignTest()
{

  static U8 storeData[] = "RIDDLE & CODE";
  U8 checkData[16] = {0};
  U8 shaData[32] = {0};
  U16 shaDataLen = sizeof(shaData);
  U8 signatureBuffer[128] = {0};
  U16 signatureBufferLen = sizeof(signatureBuffer);
  U8 signResult = 0;
  U8 pubKey[72] = {0};
  U16 pubKeyLen = sizeof(pubKey);

  if (A71_GenerateEccKeyPair(0) == SMCOM_OK)
  {
    if (A71_GetPublicKeyEccKeyPair(0, pubKey, &pubKeyLen) == SMCOM_OK)
    {
      if (A71_GetSha256(storeData, sizeof(storeData), shaData, &shaDataLen) == SMCOM_OK)
      {
        if (A71_EccSign(0, shaData, shaDataLen, signatureBuffer, &signatureBufferLen) == SMCOM_OK)
        {
          if (A71_EccVerifyWithKey(pubKey, pubKeyLen, shaData, shaDataLen, signatureBuffer, signatureBufferLen, &signResult) == SMCOM_OK)
          {
            if (signResult == 0x01) //Verify SUCCESFUL
            {
              return 0;
            }

            else //Verify FAIL
            {
              return -1;
            }
          }
        }
      }
    }
  }

  return -1;
}

int A71CHStoreTest()
{
  static U8 storeData[] = "RIDDLE & CODE";
  U8 checkData[16] = {0};

  if (A71_SetGpData(0, storeData, sizeof(storeData)) != SMCOM_OK)
  {
    return -1;
  }

  else
  {
    if (A71_GetGpData(0, checkData, sizeof(checkData)) != SMCOM_OK)
    {
      return -1;
    }

    else
    {
      if (!memcmp(storeData, checkData, sizeof(storeData)))
      {
        return 0; //test success
      }

      else
      {
        return -1; //test fail
      }
    }
  }
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{

  HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &RxMessage, RxData);

  if (RxMessage.StdId == 0x098)
  {
    g_measurements.velocity = ( (RxData[4] >> 4) | (RxData[5] << 4)) & 0xFFF;
    g_measurements.receivedFlags |= RECEIVED_VELOCITY;
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
  }
  else if (RxMessage.StdId == 0x309)
  {
    /* Rx message Error */
    g_measurements.displayed_velocity =( (RxData[7] << 8) | RxData[6]) & 0xFFF;
    g_measurements.odo = ( (RxData[2] << 16) | (RxData[1] << 8 ) | RxData[0]) & 0xFFFFFF;
    g_measurements.receivedFlags |= RECEIVED_ODO;
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
  }
  else if (RxMessage.StdId == 0x3EB)
  {
    /* Rx message Error */
    g_measurements.gps.latitude = ( RxData[0] | (RxData[1]<<8) | (RxData[2]<<16) | ( RxData[3]<<24) );
    g_measurements.gps.longitude = ( RxData[4] | (RxData[5]<<8) | (RxData[6]<<16) | ( RxData[7]<<24) );
    g_measurements.receivedFlags |= RECEIVED_GPS;
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
  }

  if (g_measurements.receivedFlags == RECEIVED_ALL)
  {
    isSynced = true;
    g_measurements.receivedFlags = 0;
  }
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  static uint8_t ledNum;
  if (GPIO_Pin == GPIO_PIN_0)
  {
    /* Request transmission */
    TxData[2] = ++ledNum;

    if (HAL_CAN_AddTxMessage(&hcan1, &TxMessage, TxData, &TxMailbox) != HAL_OK)
    {
      /* Transmission request Error */
      Error_Handler();
    }
    /* Wait transmission complete */
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) != 3)
    {
    }
  }
}

int spiWriteStatus(uint32_t status)
{
  const uint8_t c = 0x01;
  static uint8_t statusBuffer[4];

  int rv = HAL_ERROR;

  for (int i = 0; i < 4; i++)
  {
    statusBuffer[i] = ((status >> (i * 8)) & 0xFF);
  }

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);

  rv = HAL_SPI_Transmit_IT(&hspi2, &c, 1);

  if (rv != HAL_OK)
  {
    return rv;
  }
  while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
  {
  }

  for (int i = 0; i < 4; i++)
  {
    rv = HAL_SPI_Transmit_IT(&hspi2, statusBuffer + i, 1);

    if (rv != HAL_OK)
    {
      return rv;
    }

    while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
    {
    }
  }
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);

  return HAL_OK;
}
int spiWrite(uint8_t *data, uint16_t dataLen, uint8_t *header, uint8_t headerLen)
{
  uint8_t spiTxBuffer[32] = {0};
  uint16_t messageLength = dataLen + headerLen;
  spiTxBuffer[0] = (messageLength) >> 8;
  spiTxBuffer[1] = (messageLength);
  int rv = HAL_ERROR;
  if (messageLength < 32)
  {
    memcpy(spiTxBuffer + 2, header, headerLen);
    memcpy(spiTxBuffer + headerLen + 2, data, dataLen);
    rv = spiWriteData(spiTxBuffer);
    if (rv != HAL_OK)
    {
      return rv;
    }
  }
  else
  {
    memcpy(spiTxBuffer + 2, header, headerLen);
    memcpy(spiTxBuffer + headerLen + 2, data, 32 - headerLen - 2);
    rv = spiWriteData(spiTxBuffer);
    if (rv != HAL_OK)
    {
      return rv;
    }
    dataLen = dataLen - 28;
    data = data + (32 - headerLen - 2);
    while (dataLen > 0)
    {
      HAL_Delay(1);
      memset(spiTxBuffer, 0, 32);
      if (dataLen >= 32)
      {
        memcpy(spiTxBuffer, data, 32);
        data = data + 32;
        dataLen = dataLen - 32;
      }
      else
      {
        memcpy(spiTxBuffer, data, dataLen);
        data = data + dataLen;
        dataLen = 0;
      }

      rv = spiWriteData(spiTxBuffer);
      if (rv != HAL_OK)
      {
        return rv;
      }
    }
  }
  return rv;
}
int spiWriteData(uint8_t *data)
{
  const uint8_t c[2] = {0x02, 0x00};

  int rv = HAL_ERROR;

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
  sm_usleep(20);
  rv = HAL_SPI_Transmit_IT(&hspi2, &c, 2);

  if (rv != HAL_OK)
  {
    return rv;
  }
  while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
  {
  }
  rv = HAL_SPI_Transmit_IT(&hspi2, data, 32);
  while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
  {
  }
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
  sm_usleep(40);
  return HAL_OK;
}

int spiReadStatus(uint8_t *readBuffer)
{
  uint8_t c = 0x04;
  int rv = HAL_ERROR;

  memset(readBuffer, 0, sizeof(uint32_t));

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
  sm_usleep(20);

  rv = HAL_SPI_Transmit_IT(&hspi2, &c, 1);

  if (rv != HAL_OK)
  {
    return rv;
  }

  while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
  {
  }

  rv = HAL_SPI_Receive_IT(&hspi2, readBuffer, 4);

  if (rv != HAL_OK)
  {
    return rv;
  }

  while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
  {
  }

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
  sm_usleep(20);

  return HAL_OK;
}

void Can_Setup()
{

  sFilterConfig.FilterBank = 0;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh = 0x0008 << 5;
  sFilterConfig.FilterIdLow = 0x0000;
  sFilterConfig.FilterMaskIdHigh = 0x40C << 5; // All four bytes must match to accept message
  sFilterConfig.FilterMaskIdLow = 0x0000;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.SlaveStartFilterBank = 14;

  HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
  TxMessage.StdId = 0x3C8; // OUR STDID
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 3;
  TxMessage.TransmitGlobalTime = DISABLE;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
      isSynced = true;
      start_processing = true;

  }
}

void InitializeTimer()
{
  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 299;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 839999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_TIM_CLEAR_FLAG(&htim2, TIM_IT_UPDATE);

  if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
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
  while (1)
  {
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

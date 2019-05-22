/*
 * Copyright 2016-2018 NXP
 *
 * This software is owned or controlled by NXP and may only be used
 * strictly in accordance with the applicable license terms.  By expressly
 * accepting such terms or by downloading, installing, activating and/or
 * otherwise using the software, you are agreeing that you have read, and
 * that you agree to comply with and are bound by, such license terms.  If
 * you do not agree to be bound by the applicable license terms, then you
 * may not retain, install, activate or otherwise use the software.
 */

/*
 * I2C implmentation for ICs related to Freedom Family
 */

// #include <board.h>
#define FREEDOM
#define FSL_FEATURE_SOC_I2C_COUNT 1
#if defined(FSL_FEATURE_SOC_I2C_COUNT) && FSL_FEATURE_SOC_I2C_COUNT > 0 && \
    defined(FREEDOM)
#include "i2c_a7.h"
#include "sci2c_cfg.h"
#include "sm_timer.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

#define I2C_BAUDRATE (400u * 1000u)

//#define DELAY_I2C_US          (T_CMDG_USec)
#define DELAY_I2C_US (0)

#define I2C_LOG_PRINTF printf
bool readblock = false;
uint8_t readblock_length = 0;
/* Handle NAK from the A71CH */
static int gBackoffDelay;

void axI2CResetBackoffDelay()
{
    gBackoffDelay = 0;
}

static void BackOffDelay_Wait()
{
    if (gBackoffDelay < 200)
        gBackoffDelay += 1;
    sm_sleep(gBackoffDelay);
}

i2c_error_t axI2CInit(void)
{
    // unused
    return I2C_OK;
}

void axI2CTerm(int mode) {}

unsigned int axI2CWrite(unsigned char bus_unused_param, unsigned char addr,
                        unsigned char *pTx, unsigned short txLen)
{
    extern I2C_HandleTypeDef hi2c1;
    HAL_StatusTypeDef status;
    #define MAX_RETRY 100
    int retry = 0;
    while (++retry < MAX_RETRY)
    {
        if(HAL_I2C_Master_Transmit(&hi2c1, 0x90, pTx, txLen, 100) == HAL_OK)
        {
            return I2C_OK;
        }

    }
    return I2C_FAILED;
}


unsigned int axI2CWriteRead(unsigned char bus_unused_param, unsigned char addr,
                            unsigned char *pTx, unsigned short txLen,
                            unsigned char *pRx, unsigned short *pRxLen)
{
    extern I2C_HandleTypeDef hi2c1;
    HAL_StatusTypeDef status;
    uint8_t t;
    *pRxLen = 0;
    memset(pRx, 0, 2);
    uint8_t rxData[255] = {0};
    uint8_t index = 0;
    uint32_t tickstart = 0;
     if ((I2C1->CR1 & I2C_CR1_PE) != I2C_CR1_PE)
    {
      /* Enable I2C peripheral */
        I2C1->CR1 |= I2C_CR1_PE;
    }
        I2C1->CR1 &= ~I2C_CR1_POS; // clear pos bit
        I2C1->CR1 |= I2C_CR1_START;
        tickstart = HAL_GetTick();
        while(!(I2C1->SR1 & I2C_SR1_SB))
        {
             if (((HAL_GetTick() - tickstart) > 10))
            {
                I2C1->CR1 |= I2C_CR1_STOP;
                return I2C_FAILED;
            }
        }
        I2C1->DR = 0x90;
        tickstart = HAL_GetTick();
        while (!(I2C1->SR1 & I2C_SR1_ADDR))
        {
              if (I2C1->SR1 & I2C_SR1_AF)
             {
                I2C1->CR1 |= I2C_CR1_STOP;
                I2C1->SR1 &= ~I2C_SR1_AF;
                return I2C_FAILED;
             }
             if (((HAL_GetTick() - tickstart) > 10))
            {
                I2C1->CR1 |= I2C_CR1_STOP;
                return I2C_FAILED;
            }
        }
        t = I2C1->SR1;
        t = I2C1->SR2;
        UNUSED(t);

        while(index < txLen)
        {
            tickstart = HAL_GetTick();
            uint8_t a = *(pTx+index);
            while (!(I2C1->SR1 & I2C_SR1_TXE))
            {
             if (I2C1->SR1 & I2C_SR1_AF)  //Check if ackknowledge failure occured
             {
                I2C1->CR1 |= I2C_CR1_STOP;  //generate stop
                I2C1->SR1 &= ~I2C_SR1_AF ;  // clear ackknowledge failure bit
                return I2C_FAILED;
             }

            if (((HAL_GetTick() - tickstart) > 10))
            {
                I2C1->CR1 |= I2C_CR1_STOP;
                return I2C_FAILED;
            }

            }
            I2C1->DR = a;
            index ++;
            tickstart = HAL_GetTick();
            while (!(I2C1->SR1 & I2C_SR1_BTF))
            {
                if (((HAL_GetTick() - tickstart) > 10))
                {
                    I2C1->CR1 |= I2C_CR1_STOP;
                    return I2C_FAILED;
                }

            }
        }

        HAL_Delay(1);
        index = 0;
        // tickstart = HAL_GetTick();
        // while ((I2C1->SR2 & I2C_SR2_BUSY))
        // {
        //     if (((HAL_GetTick() - tickstart) > 1000))
        //     {
        //         //I2C1->CR1 |= I2C_CR1_STOP;
        //         return I2C_FAILED;
        //     }

        // }
        if ((I2C1->CR1 & I2C_CR1_PE) != I2C_CR1_PE)
        {
        /* Enable I2C peripheral */
            I2C1->CR1 |= I2C_CR1_PE;
        }
        I2C1->CR1 &= ~I2C_CR1_POS; // clear pos bit
        I2C1->CR1 |= I2C_CR1_ACK;  //enable Acknowledge
        I2C1->CR1 |= I2C_CR1_START;/* Generate Start */
        while(!(I2C1->SR1&I2C_SR1_SB));// Wait until SB is set
        I2C1->DR = 0x90|0x01;
        tickstart = HAL_GetTick();
        while (!(I2C1->SR1 & I2C_SR1_ADDR)) //Wait for flag ADDR=1
        {
            if (I2C1->SR1 & I2C_SR1_AF)
            {
                I2C1->CR1 |= I2C_CR1_STOP;
                I2C1->SR1 &= ~I2C_SR1_AF;
                return I2C_FAILED;
            }
            if (((HAL_GetTick() - tickstart) > 10))
            {
                I2C1->CR1 |= I2C_CR1_STOP;
                return I2C_FAILED;
            }
        }

        t = I2C1->SR1;
        t = I2C1->SR2; //Read SR2 for deleting ADDR
        I2C1->CR1 |= I2C_CR1_ACK;  //enable Acknowledge

        tickstart = HAL_GetTick();
        while(!(I2C1->SR1&I2C_SR1_RXNE))
        {
            if (I2C1->SR1 & I2C_FLAG_STOPF)
            {
                /* Clear STOP Flag */
                I2C1->SR1 &= ~I2C_FLAG_STOPF;
                return I2C_FAILED;
            }
            if (((HAL_GetTick() - tickstart) > 10))
            {
                I2C1->CR1 |= I2C_CR1_STOP;
                return I2C_FAILED;
            }

        }
        rxData[index] = I2C1->DR;
        uint8_t messageLength = rxData[index];
        if (messageLength == 0U)
        {
        /* Clear ADDR flag */
            t = I2C1->SR1;
            t = I2C1->SR2; //Read SR2 for deleting ADDR

        /* Generate Stop */
            I2C1->CR1 |= I2C_CR1_STOP;
        }
        else if (messageLength == 1U)
        {
        /* Disable Acknowledge */
            I2C1->CR1 &= ~I2C_CR1_ACK;

            /* Clear ADDR flag */
            t = I2C1->SR1;
            t = I2C1->SR2; //Read SR2 for deleting ADDR

            /* Generate Stop */
            I2C1->CR1 |= I2C_CR1_STOP;
        }
        else if (messageLength == 2U)
        {
        /* Disable Acknowledge */
            I2C1->CR1 &= ~I2C_CR1_ACK;

        /* Enable Pos */
            I2C1->CR1 |= I2C_CR1_POS;

        /* Clear ADDR flag */
            t = I2C1->SR1;
            t = I2C1->SR2;
        }
        else
        {
        /* Enable Acknowledge */
        I2C1->CR1 |= I2C_CR1_ACK;  //enable Acknowledge
        /* Clear ADDR flag */
        t = I2C1->SR1;
        t = I2C1->SR2;
        }

        while (messageLength > 0U)
        {
            if (messageLength <= 3U)
            {
                /* One byte */
                if (messageLength == 1U)
                {
                    tickstart = HAL_GetTick();
                    while(!(I2C1->SR1&I2C_SR1_RXNE))
                    {
                        if (I2C1->SR1 & I2C_FLAG_STOPF)
                        {
                            /* Clear STOP Flag */
                            I2C1->SR1 &= ~I2C_FLAG_STOPF;
                            return I2C_FAILED;
                        }
                        if (((HAL_GetTick() - tickstart) > 10))
                        {
                            I2C1->CR1 |= I2C_CR1_STOP;
                            return I2C_FAILED;
                        }

                    }

                    /* Read data from DR */
                    rxData[++index]  = (uint8_t)I2C1->DR;

                    /* Update counter */
                    messageLength--;
                }
                /* Two bytes */
                else if (messageLength == 2U)
                {
                     /* Wait until BTF flag is set */
                    tickstart = HAL_GetTick();
                    while (!(I2C1->SR1 & I2C_SR1_BTF))
                    {
                        if (((HAL_GetTick() - tickstart) > 10))
                        {
                            I2C1->CR1 |= I2C_CR1_STOP;
                            return I2C_FAILED;
                        }

                    }
                /* Generate Stop */
                    I2C1->CR1 |= I2C_CR1_STOP;


                /* Read data from DR */
                    rxData[++index]  = (uint8_t)I2C1->DR;

                /* Update counter */
                    messageLength--;

                    /* Read data from DR */
                    rxData[++index]  = (uint8_t)I2C1->DR;

                   /* Update counter */
                    messageLength--;
                }
                /* 3 Last bytes */
                else
                {
                /* Wait until BTF flag is set */
                    tickstart = HAL_GetTick();
                    while (!(I2C1->SR1 & I2C_SR1_BTF))
                    {
                        if (((HAL_GetTick() - tickstart) > 10))
                        {
                            I2C1->CR1 |= I2C_CR1_STOP;
                            return I2C_FAILED;
                        }

                    }

                /* Disable Acknowledge */
                    I2C1->CR1 &= ~I2C_CR1_ACK;

                /* Read data from DR */
                    rxData[++index]  = (uint8_t)I2C1->DR;

                /* Update counter */
                    messageLength--;

                /* Wait until BTF flag is set */
                    tickstart = HAL_GetTick();
                    while (!(I2C1->SR1 & I2C_SR1_BTF))
                    {
                        if (((HAL_GetTick() - tickstart) > 10))
                        {
                            I2C1->CR1 |= I2C_CR1_STOP;
                            return I2C_FAILED;
                        }

                    }

                /* Generate Stop */
                    I2C1->CR1 |= I2C_CR1_STOP;

                /* Read data from DR */
                    rxData[++index]  = (uint8_t)I2C1->DR;

                /* Update counter */
                    messageLength--;

                /* Read data from DR */
                    rxData[++index]  = (uint8_t)I2C1->DR;

                /* Update counter */
                    messageLength--;

                }
            }
            else
            {
                 /* Wait until RXNE flag is set */
                tickstart = HAL_GetTick();
                while(!(I2C1->SR1&I2C_SR1_RXNE))
                {
                    if (I2C1->SR1 & I2C_FLAG_STOPF)
                    {
                        /* Clear STOP Flag */
                        I2C1->SR1 &= ~I2C_FLAG_STOPF;
                        return I2C_FAILED;
                    }
                    if (((HAL_GetTick() - tickstart) > 10))
                    {
                        I2C1->CR1 |= I2C_CR1_STOP;
                        return I2C_FAILED;
                    }

                }

                    /* Read data from DR */
                    rxData[++index]  = (uint8_t)I2C1->DR;
                    /* Update counter */
                    messageLength--;

                    extern I2C_HandleTypeDef hi2c1;
                    if (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BTF) == SET)
                    {
                        /* Read data from DR */
                        rxData[++index]  = (uint8_t)I2C1->DR;
                        /* Update counter */
                        messageLength--;
                    }
            }
        }

        *pRxLen = rxData[0] + 1;
        memcpy(pRx, rxData, *pRxLen);

    return I2C_OK;
}

#endif /* FSL_FEATURE_SOC_I2C_COUNT */

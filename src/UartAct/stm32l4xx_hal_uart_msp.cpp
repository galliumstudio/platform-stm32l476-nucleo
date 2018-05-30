/*******************************************************************************
 * Copyright (C) Gallium Studio LLC. All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Alternatively, this program may be distributed and modified under the
 * terms of Gallium Studio LLC commercial licenses, which expressly supersede
 * the GNU General Public License and are specifically designed for licensees
 * interested in retaining the proprietary status of their code.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Contact information:
 * Website - https://www.galliumstudio.com
 * Source repository - https://github.com/galliumstudio
 * Email - admin@galliumstudio.com
 ******************************************************************************/

/**
  ******************************************************************************
  * @file    UART/UART_Printf/Src/stm32f7xx_hal_msp.c
  * @author  MCD Application Team
  * @version V1.0.1
  * @date    22-April-2016
  * @brief   HAL MSP module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#include "bsp.h"
#include "fw_log.h"
#include "app_hsmn.h"
#include "bsp.h"

using namespace FW;
using namespace APP;

// TX PA.2
// RX PA.3
// TX DMA - DMA1 Channel 7 Request 2
// RX DMA - DMA1 Channel 6 Request 2
static void InitUart2(UART_HandleTypeDef *usart) {
    static DMA_HandleTypeDef hdma_tx;
    static DMA_HandleTypeDef hdma_rx;
    GPIO_InitTypeDef  GPIO_InitStruct;
    // 1- Enable peripherals and GPIO Clocks
    // Enable GPIO TX/RX clock
    __GPIOA_CLK_ENABLE();
    // Enable USARTx clock
    __HAL_RCC_USART2_CLK_ENABLE();
    // Enable DMA clock
    __HAL_RCC_DMA1_CLK_ENABLE();

    // 2- Configure peripheral GPIO
    // UART TX GPIO pin configuration
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    // UART RX GPIO pin configuration
    GPIO_InitStruct.Pin       = GPIO_PIN_3;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    //-3- Configure the DMA
    // Configure the DMA handler for Transmission process
    hdma_tx.Instance                 = DMA1_Channel7;
    hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_tx.Init.Mode                = DMA_NORMAL;
    hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
    hdma_tx.Init.Request             = DMA_REQUEST_2;
    HAL_DMA_Init(&hdma_tx);
    // Associate the initialized DMA handle to the UART handle
    __HAL_LINKDMA(usart, hdmatx, hdma_tx);
    // Configure the DMA handler for reception process
    hdma_rx.Instance                 = DMA1_Channel6;
    hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_rx.Init.Mode                = DMA_CIRCULAR;
    hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_tx.Init.Request             = DMA_REQUEST_2;
    HAL_DMA_Init(&hdma_rx);
    // Associate the initialized DMA handle to the the UART handle
    __HAL_LINKDMA(usart, hdmarx, hdma_rx);

    // 4- Configure the NVIC for DMA
    // NVIC for DMA TX
    NVIC_SetPriority(DMA1_Channel7_IRQn, DMA1_CHANNEL7_PRIO);
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
    // NVIC for DMA RX
    // Gallium - disabled for testing.
    //NVIC_SetPriority(DMA1_Channel6_IRQn, DMA1_CHANNEL6_PRIO);
    //NVIC_EnableIRQ(DMA1_Channel6_IRQn);
    // NVIC for USART
    NVIC_SetPriority(USART2_IRQn, USART2_IRQ_PRIO);
    NVIC_EnableIRQ(USART2_IRQn);
}

static void DeInitUart2(UART_HandleTypeDef *usart) {
    (void)usart;
    // 1- Reset peripherals
    __HAL_RCC_USART2_FORCE_RESET();
    __HAL_RCC_USART2_RELEASE_RESET();

    // 2- Disable peripherals and GPIO Clocks
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);
}

extern "C" void HAL_UART_MspInit(UART_HandleTypeDef *usart)
{
    if (usart->Instance == USART2) {
        InitUart2(usart);
    } // Add more here...
}

extern "C" void HAL_UART_MspDeInit(UART_HandleTypeDef *usart)
{
    if (usart->Instance == USART2) {
        DeInitUart2(usart);
    } // Add more here...
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : mdma.c
  * Description        : This file provides code for the configuration
  *                      of all the requested global MDMA transfers.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "mdma.h"

/* USER CODE BEGIN 0 */
#include "cis.h"
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure MDMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */
void HAL_MDMA_XferCpltCallback(MDMA_HandleTypeDef *hmdma);

MDMA_LinkNodeConfTypeDef nodeConfigs[3];

static int32_t source_offset = (CIS_SP_WIDTH + CIS_OVER_SCAN) * sizeof(uint16_t);

/* USER CODE END 1 */
MDMA_HandleTypeDef hmdma_mdma_channel0_sw_0;
MDMA_HandleTypeDef hmdma_mdma_channel1_dma1_stream0_tc_0;
MDMA_HandleTypeDef hmdma_mdma_channel2_dma1_stream1_tc_0;
MDMA_HandleTypeDef hmdma_mdma_channel3_dma2_stream0_tc_0;

/**
  * Enable MDMA controller clock
  * Configure MDMA for global transfers
  *   hmdma_mdma_channel0_sw_0
  *   hmdma_mdma_channel1_dma1_stream0_tc_0
  *   hmdma_mdma_channel2_dma1_stream1_tc_0
  *   hmdma_mdma_channel3_dma2_stream0_tc_0
  */
void MX_MDMA_Init(void)
{

  /* MDMA controller clock enable */
  __HAL_RCC_MDMA_CLK_ENABLE();
  /* Local variables */

  /* Configure MDMA channel MDMA_Channel0 */
  /* Configure MDMA request hmdma_mdma_channel0_sw_0 on MDMA_Channel0 */
  hmdma_mdma_channel0_sw_0.Instance = MDMA_Channel0;
  hmdma_mdma_channel0_sw_0.Init.Request = MDMA_REQUEST_SW;
  hmdma_mdma_channel0_sw_0.Init.TransferTriggerMode = MDMA_BUFFER_TRANSFER;
  hmdma_mdma_channel0_sw_0.Init.Priority = MDMA_PRIORITY_LOW;
  hmdma_mdma_channel0_sw_0.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  hmdma_mdma_channel0_sw_0.Init.SourceInc = MDMA_SRC_INC_WORD;
  hmdma_mdma_channel0_sw_0.Init.DestinationInc = MDMA_DEST_INC_WORD;
  hmdma_mdma_channel0_sw_0.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
  hmdma_mdma_channel0_sw_0.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
  hmdma_mdma_channel0_sw_0.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  hmdma_mdma_channel0_sw_0.Init.BufferTransferLength = 2540;
  hmdma_mdma_channel0_sw_0.Init.SourceBurst = MDMA_SOURCE_BURST_4BEATS;
  hmdma_mdma_channel0_sw_0.Init.DestBurst = MDMA_DEST_BURST_4BEATS;
  hmdma_mdma_channel0_sw_0.Init.SourceBlockAddressOffset = 0;
  hmdma_mdma_channel0_sw_0.Init.DestBlockAddressOffset = 0;
  if (HAL_MDMA_Init(&hmdma_mdma_channel0_sw_0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure MDMA channel MDMA_Channel1 */
  /* Configure MDMA request hmdma_mdma_channel1_dma1_stream0_tc_0 on MDMA_Channel1 */
  hmdma_mdma_channel1_dma1_stream0_tc_0.Instance = MDMA_Channel1;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.Request = MDMA_REQUEST_DMA1_Stream0_TC;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.TransferTriggerMode = MDMA_FULL_TRANSFER;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.Endianness = MDMA_LITTLE_HALFWORD_ENDIANNESS_EXCHANGE;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.SourceInc = MDMA_SRC_INC_HALFWORD;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.DestinationInc = MDMA_DEST_INC_WORD;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.SourceDataSize = MDMA_SRC_DATASIZE_HALFWORD;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.DataAlignment = MDMA_DATAALIGN_RIGHT;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.BufferTransferLength = 3612;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.SourceBurst = MDMA_SOURCE_BURST_4BEATS;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.DestBurst = MDMA_DEST_BURST_4BEATS;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.SourceBlockAddressOffset = source_offset;
  hmdma_mdma_channel1_dma1_stream0_tc_0.Init.DestBlockAddressOffset = 0;
  if (HAL_MDMA_Init(&hmdma_mdma_channel1_dma1_stream0_tc_0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure post request address and data masks */
  if (HAL_MDMA_ConfigPostRequestMask(&hmdma_mdma_channel1_dma1_stream0_tc_0, 0, 0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure MDMA channel MDMA_Channel2 */
  /* Configure MDMA request hmdma_mdma_channel2_dma1_stream1_tc_0 on MDMA_Channel2 */
  hmdma_mdma_channel2_dma1_stream1_tc_0.Instance = MDMA_Channel2;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.Request = MDMA_REQUEST_DMA1_Stream1_TC;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.TransferTriggerMode = MDMA_FULL_TRANSFER;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.Endianness = MDMA_LITTLE_HALFWORD_ENDIANNESS_EXCHANGE;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.SourceInc = MDMA_SRC_INC_HALFWORD;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.DestinationInc = MDMA_DEST_INC_WORD;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.SourceDataSize = MDMA_SRC_DATASIZE_HALFWORD;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.DataAlignment = MDMA_DATAALIGN_RIGHT;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.BufferTransferLength = 3612;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.SourceBurst = MDMA_SOURCE_BURST_4BEATS;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.DestBurst = MDMA_DEST_BURST_4BEATS;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.SourceBlockAddressOffset = source_offset;
  hmdma_mdma_channel2_dma1_stream1_tc_0.Init.DestBlockAddressOffset = 0;
  if (HAL_MDMA_Init(&hmdma_mdma_channel2_dma1_stream1_tc_0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure post request address and data masks */
  if (HAL_MDMA_ConfigPostRequestMask(&hmdma_mdma_channel2_dma1_stream1_tc_0, 0, 0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure MDMA channel MDMA_Channel3 */
  /* Configure MDMA request hmdma_mdma_channel3_dma2_stream0_tc_0 on MDMA_Channel3 */
  hmdma_mdma_channel3_dma2_stream0_tc_0.Instance = MDMA_Channel3;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.Request = MDMA_REQUEST_DMA2_Stream0_TC;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.TransferTriggerMode = MDMA_FULL_TRANSFER;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.Endianness = MDMA_LITTLE_HALFWORD_ENDIANNESS_EXCHANGE;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.SourceInc = MDMA_SRC_INC_HALFWORD;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.DestinationInc = MDMA_DEST_INC_WORD;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.SourceDataSize = MDMA_SRC_DATASIZE_HALFWORD;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.DataAlignment = MDMA_DATAALIGN_RIGHT;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.BufferTransferLength = 3612;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.SourceBurst = MDMA_SOURCE_BURST_4BEATS;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.DestBurst = MDMA_DEST_BURST_4BEATS;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.SourceBlockAddressOffset = source_offset;
  hmdma_mdma_channel3_dma2_stream0_tc_0.Init.DestBlockAddressOffset = 0;
  if (HAL_MDMA_Init(&hmdma_mdma_channel3_dma2_stream0_tc_0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure post request address and data masks */
  if (HAL_MDMA_ConfigPostRequestMask(&hmdma_mdma_channel3_dma2_stream0_tc_0, 0, 0) != HAL_OK)
  {
    Error_Handler();
  }

  /* MDMA interrupt initialization */
  /* MDMA_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(MDMA_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(MDMA_IRQn);

}
/* USER CODE BEGIN 2 */
void MDMA_Init(void)
{
    //ADC1
    nodeConfigs[0].SrcAddress = (uint32_t)&cisData_ADC1[CIS_SP_WIDTH];
    nodeConfigs[0].DstAddress = (uint32_t)&cisDataCpy[0];
    nodeConfigs[0].BlockDataLength = cisConfig.useful_data_size_per_color_per_lane * sizeof(uint16_t);
    nodeConfigs[0].BlockCount = COLOR_CHANNELS;

    //ADC2
    nodeConfigs[1].SrcAddress = (uint32_t)&cisData_ADC2[CIS_SP_WIDTH];
    nodeConfigs[1].DstAddress = (uint32_t)&cisDataCpy[cisConfig.useful_data_size_per_lane];
    nodeConfigs[1].BlockDataLength = cisConfig.useful_data_size_per_color_per_lane * sizeof(uint16_t);
    nodeConfigs[1].BlockCount = COLOR_CHANNELS;

    //ADC3
    nodeConfigs[2].SrcAddress = (uint32_t)&cisData_ADC3[CIS_SP_WIDTH];
    nodeConfigs[2].DstAddress = (uint32_t)&cisDataCpy[cisConfig.useful_data_size_per_lane * 2];
    nodeConfigs[2].BlockDataLength = cisConfig.useful_data_size_per_color_per_lane * sizeof(uint16_t);
    nodeConfigs[2].BlockCount = COLOR_CHANNELS;

    //SW transfert
	HAL_MDMA_RegisterCallback(&hmdma_mdma_channel0_sw_0, HAL_MDMA_XFER_CPLT_CB_ID, HAL_MDMA_XferCpltCallback);
	//ADC1
	HAL_MDMA_RegisterCallback(&hmdma_mdma_channel1_dma1_stream0_tc_0, HAL_MDMA_XFER_CPLT_CB_ID, HAL_MDMA_XferCpltCallback);
	//ADC2
	HAL_MDMA_RegisterCallback(&hmdma_mdma_channel2_dma1_stream1_tc_0, HAL_MDMA_XFER_CPLT_CB_ID, HAL_MDMA_XferCpltCallback);
	//ADC3
	HAL_MDMA_RegisterCallback(&hmdma_mdma_channel3_dma2_stream0_tc_0, HAL_MDMA_XFER_CPLT_CB_ID, HAL_MDMA_XferCpltCallback);
}

/**
 * @brief MDMA transfer complete callback.
 * @param hmdma Pointer to the MDMA handle.
 */
void HAL_MDMA_XferCpltCallback(MDMA_HandleTypeDef *hmdma)
{
    if (hmdma == &hmdma_mdma_channel0_sw_0)
    {
        // Release semaphore 1 and generate an IRQ for CM4
        HAL_HSEM_FastTake(1);
        HAL_HSEM_Release(1, 0);
    }

    MDMA_XferCpltCallback(hmdma);
}

/* USER CODE END 2 */

/**
  * @}
  */

/**
  * @}
  */


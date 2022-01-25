/******************************************************************************
 * @file     basic_math_functions.h
 * @brief    Public header file for CMSIS DSP Library
 * @version  V1.9.0
 * @date     23 April 2021
 * Target Processor: Cortex-M and Cortex-A cores
 ******************************************************************************/
/*
 * Copyright (c) 2010-2020 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 
#ifndef _BASIC_MATH_FUNCTIONS_H_
#define _BASIC_MATH_FUNCTIONS_H_

#include "arm_math.h"


#ifdef   __cplusplus
extern "C"
{
#endif

/**
 * @defgroup groupMath Basic Math Functions
 */

void arm_clip_q31(const q31_t * pSrc, 
  q31_t * pDst, 
  q31_t low, 
  q31_t high, 
  uint32_t numSamples);


#endif /* ifndef _BASIC_MATH_FUNCTIONS_H_ */

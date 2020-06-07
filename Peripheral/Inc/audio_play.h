/**
  ******************************************************************************
  * @file    BSP/Src/audio_play.h
  * @author  MCD Application Team
  * @brief   This example code shows how to use the audio feature in the
  *          stm32h750b_discovery driver
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

#ifndef INC_AUDIO_PLAY_H_
#define INC_AUDIO_PLAY_H_

void AudioPlayBuffer (uint32_t *audio_file_address, uint32_t audio_file_size);
void AudioPlay_demo (void);

#endif /* INC_AUDIO_PLAY_H_ */

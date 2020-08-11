/*
 * menu.h
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MENU_H
#define __MENU_H

#define DISPLAY_AERAS_HEIGHT		(0x20 + 2 * DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_HEAD_HEIGHT			(22)
#define DISPLAY_INTER_AERAS_HEIGHT	(2)

#define DISPLAY_AERA1_YPOS			(DISPLAY_HEAD_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA2_YPOS			(DISPLAY_AERA1_YPOS + DISPLAY_AERAS_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA3_YPOS			(DISPLAY_AERA2_YPOS + DISPLAY_AERAS_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA4_YPOS			(DISPLAY_AERA3_YPOS + DISPLAY_AERAS_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA5_YPOS			(DISPLAY_AERA4_YPOS + DISPLAY_AERAS_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
uint32_t menu_StartSelection(void);

#endif /* __MENU_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

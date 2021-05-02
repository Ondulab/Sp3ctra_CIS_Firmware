/*
 * menu.h
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MENU_H
#define __MENU_H

#define DISPLAY_MAX_X_LENGTH		SSD1362_WIDTH
#define DISPLAY_MAX_Y_LENGTH		SSD1362_HEIGHT

#define DISPLAY_HEAD_HEIGHT			(9)
#define DISPLAY_AERAS1_HEIGHT		(18)
#define DISPLAY_AERAS2_HEIGHT		(16)
#define DISPLAY_AERAS3_HEIGHT		(8)
#define DISPLAY_INTER_AERAS_HEIGHT	(1)

#define DISPLAY_HEAD_Y1POS			(0)
#define DISPLAY_HEAD_Y2POS			(DISPLAY_HEAD_HEIGHT)

#define DISPLAY_AERA1_Y1POS			(DISPLAY_HEAD_HEIGHT + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA1_Y2POS			(DISPLAY_AERA1_Y1POS + DISPLAY_AERAS1_HEIGHT)

#define DISPLAY_AERA2_Y1POS			(DISPLAY_AERA1_Y2POS + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA2_Y2POS			(DISPLAY_AERA2_Y1POS + DISPLAY_AERAS2_HEIGHT)

#define DISPLAY_AERA3_Y1POS			(DISPLAY_AERA2_Y2POS + DISPLAY_INTER_AERAS_HEIGHT)
#define DISPLAY_AERA3_Y2POS			(DISPLAY_AERA3_Y1POS + DISPLAY_AERAS3_HEIGHT)

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
uint32_t menu_StartSelection(void);

#endif /* __MENU_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

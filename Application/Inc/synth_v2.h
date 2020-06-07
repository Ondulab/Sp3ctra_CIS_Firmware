/*
 * synth_v2.h
 *
 *  Created on: May 31, 2020
 *      Author: zhonx
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYNTH_V2_H
#define __SYNTH_V2_H

/* Includes ------------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
#define CIS_SP_Pin GPIO_PIN_4
#define CIS_SP_GPIO_Port GPIOE
#define USER_PB_Pin GPIO_PIN_13
#define USER_PB_GPIO_Port GPIOC
#define USER_PB_EXTI_IRQn EXTI15_10_IRQn
#define CIS_LED_G_Pin GPIO_PIN_7
#define CIS_LED_G_GPIO_Port GPIOF
#define CIS_LED_B_Pin GPIO_PIN_8
#define CIS_LED_B_GPIO_Port GPIOF
#define CIS_LED_R_Pin GPIO_PIN_9
#define CIS_LED_R_GPIO_Port GPIOF

int synth_v2(void);

#endif /* __SYNTH_V2_H */

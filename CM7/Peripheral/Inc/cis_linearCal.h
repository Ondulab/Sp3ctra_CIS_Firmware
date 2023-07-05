/**
  ******************************************************************************
  * @file           : linearCal.h
  * @brief          : Header for linearCal.c file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CIS_LINEARCAL_H__
#define __CIS_LINEARCAL_H__

/* Includes ------------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

void cis_linealCalibrationInit(void);
void cis_StartLinearCalibration(uint16_t iterationNb);

#endif /* __CIS_LINEARCAL_H__ */

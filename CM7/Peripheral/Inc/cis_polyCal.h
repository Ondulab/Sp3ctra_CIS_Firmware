/**
  ******************************************************************************
  * @file           : cis_polyCal.h
  * @brief          : Header for cis_polyCal.c file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CIS_POLYCAL_H__
#define __CIS_POLYCAL_H__

#include "cis.h"

/* Structure pour une ligne de couleur */
struct ColorLine{
    float32_t* line;
     struct CalibrationCoefficients* coeff;
};

struct RGB_CalibrationCoefficients{
     struct CalibrationCoefficients redCoeff;
     struct CalibrationCoefficients greenCoeff;
     struct CalibrationCoefficients blueCoeff;
};

/* Includes ------------------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

void cis_ConvertRAWImageToRGBImage(struct RAWImage* RAWImage, int32_t* RGBimage);
void cis_calibrateLeds(void);

#endif /* __CIS_POLYCAL_H__ */

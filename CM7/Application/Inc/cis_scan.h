/**
 ******************************************************************************
 * @file           : cis_scan.h
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2018-present Reso-nance Numerique.
 * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CIS_SCAN_H
#define CIS_SCAN_H

/* Includes ------------------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/
/* Custom return type for CIS scan functions -----------------------------*/
typedef enum {
	CISSCAN_OK = 0,
	CISSCAN_ERROR = 1
} CISSCAN_StatusTypeDef;

CISSCAN_StatusTypeDef cis_scanInit(void);

/* ---- Live preview for the embedded web viewer (GET /scan.bin) -------------
 * The browser accumulates the waterfall itself, so the device only keeps the
 * most recent lines -- a ring sized in BYTES, so the number of lines it holds
 * follows the resolution asked for (a few at full pixels, dozens decimated).
 * One HTTP request carries every line published since the client's last one:
 * the viewer is limited by the line rate it asks for, not by its poll rate.
 *
 * Lock free on purpose: the scan task never waits on the HTTP task. A line the
 * writer laps while it is being sent shows up as one torn column in a
 * waterfall of hundreds -- invisible, and cheaper than dropping lines.
 */

/** Arm (or re-arm) the preview for a couple of seconds.
 *  @param decimation  1..8, one pixel out of N kept.
 *  @param rateHz      lines per second wanted (clamped to the scan rate). */
void cisScan_requestPreview(uint8_t decimation, uint16_t rateHz);

/** Describe the batch available after line `since` (0 = newest line only).
 *  Returns the number of lines, and fills the id of the first, the pixels per
 *  line, how many lines were lost before it, and the rate being published. */
uint16_t cisScan_previewBatch(uint32_t since, uint32_t *firstId, uint16_t *pixels,
                              uint16_t *dropped, uint16_t *rateHz);

/** Pixels of one line of the batch, as interleaved RGB triplets. */
const uint8_t *cisScan_previewLine(uint32_t lineId);

#endif /* CIS_SCAN_H */

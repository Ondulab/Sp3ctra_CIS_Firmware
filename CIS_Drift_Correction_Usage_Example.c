/**
 ******************************************************************************
 * @file           : CIS_Drift_Correction_Usage_Example.c
 * @brief          : Example usage of CIS global drift correction
 ******************************************************************************
 * @attention
 *
 * This file demonstrates how to integrate the new global drift correction
 * functionality into existing CIS processing code.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cis_linearCal.h"
#include "globals.h"

/* Example 1: Basic usage - Replace existing calibration calls */
void example_basic_usage(void)
{
    // OLD CODE (without drift correction):
    // cis_applyLinearCalibration(cisDataCpy, 255);

    // NEW CODE (with drift correction):
    cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);

    // That's it! The drift correction is automatically applied.
}

/* Example 2: Conditional drift correction based on system state */
void example_conditional_usage(void)
{
    // Check if system has been running long enough for thermal drift
    static uint32_t system_uptime_minutes = 0;
    system_uptime_minutes = HAL_GetTick() / (1000 * 60); // Convert to minutes

    if (system_uptime_minutes > 10) // After 10 minutes of operation
    {
        // Use drift correction for long-running operations
        cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);
    }
    else
    {
        // Use standard calibration for initial startup
        cis_applyLinearCalibration(cisDataCpy, 255);
    }
}

/* Example 3: Dynamic drift threshold adjustment */
void example_dynamic_threshold(void)
{
    // Adjust drift threshold based on operating conditions
    float temperature = get_system_temperature(); // Hypothetical function

    if (temperature > 50.0f) // High temperature operation
    {
        cisCals.driftThreshold = 100; // Allow more drift correction
    }
    else if (temperature < 20.0f) // Low temperature operation
    {
        cisCals.driftThreshold = 25;  // Tighter drift control
    }
    else // Normal temperature
    {
        cisCals.driftThreshold = 50;  // Default threshold
    }

    // Apply calibration with adjusted threshold
    cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);
}

/* Example 4: Monitoring drift levels for diagnostics */
void example_drift_monitoring(void)
{
    int32_t globalDriftOffset[3][CIS_ADC_OUT_LANES];

    // Compute current drift levels without applying correction
    cis_computeGlobalDriftCorrection(cisDataCpy, globalDriftOffset);

    // Check for excessive drift that might indicate hardware issues
    for (int color = 0; color < 3; color++)
    {
        for (int lane = 0; lane < CIS_ADC_OUT_LANES; lane++)
        {
            int32_t drift = globalDriftOffset[color][lane];

            if (abs(drift) > 200) // Excessive drift threshold
            {
                printf("WARNING: Excessive drift detected - Color:%d Lane:%d Drift:%ld\n",
                       color, lane, drift);

                // Could trigger recalibration request or system alert
                shared_var.cis_cal_request = 1;
            }
            else if (abs(drift) > 100) // Moderate drift
            {
                printf("INFO: Moderate drift - Color:%d Lane:%d Drift:%ld\n",
                       color, lane, drift);
            }
        }
    }

    // Apply correction normally
    cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);
}

/* Example 5: Integration in main image processing loop */
void example_main_processing_loop(void)
{
    static uint32_t line_counter = 0;
    static uint32_t drift_check_interval = 100; // Check every 100 lines

    // Main image processing loop
    while (scanning_active)
    {
        // Wait for new line data
        if (cis_line_ready())
        {
            // Get raw CIS data
            cis_getRawData(cisDataCpy);

            // Apply calibration with drift correction
            if (cisCals.driftCorrectionEnabled)
            {
                cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);
            }
            else
            {
                cis_applyLinearCalibration(cisDataCpy, 255);
            }

            // Optional: Periodic drift monitoring
            line_counter++;
            if (line_counter >= drift_check_interval)
            {
                line_counter = 0;
                example_drift_monitoring(); // Check drift levels
            }

            // Continue with rest of image processing...
            process_calibrated_line(cisDataCpy);
        }
    }
}

/* Example 6: Configuration management */
void example_configuration_management(void)
{
    // Load drift correction settings from configuration
    if (shared_config.cis_advanced_calibration_enabled) // Hypothetical config flag
    {
        cisCals.driftCorrectionEnabled = 1;
        cisCals.driftThreshold = shared_config.cis_drift_threshold; // From config
    }
    else
    {
        cisCals.driftCorrectionEnabled = 0;
        cisCals.driftThreshold = 0;
    }

    printf("Drift correction: %s (threshold: %ld)\n",
           cisCals.driftCorrectionEnabled ? "ENABLED" : "DISABLED",
           cisCals.driftThreshold);
}

/* Example 7: Performance measurement */
void example_performance_measurement(void)
{
    uint32_t start_time, end_time, duration_us;

    // Measure performance of standard calibration
    start_time = DWT->CYCCNT; // Assuming DWT cycle counter is enabled
    cis_applyLinearCalibration(cisDataCpy, 255);
    end_time = DWT->CYCCNT;
    duration_us = (end_time - start_time) / (SystemCoreClock / 1000000);
    printf("Standard calibration: %lu µs\n", duration_us);

    // Measure performance of drift-corrected calibration
    start_time = DWT->CYCCNT;
    cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);
    end_time = DWT->CYCCNT;
    duration_us = (end_time - start_time) / (SystemCoreClock / 1000000);
    printf("Drift-corrected calibration: %lu µs\n", duration_us);
}

/* Example 8: Error handling and fallback */
void example_error_handling(void)
{
    // Check if calibration data is valid
    if (shared_var.cis_cal_state != CIS_CAL_END)
    {
        printf("ERROR: Calibration not completed, using raw data\n");
        return; // Skip calibration
    }

    // Check if drift correction references are available
    int references_valid = 1;
    for (int color = 0; color < 3 && references_valid; color++)
    {
        for (int lane = 0; lane < CIS_ADC_OUT_LANES && references_valid; lane++)
        {
            if (cisCals.blackRefInactiveAvg[color][lane] == 0)
            {
                references_valid = 0;
            }
        }
    }

    if (!references_valid)
    {
        printf("WARNING: Drift correction references invalid, using standard calibration\n");
        cis_applyLinearCalibration(cisDataCpy, 255);
    }
    else
    {
        // All checks passed, use drift correction
        cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);
    }
}

/* Example 9: Integration with existing CIS processing function */
void example_integration_with_existing_code(void)
{
    // This shows how to modify an existing CIS processing function
    // to use the new drift correction capability

    // Existing code structure (simplified):
    /*
    void cis_processLine(void)
    {
        // Get raw data
        cis_getRawData(cisDataCpy);

        // OLD: Apply standard calibration
        cis_applyLinearCalibration(cisDataCpy, 255);

        // Convert to RGB
        cis_convertToRGB(cisDataCpy, rgbBuffer);

        // Send data
        cis_sendRGBData(rgbBuffer);
    }
    */

    // NEW: Modified to use drift correction
    // Get raw data
    cis_getRawData(cisDataCpy);

    // NEW: Apply calibration with drift correction
    cis_applyLinearCalibrationWithDriftCorrection(cisDataCpy, 255);

    // Rest of the processing remains unchanged
    cis_convertToRGB(cisDataCpy, rgbBuffer);
    cis_sendRGBData(rgbBuffer);
}

/* Example 10: Calibration initialization with drift correction */
void example_calibration_initialization(void)
{
    // Initialize calibration system
    CISCALIBRATION_StatusTypeDef cal_status = cis_linearCalibrationInit();

    if (cal_status == CISCALIBRATION_OK)
    {
        printf("Calibration loaded successfully\n");

        // Check if drift correction data is available
        if (cisCals.blackRefInactiveAvg[0][0] != 0) // Simple validity check
        {
            printf("Drift correction references available\n");
            cisCals.driftCorrectionEnabled = 1; // Enable drift correction
        }
        else
        {
            printf("No drift correction references, using standard calibration\n");
            cisCals.driftCorrectionEnabled = 0; // Disable drift correction
        }
    }
    else
    {
        printf("Calibration initialization failed\n");
        // Handle error...
    }
}

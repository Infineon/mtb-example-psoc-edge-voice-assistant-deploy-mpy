/******************************************************************************
* File Name : audio_enhancement.h
*
* Description :
* Header for the DEEPCRAFT audio enhancement (AE)
********************************************************************************
* Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#ifndef _AUDIO_ENHANCEMENT_H_
#define _AUDIO_ENHANCEMENT_H_

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

/******************************************************************************
 * Macros
 *****************************************************************************/


/******************************************************************************
 * Typedefs
 *****************************************************************************/
typedef enum
{
    AE_RSLT_SUCCESS = 0,
    AE_RSLT_ALLOC_ERROR = -1,
    AE_RSLT_INVALID_ARGUMENT = -2,
    AE_RSLT_LICENSE_ERROR = -3,
    AE_RSLT_FAIL = -4,    
} ae_rslt_t;

typedef enum
{
    AE_READ_CONFIG,    /* Asks application to read the configuration value */
    AE_UPDATE_CONFIG,  /* Asks application to update/apply the configuration value */
    AE_NOTIFY_CONFIG   /* Notifies application */
} ae_config_action_t;

typedef enum
{
    AE_CONFIG_AEC_STATE,              /* State of AEC component */
    AE_CONFIG_AEC_BULK_DELAY,         /* Bulk delay configuration for AEC */
    AE_CONFIG_BEAM_FORMING_STATE,     /* State of beam forming component */
    AE_CONFIG_INFERENCE_CANCELLER,    /* Inference canceller configuration for beam forming */
    AE_CONFIG_DEREVEREBERATION_STATE, /* State of derevereberation component */
    AE_CONFIG_ESNS_STATE,             /* State of echo suppression/noise suppression component */
    AE_CONFIG_ECHO_SUPPRESSOR,        /* Echo suppressor configuration (dB) */
    AE_CONFIG_NOISE_SUPPRESSOR,       /* Noise suppressor configuration */
    AE_CONFIG_INPUT_GAIN,             /* Input gain configuration */
    AE_CONFIG_HPF,                    /* High pass filter configuration */
    AE_CONFIG_BULK_DELAY_CALC_START,   /* Bulk delay calculation started */
    AE_CONFIG_BULK_DELAY_CALC_STOPPED, /* Bulk delay calculation stopped */
    /* AE_CONFIG_COMPONENT_STATE,      State change of a component */
    AE_CONFIG_STREAM,                 /* Start Stop stream */
} ae_config_name_t;


/******************************************************************************
 * Structures
 ******************************************************************************/
/* Output buffer structure for filtered audio data */
typedef struct
{
    int16_t *input_buf; /* Input buffer pointer */
    int16_t *input_aec_ref_buf; /* AEC buffer pointer which was passed during feed_input() call */
    int16_t *output_buf; /* Output buffer pointer */
#ifdef CY_AFE_ENABLE_TUNING_FEATURE
    int16_t *dbg_output1; /* Debug output1 based on configuration */
    int16_t *dbg_output2; /* Debug output2 based on configuration */
    int16_t *dbg_output3; /* Debug output3 based on configuration */
    int16_t *dbg_output4; /* Debug output4 based on configuration */
#endif
 } ae_buffer_info_t;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
ae_rslt_t audio_enhancement_init(uint8_t num_channels);
ae_rslt_t audio_enhancement_feed_input(int16_t *input_buffer, 
                                       int16_t *aec_buffer);
void      audio_enhancement_process_output(ae_buffer_info_t *output_buffer);
#ifdef CY_AFE_ENABLE_TUNING_FEATURE
void      audio_enhancement_tuner_notify(ae_config_action_t action, 
                                         ae_config_name_t name, void *value);
void      audio_enhancement_tuner_read(uint8_t *buffer, uint16_t *length);
void      audio_enhancement_tuner_write(uint8_t *buffer, uint16_t length);
#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _AUDIO_ENHANCEMENT_H_ */

/* [] END OF FILE */

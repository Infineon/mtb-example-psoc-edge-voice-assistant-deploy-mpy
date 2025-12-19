/******************************************************************************
* File Name : audio_enhancement.h
*
* Description :
* Header for the DEEPCRAFT audio enhancement (AE)
********************************************************************************
 * (c) 2025, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
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

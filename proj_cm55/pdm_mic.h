/******************************************************************************
* File Name : pdm_mic.h
*
* Description :
* Header for PDM microphone driver.
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
#ifndef _PDM_MIC_H__
#define _PDM_MIC_H__

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include "cy_result.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define PDM_MIC_SAMPLE_RATE_HZ              (16000u)
#define PDM_MIC_MODE                        CYHAL_PDM_PCM_MODE_LEFT
#define PDM_MIC_NUM_CHANNEL                 (1)
#define PDM_MIC_SAMPLES_COUNT               (160*PDM_MIC_NUM_CHANNEL)

/*******************************************************************************
* Functions Prototypes
*******************************************************************************/
cy_rslt_t pdm_mic_init(void);
cy_rslt_t pdm_mic_get_data(int16_t **frame);
cy_rslt_t pdm_mic_deinit(void);


#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _PDM_MIC_INTERFACE_H__*/

/* [] END OF FILE */

/******************************************************************************
* File Name : led_pwm.h
*
* Description :
* Header for LED PWM driver.
********************************************************************************
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
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
#ifndef _LED_PWM_H__
#define _LED_PWM_H__

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include "cy_result.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define LED_PWM_MAX_BRIGHTNESS              (100u)
#define LED_PWM_MIN_BRIGHTNESS              (10u)

#define LED_PWM_BLUE_LED                    (1u)    
#define LED_PWM_GREEN_LED                   (2u)

/*******************************************************************************
* Functions Prototypes
*******************************************************************************/
cy_rslt_t led_pwm_init(void);
void led_pwm_on(uint8_t led);
void led_pwm_off(uint8_t led);
void led_pwm_toggle(uint8_t led);
cy_rslt_t led_pwm_set_brightness(uint8_t led, uint8_t brightness);
cy_rslt_t led_pwm_get_brightness(uint8_t led, uint8_t *brightness);
void led_pwm_deinit(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _LED_PWM_H__*/

/* [] END OF FILE */
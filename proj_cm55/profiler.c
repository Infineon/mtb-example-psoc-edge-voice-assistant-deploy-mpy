/******************************************************************************
* File Name : profiler.c
*
* Description :
* Code for MCPS profiler
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

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cy_pdl.h"
#include "profiler.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define RESET_CYCLE_CNT (DWT->CYCCNT=0)
#define GET_CYCLE_CNT (DWT->CYCCNT)

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

/*******************************************************************************
* Global Variables
*******************************************************************************/
uint32_t profiler_cycles = 0;

/*******************************************************************************
* Function Name: Cy_Reset_Cycles
********************************************************************************
* Summary:
* Reset the DWT counter.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
uint32_t Cy_Reset_Cycles(void)
{
    /* Call DWTCyCNTInit before first call */
    return RESET_CYCLE_CNT;
}

/*******************************************************************************
* Function Name: Cy_Get_Cycles
********************************************************************************
* Summary:
* Return the current DWT counter.
*
* Parameters:
*  None
*
* Return:
*  Current DWT counter value.
*
*******************************************************************************/
uint32_t Cy_Get_Cycles(void)
{
    /* Call DWTCyCNTInit before first call */
    return GET_CYCLE_CNT;
}

/*******************************************************************************
* Function Name: profiler_init
********************************************************************************
* Summary:
* This function configures the DWT cycle counter.
*
* Parameters:
*  none
*
* Return:
*  None
*
*******************************************************************************/
 void profiler_init(void)
 {
    /* Disable TRC */
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // ~0x01000000;
    /* Enable TRC */
    CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk; // 0x01000000;

    /* Disable clock cycle counter */
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
    /* Enable  clock cycle counter */
    DWT->CTRL |=  DWT_CTRL_CYCCNTENA_Msk; //0x00000001;
 }

/*******************************************************************************
* Function Name: profiler_start
********************************************************************************
* Summary:
* Start profiler.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void profiler_start(void)
{
    RESET_CYCLE_CNT;
}

/*******************************************************************************
* Function Name: profiler_stop
********************************************************************************
* Summary:
* Stop profiler.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void profiler_stop(void)
{
    profiler_cycles = GET_CYCLE_CNT;
}

/*******************************************************************************
* Function Name: cy_profiler_get_cycles
********************************************************************************
* Summary:
* Get profiling cycles.
*
* Parameters:
*  None
*
* Return:
*  Profiling cycles.
*
*******************************************************************************/
uint32_t profiler_get_cycles(void)
{
    return profiler_cycles;
}


/* [] END OF FILE */

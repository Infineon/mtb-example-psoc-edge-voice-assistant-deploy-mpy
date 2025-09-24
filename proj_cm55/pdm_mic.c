/******************************************************************************
* File Name : pdm_mic.c
*
* Description :
* Code for PDM microphone driver.
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

#include "pdm_mic.h"

#include "cy_pdl.h"
#include "cybsp.h"

#include "cyabs_rtos.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* PDM PCM interrupt priority */
#define PDM_PCM_ISR_PRIORITY                    (2u)

/* PDM PCM Hardware Gain */
#ifdef USE_KIT_PSE84_AI
#define PDM_PCM_GAIN                            (CY_PDM_PCM_SEL_GAIN_11DB)
#else
#define PDM_PCM_GAIN                            (CY_PDM_PCM_SEL_GAIN_23DB)
#endif

/* Channel Index */
#define RIGHT_CH_INDEX                          (3u)
#define RIGHT_CH_CONFIG                         channel_3_config

/* PDM PCM hardware FIFO size */
#define HW_FIFO_SIZE                            (64u)

/* Rx FIFO trigger level/threshold configured by user */
#define RX_FIFO_TRIG_LEVEL                      (HW_FIFO_SIZE/2)

/* Total number of interrupts to get the FRAME_SIZE number of samples*/
#define NUMBER_INTERRUPTS_FOR_FRAME             (PDM_MIC_SAMPLES_COUNT/RX_FIFO_TRIG_LEVEL)

/*******************************************************************************
* Global Variables
*******************************************************************************/
volatile bool pdm_pcm_flag = false;

/* Set up one buffer for data collection and one for processing */
int16_t audio_buffer0[PDM_MIC_SAMPLES_COUNT] = {0};
int16_t audio_buffer1[PDM_MIC_SAMPLES_COUNT] = {0};
int16_t* active_rx_buffer;
int16_t* full_rx_buffer;

cy_semaphore_t pdm_mic_sema;

/* PDM PCM interrupt configuration parameters */
const cy_stc_sysint_t PDM_IRQ_cfg =
{
    .intrSrc = (IRQn_Type)CYBSP_PDM_CHANNEL_3_IRQ,
    .intrPriority = PDM_PCM_ISR_PRIORITY
};


/*******************************************************************************
* Functions Prototypes
*******************************************************************************/

/*******************************************************************************
 * Function Name: pdm_pcm_event_handler
 ********************************************************************************
 * Summary:
 *  PDM PCM converter interrupt handler.
 *  Populates ping/pong audio data buffer and inform application every 10 ms.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 *******************************************************************************/
static void pdm_pcm_event_handler(void)
{
    /* Used to track how full the buffer is */
    static uint16_t frame_counter = 0;

    /* Check the interrupt status */
    uint32_t intr_status = Cy_PDM_PCM_Channel_GetInterruptStatusMasked(CYBSP_PDM_HW, RIGHT_CH_INDEX);
    if(CY_PDM_PCM_INTR_RX_TRIGGER & intr_status)
    {
        /* Move data from the PDM fifo and place it in a buffer */
        for(uint32_t index=0; index < RX_FIFO_TRIG_LEVEL; index++)
        {
            int32_t pdm_data = (int32_t)Cy_PDM_PCM_Channel_ReadFifo(CYBSP_PDM_HW, RIGHT_CH_INDEX);
            active_rx_buffer[frame_counter * RX_FIFO_TRIG_LEVEL + index] = (int16_t)(pdm_data);
        }
        Cy_PDM_PCM_Channel_ClearInterrupt(CYBSP_PDM_HW, RIGHT_CH_INDEX, CY_PDM_PCM_INTR_RX_TRIGGER);
        frame_counter++;
    }

    /* Check if the buffer is full */
    if((NUMBER_INTERRUPTS_FOR_FRAME) <= frame_counter)
    {
        /* Flip the active and the next rx buffers */
        int16_t* temp = active_rx_buffer;
        active_rx_buffer = full_rx_buffer;
        full_rx_buffer = temp;

        /* Set the PDM_PCM flag as true, signaling there is data ready for use */
        pdm_pcm_flag = true;
        frame_counter = 0;

        cy_rtos_semaphore_set(&pdm_mic_sema);
    }

    /* Clear the remaining interrupts */
    if((CY_PDM_PCM_INTR_RX_FIR_OVERFLOW | CY_PDM_PCM_INTR_RX_OVERFLOW|
            CY_PDM_PCM_INTR_RX_IF_OVERFLOW | CY_PDM_PCM_INTR_RX_UNDERFLOW) & intr_status)
    {
        Cy_PDM_PCM_Channel_ClearInterrupt(CYBSP_PDM_HW, RIGHT_CH_INDEX, CY_PDM_PCM_INTR_MASK);
    }
}

/*******************************************************************************
* Function Name: pdm_mic_init
********************************************************************************
* Summary:
* Initalize the PDM block with the required settings for PSoC Edge.
*
* Parameters:
*  None
*
* Return:
*  CY_RSLT_SUCCESS
*
*******************************************************************************/
cy_rslt_t pdm_mic_init(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Init internal semaphore */
    result = cy_rtos_semaphore_init(&pdm_mic_sema, 5, 0);

    if(CY_RSLT_SUCCESS != result)
    {
        printf("MIC:PDM PCM semaphore init failed %u \r\n",result);
        return result;
    }

    /* Initialize PDM PCM block */
    result = Cy_PDM_PCM_Init(CYBSP_PDM_HW, &CYBSP_PDM_config);
    if(CY_PDM_PCM_SUCCESS != result)
    {
        return result;
    }

    /* Initialize and enable PDM PCM channel 3 -Right */
    Cy_PDM_PCM_Channel_Init(CYBSP_PDM_HW, &RIGHT_CH_CONFIG, (uint8_t)RIGHT_CH_INDEX);
    Cy_PDM_PCM_Channel_Enable(CYBSP_PDM_HW, RIGHT_CH_INDEX);

    /* An interrupt is registered for right channel, clear and set masks for it. */
    Cy_PDM_PCM_Channel_ClearInterrupt(CYBSP_PDM_HW, RIGHT_CH_INDEX, CY_PDM_PCM_INTR_MASK);
    Cy_PDM_PCM_Channel_SetInterruptMask(CYBSP_PDM_HW, RIGHT_CH_INDEX, CY_PDM_PCM_INTR_MASK);

    /* Register the IRQ handler */
    result = Cy_SysInt_Init(&PDM_IRQ_cfg, &pdm_pcm_event_handler);
    if(CY_SYSINT_SUCCESS != result)
    {
        return result;
    }
    NVIC_ClearPendingIRQ(PDM_IRQ_cfg.intrSrc);
    NVIC_EnableIRQ(PDM_IRQ_cfg.intrSrc);

    /* Set up pointers to two buffers to implement a ping-pong buffer system.
    * One gets filled by the PDM while the other can be processed. */
    active_rx_buffer = audio_buffer0;
    full_rx_buffer = audio_buffer1;

    /* Set gain and activate the PDM/PCM block */
    Cy_PDM_PCM_SetGain(CYBSP_PDM_HW, RIGHT_CH_INDEX, PDM_PCM_GAIN);
    Cy_PDM_PCM_Activate_Channel(CYBSP_PDM_HW, RIGHT_CH_INDEX);
    

    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
* Function Name: pdm_mic_get_data
********************************************************************************
* Summary:
* Get data from the microphone (one frame). This function is blocking. It only
* return when data is available.
*
* Parameters:
*  frame: pointer to the data
*
* Return:
*  CY_RSLT_SUCCESS
*
*******************************************************************************/
cy_rslt_t pdm_mic_get_data(int16_t **frame)
{
    cy_rslt_t result;

    result = cy_rtos_semaphore_get(&pdm_mic_sema, CY_RTOS_NEVER_TIMEOUT);

    if (result == CY_RSLT_SUCCESS)
    {
        *frame = full_rx_buffer;
    }

    return result;
}

/*******************************************************************************
* Function Name: pdm_mic_deinit
********************************************************************************
* Summary:
*  De-initializes PDM-PCM block.
*
* Parameters:
*  None
*
* Return:
*  CY_RSLT_SUCCESS
*
*******************************************************************************/
cy_rslt_t pdm_mic_deinit(void) {

    Cy_PDM_PCM_DeInit(CYBSP_PDM_HW);
    return CY_RSLT_SUCCESS;
}

/* [] END OF FILE */

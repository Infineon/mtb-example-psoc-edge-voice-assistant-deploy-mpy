/****************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for CM55 CPU
*
* Related Document : See README.md
*
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

#include "cybsp.h"
#include <string.h>
#include "ipc_communication.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"
#include "pdm_mic.h"
#include "retarget_io_init.h"
#include "voice_assistant.h"
#include "profiler.h"

#ifdef USE_AUDIO_ENHANCEMENT
#include "audio_enhancement.h"
#endif

#include MTB_WWD_NLU_APP_HEADER(PROJECT_PREFIX)
#include MTB_WWD_NLU_CONFIG_HEADER(PROJECT_PREFIX)

/*****************************************************************************
 * Macros
 *****************************************************************************/
#define VA_TASK_NAME             ("va-task")
#define VA_TASK_STACK_SIZE       (10 * 1024)
#define VA_TASK_PRIORITY         (CY_RTOS_PRIORITY_NORMAL)
#define COMMAND_STRING_SIZE      (250U)
#define CMD_TIMEOUT_MS           (5000U)
#define IPC_SEND_MAX_RETRIES     (200U)
#define IPC_SEND_RETRY_DELAY_US  (1000U)
#define CM55_INIT_DELAY_MS       (50U)
#define RESET_VAL                (0U)

/* All CM55 output is offloaded to CM33 MicroPython via IPC - no UART prints */
#define CM55_DBG(...)  do {} while(0)

/* IPC commands CM33->CM55 (IPC_CMD_START=0x82, IPC_CMD_STOP=0x83 from ipc_communication.h) */

/* IPC status/event commands CM55->CM33 */
#define IPC_CMD_VA_READY              (0xA0)  /* VA initialised, ready to listen  */
#define IPC_CMD_VA_WAKEWORD_DETECTED  (0xA2)  /* Wake-word detected               */
#define IPC_CMD_VA_TIMEOUT            (0xA3)  /* Command listen window timed out  */
#define IPC_CMD_VA_STOPPED            (0xA4)  /* VA stopped, ack to STOP command  */
#define IPC_CMD_VA_ERROR              (0xE1)  /* Fatal VA error                   */
/* Detected command intent index is sent as-is (model-defined, handled by CM33) */

/*****************************************************************************
 * Variables
 *****************************************************************************/
CY_SECTION_SHAREDMEM static ipc_msg_t cm55_ipc_tx;  /* Shared TX IPC message buffer */

static volatile bool va_enabled  = false;
static TaskHandle_t  va_task_hdl = NULL;

/* Required by audio-voice-core */
uint8_t  bf_coeffs[1];
uint32_t bf_coeffs_total_len;


/*******************************************************************************
 * Function Name: ipc_send_to_cm33
 * Summary: Send a command byte to CM33 via IPC pipe with retry on busy.
 *******************************************************************************/
static void ipc_send_to_cm33(uint8_t cmd)
{
    cy_en_ipc_pipe_status_t status;
    uint32_t retries = 0;

    cm55_ipc_tx.client_id = CM33_IPC_PIPE_CLIENT_ID;
    cm55_ipc_tx.intr_mask = 0;
    cm55_ipc_tx.cmd       = cmd;
    cm55_ipc_tx.value     = RESET_VAL;

    Cy_SysLib_DelayUs(2000U);

    while (retries < IPC_SEND_MAX_RETRIES)
    {
        status = Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR,
                                         CM55_IPC_PIPE_EP_ADDR,
                                         (void *)&cm55_ipc_tx, NULL);
        if (status == CY_IPC_PIPE_SUCCESS)
        {
            CM55_DBG("[CM55] IPC cmd 0x%02X sent\r\n", cmd);
            return;
        }
        retries++;
        Cy_SysLib_DelayUs(IPC_SEND_RETRY_DELAY_US);
    }
    CM55_DBG("[CM55] IPC send failed: cmd 0x%02X\r\n", cmd);
}

/*******************************************************************************
 * Function Name: cm55_msg_callback
 * Summary: Called when CM55 IPC endpoint receives a message from CM33.
 *          IPC_CMD_START resumes the VA task; IPC_CMD_STOP suspends it.
 *******************************************************************************/
void cm55_msg_callback(uint32_t *msgData)
{
    if (msgData == NULL) return;

    uint8_t cmd = ((ipc_msg_t *)msgData)->cmd;
    CM55_DBG("[CM55] IPC rx: 0x%02X\r\n", cmd);

    if (cmd == IPC_CMD_START)
    {
        va_enabled = true;
        if (va_task_hdl != NULL) vTaskResume(va_task_hdl);
        CM55_DBG("[CM55] VA started\r\n");
    }
    else if (cmd == IPC_CMD_STOP)
    {
        va_enabled = false;
        if (va_task_hdl != NULL) vTaskSuspend(va_task_hdl);
        ipc_send_to_cm33(IPC_CMD_VA_STOPPED);  /* Acknowledge STOP to CM33 */
    }
}


/*******************************************************************************
 * Function Name: run_voice_assistant_process
 * Summary: Process one audio frame; on detection send matching IPC command.
 *******************************************************************************/
static void run_voice_assistant_process(int16_t *audio_frame)
{
    va_rslt_t  va_result;
    va_data_t  va_data;
    va_event_t va_event;
    char       cmd_text[COMMAND_STRING_SIZE] = {0};

    va_result = voice_assistant_process(audio_frame, &va_event, &va_data);

    if (va_result == VA_RSLT_LICENSE_ERROR)
    {
        CM55_DBG("[CM55] ERROR: VA license expired\r\n");
        handle_error();
    }
    else if (va_result != VA_RSLT_SUCCESS)
    {
        return;
    }

    switch (va_event)
    {
        case VA_EVENT_WW_DETECTED:
            ipc_send_to_cm33(IPC_CMD_VA_WAKEWORD_DETECTED);
            break;

        case VA_EVENT_CMD_DETECTED:
            if (CY_RSLT_SUCCESS == voice_assistant_get_command(cmd_text))
            {
                /* Send the intent index directly - CM33 maps it to an action */
                ipc_send_to_cm33((uint8_t)va_data.intent_index);
            }
            break;

        case VA_EVENT_CMD_TIMEOUT:
            ipc_send_to_cm33(IPC_CMD_VA_TIMEOUT);
            break;

        default:
            break;
    }
}


/*******************************************************************************
 * Function Name: voice_assistant_task
 * Summary: FreeRTOS task - initialises VA, then processes audio frames.
 *          Starts suspended; resumed via IPC_CMD_START from CM33.
 *******************************************************************************/
void voice_assistant_task(void *arg)
{
    int16_t   *audio_frame;
    va_rslt_t  va_result;
#ifdef USE_AUDIO_ENHANCEMENT
    ae_rslt_t  ae_result;
#endif

    pdm_mic_init();

    va_result = voice_assistant_init(VA_MODE_WW_SINGLE_CMD);
    if (va_result != VA_RSLT_SUCCESS)
    {
        ipc_send_to_cm33(IPC_CMD_VA_ERROR);
        handle_error();
    }

    va_result = voice_assistant_set_command_timeout(CMD_TIMEOUT_MS);
    if (va_result != VA_RSLT_SUCCESS)
    {
        ipc_send_to_cm33(IPC_CMD_VA_ERROR);
        handle_error();
    }

    /* Notify CM33 that VA is fully initialised and listening */
    ipc_send_to_cm33(IPC_CMD_VA_READY);

    for (;;)
    {
        if (!va_enabled)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        pdm_mic_get_data(&audio_frame);

#ifdef USE_AUDIO_ENHANCEMENT
        ae_result = audio_enhancement_feed_input(audio_frame, NULL);
        if (ae_result == AE_RSLT_LICENSE_ERROR)
        {
            CM55_DBG("[CM55] ERROR: AFE license expired\r\n");
            handle_error();
        }
#else
        run_voice_assistant_process(audio_frame);
#endif
    }
}

/*******************************************************************************
 * Function Name: main
 * Summary: Initialise BSP, IPC, create VA task (suspended), start scheduler.
 *******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    __enable_irq();

    cm55_ipc_communication_setup();
    Cy_SysLib_Delay(CM55_INIT_DELAY_MS);

    cy_en_ipc_pipe_status_t pipeStatus =
        Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR,
                                     &cm55_msg_callback,
                                     (uint32_t)CM55_IPC_PIPE_CLIENT_ID);
    if (pipeStatus != CY_IPC_PIPE_SUCCESS)
    {
        CM55_DBG("[CM55] IPC callback register failed\r\n");
        handle_error();
    }

#ifdef USE_AUDIO_ENHANCEMENT
    ae_rslt_t ae_result = audio_enhancement_init(1U);
    if (ae_result != AE_RSLT_SUCCESS)
    {
        CM55_DBG("[CM55] AFE init failed: %d\r\n", ae_result);
        handle_error();
    }
#endif

    result = xTaskCreate(voice_assistant_task, VA_TASK_NAME,
                         VA_TASK_STACK_SIZE, NULL,
                         VA_TASK_PRIORITY, &va_task_hdl);
    if (result != pdPASS)
    {
        CM55_DBG("[CM55] Task create failed\r\n");
        handle_error();
    }

    vTaskSuspend(va_task_hdl);  /* Suspended until IPC_CMD_START received */

    vTaskStartScheduler();

    return 0;
}

#ifdef USE_AUDIO_ENHANCEMENTP
/*******************************************************************************
 * Function Name: audio_enhancement_process_output
 * Summary: AFE output callback - feeds enhanced audio into the VA pipeline.
 *********************************************v**********************************/
void audio_enhancement_process_output(ae_buffer_info_t *output_buffer)
{
    run_voice_assistant_process(output_buffer->output_buf);
}
#endif /* USE_AUDIO_ENHANCEMENT */

/* [] END OF FILE */
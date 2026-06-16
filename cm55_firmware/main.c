/*
 * main.c — Target-side Voice Assistant application.
 *
 * Application code only.  No transport, IPC, or protocol constants appear here.
 * All DeepCraft model interface calls go through deepcraft_wrapper.h:
 *   deepcraft_wrapper_init()       — boot-time setup
 *   deepcraft_wrapper_notify_*()   — report VA events to the host
 *
 * Transport is selected in deepcraft_wrapper.c.
 *
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: MIT
 */

#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pdm_mic.h"
#include "retarget_io_init.h"
#include "voice_assistant.h"
#include "profiler.h"

#ifdef USE_AUDIO_ENHANCEMENT
#include "audio_enhancement.h"
#endif

#include MTB_WWD_NLU_APP_HEADER(PROJECT_PREFIX)
#include MTB_WWD_NLU_CONFIG_HEADER(PROJECT_PREFIX)

/* DeepCraft model interface — the only DeepCraft header main.c needs */
#include "deepcraft_wrapper.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
#define VA_TASK_NAME         ("va-task")
#define VA_TASK_STACK_SIZE   (10 * 1024)
#define VA_TASK_PRIORITY     (CY_RTOS_PRIORITY_NORMAL)
#define COMMAND_STRING_SIZE  (250U)
#define CMD_TIMEOUT_MS       (5000U)

/*******************************************************************************
 * Module state
 ******************************************************************************/
static volatile bool g_va_enabled  = false;
static TaskHandle_t  g_va_task_hdl = NULL;

/* Required by audio-voice-core */
uint8_t  bf_coeffs[1];
uint32_t bf_coeffs_total_len;

/*******************************************************************************
 * DeepCraft lifecycle callbacks
 * Called from interrupt context by the interface layer when the host sends
 * a START or STOP command.  Keep them minimal.
 ******************************************************************************/
static void on_va_start(void)
{
    g_va_enabled = true;
    if (g_va_task_hdl != NULL) {
        xTaskResumeFromISR(g_va_task_hdl);
    }
}

static void on_va_stop(void)
{
    g_va_enabled = false;
    if (g_va_task_hdl != NULL) {
        vTaskSuspend(g_va_task_hdl);
    }
    deepcraft_wrapper_notify_stopped();
}

/*******************************************************************************
 * run_va_process
 * Process one audio frame and notify the host of any detected VA events.
 ******************************************************************************/
static void run_va_process(int16_t *audio_frame)
{
    va_rslt_t  va_result;
    va_data_t  va_data;
    va_event_t va_event;
    char       cmd_text[COMMAND_STRING_SIZE];

    va_result = voice_assistant_process(audio_frame, &va_event, &va_data);

    if (va_result == VA_RSLT_LICENSE_ERROR) {
        deepcraft_wrapper_notify_error();
        handle_error();
    }
    if (va_result != VA_RSLT_SUCCESS) {
        return;
    }

    switch (va_event) {
        case VA_EVENT_WW_DETECTED:
            deepcraft_wrapper_notify_wakeword_detected();
            break;

        case VA_EVENT_CMD_DETECTED:
            if (CY_RSLT_SUCCESS == voice_assistant_get_command(cmd_text)) {
                deepcraft_wrapper_notify_intent((uint8_t)va_data.intent_index);
            }
            break;

        case VA_EVENT_CMD_TIMEOUT:
            deepcraft_wrapper_notify_timeout();
            break;

        default:
            break;
    }
}

/*******************************************************************************
 * voice_assistant_task
 * FreeRTOS task.  Initialises the VA library, signals ready to the host, then
 * processes audio frames in a loop.
 * Created suspended; resumed by on_va_start() when the host sends START.
 ******************************************************************************/
static void voice_assistant_task(void *arg)
{
    (void)arg;
    int16_t   *audio_frame;
    va_rslt_t  va_result;
#ifdef USE_AUDIO_ENHANCEMENT
    ae_rslt_t  ae_result;
#endif

    pdm_mic_init();

    va_result = voice_assistant_init(VA_MODE_WW_SINGLE_CMD);
    if (va_result != VA_RSLT_SUCCESS) {
        deepcraft_wrapper_notify_error();
        handle_error();
    }

    va_result = voice_assistant_set_command_timeout(CMD_TIMEOUT_MS);
    if (va_result != VA_RSLT_SUCCESS) {
        deepcraft_wrapper_notify_error();
        handle_error();
    }

    deepcraft_wrapper_notify_ready();

    for (;;) {
        if (!g_va_enabled) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        pdm_mic_get_data(&audio_frame);

#ifdef USE_AUDIO_ENHANCEMENT
        ae_result = audio_enhancement_feed_input(audio_frame, NULL);
        if (ae_result == AE_RSLT_LICENSE_ERROR) {
            deepcraft_wrapper_notify_error();
            handle_error();
        }
        /* Output is delivered asynchronously via audio_enhancement_process_output() callback */
#else
        run_va_process(audio_frame);
#endif
    }
}

#ifdef USE_AUDIO_ENHANCEMENT
/* AFE output callback — feeds enhanced audio directly into the VA pipeline */
void audio_enhancement_process_output(ae_buffer_info_t *output_buffer)
{
    run_va_process(output_buffer->output_buf);
}
#endif

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    __enable_irq();

    /* Initialise the DeepCraft model interface (transport configured inside) */
    deepcraft_wrapper_init(on_va_start, on_va_stop);

#ifdef USE_AUDIO_ENHANCEMENT
    ae_rslt_t ae_result = audio_enhancement_init(1U);
    if (ae_result != AE_RSLT_SUCCESS) {
        handle_error();
    }
#endif

    result = xTaskCreate(voice_assistant_task,
        VA_TASK_NAME, VA_TASK_STACK_SIZE,
        NULL, VA_TASK_PRIORITY,
        &g_va_task_hdl);
    CY_ASSERT(result == pdPASS);
    vTaskSuspend(g_va_task_hdl);

    vTaskStartScheduler();

    CY_ASSERT(false);
    return 0;
}

/* [] END OF FILE */


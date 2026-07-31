/*
 * main_tflm.c — Target-side plain TensorFlow Lite Micro (TFLM) application.
 *
 * Selected when the firmware is built with MODEL_TYPE=tflm. Runs a vendored
 * upstream TFLM runtime (./deps/tflm) through the flat, C-callable tflm_wrapper
 * API — no Infineon mtb-ml / mtb-tflite-micro packages and no U55 NPU are used,
 * so any TFLM-compatible .tflite model can be deployed by regenerating
 * model_framework/tflm/<name>/model_data.cpp and adjusting the op resolver / I/O
 * below.
 *
 * The bundled "sine" model takes a window of the last N sine samples and
 * predicts the next value; the result is printed over the debug UART while
 * sweeping the window across one full period [0, 2*pi).
 *
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "retarget_io_init.h"

#include "tflm_wrapper.h"
#include "model_data.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
#define TFLM_TASK_NAME        ("tflm-task")
#define TFLM_TASK_STACK_SIZE  (4 * 1024)      /* words */
#define TFLM_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)

/* Number of prediction steps to sweep across the sine wave. */
#define NUM_SAMPLES           (20)

#ifndef M_PI
#define M_PI                  (3.14159265358979323846)
#endif

/* Angular step (radians) between consecutive samples fed to the model. One
 * full period [0, 2*pi) is covered every NUM_SAMPLES steps. */
#define SAMPLE_STEP           (2.0 * M_PI / NUM_SAMPLES)

/* Upper bound on the model's input/output element counts (for stack buffers). */
#define MAX_IO_ELEMENTS       (64)

/* Pacing between predictions (ms). */
#define STEP_DELAY_MS         (200U)

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
/* Tensor arena: scratch memory used by TFLM for input/output/intermediate
 * tensors. The sine model is tiny; 4 KB is comfortably enough. Must be
 * 16-byte aligned. Increase this if you deploy a larger model. */
#define TENSOR_ARENA_SIZE     (4 * 1024)
static uint8_t tensor_arena[TENSOR_ARENA_SIZE] __attribute__((aligned(16)));

static TaskHandle_t g_tflm_task_hdl = NULL;

/*******************************************************************************
 * Function Name: print_signed_fixed
 ******************************************************************************
 * Prints a float with 4 decimal places without relying on %f support in the
 * C library's printf (picolibc/newlib-nano disable floating-point printf by
 * default).
 ******************************************************************************/
static void print_signed_fixed(float value)
{
    bool negative = (value < 0.0f);
    if (negative) {
        value = -value;
    }

    int32_t scaled = (int32_t)(value * 10000.0f + 0.5f);
    int32_t whole  = scaled / 10000;
    int32_t frac   = scaled % 10000;

    printf("%s%ld.%04ld", negative ? "-" : " ", (long)whole, (long)frac);
}

/*******************************************************************************
 * Function Name: tflm_task
 ******************************************************************************
 * Sets up the TFLM interpreter for the bundled model, then continuously sweeps
 * x over [0, 2*pi) printing the model's predicted sin(x) alongside the
 * mathematically correct value.
 ******************************************************************************/
static void tflm_task(void *arg)
{
    (void)arg;

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear the terminal screen. */
    printf("\x1b[2J\x1b[;H");
    printf("*********** PSOC Edge MCU: TFLM sine model on CM55 ***********\r\n\n");

    if (tflm_wrapper_init(g_model_data, tensor_arena, TENSOR_ARENA_SIZE) != TFLM_OK) {
        printf("ERROR: tflm_wrapper_init() failed (schema/op/arena)\r\n");
        CY_ASSERT(false);
    }

    printf("Arena used: %u / %u bytes\r\n\n",
           (unsigned)tflm_wrapper_arena_used(), (unsigned)TENSOR_ARENA_SIZE);

    /* Discover the model's real I/O shape. This sine model takes a window of
     * past samples (input length N) and predicts the next value (length 1). */
    size_t in_len  = tflm_wrapper_input_length(0);
    size_t out_len = tflm_wrapper_output_length(0);

    printf("Model I/O:  input length = %u,  output length = %u\r\n\n",
           (unsigned)in_len, (unsigned)out_len);

    if ((in_len == 0) || (in_len > MAX_IO_ELEMENTS) ||
        (out_len == 0) || (out_len > MAX_IO_ELEMENTS)) {
        printf("ERROR: unexpected model I/O shape\r\n");
        CY_ASSERT(false);
    }

    printf("Feeding a %u-sample sine window; predicting the next value.\r\n",
           (unsigned)in_len);
    printf("     window end x  ->  predicted sin  |  actual\r\n");
    printf("-------------------------------------------------------------\r\n");

    for (;;) {
        for (int i = 0; i < NUM_SAMPLES; i++) {
            /* Build the input window: in_len consecutive true sine samples
             * starting at phase i*SAMPLE_STEP. */
            float window[MAX_IO_ELEMENTS];
            for (size_t j = 0; j < in_len; j++) {
                window[j] = sinf((float)((i + (int)j) * SAMPLE_STEP));
            }

            if (tflm_wrapper_set_input_f32(0, window, in_len) != TFLM_OK) {
                printf("ERROR: set_input failed at step %d\r\n", i);
                continue;
            }
            if (tflm_wrapper_invoke() != TFLM_OK) {
                printf("ERROR: Invoke() failed at step %d\r\n", i);
                continue;
            }

            float y_out[MAX_IO_ELEMENTS];
            (void)tflm_wrapper_get_output_f32(0, y_out, out_len);
            float y_pred = y_out[0];
            float x_next = (float)((i + (int)in_len) * SAMPLE_STEP);
            float y_true = sinf(x_next);

            printf("x=");
            print_signed_fixed(x_next);
            printf("  pred=");
            print_signed_fixed(y_pred);
            printf("  actual=");
            print_signed_fixed(y_true);
            printf("\r\n");

            vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
        }

        printf("-------------------------------------------------------------\r\n");
    }
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    __enable_irq();

    /* Route printf() to the debug UART (CM55 owns the UART in this example). */
    init_retarget_io();

    result = xTaskCreate(tflm_task,
                         TFLM_TASK_NAME, TFLM_TASK_STACK_SIZE,
                         NULL, TFLM_TASK_PRIORITY,
                         &g_tflm_task_hdl);
    CY_ASSERT(result == pdPASS);

    vTaskStartScheduler();

    CY_ASSERT(false);
    return 0;
}

/* [] END OF FILE */

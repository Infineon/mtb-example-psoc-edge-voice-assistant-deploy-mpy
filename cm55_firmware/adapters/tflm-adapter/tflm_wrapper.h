/*******************************************************************************
* File Name        : tflm_wrapper.h
*
* Description      : Flat, C-callable interface around the TensorFlow Lite for
*                    Microcontrollers (TFLM) tflite::MicroInterpreter runtime.
*
*                    This is the only TFLM header the application (main.cpp) or
*                    the inter-core service layer needs to include. All C++/TFLM
*                    details (MicroInterpreter, op resolver, schema) are hidden
*                    behind this pointer-light API so the caller stays entirely
*                    model- and framework-agnostic.
*
*                    Typical use:
*                      tflm_wrapper_init(g_model_data, arena, sizeof(arena));
*                      tflm_wrapper_set_input_f32(0, &x, 1);
*                      tflm_wrapper_invoke();
*                      tflm_wrapper_get_output_f32(0, &y, 1);
*
* Related Document : See README.md
*
********************************************************************************
 * (c) 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
*******************************************************************************/

#ifndef TFLM_WRAPPER_H
#define TFLM_WRAPPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Status codes returned by the wrapper API. */
typedef enum
{
    TFLM_OK = 0,            /**< Operation succeeded.                        */
    TFLM_ERR_BAD_ARG,      /**< A NULL pointer or out-of-range index/size.  */
    TFLM_ERR_SCHEMA,       /**< Model schema version mismatch.              */
    TFLM_ERR_OP_RESOLVE,   /**< Failed to register a required operator.     */
    TFLM_ERR_ALLOC,        /**< AllocateTensors() failed (arena too small). */
    TFLM_ERR_TYPE,         /**< Tensor element type is not what was asked.  */
    TFLM_ERR_INVOKE,       /**< Invoke() failed.                            */
    TFLM_ERR_STATE,        /**< Called before a successful init.            */
} tflm_status_t;

/**
 * @brief Initialise the interpreter for a model and register its operators.
 *
 * Must be called once before any other wrapper function. The caller owns the
 * tensor @p arena; it must remain valid for the lifetime of the interpreter
 * and be 16-byte aligned.
 *
 * @param model_data  Pointer to the flatbuffer model (e.g. g_model_data).
 * @param arena       Caller-provided scratch memory for TFLM tensors.
 * @param arena_size  Size of @p arena in bytes.
 * @return TFLM_OK on success, otherwise an error code.
 */
tflm_status_t tflm_wrapper_init(const void *model_data,
                                uint8_t *arena, size_t arena_size);

/** @return Number of input tensors, or 0 if not initialised. */
size_t tflm_wrapper_input_count(void);

/** @return Number of output tensors, or 0 if not initialised. */
size_t tflm_wrapper_output_count(void);

/** @return Element count of input tensor @p index, or 0 on error. */
size_t tflm_wrapper_input_length(int index);

/** @return Element count of output tensor @p index, or 0 on error. */
size_t tflm_wrapper_output_length(int index);

/** @return Bytes of the arena actually used after allocation, or 0. */
size_t tflm_wrapper_arena_used(void);

/**
 * @brief Copy @p count float32 values into input tensor @p index.
 * @return TFLM_OK, TFLM_ERR_BAD_ARG, TFLM_ERR_TYPE or TFLM_ERR_STATE.
 */
tflm_status_t tflm_wrapper_set_input_f32(int index,
                                         const float *data, size_t count);

/**
 * @brief Copy @p count float32 values out of output tensor @p index.
 * @return TFLM_OK, TFLM_ERR_BAD_ARG, TFLM_ERR_TYPE or TFLM_ERR_STATE.
 */
tflm_status_t tflm_wrapper_get_output_f32(int index,
                                          float *data, size_t count);

/**
 * @brief Run inference on the currently loaded input tensors.
 * @return TFLM_OK or TFLM_ERR_INVOKE / TFLM_ERR_STATE.
 */
tflm_status_t tflm_wrapper_invoke(void);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* TFLM_WRAPPER_H */

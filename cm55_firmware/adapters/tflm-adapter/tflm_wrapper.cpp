/*******************************************************************************
* File Name        : tflm_wrapper.cpp
*
* Description      : C++ implementation of the flat tflm_wrapper API declared in
*                    tflm_wrapper.h. Wraps a single tflite::MicroInterpreter and
*                    a MicroMutableOpResolver so callers never touch TFLM types.
*
*                    Operator registration lives in register_ops() below. This
*                    is the ONE place to add operators when deploying a model
*                    that uses more than FULLY_CONNECTED (e.g. AddConv2D(),
*                    AddSoftmax(), AddReshape(), ...). Keep kMaxOps >= the number
*                    of distinct operators you register.
*
********************************************************************************
 * (c) 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
*******************************************************************************/

#include "tflm_wrapper.h"

#include <new>

#ifndef TF_LITE_STRIP_ERROR_STRINGS
#define TF_LITE_STRIP_ERROR_STRINGS 1
#endif

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

namespace {

/* Maximum number of distinct operators the resolver can hold. Increase if a
 * model needs more op kinds than are registered in register_ops(). */
constexpr size_t kMaxOps = 8;

using OpResolver = tflite::MicroMutableOpResolver<kMaxOps>;

/* Interpreter and resolver are non-default-constructible and must outlive
 * init(), so hold them in aligned static storage and placement-new them. */
alignas(tflite::MicroInterpreter) uint8_t s_interpreter_buf[sizeof(tflite::MicroInterpreter)];
alignas(OpResolver)                uint8_t s_resolver_buf[sizeof(OpResolver)];

tflite::MicroInterpreter *s_interpreter = nullptr;
OpResolver               *s_resolver    = nullptr;
bool                      s_ready       = false;

/* Register the operators used by the deployed model. */
tflm_status_t register_ops(OpResolver *resolver)
{
    /* The sine model only uses FULLY_CONNECTED. Add more ops here as needed. */
    if (resolver->AddFullyConnected() != kTfLiteOk)
    {
        return TFLM_ERR_OP_RESOLVE;
    }
    return TFLM_OK;
}

size_t tensor_length(const TfLiteTensor *t)
{
    if ((t == nullptr) || (t->dims == nullptr))
    {
        return 0;
    }
    size_t n = 1;
    for (int i = 0; i < t->dims->size; i++)
    {
        n *= (size_t)t->dims->data[i];
    }
    return n;
}

}  /* namespace */

extern "C" tflm_status_t tflm_wrapper_init(const void *model_data,
                                           uint8_t *arena, size_t arena_size)
{
    if ((model_data == nullptr) || (arena == nullptr) || (arena_size == 0))
    {
        return TFLM_ERR_BAD_ARG;
    }

    s_ready = false;

    const tflite::Model *model =
        tflite::GetModel(static_cast<const void *>(model_data));
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        return TFLM_ERR_SCHEMA;
    }

    s_resolver = new (s_resolver_buf) OpResolver();
    tflm_status_t st = register_ops(s_resolver);
    if (st != TFLM_OK)
    {
        return st;
    }

    s_interpreter = new (s_interpreter_buf) tflite::MicroInterpreter(
        model, *s_resolver, arena, arena_size);

    if (s_interpreter->AllocateTensors() != kTfLiteOk)
    {
        return TFLM_ERR_ALLOC;
    }

    s_ready = true;
    return TFLM_OK;
}

extern "C" size_t tflm_wrapper_input_count(void)
{
    return s_ready ? s_interpreter->inputs_size() : 0;
}

extern "C" size_t tflm_wrapper_output_count(void)
{
    return s_ready ? s_interpreter->outputs_size() : 0;
}

extern "C" size_t tflm_wrapper_input_length(int index)
{
    if (!s_ready || (index < 0) ||
        ((size_t)index >= s_interpreter->inputs_size()))
    {
        return 0;
    }
    return tensor_length(s_interpreter->input(index));
}

extern "C" size_t tflm_wrapper_output_length(int index)
{
    if (!s_ready || (index < 0) ||
        ((size_t)index >= s_interpreter->outputs_size()))
    {
        return 0;
    }
    return tensor_length(s_interpreter->output(index));
}

extern "C" size_t tflm_wrapper_arena_used(void)
{
    return s_ready ? s_interpreter->arena_used_bytes() : 0;
}

extern "C" tflm_status_t tflm_wrapper_set_input_f32(int index,
                                                    const float *data,
                                                    size_t count)
{
    if (!s_ready)
    {
        return TFLM_ERR_STATE;
    }
    if ((data == nullptr) || (index < 0) ||
        ((size_t)index >= s_interpreter->inputs_size()))
    {
        return TFLM_ERR_BAD_ARG;
    }

    TfLiteTensor *t = s_interpreter->input(index);
    if (t->type != kTfLiteFloat32)
    {
        return TFLM_ERR_TYPE;
    }
    if (count != tensor_length(t))
    {
        return TFLM_ERR_BAD_ARG;
    }

    for (size_t i = 0; i < count; i++)
    {
        t->data.f[i] = data[i];
    }
    return TFLM_OK;
}

extern "C" tflm_status_t tflm_wrapper_get_output_f32(int index,
                                                     float *data,
                                                     size_t count)
{
    if (!s_ready)
    {
        return TFLM_ERR_STATE;
    }
    if ((data == nullptr) || (index < 0) ||
        ((size_t)index >= s_interpreter->outputs_size()))
    {
        return TFLM_ERR_BAD_ARG;
    }

    TfLiteTensor *t = s_interpreter->output(index);
    if (t->type != kTfLiteFloat32)
    {
        return TFLM_ERR_TYPE;
    }
    if (count != tensor_length(t))
    {
        return TFLM_ERR_BAD_ARG;
    }

    for (size_t i = 0; i < count; i++)
    {
        data[i] = t->data.f[i];
    }
    return TFLM_OK;
}

extern "C" tflm_status_t tflm_wrapper_invoke(void)
{
    if (!s_ready)
    {
        return TFLM_ERR_STATE;
    }
    if (s_interpreter->Invoke() != kTfLiteOk)
    {
        return TFLM_ERR_INVOKE;
    }
    return TFLM_OK;
}

/* [] END OF FILE */

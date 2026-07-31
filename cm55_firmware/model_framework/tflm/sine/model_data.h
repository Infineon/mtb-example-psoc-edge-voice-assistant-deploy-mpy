/*******************************************************************************
 * File Name:   model_data.h
 *
 * Description: Declares the TensorFlow Lite for Microcontrollers model buffer
 *              that is deployed on the CM55 core. The model bytes live in
 *              model_data.cpp (auto-generated from the .tflite flatbuffer).
 *
 *              To deploy a different TFLM model, regenerate model_data.cpp from
 *              your own .tflite file (keep the 16-byte alignment) and update the
 *              input/output handling in main.cpp accordingly.
 *******************************************************************************/

#ifndef MODEL_DATA_H_
#define MODEL_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

/* TFLite flatbuffer model, 16-byte aligned as required by TFLM.
 * Symbol names are model-agnostic so the application/wrapper can build any
 * model folder unchanged; only the bytes in model_data.cpp differ per model. */
extern const unsigned char g_model_data[];
extern const unsigned int g_model_data_len;

#ifdef __cplusplus
}
#endif

#endif /* MODEL_DATA_H_ */

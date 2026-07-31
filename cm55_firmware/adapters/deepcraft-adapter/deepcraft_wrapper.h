/**
 * @file deepcraft_wrapper.h
 * @brief Target-side DeepCraft model interface — public application API.
 *
 * This is the **only** DeepCraft header that application code (@c main.c)
 * needs to include.  All transport (IPC, UART, …) details are hidden behind
 * a flat, pointer-free function API.
 *
 * @par File layout (target project)
 * @code
 *  deepcraft_wrapper.h / .c  — Public API + bridge  (you are here)
 *  ipc.h / ipc.c             — IPC transport vtable implementation
 *  main.c                    — Application: VA task, audio pipeline
 * @endcode
 *
 * To swap transports, only @c deepcraft_wrapper.c changes — @c main.c is
 * entirely unaffected.
 *
 * @copyright Copyright (c) 2026 Infineon Technologies AG
 * @license SPDX-License-Identifier: MIT
 */

#ifndef DEEPCRAFT_WRAPPER_H
#define DEEPCRAFT_WRAPPER_H

#include <stdint.h>

/**
 * @defgroup deepcraft_wrapper DeepCraft Wrapper API
 * @brief Pointer-free API for the CM55 application to interact with the
 *        DeepCraft model interface.
 * @{
 */

/**
 * @brief Callback invoked when the host sends @ref DEEPCRAFT_CMD_START.
 *
 * Called from interrupt context; defer heavy work to a FreeRTOS task
 * (e.g. via @c xTaskResumeFromISR()).
 */
typedef void (*deepcraft_on_start_t)(void);

/**
 * @brief Callback invoked when the host sends @ref DEEPCRAFT_CMD_STOP.
 *
 * Called from interrupt context; defer heavy work to a FreeRTOS task.
 */
typedef void (*deepcraft_on_stop_t)(void);

/**
 * @brief Initialise the transport and register application lifecycle callbacks.
 *
 * Must be called once at boot, before starting the RTOS scheduler.
 * Internally selects and initialises the configured transport (currently IPC).
 *
 * @param on_start  Called when the host sends CMD_START. Must not be NULL.
 * @param on_stop   Called when the host sends CMD_STOP.  Must not be NULL.
 */
void deepcraft_wrapper_init(deepcraft_on_start_t on_start,
    deepcraft_on_stop_t on_stop);

/**
 * @brief Notify the host that the VA model is initialised and ready.
 *
 * Sends @ref DEEPCRAFT_CMD_VA_READY via the configured transport.
 * The host receives @ref VA_EVENT_READY.
 */
void deepcraft_wrapper_notify_ready(void);

/**
 * @brief Notify the host that the wake-word was detected.
 *
 * Sends @ref DEEPCRAFT_CMD_VA_WAKEWORD_DETECTED via the configured transport.
 * The host receives @ref VA_EVENT_WAKEWORD_DETECTED.
 */
void deepcraft_wrapper_notify_wakeword_detected(void);

/**
 * @brief Notify the host of a recognised intent.
 *
 * Sends the raw @p intent_index byte as a wire command.
 * The host receives @ref VA_EVENT_INTENT with @c value = @p intent_index.
 *
 * @param intent_index  Zero-based index of the recognised command as defined
 *                      in the DEEPCRAFT model project.
 */
void deepcraft_wrapper_notify_intent(uint8_t intent_index);

/**
 * @brief Notify the host that the command listen window timed out.
 *
 * Sends @ref DEEPCRAFT_CMD_VA_TIMEOUT via the configured transport.
 * The host receives @ref VA_EVENT_TIMEOUT.
 */
void deepcraft_wrapper_notify_timeout(void);

/**
 * @brief Notify the host that the VA has stopped (ack to CMD_STOP).
 *
 * Sends @ref DEEPCRAFT_CMD_VA_STOPPED via the configured transport.
 * The host receives @ref VA_EVENT_STOPPED.
 */
void deepcraft_wrapper_notify_stopped(void);

/**
 * @brief Notify the host of a fatal VA error.
 *
 * Sends @ref DEEPCRAFT_CMD_VA_ERROR via the configured transport.
 * The host receives @ref VA_EVENT_ERROR.
 */
void deepcraft_wrapper_notify_error(void);

/** @} */ /* end of deepcraft_wrapper group */

#endif /* DEEPCRAFT_WRAPPER_H */

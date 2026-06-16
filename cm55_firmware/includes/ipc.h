/*
 * ipc.h — IPC transport implementation of deepcraft_interface_t
 *          (target / C-application side, PSoC Edge IPC pipe).
 *
 * Included only by deepcraft_wrapper.c — application code (main.c) must NOT
 * include this file directly; use deepcraft_wrapper.h instead.
 *
 * Provides:
 *   ipc_interface_init()  — sets up IPC pipe and registers callbacks
 *   ipc_notify_*()        — send VA events to the host
 *
 * To swap transports, only deepcraft_wrapper.c needs to change: replace the
 * include of this header and the init call.  The vtable contract
 * (deepcraft_interface_t) is unchanged.
 *
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: MIT
 */

#ifndef IPC_H
#define IPC_H

#include <stdint.h>
#include "deepcraft_interface.h"   /* from deepcraft-model-interface/src/    */
#include "deepcraft_wrapper.h"     /* deepcraft_on_start_t / on_stop_t types */

/*
 * IPC transport instance.
 * `base` MUST be first — allows cast to deepcraft_interface_t *.
 */
typedef struct {
    deepcraft_interface_t  base;       /* vtable — MUST be first      */
    void (*on_receive)(uint8_t cmd, uint32_t value);  /* ISR relay    */
} ipc_interface_t;

/*
 * ipc_interface_init
 *
 * Sets up the IPC pipe, registers the ISR receive callback, and stores the
 * on_start / on_stop application callbacks.
 * Call once at boot, before starting the RTOS scheduler.
 */
void ipc_interface_init(ipc_interface_t *self,
    deepcraft_on_start_t on_start,
    deepcraft_on_stop_t  on_stop);

/* ── Notify helpers: send VA model events to the host ───────────────────── */
void ipc_notify_ready(ipc_interface_t *self);
void ipc_notify_wakeword_detected(ipc_interface_t *self);
void ipc_notify_timeout(ipc_interface_t *self);
void ipc_notify_stopped(ipc_interface_t *self);
void ipc_notify_intent(ipc_interface_t *self, uint8_t intent_index);
void ipc_notify_error(ipc_interface_t *self);

#endif /* IPC_H */

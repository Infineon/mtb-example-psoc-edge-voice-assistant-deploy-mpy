/*
 * wrapper.c — Target-side DeepCraft model interface bridge.
 *
 * Sits between application code (main.c) and the transport layer (ipc.c):
 *
 *   main.c
 *     │  deepcraft_wrapper_init() / deepcraft_wrapper_notify_*()
 *     │
 *   adapters/deepcraft/wrapper.c ← you are here
 *     │  ipc_interface_init() / ipc_notify_*()
 *     │
 *   ipc.c                        ← IPC transport vtable
 *     │
 *     │ IPC pipe
 *   Host (MicroPython running va_app.py + DEEPCRAFTModel)
 *
 * To swap transports:
 *   1. Replace the #include + ipc_interface_init() call below with equivalents.
 *   2. main.c and wrapper.h are unchanged.
 *
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>
#include "wrapper.h"

/*
 * Transport selection — swap this include to change the transport.
 * The only requirement is that the replacement header exposes:
 *   - a struct whose first member is deepcraft_interface_t (vtable)
 *   - an init function: <transport>_interface_init(self *, on_start, on_stop)
 *   - notify helpers:   <transport>_notify_*(self *)
 */
/* ── Module state ────────────────────────────────────────────────── */
static deepcraft_interface_t *s_interface;
static deepcraft_on_start_t s_on_start;
static deepcraft_on_stop_t s_on_stop;

static void on_cmd_from_host(uint8_t cmd, uint32_t value)
{
    (void)value;
    if (cmd == DEEPCRAFT_CMD_START && s_on_start != NULL) {
        s_on_start();
    } else if (cmd == DEEPCRAFT_CMD_STOP && s_on_stop != NULL) {
        s_on_stop();
    }
}

/* ── deepcraft_wrapper_init ─────────────────────────────────────────────── */
void deepcraft_wrapper_init(deepcraft_interface_t *interface,
    deepcraft_on_start_t on_start,
    deepcraft_on_stop_t on_stop)
{
    s_interface = interface;
    s_on_start = on_start;
    s_on_stop = on_stop;
    s_interface->register_receive_cb(s_interface, on_cmd_from_host);
}

/* ── Notify wrappers ─────────────────────────────────────────────────────── */

void deepcraft_wrapper_notify_ready(void)
{
    s_interface->send(s_interface, DEEPCRAFT_CMD_VA_READY, 0U);
}

void deepcraft_wrapper_notify_wakeword_detected(void)
{
    s_interface->send(s_interface, DEEPCRAFT_CMD_VA_WAKEWORD_DETECTED, 0U);
}

void deepcraft_wrapper_notify_intent(uint8_t intent_index)
{
    s_interface->send(s_interface, intent_index, 0U);
}

void deepcraft_wrapper_notify_timeout(void)
{
    s_interface->send(s_interface, DEEPCRAFT_CMD_VA_TIMEOUT, 0U);
}

void deepcraft_wrapper_notify_stopped(void)
{
    s_interface->send(s_interface, DEEPCRAFT_CMD_VA_STOPPED, 0U);
}

void deepcraft_wrapper_notify_error(void)
{
    s_interface->send(s_interface, DEEPCRAFT_CMD_VA_ERROR, 0U);
}

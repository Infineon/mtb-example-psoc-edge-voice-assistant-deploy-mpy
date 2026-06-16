/*
 * deepcraft_wrapper.c — Target-side DeepCraft model interface bridge.
 *
 * Sits between application code (main.c) and the transport layer (ipc.c):
 *
 *   main.c
 *     │  deepcraft_wrapper_init() / deepcraft_wrapper_notify_*()
 *     │
 *   deepcraft_wrapper.c          ← you are here
 *     │  ipc_interface_init() / ipc_notify_*()
 *     │
 *   ipc.c                        ← IPC transport vtable
 *     │
 *     │ IPC pipe
 *   Host (MicroPython running va_app.py + DEEPCRAFTModel)
 *
 * To swap transports:
 *   1. Replace the #include + ipc_interface_init() call below with equivalents.
 *   2. main.c and deepcraft_wrapper.h are unchanged.
 *
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: MIT
 */

#include "deepcraft_wrapper.h"

/*
 * Transport selection — swap this include to change the transport.
 * The only requirement is that the replacement header exposes:
 *   - a struct whose first member is deepcraft_interface_t (vtable)
 *   - an init function: <transport>_interface_init(self *, on_start, on_stop)
 *   - notify helpers:   <transport>_notify_*(self *)
 */
#include "ipc.h"

/* ── Module state ────────────────────────────────────────────────── */
/*
 * Transport interface instance.  Owned here so neither main.c nor the
 * transport file needs to expose it as a global.
 */
static ipc_interface_t s_iface;

/* ── deepcraft_wrapper_init ─────────────────────────────────────────────── */
void deepcraft_wrapper_init(deepcraft_on_start_t on_start,
    deepcraft_on_stop_t on_stop)
{
    ipc_interface_init(&s_iface, on_start, on_stop);
}

/* ── Notify wrappers ─────────────────────────────────────────────────────── */

void deepcraft_wrapper_notify_ready(void)
{
    ipc_notify_ready(&s_iface);
}

void deepcraft_wrapper_notify_wakeword_detected(void)
{
    ipc_notify_wakeword_detected(&s_iface);
}

void deepcraft_wrapper_notify_intent(uint8_t intent_index)
{
    ipc_notify_intent(&s_iface, intent_index);
}

void deepcraft_wrapper_notify_timeout(void)
{
    ipc_notify_timeout(&s_iface);
}

void deepcraft_wrapper_notify_stopped(void)
{
    ipc_notify_stopped(&s_iface);
}

void deepcraft_wrapper_notify_error(void)
{
    ipc_notify_error(&s_iface);
}

/*
 * ipc.c — IPC transport implementation of deepcraft_interface_t
 *          (target / C-application side, PSoC Edge IPC pipe).
 *
 * Implements the two vtable function pointers (send, register_receive_cb) and
 * the notify_* API so deepcraft_target.c contains zero raw IPC or PDL calls.
 *
 * To use a different transport, create a new transport file that fills in a
 * deepcraft_interface_t with its own send / register_receive_cb and exposes
 * the same init + notify_* signatures.  deepcraft_target.c is unchanged.
 *
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: MIT
 */

#include "ipc.h"
#include "ipc_communication.h"   /* from shared/include — IPC pipe constants   */

#include "cybsp.h"
#include "cy_pdl.h"
#include "cy_ipc_pipe.h"

/* ── Tunable send parameters ─────────────────────────────────────────────── */
#define IPC_SEND_MAX_RETRIES     (200U)
#define IPC_SEND_RETRY_DELAY_US  (1000U)

/* ── Shared TX buffer ────────────────────────────────────────────────────── */
/* Must reside in IPC-visible SRAM so both processors can access it.         */
CY_SECTION_SHAREDMEM static ipc_msg_t s_tx_msg;

/* Singleton — static ISR trampoline needs to reach the instance             */
static ipc_interface_t *s_iface = NULL;

/* Application-level start / stop callbacks, set during init                 */
static deepcraft_on_start_t s_on_start = NULL;
static deepcraft_on_stop_t  s_on_stop  = NULL;

/* ── Internal receive dispatcher ─────────────────────────────────────────── */
/*
 * Called (via on_receive) when a command arrives from the host.
 * Routes DEEPCRAFT_CMD_START / DEEPCRAFT_CMD_STOP to the application callbacks.
 */
static void on_cmd_from_host(uint8_t cmd, uint32_t value)
{
    (void)value;
    if (cmd == DEEPCRAFT_CMD_START && s_on_start != NULL) {
        s_on_start();
    } else if (cmd == DEEPCRAFT_CMD_STOP && s_on_stop != NULL) {
        s_on_stop();
    }
}

/* IPC pipe ISR trampoline — called by the PDL pipe driver */
static void ipc_rx_callback(uint32_t *msg_data)
{
    if (msg_data == NULL || s_iface == NULL) {
        return;
    }
    const ipc_msg_t *msg = (const ipc_msg_t *)msg_data;
    if (s_iface->on_receive != NULL) {
        s_iface->on_receive(msg->cmd, msg->value);
    }
}

/* ── vtable: send ────────────────────────────────────────────────────────── */
static bool ipc_send(deepcraft_interface_t *self, uint8_t cmd, uint32_t value)
{
    (void)self;
    cy_en_ipc_pipe_status_t status;
    uint32_t retries = 0;

    s_tx_msg.client_id = CM33_IPC_PIPE_CLIENT_ID;
    s_tx_msg.intr_mask = 0;
    s_tx_msg.cmd       = cmd;
    s_tx_msg.value     = value;

    Cy_SysLib_DelayUs(2000U);

    while (retries < IPC_SEND_MAX_RETRIES) {
        status = Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR,
            CM55_IPC_PIPE_EP_ADDR,
            (void *)&s_tx_msg, NULL);
        if (status == CY_IPC_PIPE_SUCCESS) {
            return true;
        }
        retries++;
        Cy_SysLib_DelayUs(IPC_SEND_RETRY_DELAY_US);
    }
    return false;
}

/* ── vtable: register_receive_cb ─────────────────────────────────────────── */
static void ipc_register_receive_cb(deepcraft_interface_t *self,
    void (*cb)(uint8_t cmd, uint32_t value))
{
    ipc_interface_t *iface = (ipc_interface_t *)self;
    iface->on_receive = cb;
    Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR,
        &ipc_rx_callback,
        (uint32_t)CM55_IPC_PIPE_CLIENT_ID);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Public init
 * ═══════════════════════════════════════════════════════════════════════════ */
void ipc_interface_init(ipc_interface_t *self,
    deepcraft_on_start_t on_start,
    deepcraft_on_stop_t  on_stop)
{
    self->base.send               = ipc_send;
    self->base.register_receive_cb = ipc_register_receive_cb;
    self->on_receive               = NULL;

    s_iface    = self;
    s_on_start = on_start;
    s_on_stop  = on_stop;

    /* Platform-specific IPC pipe setup (defined in shared/source) */
    cm55_ipc_communication_setup();

    /* Wire the internal command dispatcher as the receive callback */
    self->base.register_receive_cb(&self->base, on_cmd_from_host);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Notify helpers — send VA model events to the host
 * ═══════════════════════════════════════════════════════════════════════════ */
void ipc_notify_ready(ipc_interface_t *self)
{
    self->base.send(&self->base, DEEPCRAFT_CMD_VA_READY, 0U);
}

void ipc_notify_wakeword_detected(ipc_interface_t *self)
{
    self->base.send(&self->base, DEEPCRAFT_CMD_VA_WAKEWORD_DETECTED, 0U);
}

void ipc_notify_timeout(ipc_interface_t *self)
{
    self->base.send(&self->base, DEEPCRAFT_CMD_VA_TIMEOUT, 0U);
}

void ipc_notify_stopped(ipc_interface_t *self)
{
    self->base.send(&self->base, DEEPCRAFT_CMD_VA_STOPPED, 0U);
}

void ipc_notify_intent(ipc_interface_t *self, uint8_t intent_index)
{
    /* Intent index is sent as the cmd byte; host interprets the value */
    self->base.send(&self->base, intent_index, 0U);
}

void ipc_notify_error(ipc_interface_t *self)
{
    self->base.send(&self->base, DEEPCRAFT_CMD_VA_ERROR, 0U);
}

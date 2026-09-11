#include "cy_pdl.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ipc.h"
#include <string.h>

#define IPC_TEST_TASK_NAME       ("ipc-test")
#define IPC_TEST_TASK_STACK_SIZE (2048U)
#define IPC_TEST_TASK_PRIORITY   (CY_RTOS_PRIORITY_NORMAL)

static ipc_interface_t g_ipc_interface;
static TaskHandle_t g_ipc_task_hdl = NULL;
static uint8_t g_echo_buffer[65536U];
static volatile size_t g_echo_length;

static void ipc_cmd_cb(uint8_t cmd, uint32_t value)
{
    (void)cmd;
    (void)value;
}

static void ipc_echo_cb(const uint8_t *data, size_t len)
{
    if (len <= sizeof(g_echo_buffer)) {
        memcpy(g_echo_buffer, data, len);
        __DMB();
        g_echo_length = len;
    }
}

static void ipc_test_task(void *argument)
{
    (void)argument;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ipc_interface_process();
        if (g_echo_length != 0U) {
            size_t len = g_echo_length;
            __DMB();
            ipc_interface_send_data(g_echo_buffer, len);
            g_echo_length = 0U;
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
}

int main(void)
{
    cy_rslt_t result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    __enable_irq();

    ipc_interface_init(&g_ipc_interface);
    g_ipc_interface.base.register_receive_cb(&g_ipc_interface.base, ipc_cmd_cb);
    ipc_interface_set_data_cb(ipc_echo_cb);

    BaseType_t task_result = xTaskCreate(ipc_test_task,
        IPC_TEST_TASK_NAME, IPC_TEST_TASK_STACK_SIZE,
        NULL, IPC_TEST_TASK_PRIORITY, &g_ipc_task_hdl);
    CY_ASSERT(task_result == pdPASS);
    ipc_interface_set_process_task(g_ipc_task_hdl);
    vTaskStartScheduler();

    CY_ASSERT(false);
    return 0;
}

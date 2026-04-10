from machine import IPC, Pin
import time

# --- Pin setup ---
pin_p17_1 = Pin("P17_1", Pin.OUT, value=1)

IPC_CMD_VA_READY             = 0xA0
IPC_CMD_VA_WAKEWORD_DETECTED = 0xA2
IPC_CMD_VA_TIMEOUT           = 0xA3
IPC_CMD_VA_STOPPED           = 0xA4
IPC_CMD_VA_ERROR             = 0xE1

# --- Model intent index mapping (update to match your DEEPCRAFT model) ---
# intent_index 0 -> "Make P17_1 high"
# intent_index 1 -> "Make P17_1 low"
INTENT_ACTIONS = {
    0: lambda: (pin_p17_1.value(0), print("P17_1 -> LOW")),
    1: lambda: (pin_p17_1.value(1), print("P17_1 -> HIGH")),
}

# --- IPC setup ---
ipc = IPC(src_core=IPC.CM33, target_core=IPC.CM55)
ipc.init()

va_svc = {"received": False, "cmd": None}

def va_svc_cb(cmd, val, cid):
    va_svc["received"] = True
    va_svc["cmd"] = cmd

r1 = ipc.register_client(3, va_svc_cb, 1, 1)
print("Voice Assistant Model service registered:", r1)

ipc.enable_core(IPC.CM55)
time.sleep_ms(500)

ipc.send(IPC.CMD_START, 0, 5)
print('\nSay the wake-word "OK test" followed by a command: \n1) Make P17_1 high \n2) Make P17_1 low\n')

# --- Cycle tracking ---
pin_low_done  = False
pin_high_done = False

# --- Main receive loop ---
while True:
    if va_svc["received"]:
        cmd = va_svc["cmd"]
        va_svc["received"] = False
        va_svc["cmd"] = None

        if cmd == IPC_CMD_VA_READY:
            print("CM55: VA ready and listening")

        elif cmd == IPC_CMD_VA_WAKEWORD_DETECTED:
            print("CM55: Wake-word detected!")

        elif cmd == IPC_CMD_VA_TIMEOUT:
            print("CM55: Command timeout - say wake-word again")

        elif cmd == IPC_CMD_VA_STOPPED:
            print("CM55: VA stopped - exiting")
            break

        elif cmd == IPC_CMD_VA_ERROR:
            print("CM55: Fatal VA error - exiting")
            break

        elif cmd in INTENT_ACTIONS:
            INTENT_ACTIONS[cmd]()
            # Track cycle completion
            if cmd == 0: pin_low_done  = True
            if cmd == 1: pin_high_done = True

        else:
            print("CM55: unknown cmd=0x{:02X}".format(cmd))

        # Stop after one complete cycle
        if pin_low_done and pin_high_done:
            print("\nCycle complete - sending STOP to CM55")
            ipc.send(IPC.CMD_STOP, 0, 5)
            timeout = 1000
            while timeout > 0 and not va_svc["received"]:
                time.sleep_ms(10)
                timeout -= 10
            break

    time.sleep_ms(10)

# Cleanup
pin_p17_1.value(1)
time.sleep_ms(20)
print("\nVA Assistant model execution stopped")

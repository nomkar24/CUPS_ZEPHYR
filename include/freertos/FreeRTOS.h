#ifndef FREERTOS_H
#define FREERTOS_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/kernel.h>

#define pdFALSE 0
#define pdTRUE 1
#define pdPASS 1
#define pdFAIL 0

typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;

#define portMAX_DELAY 0xFFFFFFFFUL
#define portTICK_PERIOD_MS 1

typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0

#define portENTER_CRITICAL(mux) k_sched_lock()
#define portEXIT_CRITICAL(mux) k_sched_unlock()
#define portENTER_CRITICAL_ISR(mux) do {} while(0)
#define portEXIT_CRITICAL_ISR(mux) do {} while(0)
#define portENTER_CRITICAL_SAFE(mux) k_sched_lock()
#define portEXIT_CRITICAL_SAFE(mux) k_sched_unlock()

#define portYIELD_FROM_ISR(...) do {} while(0)
#define portYIELD() k_yield()

// Default ESP-IDF USB Kconfig values
#define CONFIG_USB_HOST_SET_ADDR_RECOVERY_MS 10
#define CONFIG_USB_HOST_CONTROL_TRANSFER_MAX_SIZE 256
#define CONFIG_USB_HOST_HW_ITERATIONS 10
#define CONFIG_USB_HOST_DEBOUNCE_DELAY_MS 100
#define CONFIG_USB_HOST_RESET_HOLD_MS 50
#define CONFIG_USB_HOST_RESET_RECOVERY_MS 250
#define CONFIG_USB_HOST_RESUME_HOLD_MS 20
#define CONFIG_USB_HOST_RESUME_RECOVERY_MS 10

#endif // FREERTOS_H

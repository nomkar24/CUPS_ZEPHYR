#ifndef TASK_H
#define TASK_H

#include "FreeRTOS.h"
#include <stdlib.h>

typedef void* TaskHandle_t;

#define pdMS_TO_TICKS(ms) (ms)

static inline void vTaskDelay(TickType_t ticks) {
    k_msleep(ticks);
}

// Thread-local pointer to a semaphore representing task notification
extern __thread struct k_sem *current_thread_sem;

static inline TaskHandle_t xTaskGetCurrentTaskHandle(void) {
    if (!current_thread_sem) {
        current_thread_sem = malloc(sizeof(struct k_sem));
        k_sem_init(current_thread_sem, 0, 1);
    }
    return (TaskHandle_t)current_thread_sem;
}

static inline void vTaskNotifyGiveFromISR(TaskHandle_t task, BaseType_t *woken) {
    if (woken) *woken = pdFALSE;
    k_sem_give((struct k_sem *)task);
}

static inline void xTaskNotifyGive(TaskHandle_t task) {
    k_sem_give((struct k_sem *)task);
}

static inline uint32_t ulTaskNotifyTake(BaseType_t clear, TickType_t timeout) {
    struct k_sem *sem = (struct k_sem *)xTaskGetCurrentTaskHandle();
    k_timeout_t t = (timeout == portMAX_DELAY) ? K_FOREVER : K_MSEC(timeout);
    int ret = k_sem_take(sem, t);
    return (ret == 0) ? 1 : 0;
}

#endif // TASK_H

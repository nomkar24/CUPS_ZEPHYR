#ifndef QUEUE_H
#define QUEUE_H

#include "FreeRTOS.h"
#include <stdlib.h>

typedef void* QueueHandle_t;

static inline QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize) {
    struct k_msgq *msgq = malloc(sizeof(struct k_msgq));
    if (!msgq) return NULL;
    char *buffer = malloc(uxQueueLength * uxItemSize);
    if (!buffer) {
        free(msgq);
        return NULL;
    }
    k_msgq_init(msgq, buffer, uxItemSize, uxQueueLength);
    return (QueueHandle_t)msgq;
}

static inline void vQueueDelete(QueueHandle_t xQueue) {
    if (xQueue) {
        struct k_msgq *msgq = (struct k_msgq *)xQueue;
        free(msgq->buffer_start);
        free(msgq);
    }
}

static inline BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait) {
    struct k_msgq *msgq = (struct k_msgq *)xQueue;
    k_timeout_t t = (xTicksToWait == portMAX_DELAY) ? K_FOREVER : K_MSEC(xTicksToWait);
    int ret = k_msgq_put(msgq, pvItemToQueue, t);
    return (ret == 0) ? pdTRUE : pdFALSE;
}

static inline BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait) {
    struct k_msgq *msgq = (struct k_msgq *)xQueue;
    k_timeout_t t = (xTicksToWait == portMAX_DELAY) ? K_FOREVER : K_MSEC(xTicksToWait);
    int ret = k_msgq_get(msgq, pvBuffer, t);
    return (ret == 0) ? pdTRUE : pdFALSE;
}

static inline UBaseType_t uxQueueMessagesWaiting(const QueueHandle_t xQueue) {
    struct k_msgq *msgq = (struct k_msgq *)xQueue;
    return k_msgq_num_used_get(msgq);
}

#endif // QUEUE_H

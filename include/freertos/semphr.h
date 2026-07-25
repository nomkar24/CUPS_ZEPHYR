#ifndef SEMPHR_H
#define SEMPHR_H

#include "FreeRTOS.h"
#include <stdlib.h>

typedef enum {
    SEM_TYPE_BINARY,
    SEM_TYPE_MUTEX
} sem_type_t;

typedef struct {
    sem_type_t type;
    union {
        struct k_sem sem;
        struct k_mutex mutex;
    } u;
} sem_wrapper_t;

typedef void* SemaphoreHandle_t;

static inline SemaphoreHandle_t xSemaphoreCreateBinary(void) {
    sem_wrapper_t *w = malloc(sizeof(sem_wrapper_t));
    if (!w) return NULL;
    w->type = SEM_TYPE_BINARY;
    k_sem_init(&w->u.sem, 0, 1);
    return (SemaphoreHandle_t)w;
}

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    sem_wrapper_t *w = malloc(sizeof(sem_wrapper_t));
    if (!w) return NULL;
    w->type = SEM_TYPE_MUTEX;
    k_mutex_init(&w->u.mutex);
    return (SemaphoreHandle_t)w;
}

static inline void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) {
    if (xSemaphore) {
        free(xSemaphore);
    }
}

static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xBlockTime) {
    sem_wrapper_t *w = (sem_wrapper_t *)xSemaphore;
    int ret;
    k_timeout_t t = (xBlockTime == portMAX_DELAY) ? K_FOREVER : K_MSEC(xBlockTime);
    if (w->type == SEM_TYPE_BINARY) {
        ret = k_sem_take(&w->u.sem, t);
    } else {
        ret = k_mutex_lock(&w->u.mutex, t);
    }
    return (ret == 0) ? pdTRUE : pdFALSE;
}

static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore) {
    sem_wrapper_t *w = (sem_wrapper_t *)xSemaphore;
    int ret;
    if (w->type == SEM_TYPE_BINARY) {
        k_sem_give(&w->u.sem);
        ret = 0;
    } else {
        ret = k_mutex_unlock(&w->u.mutex);
    }
    return (ret == 0) ? pdTRUE : pdFALSE;
}

static inline BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken) {
    if (pxHigherPriorityTaskWoken) *pxHigherPriorityTaskWoken = pdFALSE;
    sem_wrapper_t *w = (sem_wrapper_t *)xSemaphore;
    if (w->type == SEM_TYPE_BINARY) {
        k_sem_give(&w->u.sem);
    }
    return pdTRUE;
}

#endif // SEMPHR_H

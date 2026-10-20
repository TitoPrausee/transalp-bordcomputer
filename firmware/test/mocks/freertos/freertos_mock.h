#ifndef _FREERTOS_MOCK_H_
#define _FREERTOS_MOCK_H_

/*
 * freertos_mock.h — Minimal FreeRTOS mock for host-side unit tests.
 *
 * Provides POSIX-based implementations of the FreeRTOS primitives
 * used by the firmware components under test. This allows the
 * same source files to compile and run on a host PC with GCC
 * without the ESP-IDF toolchain.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdio.h>

/* ---- FreeRTOS types ---- */

typedef void *TaskHandle_t;
typedef void *QueueHandle_t;
typedef void *EventGroupHandle_t;
typedef void *SemaphoreHandle_t;
typedef int BaseType_t;
typedef uint32_t EventBits_t;
typedef uint32_t TickType_t;

#define pdTRUE    1
#define pdFALSE   0
#define pdPASS    1
#define pdFAIL    0
#define pdMS_TO_TICKS(ms)  ((TickType_t)(ms))
#define portMAX_DELAY      ((TickType_t)0xFFFFFFFF)
#define portTICK_PERIOD_MS 1

/* ---- Static semaphore buffer (unused on host, but required by API) ---- */
typedef struct {
    uint8_t dummy[64];
} StaticSemaphore_t;

/* ---- Task creation (mock: no real thread spawned) ---- */
static inline BaseType_t xTaskCreateStatic(
    void (*task_func)(void *),
    const char *name,
    uint32_t stack_depth,
    void *params,
    uint32_t prio,
    uint8_t *stack_buf,
    void *task_buf)
{
    (void)task_func; (void)name; (void)stack_depth;
    (void)params; (void)prio; (void)stack_buf; (void)task_buf;
    return pdTRUE;
}

static inline BaseType_t xTaskCreate(
    void (*task_func)(void *),
    const char *name,
    uint32_t stack_depth,
    void *params,
    uint32_t prio,
    TaskHandle_t *handle)
{
    (void)task_func; (void)name; (void)stack_depth;
    (void)params; (void)prio;
    if (handle) *handle = (TaskHandle_t)1;
    return pdTRUE;
}

static inline void vTaskDelay(TickType_t ticks) { (void)ticks; }

static inline BaseType_t xTaskCreatePinnedToCore(
    void (*task_func)(void *),
    const char *name,
    uint32_t stack_depth,
    void *params,
    uint32_t prio,
    TaskHandle_t *handle,
    BaseType_t core_id)
{
    (void)task_func; (void)name; (void)stack_depth;
    (void)params; (void)prio; (void)core_id;
    if (handle) *handle = (TaskHandle_t)1;
    return pdTRUE;
}

static inline BaseType_t xPortGetCoreID(void)
{
    return 0;
}

/* ---- Tick count (monotonic clock) ---- */
static inline TickType_t xTaskGetTickCount(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (TickType_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* ---- Queue (ring buffer with mutex) ---- */
typedef struct {
    void *data;
    int item_size;
    int capacity;
    int count;
    int head;
    int tail;
    pthread_mutex_t mutex;
    sem_t sem;
} mock_queue_t;

static inline QueueHandle_t xQueueCreate(int queue_len, int item_size)
{
    mock_queue_t *q = (mock_queue_t *)calloc(1, sizeof(mock_queue_t));
    if (!q) return NULL;
    q->data = calloc(queue_len, item_size);
    if (!q->data) { free(q); return NULL; }
    q->item_size = item_size;
    q->capacity = queue_len;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    pthread_mutex_init(&q->mutex, NULL);
    sem_init(&q->sem, 0, 0);
    return (QueueHandle_t)q;
}

static inline BaseType_t xQueueSend(QueueHandle_t h, const void *item, TickType_t timeout)
{
    (void)timeout;
    mock_queue_t *q = (mock_queue_t *)h;
    if (!q) return pdFALSE;
    pthread_mutex_lock(&q->mutex);
    if (q->count >= q->capacity) {
        pthread_mutex_unlock(&q->mutex);
        return pdFALSE;
    }
    memcpy((char *)q->data + q->head * q->item_size, item, q->item_size);
    q->head = (q->head + 1) % q->capacity;
    q->count++;
    pthread_mutex_unlock(&q->mutex);
    sem_post(&q->sem);
    return pdTRUE;
}

static inline BaseType_t xQueueReceive(QueueHandle_t h, void *item, TickType_t timeout)
{
    (void)timeout;
    mock_queue_t *q = (mock_queue_t *)h;
    if (!q) return pdFALSE;
    pthread_mutex_lock(&q->mutex);
    if (q->count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return pdFALSE;
    }
    memcpy(item, (char *)q->data + q->tail * q->item_size, q->item_size);
    q->tail = (q->tail + 1) % q->capacity;
    q->count--;
    pthread_mutex_unlock(&q->mutex);
    return pdTRUE;
}

static inline BaseType_t xQueuePeek(QueueHandle_t h, void *item, TickType_t timeout)
{
    (void)timeout;
    mock_queue_t *q = (mock_queue_t *)h;
    if (!q) return pdFALSE;
    pthread_mutex_lock(&q->mutex);
    if (q->count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return pdFALSE;
    }
    memcpy(item, (char *)q->data + q->tail * q->item_size, q->item_size);
    pthread_mutex_unlock(&q->mutex);
    return pdTRUE;
}

static inline void vQueueDelete(QueueHandle_t h)
{
    mock_queue_t *q = (mock_queue_t *)h;
    if (!q) return;
    free(q->data);
    pthread_mutex_destroy(&q->mutex);
    sem_destroy(&q->sem);
    free(q);
}

/* ---- Event Groups (bitmask with mutex) ---- */
typedef struct {
    EventBits_t bits;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} mock_event_group_t;

static inline EventGroupHandle_t xEventGroupCreate(void)
{
    mock_event_group_t *eg = (mock_event_group_t *)calloc(1, sizeof(mock_event_group_t));
    if (!eg) return NULL;
    eg->bits = 0;
    pthread_mutex_init(&eg->mutex, NULL);
    pthread_cond_init(&eg->cond, NULL);
    return (EventGroupHandle_t)eg;
}

static inline EventBits_t xEventGroupSetBits(EventGroupHandle_t h, EventBits_t bits)
{
    mock_event_group_t *eg = (mock_event_group_t *)h;
    if (!eg) return 0;
    pthread_mutex_lock(&eg->mutex);
    eg->bits |= bits;
    EventBits_t result = eg->bits;
    pthread_cond_broadcast(&eg->cond);
    pthread_mutex_unlock(&eg->mutex);
    return result;
}

static inline EventBits_t xEventGroupClearBits(EventGroupHandle_t h, EventBits_t bits)
{
    mock_event_group_t *eg = (mock_event_group_t *)h;
    if (!eg) return 0;
    pthread_mutex_lock(&eg->mutex);
    EventBits_t prev = eg->bits;
    eg->bits &= ~bits;
    pthread_mutex_unlock(&eg->mutex);
    return prev;
}

static inline EventBits_t xEventGroupWaitBits(
    EventGroupHandle_t h, EventBits_t bits,
    BaseType_t clear_on_exit, BaseType_t wait_all, TickType_t timeout)
{
    mock_event_group_t *eg = (mock_event_group_t *)h;
    if (!eg) return 0;
    pthread_mutex_lock(&eg->mutex);
    EventBits_t result = eg->bits;
    if (clear_on_exit) {
        eg->bits &= ~bits;
    }
    pthread_mutex_unlock(&eg->mutex);
    (void)wait_all; (void)timeout;
    return result;
}

/* ---- Mutex (POSIX) ---- */
static inline SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *buf)
{
    (void)buf;
    pthread_mutex_t *m = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    if (!m) return NULL;
    pthread_mutex_init(m, NULL);
    return (SemaphoreHandle_t)m;
}

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    pthread_mutex_t *m = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    if (!m) return NULL;
    pthread_mutex_init(m, NULL);
    return (SemaphoreHandle_t)m;
}

static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t h, TickType_t timeout)
{
    (void)timeout;
    pthread_mutex_t *m = (pthread_mutex_t *)h;
    if (!m) return pdFALSE;
    return (pthread_mutex_lock(m) == 0) ? pdTRUE : pdFALSE;
}

static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t h)
{
    pthread_mutex_t *m = (pthread_mutex_t *)h;
    if (!m) return pdFALSE;
    return (pthread_mutex_unlock(m) == 0) ? pdTRUE : pdFALSE;
}

static inline void vSemaphoreDelete(SemaphoreHandle_t h)
{
    pthread_mutex_t *m = (pthread_mutex_t *)h;
    if (!m) return;
    pthread_mutex_destroy(m);
    free(m);
}

/* ---- esp_timer mock ---- */
static inline int64_t esp_timer_get_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)(ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL);
}

/* ---- esp_err_t mock ---- */
typedef int esp_err_t;
#define ESP_OK          0
#define ESP_FAIL        -1
#define ESP_ERR_NO_MEM  0x101
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_NOT_FOUND   0x103

static inline const char *esp_err_to_name(esp_err_t err) {
    (void)err;
    return "ESP_OK";
}

#endif /* _FREERTOS_MOCK_H_ */

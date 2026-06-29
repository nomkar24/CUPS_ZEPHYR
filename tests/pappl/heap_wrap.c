#include <zephyr/kernel.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

static K_MUTEX_DEFINE(smh_wrap_mutex);

extern void *__real_shared_multi_heap_alloc(enum shared_multi_heap_attr attr, size_t bytes);
extern void *__real_shared_multi_heap_aligned_alloc(enum shared_multi_heap_attr attr, size_t align, size_t bytes);
extern void *__real_shared_multi_heap_realloc(enum shared_multi_heap_attr attr, void *ptr, size_t bytes);
extern void __real_shared_multi_heap_free(void *block);

void *__wrap_shared_multi_heap_alloc(enum shared_multi_heap_attr attr, size_t bytes)
{
  void *ptr;
  k_mutex_lock(&smh_wrap_mutex, K_FOREVER);
  ptr = __real_shared_multi_heap_alloc(attr, bytes);
  k_mutex_unlock(&smh_wrap_mutex);
  return ptr;
}

void *__wrap_shared_multi_heap_aligned_alloc(enum shared_multi_heap_attr attr, size_t align, size_t bytes)
{
  void *ptr;
  k_mutex_lock(&smh_wrap_mutex, K_FOREVER);
  ptr = __real_shared_multi_heap_aligned_alloc(attr, align, bytes);
  k_mutex_unlock(&smh_wrap_mutex);
  return ptr;
}

void *__wrap_shared_multi_heap_realloc(enum shared_multi_heap_attr attr, void *ptr, size_t bytes)
{
  void *new_ptr;
  k_mutex_lock(&smh_wrap_mutex, K_FOREVER);
  new_ptr = __real_shared_multi_heap_realloc(attr, ptr, bytes);
  k_mutex_unlock(&smh_wrap_mutex);
  return new_ptr;
}

void __wrap_shared_multi_heap_free(void *block)
{
  k_mutex_lock(&smh_wrap_mutex, K_FOREVER);
  __real_shared_multi_heap_free(block);
  k_mutex_unlock(&smh_wrap_mutex);
}

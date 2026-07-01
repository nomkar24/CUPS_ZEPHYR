#include <zephyr/kernel.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/spinlock.h>

static struct k_spinlock smh_wrap_lock;

extern void *__real_shared_multi_heap_alloc(enum shared_multi_heap_attr attr, size_t bytes);
extern void *__real_shared_multi_heap_aligned_alloc(enum shared_multi_heap_attr attr, size_t align, size_t bytes);
extern void *__real_shared_multi_heap_realloc(enum shared_multi_heap_attr attr, void *ptr, size_t bytes);
extern void __real_shared_multi_heap_free(void *block);

void *__wrap_shared_multi_heap_alloc(enum shared_multi_heap_attr attr, size_t bytes)
{
  void *ptr;
  k_spinlock_key_t key = k_spin_lock(&smh_wrap_lock);
  ptr = __real_shared_multi_heap_alloc(attr, bytes);
  k_spin_unlock(&smh_wrap_lock, key);
  return ptr;
}

void *__wrap_shared_multi_heap_aligned_alloc(enum shared_multi_heap_attr attr, size_t align, size_t bytes)
{
  void *ptr;
  k_spinlock_key_t key = k_spin_lock(&smh_wrap_lock);
  ptr = __real_shared_multi_heap_aligned_alloc(attr, align, bytes);
  k_spin_unlock(&smh_wrap_lock, key);
  return ptr;
}

void *__wrap_shared_multi_heap_realloc(enum shared_multi_heap_attr attr, void *ptr, size_t bytes)
{
  void *new_ptr;
  k_spinlock_key_t key = k_spin_lock(&smh_wrap_lock);
  new_ptr = __real_shared_multi_heap_realloc(attr, ptr, bytes);
  k_spin_unlock(&smh_wrap_lock, key);
  return new_ptr;
}

void __wrap_shared_multi_heap_free(void *block)
{
  k_spinlock_key_t key = k_spin_lock(&smh_wrap_lock);
  __real_shared_multi_heap_free(block);
  k_spin_unlock(&smh_wrap_lock, key);
}

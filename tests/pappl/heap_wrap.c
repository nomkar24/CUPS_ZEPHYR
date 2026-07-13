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

// Override ROM string functions to prevent LoadStoreError crashes when accessing PSRAM on ESP32-S3.
// The ESP32-S3 ROM libc implementations use optimized assembly that can cause alignment/bus errors on SPIRAM.
int __wrap_strcmp(const char *s1, const char *s2)
{
  while (*s1 && (*s1 == *s2))
  {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

size_t __wrap_strlen(const char *s)
{
  const char *p = s;
  while (*p)
    p++;
  return p - s;
}

char *__wrap_strcpy(char *dest, const char *src)
{
  char *d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

int __wrap_strncmp(const char *s1, const char *s2, size_t n)
{
  if (n == 0)
    return 0;
  while (n-- > 0 && *s1 == *s2)
  {
    if (n == 0 || *s1 == '\0')
      break;
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *__wrap_strchr(const char *s, int c)
{
  while (*s != (char)c)
  {
    if (!*s)
      return NULL;
    s++;
  }
  return (char *)s;
}


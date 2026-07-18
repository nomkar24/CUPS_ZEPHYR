#include <zephyr/kernel.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <stdio.h>
#include <string.h>

static K_MUTEX_DEFINE(smh_wrap_mutex);

extern void *__real_shared_multi_heap_alloc(enum shared_multi_heap_attr attr, size_t bytes);
extern void *__real_shared_multi_heap_aligned_alloc(enum shared_multi_heap_attr attr, size_t align, size_t bytes);
extern void *__real_shared_multi_heap_realloc(enum shared_multi_heap_attr attr, void *ptr, size_t bytes);
extern void __real_shared_multi_heap_free(void *block);

typedef struct
{
  void *orig_ptr;
  size_t size;
  uint32_t magic;
  uint32_t padding;
} smh_header_t;

#define SMH_MAGIC_NORMAL  0xDEADBEEF
#define SMH_MAGIC_ALIGNED 0xBAADF00D

void __wrap_shared_multi_heap_free(void *block);

void *__wrap_shared_multi_heap_alloc(enum shared_multi_heap_attr attr, size_t bytes)
{
  k_mutex_lock(&smh_wrap_mutex, K_FOREVER);
  void *orig_ptr = __real_shared_multi_heap_alloc(attr, bytes + sizeof(smh_header_t));
  if (!orig_ptr)
  {
    k_mutex_unlock(&smh_wrap_mutex);
    return NULL;
  }

  smh_header_t *header = (smh_header_t *)orig_ptr;
  header->orig_ptr = orig_ptr;
  header->size = bytes;
  header->magic = SMH_MAGIC_NORMAL;

  void *user_ptr = (char *)orig_ptr + sizeof(smh_header_t);
  k_mutex_unlock(&smh_wrap_mutex);
  return user_ptr;
}

void *__wrap_shared_multi_heap_aligned_alloc(enum shared_multi_heap_attr attr, size_t align, size_t bytes)
{
  k_mutex_lock(&smh_wrap_mutex, K_FOREVER);
  void *orig_ptr = __real_shared_multi_heap_alloc(attr, bytes + align + sizeof(smh_header_t));
  if (!orig_ptr)
  {
    k_mutex_unlock(&smh_wrap_mutex);
    return NULL;
  }

  uintptr_t raw_addr = (uintptr_t)orig_ptr + sizeof(smh_header_t);
  uintptr_t aligned_addr = (raw_addr + align - 1) & ~(align - 1);
  void *user_ptr = (void *)aligned_addr;

  smh_header_t *header = (smh_header_t *)((char *)user_ptr - sizeof(smh_header_t));
  header->orig_ptr = orig_ptr;
  header->size = bytes;
  header->magic = SMH_MAGIC_ALIGNED;

  k_mutex_unlock(&smh_wrap_mutex);
  return user_ptr;
}

void *__wrap_shared_multi_heap_realloc(enum shared_multi_heap_attr attr, void *ptr, size_t bytes)
{
  if (bytes == 0)
  {
    __wrap_shared_multi_heap_free(ptr);
    return NULL;
  }
  if (!ptr)
  {
    return __wrap_shared_multi_heap_alloc(attr, bytes);
  }

  k_mutex_lock(&smh_wrap_mutex, K_FOREVER);
  smh_header_t *header = (smh_header_t *)((char *)ptr - sizeof(smh_header_t));
  if (header->magic != SMH_MAGIC_NORMAL && header->magic != SMH_MAGIC_ALIGNED)
  {
    printf("[heap_wrap] CRITICAL ERROR: realloc of invalid/corrupted pointer %p (magic=%08x)\n", ptr, header->magic);
    fflush(stdout);
    k_mutex_unlock(&smh_wrap_mutex);
    return NULL;
  }

  if (header->size >= bytes)
  {
    header->size = bytes;
    k_mutex_unlock(&smh_wrap_mutex);
    return ptr;
  }
  k_mutex_unlock(&smh_wrap_mutex);

  void *new_ptr = __wrap_shared_multi_heap_alloc(attr, bytes);
  if (!new_ptr)
    return NULL;

  memcpy(new_ptr, ptr, header->size);
  __wrap_shared_multi_heap_free(ptr);
  return new_ptr;
}

void __wrap_shared_multi_heap_free(void *block)
{
  if (!block)
    return;

  k_mutex_lock(&smh_wrap_mutex, K_FOREVER);
  smh_header_t *header = (smh_header_t *)((char *)block - sizeof(smh_header_t));
  if (header->magic != SMH_MAGIC_NORMAL && header->magic != SMH_MAGIC_ALIGNED)
  {
    printf("[heap_wrap] CRITICAL ERROR: free of invalid/corrupted pointer %p (magic=%08x)\n", block, header->magic);
    fflush(stdout);
    k_mutex_unlock(&smh_wrap_mutex);
    return;
  }

  header->magic = 0;
  __real_shared_multi_heap_free(header->orig_ptr);
  k_mutex_unlock(&smh_wrap_mutex);
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


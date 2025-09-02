#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cups/tests.h>
#include <cups/cups.h>
#include <cups/test-internal.h>
#include <math.h>
#include <pthread.h>

#define MAX_STACK_SIZE 1024
#define THREAD_PRIORITY 7

K_THREAD_STACK_DEFINE(thread_stack, MAX_STACK_SIZE);

#define _CUPS_MAXSAVE	32

void runTest(k_thread_entry_t entry)
{
    struct k_thread thread;
    k_tid_t id;
    id = k_thread_create(&thread, thread_stack, MAX_STACK_SIZE, entry, NULL, NULL, NULL, THREAD_PRIORITY, K_INHERIT_PERMS, K_NO_WAIT);
    k_thread_join(id, K_FOREVER);
}

/* Matches LFS_NAME_MAX */
#define MAX_PATH_LEN 255
#define TEST_FILE_SIZE 547

void testpthread_set_specific(void *p1, void *p2, void *p3)
{
	pthread_key_t key;
	void *value = malloc(69);
	int ret = 1;
	strcpy(value, "hello world");
	if (!value)
	{
		printf("could not alloc value");
		return;
	}
	pthread_key_create(&key, NULL);
	ret = pthread_setspecific(key, value);
	free(value);
	printf("returning %d", ret);
}
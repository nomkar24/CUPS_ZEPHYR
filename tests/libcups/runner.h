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
#include <cups/thread.h>

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

void *testpthread_timers(void *p1)
{
	ARG_UNUSED(p1);
	LOG_MODULE_DECLARE(MAIN);
	for (;;)
	{
		LOG_INF("testpthread_timers");
		k_sleep(K_MSEC(1000));
	}
	return NULL;
}

void *testpthread_nested_pthread(void *p1)
{
	ARG_UNUSED(p1);
	cups_thread_t t = cupsThreadCreate(testpthread_timers, NULL);
	cupsThreadWait(t);
	return NULL;
}
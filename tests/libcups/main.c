#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/dns_sd.h>
#include <zephyr/posix/netinet/in.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "wifi_config.h"
#include "net_sample_common.h"
#include "ippeveprinter.h"
#include "runner.h"
#include "ipptest.h"
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <zephyr/posix/netinet/in.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_DBG);

static K_SEM_DEFINE(wifi_connected, 0, 1);
static K_SEM_DEFINE(ipv4_address_obtained, 0, 1);

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static int welcome(int fd)
{
	static const char msg[] = "Bonjour, Zephyr world!\n";

	return send(fd, msg, sizeof(msg), 0);
}

/**
 * @brief Pass callback in to handle status upon connect
 * @param cb: Callback structure
 *
 * Reboots if failed, else gives wifi_connected semaphore
 */
static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_INF("Connection request failed (%d)\n", status->status);
		sys_reboot(SYS_REBOOT_WARM);
	} else {
		LOG_INF("Connected\n");
		k_sem_give(&wifi_connected);
	}
}

/**
 * @brief Pass callback in to handle status upon disconnect
 * @param cb: Callback structure
 *
 * Always reboots
 */
static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_INF("Disconnection request (%d)\n", status->status);
		sys_reboot(SYS_REBOOT_WARM);
	} else {
		LOG_INF("Disconnected\n");
		sys_reboot(SYS_REBOOT_WARM);
	}
}

/**
 * @brief Checks if IPV4 address has been obtained from DHCP
 * @param iface: Network interface structure
 *
 * Prints obtained address(es), the subnet mask, and router address
 * Also gives the ipv4_address_obtained semaphore
 */
static void handle_ipv4_result(struct net_if *iface)
{
	int i = 0;

	for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {

		char buf[NET_IPV4_ADDR_LEN];

		if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type != NET_ADDR_DHCP) {
			continue;
		}

		LOG_INF("IPv4 address: %s\n",
		       net_addr_ntop(AF_INET,
				     &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr, buf,
				     sizeof(buf)));
		LOG_INF("Subnet: %s\n",
		       net_addr_ntop(AF_INET, &iface->config.ip.ipv4->unicast[i].netmask, buf,
				     sizeof(buf)));
		LOG_INF("Router: %s\n",
		       net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw, buf, sizeof(buf)));
	}

	k_sem_give(&ipv4_address_obtained);
}

/**
 * @brief Logic function that calls the appropriate handling function
 *
 * @param cb: Callback Structure
 * @param mgmt_event: Unsigned integer code for management event
 * @param iface: Network Interface Structure
 */
static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, long long unsigned int mgmt_event,
				    struct net_if *iface)
{
	switch (mgmt_event) {

	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;

	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;

	case NET_EVENT_IPV4_ADDR_ADD:
		handle_ipv4_result(iface);
		break;

	default:
		break;
	}
}

/**
 * @brief Attempts to connect to Wi-FI based on wifi_settings.h
 *
 * Set to only work with open networks (no security)
 */
void wifi_connect(void)
{
	struct net_if *iface = net_if_get_default();

	struct wifi_connect_req_params wifi_params = {0};

	wifi_params.ssid = SSID;
	wifi_params.ssid_length = strlen(SSID);
	wifi_params.channel = WIFI_CHANNEL_ANY;
	wifi_params.security = WIFI_SECURITY_TYPE_NONE;
/*     wifi_params.psk = PSK;
    wifi_params.psk_length = strlen(PSK); */
	wifi_params.band = WIFI_FREQ_BAND_2_4_GHZ;
	wifi_params.mfp = WIFI_MFP_OPTIONAL;

	LOG_INF("Connecting to SSID: %s\n", wifi_params.ssid);

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params,
		     sizeof(struct wifi_connect_req_params))) {
		LOG_INF("WiFi Connection Request Failed\n");
		sys_reboot(SYS_REBOOT_WARM);
	} else {
		LOG_INF("Connected successfully\n");
	}
}

/**
 * @brief Get the current network status
 *
 */
void wifi_status(void)
{
	struct net_if *iface = net_if_get_default();

	struct wifi_iface_status status = {0};

	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
		     sizeof(struct wifi_iface_status))) {
		LOG_INF("WiFi Status Request Failed\n");
	}

	if (status.state >= WIFI_STATE_ASSOCIATED) {
		LOG_INF("SSID: %-32s\n", status.ssid);
		LOG_INF("Band: %s\n", wifi_band_txt(status.band));
		LOG_INF("Channel: %d\n", status.channel);
		LOG_INF("Security: %s\n", wifi_security_txt(status.security));
		LOG_INF("RSSI: %d\n", status.rssi);
	}
}

/**
 * @brief Attempt to disconnect from Wi-Fi
 *
 */
void wifi_disconnect(void)
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0)) {
		LOG_INF("WiFi Disconnection Request Failed\n");
	}
}

void service(void)
{
	int r;
	int server_fd;
	int client_fd;
	socklen_t len;
	void *addrp;
	uint16_t *portp;
	struct sockaddr client_addr;
	char addrstr[INET6_ADDRSTRLEN];
	uint8_t line[64];

	static struct sockaddr server_addr;

#if DEFAULT_PORT == 0
	/* The advanced use case: ephemeral port */
#if defined(CONFIG_NET_IPV6)
	DNS_SD_REGISTER_SERVICE(zephyr, CONFIG_NET_HOSTNAME,
				"_zephyr", "_tcp", "local", DNS_SD_EMPTY_TXT,
				&((struct sockaddr_in6 *)&server_addr)->sin6_port);
#elif defined(CONFIG_NET_IPV4)
	DNS_SD_REGISTER_SERVICE(zephyr, CONFIG_NET_HOSTNAME,
				"_zephyr", "_tcp", "local", DNS_SD_EMPTY_TXT,
				&((struct sockaddr_in *)&server_addr)->sin_port);
#endif
#else
	/* The simple use case: fixed port */
	DNS_SD_REGISTER_TCP_SERVICE(zephyr, "zephyr",
				    "_http", "local", DNS_SD_EMPTY_TXT, DEFAULT_PORT);
#endif

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_sin6(&server_addr)->sin6_family = AF_INET6;
		net_sin6(&server_addr)->sin6_addr = in6addr_any;
		net_sin6(&server_addr)->sin6_port = sys_cpu_to_be16(DEFAULT_PORT);
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_sin(&server_addr)->sin_family = AF_INET;
		net_sin(&server_addr)->sin_addr.s_addr = htonl(INADDR_ANY);
		net_sin(&server_addr)->sin_port = sys_cpu_to_be16(DEFAULT_PORT);
	} else {
		__ASSERT(false, "Neither IPv6 nor IPv4 are enabled");
	}

	r = socket(server_addr.sa_family, SOCK_STREAM, 0);
	if (r == -1) {
		NET_DBG("socket() failed (%d)", errno);
		return;
	}

	server_fd = r;
	NET_DBG("server_fd is %d", server_fd);

	r = bind(server_fd, &server_addr, sizeof(server_addr));
	if (r == -1) {
		NET_DBG("bind() failed (%d)", errno);
		close(server_fd);
		return;
	}

	if (server_addr.sa_family == AF_INET6) {
		addrp = &net_sin6(&server_addr)->sin6_addr;
		portp = &net_sin6(&server_addr)->sin6_port;
	} else {
		addrp = &net_sin(&server_addr)->sin_addr;
		portp = &net_sin(&server_addr)->sin_port;
	}

	inet_ntop(server_addr.sa_family, addrp, addrstr, sizeof(addrstr));
	NET_DBG("bound to [%s]:%u",
		addrstr, ntohs(*portp));

	r = listen(server_fd, 1);
	if (r == -1) {
		NET_DBG("listen() failed (%d)", errno);
		close(server_fd);
		return;
	}

	
}

#if 0
void service(void)
{
	int r;
	int server_fd;
	int client_fd;
	socklen_t len;
	void *addrp;
	uint16_t *portp;
	struct sockaddr client_addr;
	char addrstr[INET6_ADDRSTRLEN];
	uint8_t line[64];

	static struct sockaddr server_addr;

 	static const char txt_record[] = {
		"\x0c" "rp=ipp/print"
		"\x27" "ty=ExampleCorp LaserPrinter 4000 Series"
		/* "\x1c" "pdl=application/octet-stream"
		"\x07" "Color=F"
		"\x08" "Duplex=F"
		"\x29" "UUID=a85c6a0f-145b-4b0a-97e8-1e8416468b4c"
		"\x07" "TLS=1.3"
		"\x09" "txtvers=1"
		"\x08" "qtotal=1" */
	  };

#if DEFAULT_PORT == 0
	/* The advanced use case: ephemeral port */
#if defined(CONFIG_NET_IPV6)
	DNS_SD_REGISTER_SERVICE(zephyr, "zephyrr",
				"_ipp", "_tcp", "local", DNS_SD_EMPTY_TXT,
				&((struct sockaddr_in6 *)&server_addr)->sin6_port);
#elif defined(CONFIG_NET_IPV4)
	DNS_SD_REGISTER_SERVICE(zephyr, "zephyrr",
				"_ipp", "_tcp", "local", DNS_SD_EMPTY_TXT,
				&((struct sockaddr_in *)&server_addr)->sin_port);
#endif
#else
  // Register the _printer._tcp (LPD) service type with a port number of 0 to
  // defend our service name but not actually support LPD...
  CUPS_DNSSD_SERVICE_REGISTER(lpd_service, "My Example Printer", "_ipp", txt_record, 0);

  // Then register the _ipp._tcp (IPP) service type with the real port number to
  // advertise our IPP printer...
  CUPS_DNSSD_SERVICE_REGISTER(ipp_service, "My Example Printer IPP", "_ipp", txt_record, DEFAULT_PORT);

  // Then register the _ipps._tcp (IPP) service type with the real port number
  // to advertise our IPPS printer...
  CUPS_DNSSD_SERVICE_REGISTER(ipps_service, "My Example Printer IPPS", "_ipps", txt_record, DEFAULT_PORT);

  // Similarly, register the _http._tcp,_printer (HTTP) service type with the
  // real port number to advertise our IPP printer's web interface...
  CUPS_DNSSD_SERVICE_REGISTER(http_service, "My Example Printer", "_http", txt_record, DEFAULT_PORT);
#endif

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_sin6(&server_addr)->sin6_family = AF_INET6;
		net_sin6(&server_addr)->sin6_addr = in6addr_any;
		net_sin6(&server_addr)->sin6_port = sys_cpu_to_be16(DEFAULT_PORT);
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_sin(&server_addr)->sin_family = AF_INET;
		net_sin(&server_addr)->sin_addr.s_addr = htonl(INADDR_ANY);
		net_sin(&server_addr)->sin_port = sys_cpu_to_be16(DEFAULT_PORT);
	} else {
		__ASSERT(false, "Neither IPv6 nor IPv4 are enabled");
	}

	r = create_listener(NULL, DEFAULT_PORT, AF_INET);
	NET_DBG("r is %d", r);
	server_fd = r;
	NET_DBG("server_fd is %d", server_fd);
}
#endif
/* Matches LFS_NAME_MAX */
#define MAX_PATH_LEN 255
#define TEST_FILE_SIZE 547

static int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		LOG_ERR("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	LOG_PRINTK("\nListing dir %s ...\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			if (res < 0) {
				LOG_ERR("Error reading dir [%d]\n", res);
			}
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_PRINTK("[DIR ] %s\n", entry.name);
		} else {
			LOG_PRINTK("[FILE] %s (size = %zu)\n",
				   entry.name, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}

#ifdef CONFIG_APP_LITTLEFS_STORAGE_FLASH
static int littlefs_flash_erase(unsigned int id)
{
	const struct flash_area *pfa;
	int rc;

	rc = flash_area_open(id, &pfa);
	if (rc < 0) {
		LOG_ERR("FAIL: unable to find flash area %u: %d\n",
			id, rc);
		return rc;
	}

	LOG_PRINTK("Area %u at 0x%x on %s for %u bytes\n",
		   id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
		   (unsigned int)pfa->fa_size);

	/* Optional wipe flash contents */
	if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
		rc = flash_area_flatten(pfa, 0, pfa->fa_size);
		LOG_ERR("Erasing flash area ... %d", rc);
	}

	flash_area_close(pfa);
	return rc;
}
#define PARTITION_NODE DT_NODELABEL(lfs1)

#if DT_NODE_EXISTS(PARTITION_NODE)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#else /* PARTITION_NODE */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
	.mnt_point = "/lfs",
};
#endif /* PARTITION_NODE */

	struct fs_mount_t *mountpoint =
#if DT_NODE_EXISTS(PARTITION_NODE)
		&FS_FSTAB_ENTRY(PARTITION_NODE)
#else
		&lfs_storage_mnt
#endif
		;

static int littlefs_mount(struct fs_mount_t *mp)
{
	int rc;

    /*
	rc = littlefs_flash_erase((uintptr_t)mp->storage_dev);
	if (rc < 0) {
		return rc;
	}
    */

	/* Do not mount if auto-mount has been enabled */
#if !DT_NODE_EXISTS(PARTITION_NODE) ||						\
	!(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)
	rc = fs_mount(mp);
	if (rc < 0) {
		LOG_PRINTK("FAIL: mount id %" PRIuPTR " at %s: %d\n",
		       (uintptr_t)mp->storage_dev, mp->mnt_point, rc);
		return rc;
	}
	LOG_PRINTK("%s mount: %d\n", mp->mnt_point, rc);
#else
	LOG_PRINTK("%s automounted\n", mp->mnt_point);
#endif

	return 0;
}
#endif /* CONFIG_APP_LITTLEFS_STORAGE_FLASH */

static int load_file(const char *fname, const void *towrite, size_t len)
{
	struct fs_dirent dirent;
	struct fs_file_t file;
	int rc, ret;

	/*
	 * Uncomment below line to force re-creation of the test pattern
	 * file on the littlefs FS.
	 */
	fs_unlink(fname);
	fs_file_t_init(&file);

	rc = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
	if (rc < 0) {
		LOG_ERR("FAIL: open %s: %d", fname, rc);
		return rc;
	}

	rc = fs_stat(fname, &dirent);
	if (rc < 0) {
		LOG_ERR("FAIL: stat %s: %d", fname, rc);
		goto out;
	}

	/* Check if the file exists*/
	if (rc == 0 && dirent.type == FS_DIR_ENTRY_FILE && dirent.size == 0) {
		LOG_INF("Test file: %s not found, create one!",
			fname);
	} else {
		goto out;
	}

	rc = fs_seek(&file, 0, FS_SEEK_SET);
	if (rc < 0) {
		LOG_ERR("FAIL: seek %s: %d", fname, rc);
		goto out;
	}

	rc = fs_write(&file, towrite, len);
	if (rc < 0) {
		LOG_ERR("FAIL: write %s: %d", fname, rc);
	}

 out:
	ret = fs_close(&file);
	if (ret < 0) {
		LOG_ERR("FAIL: close %s: %d", fname, ret);
		return ret;
	}

	return (rc < 0 ? rc : 0);
}

int main()
{
	struct fs_statvfs sbuf;
	int rc;
	pthread_t top_th;
	int err;
	pthread_attr_t attr;
	void *pthread_stack = NULL;

	LOG_PRINTK("Initializing file system with littlefs\n");

	rc = littlefs_mount(mountpoint);
	if (rc < 0) {
		return 0;
	}

	rc = fs_statvfs(mountpoint->mnt_point, &sbuf);
	if (rc < 0) {
		LOG_PRINTK("FAIL: statvfs: %d\n", rc);
		goto out;
	}

	LOG_PRINTK("%s: bsize = %lu ; frsize = %lu ;"
		   " blocks = %lu ; bfree = %lu\n",
		   mountpoint->mnt_point,
		   sbuf.f_bsize, sbuf.f_frsize,
		   sbuf.f_blocks, sbuf.f_bfree);
	
    net_mgmt_init_event_callback(&wifi_cb, wifi_mgmt_event_handler,
                                NET_EVENT_WIFI_CONNECT_RESULT |
                                NET_EVENT_WIFI_DISCONNECT_RESULT);

	net_mgmt_init_event_callback(&ipv4_cb, wifi_mgmt_event_handler, NET_EVENT_IPV4_ADDR_ADD);

	net_mgmt_add_event_callback(&wifi_cb);
	net_mgmt_add_event_callback(&ipv4_cb);

	wifi_connect();
	k_sem_take(&wifi_connected, K_FOREVER);
	wifi_status();
	k_sem_take(&ipv4_address_obtained, K_FOREVER);
	printk("Ready...\n\n");
	
	LOG_INF("Waiting mDNS queries...");

	if (rc = load_file("/lfs/test.conf", testconfstr, sizeof(testconfstr)));
	{
		LOG_INF("Error with loading test conf to file: %d", rc);
	}

	if (rc = load_file("/lfs/testipp.test", ippteststr, sizeof(ippteststr)));
	{
		LOG_INF("Error with loading testipp.test file: %d", rc);
	}

	int r;
	int server_fd;
	int client_fd;
	socklen_t len;
	void *addrp;
	uint16_t *portp;
	struct sockaddr client_addr;
	char addrstr[INET6_ADDRSTRLEN];
	uint8_t line[64];

	static struct sockaddr server_addr;

#if DEFAULT_PORT == 0
	/* The advanced use case: ephemeral port */
#if defined(CONFIG_NET_IPV6)
	DNS_SD_REGISTER_SERVICE(zephyr, CONFIG_NET_HOSTNAME,
				"_zephyr", "_tcp", "local", DNS_SD_EMPTY_TXT,
				&((struct sockaddr_in6 *)&server_addr)->sin6_port);
#elif defined(CONFIG_NET_IPV4)
	DNS_SD_REGISTER_SERVICE(zephyr, CONFIG_NET_HOSTNAME,
				"_zephyr", "_tcp", "local", DNS_SD_EMPTY_TXT,
				&((struct sockaddr_in *)&server_addr)->sin_port);
#endif
#else
	/* The simple use case: fixed port */
	DNS_SD_REGISTER_TCP_SERVICE(zephyr, "zephyr",
				    "_http", "local", DNS_SD_EMPTY_TXT, DEFAULT_PORT);
#endif

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_sin6(&server_addr)->sin6_family = AF_INET6;
		net_sin6(&server_addr)->sin6_addr = in6addr_any;
		net_sin6(&server_addr)->sin6_port = sys_cpu_to_be16(DEFAULT_PORT);
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_sin(&server_addr)->sin_family = AF_INET;
		net_sin(&server_addr)->sin_addr.s_addr = htonl(INADDR_ANY);
		net_sin(&server_addr)->sin_port = sys_cpu_to_be16(DEFAULT_PORT);
	} else {
		__ASSERT(false, "Neither IPv6 nor IPv4 are enabled");
	}

	r = socket(server_addr.sa_family, SOCK_STREAM, 0);
	if (r == -1) {
		NET_DBG("socket() failed (%d)", errno);
		return;
	}

	server_fd = r;
	NET_DBG("server_fd is %d", server_fd);

	r = bind(server_fd, &server_addr, sizeof(server_addr));
	if (r == -1) {
		NET_DBG("bind() failed (%d)", errno);
		close(server_fd);
		return;
	}

	if (server_addr.sa_family == AF_INET6) {
		addrp = &net_sin6(&server_addr)->sin6_addr;
		portp = &net_sin6(&server_addr)->sin6_port;
	} else {
		addrp = &net_sin(&server_addr)->sin_addr;
		portp = &net_sin(&server_addr)->sin_port;
	}

	inet_ntop(server_addr.sa_family, addrp, addrstr, sizeof(addrstr));
	NET_DBG("bound to [%s]:%u",
		addrstr, ntohs(*portp));

	r = listen(server_fd, 1);
	if (r == -1) {
		NET_DBG("listen() failed (%d)", errno);
		close(server_fd);
		return;
	}

	pthread_attr_init(&attr);
	if ((pthread_stack = shared_multi_heap_alloc(SMH_REG_ATTR_EXTERNAL, 65536)) == NULL)
		goto out;
	pthread_attr_setstack(&attr, pthread_stack, 65536);
	pthread_attr_setinsmh(&attr);
	err = pthread_create(&top_th, &attr, testarray_main, NULL);
	pthread_detach(top_th);
	LOG_INF("create pthread error: %d", err);
	for (;;) {
		len = sizeof(client_addr);
		r = accept(server_fd, (struct sockaddr *)&client_addr, &len);
		if (r == -1) {
			NET_DBG("accept() failed (%d)", errno);
			continue;
		}

		client_fd = r;

		inet_ntop(server_addr.sa_family, addrp, addrstr, sizeof(addrstr));
		NET_DBG("accepted connection from [%s]:%u",
			addrstr, ntohs(*portp));

		/* send a banner */
		r = welcome(client_fd);
		if (r == -1) {
			NET_DBG("send() failed (%d)", errno);
			close(client_fd);
			return;
		}

		// for (;;) {
		// 	/* echo 1 line at a time */
		// 	r = recv(client_fd, line, sizeof(line), 0);
		// 	if (r == -1) {
		// 		NET_DBG("recv() failed (%d)", errno);
		// 		close(client_fd);
		// 		break;
		// 	}
		// 	len = r;

		// 	r = send(client_fd, line, len, 0);
		// 	if (r == -1) {
		// 		NET_DBG("send() failed (%d)", errno);
		// 		close(client_fd);
		// 		break;
		// 	}
		// }
	}
	out:
	rc = fs_unmount(mountpoint);
	LOG_PRINTK("%s unmount: %d\n", mountpoint->mnt_point, rc);
    return 0;
}
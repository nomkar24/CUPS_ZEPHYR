#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zlib.h>

LOG_MODULE_REGISTER(zlib_test, LOG_LEVEL_INF);

int main(void) {
  const char *test_string = "zlib compression test";

  uLong source_len = strlen(test_string) + 1;
  uLong dest_len = compressBound(source_len);
  unsigned char *dest_buffer = k_malloc(dest_len);
  unsigned char *uncomp_buffer = k_malloc(source_len);

  if (!dest_buffer || !uncomp_buffer) {
    LOG_ERR("Failed to allocate memory for zlib test!");
    k_free(dest_buffer);
    k_free(uncomp_buffer);
    return 1;
  }

  LOG_INF("Starting zlib test...");
  LOG_INF("Source String: %s", test_string);
  LOG_INF("Source Length: %lu", source_len);

  // 1. Compress
  int res =
      compress(dest_buffer, &dest_len, (const Bytef *)test_string, source_len);
  if (res != Z_OK) {
    LOG_ERR("Compression failed with error code: %d", res);
    goto cleanup;
  }
  LOG_INF("Compressed Length: %lu", dest_len);

  // 2. Decompress
  uLong uncomp_len = source_len;
  res = uncompress(uncomp_buffer, &uncomp_len, dest_buffer, dest_len);
  if (res != Z_OK) {
    LOG_ERR("Decompression failed with error code: %d", res);
    goto cleanup;
  }
  LOG_INF("Decompressed Length: %lu", uncomp_len);
  LOG_INF("Decompressed String: %s", uncomp_buffer);

  // 3. Verify
  if (strcmp(test_string, (char *)uncomp_buffer) == 0) {
    LOG_INF("zlib Round-trip: SUCCESS");
  } else {
    LOG_ERR("zlib Round-trip: FAILURE (Mismatch)");
  }

cleanup:
  k_free(dest_buffer);
  k_free(uncomp_buffer);

  return 0;
}

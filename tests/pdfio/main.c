#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <pdfio.h>

LOG_MODULE_REGISTER(pdfio_test, LOG_LEVEL_INF);

// Simple output callback to verify writing
static ssize_t output_cb(void *ctx, const void *data, size_t datalen) {
    (void)ctx;
    // In a real scenario, this would write to a buffer or file
    return (ssize_t)datalen;
}

static bool error_cb(pdfio_file_t *pdf, const char *message, void *data) {
    (void)pdf;
    (void)data;
    LOG_ERR("PDFio Error: %s", message);
    return true;
}

int main(void) {
    LOG_INF("Starting pdfio test...");
    LOG_INF("PDFio Version: %s", PDFIO_VERSION);

    pdfio_rect_t media_box = {0.0, 0.0, 612.0, 792.0}; // US Letter
    // Use pdfioFileCreateOutput to test the library without needing a filesystem initially
    pdfio_file_t *pdf = pdfioFileCreateOutput(output_cb, NULL, "1.4", &media_box, NULL, error_cb, NULL);

    if (!pdf) {
        LOG_ERR("Failed to create PDF!");
        return 1;
    }

    LOG_INF("Created PDF file object.");

    pdfio_rect_t page_media_box = {0.0, 0.0, 612.0, 792.0};
    pdfio_dict_t *dict = pdfioDictCreate(pdf);
    pdfioDictSetRect(dict, "MediaBox", &page_media_box);

    pdfio_stream_t *st = pdfioFileCreatePage(pdf, dict);
    if (!st) {
        LOG_ERR("Failed to create page!");
        pdfioFileClose(pdf);
        return 1;
    }

    LOG_INF("Created PDF page.");

    // Simple PDF content: Move to 100, 100 and show text (Note: Font would need to be defined for real display)
    pdfioStreamPuts(st, "BT /F1 24 Tf 100 100 Td (Hello from Zephyr!) Tj ET");
    pdfioStreamClose(st);

    if (pdfioFileClose(pdf)) {
        LOG_INF("pdfio Round-trip (Creation): SUCCESS");
    } else {
        LOG_ERR("pdfio File Close failed!");
        return 1;
    }

    return 0;
}

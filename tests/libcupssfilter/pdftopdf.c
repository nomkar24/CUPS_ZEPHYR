#include <pdfio.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

static int Verbosity = 0; // for debugging purpose

#define XFORM_TEXT_SIZE 10.0   // Point size of plain text output
#define XFORM_TEXT_HEIGHT 12.0 // Point height of plain text output
#define XFORM_TEXT_WIDTH 0.6   // Width of monospaced characters

// pdfio_start_page() - start a page , applies transform on the back side of
// pages (used for back to back print it rotates the page by 180 degrees i.e
// duplex form)
static pdfio_stream_t *              // O - Page content stream
pdfio_start_page(xform_prepare_t *p, // I - Preparation data
                 pdfio_dict_t *dict) // I - Page dictionary
{
  pdfio_stream_t *st; // Page content stream

  if ((st = pdfioFileCreatePage(p->pdf, dict)) != NULL) {
    if (p->use_duplex_xform && !(pdfioFileGetNumPages(p->pdf) & 1)) {
      pdfioContentSave(st);
      pdfioContentMatrixConcat(st, p->duplex_xform);
    }
  }

  return (st);
}

//
// 'pdfio_end_page()' - End a page.
//
// This function restores graphics state when ending a back side page.
//

static void pdfio_end_page(xform_prepare_t *p, // I - Preparation data
                           pdfio_stream_t *st) // I - Page content stream
{
  if (p->use_duplex_xform && !(pdfioFileGetNumPages(p->pdf) & 1))
    pdfioContentRestore(st);

  pdfioStreamClose(st);
}

//
// 'generate_job_error_sheet()' - Generate a job error sheet.
//

static bool // O - `true` on success, `false` on failure
generate_job_error_sheet(xform_prepare_t *p) // I - Preparation data
{
  pdfio_stream_t *st;   // Page stream
  pdfio_obj_t *courier; // Courier font
  pdfio_dict_t *dict;   // Page dictionary
  size_t i,             // Looping var
      count;            // Number of pages
  const char *msg;      // Current message
  size_t mcount;        // Number of messages

  // Create a page dictionary with the Courier font...
  courier = pdfioFileCreateFontObjFromBase(p->pdf, "Courier");
  dict = pdfioDictCreate(p->pdf);

  pdfioPageDictAddFont(dict, "F1", courier);

  // Figure out how many impressions to produce...
  if (!strcmp(p->options->sides, "one-sided"))
    count = 1;
  else
    count = 2;

  // Create pages...
  for (i = 0; i < count; i++) {
    // Create the error sheet...
    st = pdfio_start_page(p, dict);

    // The job error sheet is a banner with the following information:
    //
    //   Errors:
    //     ...
    //
    //   Warnings:
    //     ...

    pdfioContentSetFillColorDeviceGray(st, 0.0);
    pdfioContentTextBegin(st);
    pdfioContentTextMoveTo(st, p->crop.x1, p->crop.y2 - 2.0 * XFORM_TEXT_SIZE);
    pdfioContentSetTextFont(st, "F1", 2.0 * XFORM_TEXT_SIZE);
    pdfioContentSetTextLeading(st, 2.0 * XFORM_TEXT_HEIGHT);
    pdfioContentTextShow(st, false, "Errors:\n");

    pdfioContentSetTextFont(st, "F1", XFORM_TEXT_SIZE);
    pdfioContentSetTextLeading(st, XFORM_TEXT_HEIGHT);

    for (msg = (const char *)cupsArrayGetFirst(p->errors), mcount = 0; msg;
         msg = (const char *)cupsArrayGetNext(p->errors)) {
      if (*msg == 'E') {
        pdfioContentTextShowf(st, false, "  %s\n", msg + 1);
        mcount++;
      }
    }

    if (mcount == 0)
      pdfioContentTextShow(st, false, "  No Errors\n");

    pdfioContentSetTextFont(st, "F1", 2.0 * XFORM_TEXT_SIZE);
    pdfioContentSetTextLeading(st, 2.0 * XFORM_TEXT_HEIGHT);
    pdfioContentTextShow(st, false, "\n");
    pdfioContentTextShow(st, false, "Warnings:\n");

    pdfioContentSetTextFont(st, "F1", XFORM_TEXT_SIZE);
    pdfioContentSetTextLeading(st, XFORM_TEXT_HEIGHT);

    for (msg = (const char *)cupsArrayGetFirst(p->errors), mcount = 0; msg;
         msg = (const char *)cupsArrayGetNext(p->errors)) {
      if (*msg == 'I') {
        pdfioContentTextShowf(st, false, "  %s\n", msg + 1);
        mcount++;
      }
    }

    if (mcount == 0)
      pdfioContentTextShow(st, false, "  No Warnings\n");

    pdfioContentTextEnd(st);
    pdfio_end_page(p, st);
  }

  return (true);
}

//
// 'generate_job_sheets()' - Generate a job banner sheet.
//

static bool // O - `true` on success, `false` on failure
generate_job_sheets(xform_prepare_t *p) // I - Preparation data
{
  pdfio_stream_t *st;   // Page stream
  pdfio_obj_t *courier; // Courier font
  pdfio_dict_t *dict;   // Page dictionary
  size_t i,             // Looping var
      count;            // Number of pages

  // Create a page dictionary with the Courier font...
  courier = pdfioFileCreateFontObjFromBase(p->pdf, "Courier");
  dict = pdfioDictCreate(p->pdf);

  pdfioPageDictAddFont(dict, "F1", courier);

  // Figure out how many impressions to produce...
  if (!strcmp(p->options->sides, "one-sided"))
    count = 1;
  else
    count = 2;

  // Create pages...
  for (i = 0; i < count; i++) {
    st = pdfio_start_page(p, dict);

    // The job sheet is a banner with the following information:
    //
    //     Title: job-title
    //      User: job-originating-user-name
    //     Pages: job-media-sheets
    //   Message: job-sheet-message

    pdfioContentTextBegin(st);
    pdfioContentSetTextFont(st, "F1", 2.0 * XFORM_TEXT_SIZE);
    pdfioContentSetTextLeading(st, 2.0 * XFORM_TEXT_HEIGHT);
    pdfioContentTextMoveTo(st, p->media.x2 / 8.0,
                           p->media.y2 / 2.0 +
                               2 * (XFORM_TEXT_HEIGHT + XFORM_TEXT_SIZE));
    pdfioContentSetFillColorDeviceGray(st, 0.0);

    pdfioContentTextShowf(st, false, "  Title: %s\n", p->options->job_name);
    pdfioContentTextShowf(st, false, "   User: %s\n",
                          p->options->job_originating_user_name);
    pdfioContentTextShowf(st, false, "  Pages: %u\n",
                          (unsigned)(p->num_outpages / count));
    if (p->options->job_sheet_message[0])
      pdfioContentTextShowf(st, false, "Message: %s\n",
                            p->options->job_sheet_message);

    pdfioContentTextEnd(st);
    pdfio_end_page(p, st);
  }

  return (true);
}

//
// 'media_to_rect()' - Convert `cups_media_t` to `pdfio_rect_t` for media and
// crop boxes. unit converter for bridging the gap between CUPS and PDF
// coordinates.

static void
media_to_rect(cups_media_t *size,  // I - CUPS media (size) information
              pdfio_rect_t *media, // O - PDF MediaBox value
              pdfio_rect_t *crop)  // O - PDF CropBox value
{
  // cups_media_t uses hundredths of millimeters, pdf_rect_t uses points...
  media->x1 = 0.0;
  media->y1 = 0.0;
  media->x2 = 72.0 * size->width / 2540.0;
  media->y2 = 72.0 * size->length / 2540.0;

  crop->x1 = 72.0 * size->left / 2540.0;
  crop->y1 = 72.0 * size->bottom / 2540.0;
  crop->x2 = 72.0 * (size->width - size->right) / 2540.0;
  crop->y2 = 72.0 * (size->length - size->top) / 2540.0;
}

//
// 'prepare_log()' - Log an informational or error message while preparing
//                   documents for printing.
//

static void prepare_log(xform_prepare_t *p, // I - Preparation data
                        bool error, // I - `true` for error, `false` for info
                        const char *message, // I - Printf-style message string
                        ...)                 // I - Addition arguments as needed
{
  va_list ap;        // Argument pointer
  char buffer[1024]; // Output buffer

  va_start(ap, message);
  vsnprintf(buffer + 1, sizeof(buffer) - 1, message, ap);
  va_end(ap);

  buffer[0] = error ? 'E' : 'I';

  cupsArrayAdd(p->errors, buffer);

  /*
  if (error)
    fprintf(stderr, _("%s: %s"), Prefix, buffer + 1);
  else
    fprintf(stderr, "INFO: %s\n", buffer + 1);
    */
}

//
// 'pdfio_error_cb()' - Log an error from the PDFio library.
//

static bool                         // O - `false` to stop
pdfio_error_cb(pdfio_file_t *pdf,   // I - PDF file (unused)
               const char *message, // I - Error message
               void *cb_data)       // I - Preparation data
{
  xform_prepare_t *p = (xform_prepare_t *)cb_data;
  // Preparation data

  if (pdf != p->pdf)
    prepare_log(p, true, "Input Document %d: %s", p->document, message);
  else
    prepare_log(p, true, "Output Document: %s", message);

  return (false);
}

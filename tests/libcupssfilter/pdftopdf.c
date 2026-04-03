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
//
// 'resource_dict_cb()' - Merge resource dictionaries from multiple input pages.
//
// This function detects resource conflicts and maps conflicting names as
// needed.
//

static bool
resource_dict_cb(
    pdfio_dict_t *dict,            // I - Dictionary
    const char   *key,             // I - Dictionary key
    xform_page_t *outpage)         // I - Output page
{
  pdfio_array_t *arrayval;         // Array value
  pdfio_dict_t  *dictval;          // Dictionary value
  const char    *nameval,          // Name value
                *curname;          // Current name value
  pdfio_obj_t   *objval;           // Object value
  char          mapname[256];      // Mapped resource name


  fprintf(stderr, "DEBUG: resource_dict_cb(dict=%p, key=\"%s\", outpage=%p)\n", (void *)dict, key, (void *)outpage);

  snprintf(mapname, sizeof(mapname), "%c%s", (int)('a' + outpage->layout), key);

  switch (pdfioDictGetType(dict, key))
  {
    case PDFIO_VALTYPE_ARRAY : // Array
        arrayval = pdfioDictGetArray(dict, key);
        if (pdfioDictGetArray(outpage->restype, key))
        {
	  if (!outpage->resmap[outpage->layout])
	    outpage->resmap[outpage->layout] = pdfioDictCreate(outpage->pdf);

	  pdfioDictSetName(outpage->resmap[outpage->layout], pdfioStringCreate(outpage->pdf, key), pdfioStringCreate(outpage->pdf, mapname));
	  key = mapname;
	}

        pdfioDictSetArray(outpage->restype, pdfioStringCreate(outpage->pdf, key), pdfioArrayCopy(outpage->pdf, arrayval));
        break;

    case PDFIO_VALTYPE_DICT : // Dictionary
        dictval = pdfioDictGetDict(dict, key);
        if (pdfioDictGetDict(outpage->restype, key))
        {
	  if (!outpage->resmap[outpage->layout])
	    outpage->resmap[outpage->layout] = pdfioDictCreate(outpage->pdf);

	  pdfioDictSetName(outpage->resmap[outpage->layout], pdfioStringCreate(outpage->pdf, key), pdfioStringCreate(outpage->pdf, mapname));
	  key = mapname;
	}

        pdfioDictSetDict(outpage->restype, pdfioStringCreate(outpage->pdf, key), pdfioDictCopy(outpage->pdf, dictval));
        break;

    case PDFIO_VALTYPE_NAME : // Name
        nameval = pdfioDictGetName(dict, key);
        if ((curname = pdfioDictGetName(outpage->restype, key)) != NULL)
        {
          if (!strcmp(nameval, curname))
            break;

	  if (!outpage->resmap[outpage->layout])
	    outpage->resmap[outpage->layout] = pdfioDictCreate(outpage->pdf);

	  pdfioDictSetName(outpage->resmap[outpage->layout], pdfioStringCreate(outpage->pdf, key), pdfioStringCreate(outpage->pdf, mapname));
	  key = mapname;
	}

        pdfioDictSetName(outpage->restype, pdfioStringCreate(outpage->pdf, key), pdfioStringCreate(outpage->pdf, nameval));
        break;

    case PDFIO_VALTYPE_INDIRECT : // Object reference
        objval = pdfioDictGetObj(dict, key);
        if (pdfioDictGetObj(outpage->restype, key))
        {
	  if (!outpage->resmap[outpage->layout])
	    outpage->resmap[outpage->layout] = pdfioDictCreate(outpage->pdf);

	  pdfioDictSetName(outpage->resmap[outpage->layout], pdfioStringCreate(outpage->pdf, key), pdfioStringCreate(outpage->pdf, mapname));
	  key = mapname;
	}

        pdfioDictSetObj(outpage->restype, pdfioStringCreate(outpage->pdf, key), pdfioObjCopy(outpage->pdf, objval));
        break;

    default :
        break;
  }

  return (true);
}

static int
merge_resources_cb(pdfio_dict_t *dict,
	       	   const char *key,
		   pdfio_obj_t *value,
		   void *data)
{
    pdfio_dict_t *dest_dict = (pdfio_dict_t *)data;
    pdfioDictSetObj(dest_dict, key, pdfioObjCopy(NULL, value));
    return 1; // Continue iteration
}

//
// 'page_dict_cb()' - Merge page dictionaries from multiple input pages.
//
// This function detects resource conflicts and maps conflicting names as
// needed.
//combine multiple PDF pages onto one sheet (N-up), each page's fonts and images are perfectly translated and isolated in the new PDF structure.


static bool				// O - `true` to continue, `false` to stop
page_dict_cb(pdfio_dict_t *dict,	// I - Dictionary
             const char   *key,		// I - Dictionary key
             xform_page_t *outpage)	// I - Output page
{
  pdfio_array_t	*arrayres;		// Array resource
  pdfio_array_t	*arrayval = NULL;	// Array value
  pdfio_dict_t	*dictval = NULL;	// Dictionary value
  pdfio_obj_t	*objval;		// Object value


  fprintf(stderr, "DEBUG: page_dict_cb(dict=%p, key=\"%s\", outpage=%p), type=%d\n", (void *)dict, key, (void *)outpage, pdfioDictGetType(dict, key));

  if (strcmp(key, "ColorSpace") && strcmp(key, "ExtGState") && strcmp(key, "Font") && strcmp(key, "Pattern") && strcmp(key, "ProcSet") && strcmp(key, "Properties") && strcmp(key, "Shading") && strcmp(key, "XObject"))
    return (true);

  switch (pdfioDictGetType(dict, key))
  {
    case PDFIO_VALTYPE_ARRAY : // Array resource
        arrayval = pdfioDictGetArray(dict, key);
        break;

    case PDFIO_VALTYPE_DICT : // Dictionary resource
        dictval = pdfioDictGetDict(dict, key);
        break;

    case PDFIO_VALTYPE_INDIRECT : // Object reference to dictionary
        objval   = pdfioDictGetObj(dict, key);
        arrayval = pdfioObjGetArray(objval);
        dictval  = pdfioObjGetDict(objval);

        fprintf(stderr, "DEBUG: page_dict_cb: objval=%p(%u), arrayval=%p, dictval=%p\n", (void *)objval, (unsigned)pdfioObjGetNumber(objval), (void *)arrayval, (void *)dictval);
        break;

    default :
        break;
  }

  if (arrayval)
  {
    // Copy/merge an array resource...
    if ((arrayres = pdfioDictGetArray(outpage->resdict, key)) == NULL)
    {
      // Copy array
      pdfioDictSetArray(outpage->resdict, pdfioStringCreate(outpage->pdf, key), pdfioArrayCopy(outpage->pdf, arrayval));
    }
    else if (!strcmp(key, "ProcSet"))
    {
      // Merge ProcSet array
      size_t		i, j,		// Looping var
			ic, jc;		// Counts
      const char	*iv, *jv;	// Values

      for (i = 0, ic = pdfioArrayGetSize(arrayval); i < ic; i ++)
      {
	if ((iv = pdfioArrayGetName(arrayval, i)) == NULL)
	  continue;

	for (j = 0, jc = pdfioArrayGetSize(arrayres); j < jc; j ++)
	{
	  if ((jv = pdfioArrayGetName(arrayres, j)) == NULL)
	    continue;

	  if (!strcmp(iv, jv))
	    break;
	}

	if (j >= jc)
	  pdfioArrayAppendName(arrayres, pdfioStringCreate(outpage->pdf, iv));
      }
    }
  }
  else if (dictval)
  {
    // Copy/merge dictionary...
    if ((outpage->restype = pdfioDictGetDict(outpage->resdict, key)) == NULL)
      pdfioDictSetDict(outpage->resdict, pdfioStringCreate(outpage->pdf, key), pdfioDictCopy(outpage->pdf, dictval));
    else
      pdfioDictIterateKeys(dictval, (pdfio_dict_cb_t)resource_dict_cb, outpage);
  }

  return (true);
}

//
// 'page_ext_dict_cb()' - Merge page dictionaries from multiple input pages.
//
// This function detects resource conflicts and maps conflicting names as
// needed.
//

static bool				// O - `true` to continue, `false` to stop
page_ext_dict_cb(pdfio_dict_t *dict,	// I - Dictionary
             const char   *key,		// I - Dictionary key
             xform_page_ext_t *outpage)	// I - Output page
{
  pdfio_array_t	*arrayres;		// Array resource
  pdfio_array_t	*arrayval = NULL;	// Array value
  pdfio_dict_t	*dictval = NULL;	// Dictionary value
  pdfio_obj_t	*objval;		// Object value


  fprintf(stderr, "DEBUG: page_dict_cb(dict=%p, key=\"%s\", outpage=%p), type=%d\n", (void *)dict, key, (void *)outpage, pdfioDictGetType(dict, key));

  if (strcmp(key, "ColorSpace") && strcmp(key, "ExtGState") && strcmp(key, "Font") && strcmp(key, "Pattern") && strcmp(key, "ProcSet") && strcmp(key, "Properties") && strcmp(key, "Shading") && strcmp(key, "XObject"))
    return (true);

  switch (pdfioDictGetType(dict, key))
  {
    case PDFIO_VALTYPE_ARRAY : // Array resource
        arrayval = pdfioDictGetArray(dict, key);
        break;

    case PDFIO_VALTYPE_DICT : // Dictionary resource
        dictval = pdfioDictGetDict(dict, key);
        break;

    case PDFIO_VALTYPE_INDIRECT : // Object reference to dictionary
        objval   = pdfioDictGetObj(dict, key);
        arrayval = pdfioObjGetArray(objval);
        dictval  = pdfioObjGetDict(objval);

        fprintf(stderr, "DEBUG: page_dict_cb: objval=%p(%u), arrayval=%p, dictval=%p\n", (void *)objval, (unsigned)pdfioObjGetNumber(objval), (void *)arrayval, (void *)dictval);
        break;

    default :
        break;
  }

  if (arrayval)
  {
    // Copy/merge an array resource...
    if ((arrayres = pdfioDictGetArray(outpage->resdict, key)) == NULL)
    {
      // Copy array
      pdfioDictSetArray(outpage->resdict, pdfioStringCreate(outpage->pdf, key), pdfioArrayCopy(outpage->pdf, arrayval));
    }
    else if (!strcmp(key, "ProcSet"))
    {
      // Merge ProcSet array
      size_t		i, j,		// Looping var
			ic, jc;		// Counts
      const char	*iv, *jv;	// Values

      for (i = 0, ic = pdfioArrayGetSize(arrayval); i < ic; i ++)
      {
	if ((iv = pdfioArrayGetName(arrayval, i)) == NULL)
	  continue;

	for (j = 0, jc = pdfioArrayGetSize(arrayres); j < jc; j ++)
	{
	  if ((jv = pdfioArrayGetName(arrayres, j)) == NULL)
	    continue;

	  if (!strcmp(iv, jv))
	    break;
	}

	if (j >= jc)
	  pdfioArrayAppendName(arrayres, pdfioStringCreate(outpage->pdf, iv));
      }
    }
  }
  else if (dictval)
  {
    // Copy/merge dictionary...
    if ((outpage->restype = pdfioDictGetDict(outpage->resdict, key)) == NULL)
      pdfioDictSetDict(outpage->resdict, pdfioStringCreate(outpage->pdf, key), pdfioDictCopy(outpage->pdf, dictval));
    else
      pdfioDictIterateKeys(dictval, (pdfio_dict_cb_t)resource_dict_cb, outpage);
  }

  return (true);
}

//
// 'pdfio_password_cb()' - Return the password, if any, for the input document.
//currently deactivating it as it is not required for now

// static const char * // O - Document password
// pdfio_password_cb(void       *cb_data,  // I - Document number
//                   const char *filename) // I - Filename (unused)
// {
//   int   document = *((int *)cb_data);   // Document number
//   char  name[128];                      // Environment variable name


//   (void)filename;

//   if (document > 1)
//   {
//     snprintf(name, sizeof(name), "IPP_DOCUMENT_PASSWORD%d", document);
//     return (getenv(name));
//   }
//   else
//   {
//     return (getenv("IPP_DOCUMENT_PASSWORD"));
//   }
// }

static void
prepare_number_up(xform_prepare_t *p)	// I - Preparation data
{
  size_t	i,			// Looping var
		cols,			// Number of columns
		rows;			// Number of rows
  pdfio_rect_t	*r;			// Current layout rectangle...
  double	width,			// Width of layout rectangle
		height;			// Height of layout rectangle


  if (!strcmp(p->options->imposition_template, "booklet"))
  {
    // "imposition-template" = 'booklet' forces 2-up output...
    p->num_layout   = 2;
    p->layout[0]    = p->media;
    p->layout[0].y2 = p->media.y2 / 2.0;
    p->layout[1]    = p->media;
    p->layout[1].y1 = p->media.y2 / 2.0;

    if (p->options->number_up != 1)
      prepare_log(p, false, "Ignoring \"number-up\" = '%d'.", p->options->number_up);

    return;
  }
  else
  {
    p->num_layout = (size_t)p->options->number_up;
  }

  // Figure out the number of rows and columns...
  switch (p->num_layout)
  {
    default : // 1-up or unknown
	cols = 1;
	rows = 1;
	break;
    case 2 : // 2-up
        cols = 1;
        rows = 2;
        break;
    case 3 : // 3-up
        cols = 1;
        rows = 3;
        break;
    case 4 : // 4-up
        cols = 2;
        rows = 2;
        break;
    case 6 : // 6-up
        cols = 2;
        rows = 3;
        break;
    case 8 : // 8-up
        cols = 2;
        rows = 4;
        break;
    case 9 : // 9-up
        cols = 3;
        rows = 3;
        break;
    case 10 : // 10-up
        cols = 2;
        rows = 5;
        break;
    case 12 : // 12-up
        cols = 3;
        rows = 4;
        break;
    case 15 : // 15-up
        cols = 3;
        rows = 5;
        break;
    case 16 : // 16-up
        cols = 4;
        rows = 4;
        break;
  }
  
  // Then arrange the page rectangles evenly across the page...
  width  = (p->crop.x2 - p->crop.x1) / cols;
  height = (p->crop.y2 - p->crop.y1) / rows;
  
  switch (p->options->orientation_requested)
  {
    default : // Portrait or "none"...
        for (i = 0, r = p->layout; i < p->num_layout; i ++, r ++)
        {
          r->x1 = p->crop.x1 + width * (i % cols);
          r->y1 = p->crop.y1 + height * (rows - 1 - i / cols);
          r->x2 = r->x1 + width;
          r->y2 = r->y1 + height;
        }
        break;

    case CF_FILTER_ORIENT_LANDSCAPE : // Landscape
        for (i = 0, r = p->layout; i < p->num_layout; i ++, r ++)
        {
          r->x1 = p->crop.x1 + width * (cols - 1 - i / rows);
          r->y1 = p->crop.y1 + height * (rows - 1 - (i % rows));
          r->x2 = r->x1 + width;
          r->y2 = r->y1 + height;
        }
        break;

    case CF_FILTER_ORIENT_REVERSE_PORTRAIT : // Reverse portrait
        for (i = 0, r = p->layout; i < p->num_layout; i ++, r ++)
        {
          r->x1 = p->crop.x1 + width * (cols - 1 - (i % cols));
          r->y1 = p->crop.y1 + height * (i / cols);
          r->x2 = r->x1 + width;
          r->y2 = r->y1 + height;
        }
        break;

    case CF_FILTER_ORIENT_REVERSE_LANDSCAPE : // Reverse landscape
        for (i = 0, r = p->layout; i < p->num_layout; i ++, r ++)
        {
          r->x1 = p->crop.x1 + width * (i / rows);
          r->y1 = p->crop.y1 + height * (i % rows);
          r->x2 = r->x1 + width;
          r->y2 = r->y1 + height;
        }
        break;
  }
}

//
// 'prepare_pages()' - Prepare the pages for the output document.
//

static void
prepare_pages(
    xform_prepare_t  *p,		// I - Preparation data
    size_t           num_documents,	// I - Number of documents
    xform_document_t *documents)	// I - Documents
{
  int		page;			// Current page number in output
  size_t	i,			// Looping var
		current,		// Current output page index
		layout;			// Current layout cell
  xform_page_t	*outpage;		// Current output page
  xform_document_t *d;			// Current document
  bool		use_page;		// Use this page?


  if (!strcmp(p->options->imposition_template, "booklet"))
  {
    // Booklet printing arranges input pages so that the folded output can be
    // stapled along the midline...
    p->num_outpages = (size_t)(p->num_inpages + 1) / 2;
    if (p->num_outpages & 1)
      p->num_outpages ++;

    for (current = 0, layout = 0, page = 1, i = num_documents, d = documents; i > 0; i --, d ++)
    {
      while (page <= d->last_page)
      {
	if (p->options->multiple_document_handling < CF_FILTER_HANDLING_SINGLE_DOCUMENT)
	  use_page = cfFilterOptionsIsPageInRange(p->options, page - d->first_page + 1);
	else
	  use_page = cfFilterOptionsIsPageInRange(p->options, page);

        if (use_page)
        {
	  if (current < p->num_outpages)
	    outpage = p->outpages + current;
	  else
	    outpage = p->outpages + 2 * p->num_outpages - current - 1;

	  outpage->pdf           = p->pdf;
	  outpage->input[layout] = pdfioFileGetPage(d->pdf, (size_t)(page - d->first_page));
	  layout = 1 - layout;
	  current ++;
	}

        page ++;
      }

      if (p->options->multiple_document_handling < CF_FILTER_HANDLING_SINGLE_DOCUMENT)
        page = 1;
    }
  }
  else
  {
    // Normal printing lays out N input pages on each output page...
    for (current = 0, outpage = p->outpages, layout = 0, page = 1, i = num_documents, d = documents; i > 0; i --, d ++)
    {
      while (page <= d->last_page)
      {
	if (p->options->multiple_document_handling < CF_FILTER_HANDLING_SINGLE_DOCUMENT)
	  use_page = cfFilterOptionsIsPageInRange(p->options, page - d->first_page + 1);
	else
	  use_page = cfFilterOptionsIsPageInRange(p->options, page);

        if (use_page)
        {
          outpage->pdf           = p->pdf;
          outpage->input[layout] = pdfioFileGetPage(d->pdf, (size_t)(page - d->first_page));

          if (Verbosity)
	    fprintf(stderr, "DEBUG: Using page %d (%p) of document %d, cell=%u/%u, current=%u\n", page, (void *)outpage->input[layout], (int)(d - documents + 1), (unsigned)layout + 1, (unsigned)p->num_layout, (unsigned)current);

          layout ++;
          if (layout == p->num_layout)
          {
            current ++;
            outpage ++;
            layout = 0;
          }
        }

        page ++;
      }

      if (p->options->multiple_document_handling < CF_FILTER_HANDLING_SINGLE_DOCUMENT)
      {
        page = 1;

        if (layout)
        {
	  current ++;
	  outpage ++;
	  layout = 0;
        }
      }
      else if (p->options->multiple_document_handling == CF_FILTER_HANDLING_SINGLE_NEW_SHEET && (current & 1))
      {
	current ++;
	outpage ++;
	layout = 0;
      }
    }

    if (layout)
      current ++;

    p->num_outpages = current;
  }
}
// Converts a 6-element PDF array [a b c d e f] into a pdfio_matrix_t for coordinate transformations.
void
getArrayAsMatrix(pdfio_array_t *array,
		 pdfio_matrix_t cm)
{
  size_t array_size = pdfioArrayGetSize(array);
  if(array_size != 6)
    return;
  double items[6];
  for(size_t i = 0; i<array_size; i++)
  {
    items[i] = pdfioArrayGetNumber(array, i);
  }

  cm[0][0] = items[0];
  cm[0][1] = items[1];
  cm[1][0] = items[2];
  cm[1][1] = items[3];
  cm[2][0] = items[4];
  cm[2][1] = items[5];
  return;
}


double
get_flags(pdfio_dict_t *annots_dict)
{
  double val = pdfioDictGetNumber(annots_dict, "F");
  return val;
}

static inline void matrix_set_identity(pdfio_matrix_t m) { // Resets matrix to identity (no transformation)
  m[0][0] = 1.0; m[0][1] = 0.0;   // a b
  m[1][0] = 0.0; m[1][1] = 1.0;   // c d
  m[2][0] = 0.0; m[2][1] = 0.0;   // e f
}

// Transforms a single 2D point (x, y) using the provided matrix.
static inline void matrix_apply_point(const pdfio_matrix_t m, double x, double y, double *ox, double *oy) {
  *ox = m[0][0]*x + m[1][0]*y + m[2][0];
  *oy = m[0][1]*x + m[1][1]*y + m[2][1];
}


// Transforms a single 2D point (x, y) using the provided matrix.
static inline void matrix_translate(pdfio_matrix_t m, double tx, double ty) {
  matrix_set_identity(m);
  m[2][0] = tx;  // e
  m[2][1] = ty;  // f
}

// Converts a matrix to a string "a b c d e f".
static char *matrix_unparse(const pdfio_matrix_t m) 
{
  // Emit "a b c d e f" with common PDF precision
  char *s = (char*)malloc(128);
  if (!s) 
    return NULL;
  snprintf(s, 128, "%.6f %.6f %.6f %.6f %.6f %.6f",
           m[0][0], m[0][1],  // a b
           m[1][0], m[1][1],  // c d
           m[2][0], m[2][1]); // e f
  return s;
}

// Configures matrix for rotation in 90-degree increments (90, 180, 270).
static inline void 
matrix_rotatex90(pdfio_matrix_t m, 
		 int degrees) 
{
  int k = ((degrees % 360) + 360) % 360;
  matrix_set_identity(m);
  switch (k) 
  {
    case 90:   
      m[0][0]=0;  
      m[0][1]=1;  
      m[1][0]=-1; 
      m[1][1]=0;  
      break;  		// [0 1 -1 0 0 0]
    case 180:  
      m[0][0]=-1; 
      m[0][1]=0;  
      m[1][0]=0;  
      m[1][1]=-1; 
      break;  		// [-1 0 0 -1 0 0]
    case 270:  
      m[0][0]=0; 
      m[0][1]=-1;
      m[1][0]=1;  
      m[1][1]=0; 
      break;  		// [0 -1 1 0 0 0]
    default: /* 0° */ 
      break;
  }
}

// Creates a scaling matrix.
static inline void 
matrix_scale(pdfio_matrix_t m,
	     double sx, double sy) 
{
  matrix_set_identity(m);
  m[0][0] = sx;  // a
  m[1][1] = sy;  // d
}

// Copies a matrix.
static inline void 
matrix_copy(pdfio_matrix_t dst, 
	    const pdfio_matrix_t src) 
{
  dst[0][0]=src[0][0]; 
  dst[0][1]=src[0][1];
  dst[1][0]=src[1][0]; 
  dst[1][1]=src[1][1];
  dst[2][0]=src[2][0]; 
  dst[2][1]=src[2][1];
}

// Concatenates (multiplies) two matrices to combine multiple transformations.
static inline void 
matrix_concat(pdfio_matrix_t out, 
	      const pdfio_matrix_t L, 
	      const pdfio_matrix_t R) 
{
  pdfio_matrix_t t;
  t[0][0] = L[0][0]*R[0][0] + L[1][0]*R[0][1];  // a' = La*Ra + Lc*Rb
  t[0][1] = L[0][1]*R[0][0] + L[1][1]*R[0][1];  // b' = Lb*Ra + Ld*Rb
  t[1][0] = L[0][0]*R[1][0] + L[1][0]*R[1][1];  // c' = La*Rc + Lc*Rd
  t[1][1] = L[0][1]*R[1][0] + L[1][1]*R[1][1];  // d' = Lb*Rc + Ld*Rd
  t[2][0] = L[0][0]*R[2][0] + L[1][0]*R[2][1] + L[2][0]; // e' = La*Re + Lc*Rf + Le
  t[2][1] = L[0][1]*R[2][0] + L[1][1]*R[2][1] + L[2][1]; // f' = Lb*Re + Ld*Rf + Lf
  matrix_copy(out, t);
}

// Transforms a rectangle by applying the matrix transformation to all four corners and finding the new bounding box.
static inline pdfio_rect_t 
matrix_transform_rect(const pdfio_matrix_t m, 
		      pdfio_rect_t r) 
{
  double X[4], Y[4];
  matrix_apply_point(m, r.x1, r.y1, &X[0], &Y[0]);
  matrix_apply_point(m, r.x2, r.y1, &X[1], &Y[1]);
  matrix_apply_point(m, r.x2, r.y2, &X[2], &Y[2]);
  matrix_apply_point(m, r.x1, r.y2, &X[3], &Y[3]);

  double minx=X[0], maxx=X[0], miny=Y[0], maxy=Y[0];
  for (int i=1;i<4;i++) 
  {
    if (X[i]<minx) 
      minx=X[i]; 

    if (X[i]>maxx) 
      maxx=X[i];

    if (Y[i]<miny) 
      miny=Y[i]; 
    
    if (Y[i]>maxy) 
      maxy=Y[i];
  }
  pdfio_rect_t out = { minx, miny, maxx, maxy };
  return out;
}



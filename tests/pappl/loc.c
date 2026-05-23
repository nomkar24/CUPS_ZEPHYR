//
// Localization functions for the Printer Application Framework
//
#include "loc-private.h"
#include "strings/en_strings.h"
//
// Local globals...
//

static pappl_loc_t	loc_default = { CUPS_RWLOCK_INITIALIZER, NULL, NULL, NULL };
static cups_mutex_t	loc_mutex = CUPS_MUTEX_INITIALIZER;

//
// Local functions...
//

static void		loc_load_resource(pappl_loc_t *loc, _pappl_resource_t *r);
static int		locpair_compare(_pappl_locpair_t *a, _pappl_locpair_t *b);
static _pappl_locpair_t	*locpair_copy(_pappl_locpair_t *pair);
static void		locpair_free(_pappl_locpair_t *pair);

//
// '_papplLocCompare()' - Compare two localizations.
//

int					// O - Result of comparison
_papplLocCompare(pappl_loc_t *a,	// I - First localization
                 pappl_loc_t *b)	// I - Second localization
{
  return (strcasecmp(a->name, b->name));
}


//
// '_papplLocCreate()' - Create/update a localization for the given system and language.
//

pappl_loc_t *				// O - Localization or `NULL` on error
_papplLocCreate(
    pappl_system_t    *system,		// I - System
    _pappl_resource_t *r)		// I - Resource
{
  pappl_loc_t		*loc;		// Localization data


  // See if we already have a localization for this language...
  if ((loc = papplSystemFindLoc(system, r->language)) == NULL)
  {
    // No, allocate memory for a new one...
    if ((loc = (pappl_loc_t *)calloc(1, sizeof(pappl_loc_t))) == NULL)
      return (NULL);

    cupsRWInit(&loc->rwlock);

    loc->system = system;
    loc->name   = strdup(r->language);
    loc->pairs  = cupsArrayNew((cups_array_cb_t)locpair_compare, NULL, NULL, 0, (cups_acopy_cb_t)locpair_copy, (cups_afree_cb_t)locpair_free);

    if (!loc->name || !loc->pairs)
    {
      _papplLocDelete(loc);
      return (NULL);
    }

    // Add it to the system...
    _papplSystemAddLoc(system, loc);
  }

  // Load resource...
  _papplRWLockWrite(loc);

  loc_load_resource(loc, r);

  _papplRWUnlock(loc);

  // Return it...
  return (loc);
}


//
// '_papplLocDelete()' - Free memory used by a localization.
//

void
_papplLocDelete(pappl_loc_t *loc)	// I - Localization
{
  cupsRWDestroy(&loc->rwlock);

  free(loc->name);
  cupsArrayDelete(loc->pairs);
  free(loc);
}


//
// 'papplLocFormatString()' - Format a localized string into a buffer.
//
// This function formats a localized string into a buffer using the specified
// localization data.  Numbers are formatted according to the locale and
// language used.
//

const char *				// O - Localized formatted string
papplLocFormatString(
    pappl_loc_t *loc,			// I - Localization data
    char        *buffer,		// I - Output buffer
    size_t      bufsize,		// I - Size of output buffer
    const char  *key,			// I - Printf-style key string to localize
    ...)				// I - Additional arguments as needed
{
  va_list	ap;			// Argument pointer


  // Range-check input
  if (!buffer || bufsize < 10 || !key)
  {
    if (buffer)
      *buffer = '\0';
    return (NULL);
  }

  // Format string
  va_start(ap, key);
  vsnprintf(buffer, bufsize, papplLocGetString(loc, key), ap);
  va_end(ap);

  return (buffer);
}


//
// 'papplLocGetDefaultMediaSizeName()' - Get the default media size name
//                                       associated with a locale.
//
// This function returns the default PWG media size name corresponding to the
// current locale.  Currently only "na_letter_8.5x11in" is returned.
//

const char *
papplLocGetDefaultMediaSizeName(void)
{
  return ("na_letter_8.5x11in");
}


//
// 'papplLocGetString()' - Get a localized version of a key string.
//
// This function looks up the specified key string in the localization data and
// returns either the localized value or the original key string if no
// localization is available.
//

const char *				// O - Localized text string
papplLocGetString(pappl_loc_t *loc,	// I - Localization data
                  const char  *key)	// I - Key text
{
  _pappl_locpair_t	search,		// Search key
			*match;		// Matching pair, if any


  // Range check input...
  if (!loc)
    return (key);

  // Look up the key...
  _papplRWLockRead(loc);
  search.key = (char *)key;
  match      = cupsArrayFind(loc->pairs, &search);
  _papplRWUnlock(loc);

  // Return a string to use...
  return (match ? match->text : key);
}


//
// '_papplLocLoadAll()' - Load all (en only supported to save memory ) base localizations.
//

void
_papplLocLoadAll(pappl_system_t *system)  // I - System
{
  _pappl_resource_t r;                    // Temporary resource data
  memset(&r, 0, sizeof(r));
  r.language = "en";
  r.data     = (const void *)en_strings;
  _papplLocCreate(system, &r);
}


//
// '_papplLocPrintf()' - Print a localized string.
//

void
_papplLocPrintf(FILE       *fp,       // I - Output file
                const char *message,  // I - Printf-style message string
                ...)                  // I - Additional arguments as needed
{
  va_list ap;                         // Argument pointer
  va_start(ap, message);
  vfprintf(fp, message, ap);
  putc('\n', fp);
  va_end(ap);
}

//
// 'loc_load_resource()' - Load a strings resource into a localization.
//
// file-loading branch removed (no filesystem needed).
// Only in-memory string data (en_strings) is supported.
//

static void
loc_load_resource(
    pappl_loc_t       *loc,   // I - Localization data
    _pappl_resource_t *r)     // I - Resource
{
  const char       *data,     // Pointer to strings data
                   *dataptr;  // Pointer into string data
  char             key[1024], // Key string
                   text[1024],// Localized text string
                   *ptr;      // Pointer into strings
  int              linenum;   // Line number
  _pappl_locpair_t pair;      // New pair


  // Zephyr: only in-memory data supported, no file loading
  if (!r->data)
    return;

  data = (const char *)r->data;

  // Scan the in-memory strings data and add key/text pairs...
  //
  // Format of strings files is:
  //
  // "key" = "text";
  pair.key  = key;
  pair.text = text;

  for (dataptr = data, linenum = 1; *dataptr; dataptr++)
  {
    // Skip leading whitespace...
    while (*dataptr && isspace(*dataptr & 255))
    {
      if (*dataptr == '\n')
        linenum++;
      dataptr++;
    }

    if (!*dataptr)
      break;
    else if (*dataptr == '/' && dataptr[1] == '*')
    {
      // Start of C-style comment...
      for (dataptr += 2; *dataptr; dataptr++)
      {
        if (*dataptr == '*' && dataptr[1] == '/')
        {
          dataptr += 2;
          break;
        }
        else if (*dataptr == '\n')
          linenum++;
      }

      if (!*dataptr)
        break;
    }
    else if (*dataptr != '\"')
    {
      // Something else we don't recognize...
      papplLog(loc->system, PAPPL_LOGLEVEL_WARN,
               "Syntax error on line %d of in-memory strings.", linenum);
      break;
    }

    // Parse key string...
    dataptr++;
    for (ptr = key; *dataptr && *dataptr != '\"'; dataptr++)
    {
      if (*dataptr == '\\' && dataptr[1])
      {
        int ch;
        dataptr++;
        if (*dataptr == '\\' || *dataptr == '\'' || *dataptr == '\"')
          ch = *dataptr;
        else if (*dataptr == 'n')
          ch = '\n';
        else if (*dataptr == 'r')
          ch = '\r';
        else if (*dataptr == 't')
          ch = '\t';
        else if (*dataptr >= '0' && *dataptr <= '3' &&
                 dataptr[1] >= '0' && dataptr[1] <= '7' &&
                 dataptr[2] >= '0' && dataptr[2] <= '7')
        {
          ch = ((*dataptr - '0') << 6) | ((dataptr[1] - '0') << 3) | (dataptr[2] - '0');
          dataptr += 2;
        }
        else
        {
          papplLog(loc->system, PAPPL_LOGLEVEL_WARN,
                   "Invalid escape in key on line %d.", linenum);
          break;
        }

        if (ptr < (key + sizeof(key) - 1))
          *ptr++ = (char)ch;
      }
      else if (ptr < (key + sizeof(key) - 1))
      {
        *ptr++ = *dataptr;
      }
    }

    if (!*dataptr)
    {
      papplLog(loc->system, PAPPL_LOGLEVEL_WARN,
               "Unterminated key string on line %d.", linenum);
      break;
    }

    dataptr++;
    *ptr = '\0';

    // Parse separator...
    while (*dataptr && isspace(*dataptr & 255))
    {
      if (*dataptr == '\n')
        linenum++;
      dataptr++;
    }

    if (*dataptr != '=')
    {
      papplLog(loc->system, PAPPL_LOGLEVEL_WARN,
               "Missing separator on line %d.", linenum);
      break;
    }

    dataptr++;
    while (*dataptr && isspace(*dataptr & 255))
    {
      if (*dataptr == '\n')
        linenum++;
      dataptr++;
    }

    if (*dataptr != '\"')
    {
      papplLog(loc->system, PAPPL_LOGLEVEL_WARN,
               "Missing text string on line %d.", linenum);
      break;
    }

    // Parse text string...
    dataptr++;
    for (ptr = text; *dataptr && *dataptr != '\"'; dataptr++)
    {
      if (*dataptr == '\\')
      {
        int ch;
        dataptr++;
        if (*dataptr == '\\' || *dataptr == '\'' || *dataptr == '\"')
          ch = *dataptr;
        else if (*dataptr == 'n')
          ch = '\n';
        else if (*dataptr == 'r')
          ch = '\r';
        else if (*dataptr == 't')
          ch = '\t';
        else if (*dataptr >= '0' && *dataptr <= '3' &&
                 dataptr[1] >= '0' && dataptr[1] <= '7' &&
                 dataptr[2] >= '0' && dataptr[2] <= '7')
        {
          ch = ((*dataptr - '0') << 6) | ((dataptr[1] - '0') << 3) | (dataptr[2] - '0');
          dataptr += 2;
        }
        else
        {
          papplLog(loc->system, PAPPL_LOGLEVEL_WARN,
                   "Invalid escape in text on line %d.", linenum);
          break;
        }

        if (ptr < (text + sizeof(text) - 1))
          *ptr++ = (char)ch;
      }
      else if (ptr < (text + sizeof(text) - 1))
      {
        *ptr++ = *dataptr;
      }
    }

    if (!*dataptr)
    {
      papplLog(loc->system, PAPPL_LOGLEVEL_WARN,
               "Unterminated text string on line %d.", linenum);
      break;
    }

    dataptr++;
    *ptr = '\0';

    // Look for terminator, then add the pair...
    if (*dataptr != ';')
    {
      papplLog(loc->system, PAPPL_LOGLEVEL_WARN,
               "Missing terminator on line %d.", linenum);
      break;
    }

    dataptr++;

    if (!cupsArrayFind(loc->pairs, &pair))
      cupsArrayAdd(loc->pairs, &pair);
  }

  // NOTE: no free() needed — data points to static en_strings, not malloc'd memory
}


//
// 'locpair_compare()' - Compare the keys of two key/text pairs.
//

static int				// O - Result of comparison
locpair_compare(_pappl_locpair_t *a,	// I - First key/text pair
                _pappl_locpair_t *b)	// I - Second key/text pair
{
  return (strcmp(a->key, b->key));
}


//
// 'locpair_copy()' - Copy a key/text pair.
//

static _pappl_locpair_t	*		// O - Copy of pair
locpair_copy(_pappl_locpair_t *pair)	// I - Pair to copy
{
  _pappl_locpair_t	*npair;		// New pair


  // Allocate a new pair...
  if ((npair = calloc(1, sizeof(_pappl_locpair_t))) != NULL)
  {
    // Duplicate the strings...
    npair->key  = strdup(pair->key);
    npair->text = strdup(pair->text);

    if (!npair->key || !npair->text)
    {
      // Wasn't able to allocate everything...
      locpair_free(npair);
      npair = NULL;
    }
  }

  return (npair);
}


//
// 'locpair_free()' - Free memory used by a key/text pair.
//

static void
locpair_free(_pappl_locpair_t *pair)	// I - Pair
{
  free(pair->key);
  free(pair->text);
  free(pair);
}


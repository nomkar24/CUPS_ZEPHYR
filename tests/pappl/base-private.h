//
// Private base definitions for the Printer Application Framework
//

#ifndef _PAPPL_BASE_PRIVATE_H_
#  define _PAPPL_BASE_PRIVATE_H_
#  include "config.h"              // Zephyr-generated: PAPPL_VERSION, PAPPL_STATEDIR, HAVE_* flags
#  include <zephyr/kernel.h>
#  include <zephyr/sys/printk.h>
#  include <zephyr/net/socket.h>    // zsock_poll replaces poll()
#  include <zephyr/fs/fs.h>         // Zephyr VFS
#  include "base.h"
#  include <limits.h>
#  include <cups/dnssd.h>
#  include <cups/thread.h>

// Zephyr: no Win32, no fork/exec, no grp.h, no environ.
// sys/wait.h and poll.h are replaced by Zephyr equivalents above.
#  define O_BINARY  0               // keep sources portable
#  define getuid()  0               // single-user embedded system

//
// CUPS v3 API note: when building the CUPS compat layer, include
// cups/cups-private.h shim before this header.
//

//
// Macros...
//

#  ifdef DEBUG
// Zephyr: use printk + k_current_get() instead of fprintf/pthread_self()
#    define _PAPPL_DEBUG(...) printk(__VA_ARGS__)
#    define _papplRWLockRead(obj)  (printk("%p/%s,%d: rdlock  %p(%s)\n",  (void *)k_current_get(), __func__, __LINE__, (void *)(obj), (obj)->name), cupsRWLockRead(&(obj)->rwlock))
#    define _papplRWLockWrite(obj) (printk("%p/%s,%d: wrlock  %p(%s)\n",  (void *)k_current_get(), __func__, __LINE__, (void *)(obj), (obj)->name), cupsRWLockWrite(&(obj)->rwlock))
#    define _papplRWUnlock(obj)    (printk("%p/%s,%d: unlock  %p(%s)\n",  (void *)k_current_get(), __func__, __LINE__, (void *)(obj), (obj)->name), cupsRWUnlock(&(obj)->rwlock))
#  else
#    define _PAPPL_DEBUG(...)      do {} while (0)
#    define _papplRWLockRead(obj)  cupsRWLockRead(&(obj)->rwlock)
#    define _papplRWLockWrite(obj) cupsRWLockWrite(&(obj)->rwlock)
#    define _papplRWUnlock(obj)    cupsRWUnlock(&(obj)->rwlock)
#  endif // DEBUG

#  define _PAPPL_LOC(s) s
#  define _PAPPL_LOOKUP_STRING(bit,strings) _papplLookupString(bit, sizeof(strings) / sizeof(strings[0]), strings)
#  define _PAPPL_LOOKUP_VALUE(keyword,strings) _papplLookupValue(keyword, sizeof(strings) / sizeof(strings[0]), strings)


//
// Macros to implement a simple Fibonacci sequence for variable back-off...
//

#  define _PAPPL_FIB_NEXT(v) (((((v >> 8) + (v & 255) - 1) % 60) + 1) | ((v & 255) << 8))
#  define _PAPPL_FIB_VALUE(v) (v & 255)


//
// Types and structures...
//

typedef struct _pappl_attr_s		// Input attribute structure
{
  const char	*name;			// Attribute name
  ipp_tag_t	value_tag;		// Value tag
  size_t	max_count;		// Max number of values
} _pappl_attr_t;

typedef struct _pappl_ipp_filter_s	// Attribute filter
{
  cups_array_t		*ra;			// Requested attributes
  ipp_tag_t		group_tag;		// Group to copy
} _pappl_ipp_filter_t;

typedef struct _pappl_link_s		// Web interface navigation link
{
  char			*label,			// Label
			*path_or_url;		// Path or URL
  pappl_loptions_t	options;		// Link options
} _pappl_link_t;

typedef struct _pappl_odevice_s _pappl_odevice_t;
					// Output device
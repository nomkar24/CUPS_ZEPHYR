//
// Test suite header file for the Printer Application Framework
//
// Copyright © 2020 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef _TESTPAPPL_H_
#  define _TESTPAPPL_H_


//
// Include necessary headers...
//

#include <pappl/pappl.h>


//
// Globals...
//

extern pappl_pr_driver_t pwg_drivers[11];
#ifdef PWG_DRIVER
pappl_pr_driver_t pwg_drivers[11] =
{					// Drivers
  { "pwg_2inch-203dpi-black_1",		"PWG 2inch Label 203DPI Black",		NULL,	NULL },
  { "pwg_2inch-300dpi-black_1",		"PWG 2inch Label 300DPI Black",		NULL,	NULL },
  { "pwg_4inch-203dpi-black_1",		"PWG 4inch Label 203DPI Black",		NULL,	NULL },
  { "pwg_4inch-300dpi-black_1",		"PWG 4inch Label 300DPI Black",		NULL,	NULL },
  { "pwg_common-300dpi-black_1",	"PWG Office 300DPI Black",		NULL,	NULL },
  { "pwg_common-300dpi-sgray_8",	"PWG Office 300DPI sGray 8-bit",		NULL,	NULL },
  { "pwg_common-300dpi-srgb_8",		"PWG Office 300DPI sRGB 8-bit",		NULL,	NULL },
  { "pwg_common-300dpi-600dpi-black_1",	"PWG Office 300DPI 600DPI Black",		NULL,	NULL },
  { "pwg_common-300dpi-600dpi-sgray_8",	"PWG Office 300DPI 600DPI sGray 8-bit",		NULL,	NULL },
  { "pwg_common-300dpi-600dpi-srgb_8",	"PWG Office 300DPI 600DPI sRGB 8-bit",		NULL,	NULL },
  { "pwg_fail-300dpi-black_1",	"PWG Always Fails 300DPI Black",		NULL,	NULL }
};
#endif // PWG_DRIVER


//
// Functions..
//

extern const char *pwg_autoadd(const char *device_info, const char *device_uri, const char *device_id, void *data);
extern bool	pwg_callback(pappl_system_t *system, const char *driver_name, const char *device_uri, const char *device_id, pappl_pr_driver_data_t *driver_data, ipp_t **driver_attrs, void *data);

#ifndef F_OK
#  define F_OK 0
#endif
#ifndef X_OK
#  define X_OK 1
#endif
#ifndef W_OK
#  define W_OK 2
#endif
#ifndef R_OK
#  define R_OK 4
#endif

#endif // !_TESTPAPPL_H_

#ifndef _INCL_RSA_COMMON_H_
#define _INCL_RSA_COMMON_H_ 1

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

#define TFM_DESC
#define LTC_NO_ROLC
#include "tomcrypt.h"

#define SALT_LEN 8

#if defined(__GNUC__) || defined(__clang__)
#define NEVER_RETURNS __attribute__((noreturn))
#define PRINTF_FUNC(fmtargnum, dotargnum) __attribute__ (( format( __printf__, fmtargnum, dotargnum )))
#else
#define NEVER_RETURNS
#define PRINTF_FUNC(fmtargnum, dotargnum)
#endif

void fail(const char *fmt, ...) NEVER_RETURNS PRINTF_FUNC(1, 2);
void write_file(const char *fname, const void *buf, const unsigned long len);
void read_file(const char *fname, void *buf, unsigned long *len);
void read_rsakey(rsa_key *key, const char *fname);

#endif

/* end of rsa_common.h ... */


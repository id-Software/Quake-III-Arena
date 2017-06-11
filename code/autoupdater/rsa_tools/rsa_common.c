#include "rsa_common.h"

void fail(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    fflush(stderr);
    exit(1);
}

void write_file(const char *fname, const void *buf, const unsigned long len)
{
    FILE *io = fopen(fname, "wb");
    if (!io) {
        fail("Can't open '%s' for writing: %s", fname, strerror(errno));
    }

    if (fwrite(buf, len, 1, io) != 1) {
        fail("Couldn't write '%s': %s", fname, strerror(errno));
    }

    if (fclose(io) != 0) {
        fail("Couldn't flush '%s' to disk: %s", fname, strerror(errno));
    }
}

void read_file(const char *fname, void *buf, unsigned long *len)
{
    ssize_t br;
    FILE *io = fopen(fname, "rb");
    if (!io) {
        fail("Can't open '%s' for reading: %s", fname, strerror(errno));
    }

    br = fread(buf, 1, *len, io);
    if (ferror(io)) {
        fail("Couldn't read '%s': %s", fname, strerror(errno));
    } else if (!feof(io)) {
        fail("Buffer too small to read '%s'", fname);
    }
    fclose(io);

    *len = (unsigned long) br;
}

void read_rsakey(rsa_key *key, const char *fname)
{
    unsigned char buf[4096];
    unsigned long len = sizeof (buf);
    int rc;

    read_file(fname, buf, &len);

    if ((rc = rsa_import(buf, len, key)) != CRYPT_OK) {
        fail("rsa_import for '%s' failed: %s", fname, error_to_string(rc));
    }
}


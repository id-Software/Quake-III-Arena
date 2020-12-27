#include "rsa_common.h"

static void write_rsakey(rsa_key *key, const int type, const char *fname)
{
    unsigned char buf[4096];
    unsigned long len = sizeof (buf);
    int rc;

    if ((rc = rsa_export(buf, &len, type, key)) != CRYPT_OK) {
        fail("rsa_export for '%s' failed: %s", fname, error_to_string(rc));
    }
    write_file(fname, buf, len);
}

int main(int argc, char **argv)
{
    int rc = 0;
    prng_state prng;
    int prng_index;
    rsa_key key;

    ltc_mp = tfm_desc;
    prng_index = register_prng(&sprng_desc);  /* (fortuna_desc is a good choice if your platform's PRNG sucks.) */

    if (prng_index == -1) {
        fail("Failed to register a RNG");
    }

    if ((rc = rng_make_prng(128, prng_index, &prng, NULL)) != CRYPT_OK) {
        fail("rng_make_prng failed: %s", error_to_string(rc));
    }

    if ((rc = rsa_make_key(&prng, prng_index, 256, 65537, &key)) != CRYPT_OK) {
        fail("rng_make_key failed: %s", error_to_string(rc));
    }

    write_rsakey(&key, PK_PRIVATE, "privatekey.bin");
    write_rsakey(&key, PK_PUBLIC, "publickey.bin");

    rsa_free(&key);

    return 0;
}

/* end of rsa_make_keys.c ... */

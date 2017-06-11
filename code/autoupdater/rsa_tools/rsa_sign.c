#include "rsa_common.h"

static void sign_file(const char *fname, rsa_key *key, prng_state *prng, const int prng_index, const int hash_index)
{
    const size_t sigfnamelen = strlen(fname) + 5;
    char *sigfname = (char *) malloc(sigfnamelen);
    unsigned char hash[256];
    unsigned long hashlen = sizeof (hash);
    unsigned char sig[1024];
    unsigned long siglen = sizeof (sig);
    int rc = 0;
    int status = 0;

    if (!sigfname) {
        fail("out of memory");
    }

    if ((rc = hash_file(hash_index, fname, hash, &hashlen)) != CRYPT_OK) {
        fail("hash_file for '%s' failed: %s", fname, error_to_string(rc));
    }

    if ((rc = rsa_sign_hash(hash, hashlen, sig, &siglen, prng, prng_index, hash_index, SALT_LEN, key)) != CRYPT_OK) {
        fail("rsa_sign_hash for '%s' failed: %s", fname, error_to_string(rc));
    }

    if ((rc = rsa_verify_hash(sig, siglen, hash, hashlen, hash_index, SALT_LEN, &status, key)) != CRYPT_OK) {
        fail("rsa_verify_hash for '%s' failed: %s", fname, error_to_string(rc));
    }

    if (!status) {
        fail("Generated signature isn't valid! Bug in the program!");
    }

    snprintf(sigfname, sigfnamelen, "%s.sig", fname);
    write_file(sigfname, sig, siglen);
    free(sigfname);
}

int main(int argc, char **argv)
{
    int rc = 0;
    prng_state prng;
    int prng_index, hash_index;
    rsa_key key;
    int i;

    ltc_mp = tfm_desc;

    prng_index = register_prng(&sprng_desc);  /* (fortuna_desc is a good choice if your platform's PRNG sucks.) */
    if (prng_index == -1) {
        fail("Failed to register a RNG");
    }

    hash_index = register_hash(&sha256_desc);
    if (hash_index == -1) {
        fail("Failed to register sha256 hasher");
    }

    if ((rc = rng_make_prng(128, prng_index, &prng, NULL)) != CRYPT_OK) {
        fail("rng_make_prng failed: %s", error_to_string(rc));
    }

    read_rsakey(&key, "privatekey.bin");

    for (i = 1; i < argc; i++) {
        sign_file(argv[i], &key, &prng, prng_index, hash_index);
    }

    rsa_free(&key);

    return 0;
}

/* end of rsa_sign.c ... */


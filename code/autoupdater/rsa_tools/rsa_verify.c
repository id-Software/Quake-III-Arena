#include "rsa_common.h"

static void verify_file(const char *fname, rsa_key *key, const int hash_index)
{
    const size_t sigfnamelen = strlen(fname) + 5;
    char *sigfname = (char *) malloc(sigfnamelen);
    unsigned char hash[256];
    unsigned long hashlen = sizeof (hash);
    unsigned char sig[1024];
    unsigned long siglen = sizeof (sig);
    int status = 0;
    int rc = 0;

    if (!sigfname) {
        fail("out of memory");
    }

    snprintf(sigfname, sigfnamelen, "%s.sig", fname);
    read_file(sigfname, sig, &siglen);
    free(sigfname);

    if ((rc = hash_file(hash_index, fname, hash, &hashlen)) != CRYPT_OK) {
        fail("hash_file for '%s' failed: %s", fname, error_to_string(rc));
    }

    if ((rc = rsa_verify_hash(sig, siglen, hash, hashlen, hash_index, SALT_LEN, &status, key)) != CRYPT_OK) {
        fail("rsa_verify_hash for '%s' failed: %s", fname, error_to_string(rc));
    }

    if (!status) {
        fail("Invalid signature for '%s'! Don't trust this file!", fname);
    }
}

int main(int argc, char **argv)
{
    int hash_index;
    rsa_key key;
    int i;

    ltc_mp = tfm_desc;

    hash_index = register_hash(&sha256_desc);
    if (hash_index == -1) {
        fail("Failed to register sha256 hasher");
    }

    read_rsakey(&key, "publickey.bin");

    for (i = 1; i < argc; i++) {
        verify_file(argv[i], &key, hash_index);
    }

    rsa_free(&key);

    return 0;
}

/* end of rsa_verify.c ... */


/*
The code in this file is in the public domain. The rest of ioquake3
is licensed under the GPLv2. Do not mingle code, please!
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif

#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <wininet.h>
#include <tlhelp32.h>
#define PIDFMT "%u"
#define PIDFMTCAST unsigned int
typedef DWORD PID;
#else
#include <signal.h>
#include <curl/curl.h>
typedef pid_t PID;
#define PIDFMT "%llu"
#define PIDFMTCAST unsigned long long
#endif

/* If your build fails here with tomcrypt.h missing, you probably need to
   run the build-libtom script in the rsa_tools subdirectory. */
#define TFM_DESC
#define LTC_NO_ROLC
#include "tomcrypt.h"

#define PUBLICKEY_FNAME "updater-publickey.bin"
#define SALT_LEN 8
static int sha256_hash_index = 0;


#ifndef AUTOUPDATE_USER_AGENT
#define AUTOUPDATE_USER_AGENT "ioq3autoupdater/0.1"
#endif


#ifndef AUTOUPDATE_URL

#ifndef AUTOUPDATE_BASEURL
#define AUTOUPDATE_BASEURL "https://upd.ioquake3.org/updates/v1"
#endif

#ifndef AUTOUPDATE_PACKAGE 
#define AUTOUPDATE_PACKAGE "ioquake3"
#endif

#ifdef __APPLE__
#define AUTOUPDATE_PLATFORM "mac"
#elif defined(__linux__)
#define AUTOUPDATE_PLATFORM "linux"
#elif defined(_WIN32)
#define AUTOUPDATE_PLATFORM "windows"
#else
#error Please define your platform.
#endif

#ifdef __i386__
#define AUTOUPDATE_ARCH "x86"
#elif defined(__x86_64__)
#define AUTOUPDATE_ARCH "x86_64"
#else
#error Please define your platform.
#endif

#define AUTOUPDATE_URL AUTOUPDATE_BASEURL "/" AUTOUPDATE_PACKAGE "/" AUTOUPDATE_PLATFORM "/" AUTOUPDATE_ARCH "/"
#endif

#if defined(__GNUC__) || defined(__clang__)
#define NEVER_RETURNS __attribute__((noreturn))
#define PRINTF_FUNC(fmtargnum, dotargnum) __attribute__ (( format( __printf__, fmtargnum, dotargnum )))
#else
#define NEVER_RETURNS
#define PRINTF_FUNC(fmtargnum, dotargnum)
#endif


typedef struct
{
    pid_t waitforprocess;
    const char *updateself;
} Options;

static Options options;


typedef struct ManifestItem
{
    char *fname;
    unsigned char sha256[32];
    int64_t len;
    int update;
    int rollback;
    struct ManifestItem *next;
} ManifestItem;

static ManifestItem *manifest = NULL;

static void freeManifest(void)
{
    ManifestItem *item = manifest;
    manifest = NULL;

    while (item != NULL) {
        ManifestItem *next = item->next;
        free(item->fname);
        free(item);
        item = next;
    }
    manifest = NULL;
}

static const char *timestamp(void)
{
    time_t t = time(NULL);
    char *retval = asctime(localtime(&t));
    if (retval) {
        char *ptr;
        for (ptr = retval; *ptr; ptr++) {
            if ((*ptr == '\r') || (*ptr == '\n')) {
                *ptr = '\0';
                break;
            }
        }
    }
    return retval ? retval : "[date unknown]";
}


static FILE *logfile = NULL;

static void info(const char *str)
{
    fputs(str, logfile);
    fputs("\n", logfile);
    fflush(logfile);
}

static void infof(const char *fmt, ...) PRINTF_FUNC(1, 2);
static void infof(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(logfile, fmt, ap);
    va_end(ap);
    fputs("\n", logfile);
    fflush(logfile);
}

static void restoreRollbacks(void)
{
    /* you can't call die() in this function! If this fails, you're doomed. */
    ManifestItem *item;
    for (item = manifest; item != NULL; item = item->next) {
        if (item->rollback) {
            char rollbackPath[64];
            snprintf(rollbackPath, sizeof (rollbackPath), "updates/rollbacks/%d", item->rollback);
            infof("restore rollback: '%s' -> '%s'", rollbackPath, item->fname);
            remove(item->fname);
            rename(rollbackPath, item->fname);
        }
    }
}

static void die(const char *why) NEVER_RETURNS;



#ifdef _WIN32

#define chmod(a,b) do {} while (0)
#define makeDir(path) mkdir(path)

static void windowsWaitForProcessToDie(const DWORD pid)
{
    HANDLE h;
    infof("Waiting on process ID #%u", (unsigned int) pid);
    h = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!h) {
        const DWORD err = GetLastError();
        if (err == ERROR_INVALID_PARAMETER) {
            info("No such process; probably already dead. Carry on.");
            return;  /* process is (probably) already gone. */
        }
        infof("OpenProcess failed. err=%d", (unsigned int) err);
        die("OpenProcess failed");
    }
    if (WaitForSingleObject(h, INFINITE) != WAIT_OBJECT_0) {
        die("WaitForSingleObject failed");
    }
    CloseHandle(h);
}

static void launchProcess(const char *exe, ...)
{
     PROCESS_INFORMATION procinfo;
     STARTUPINFO startinfo;
     va_list ap;
     char cmdline[1024];
     char *ptr = cmdline;
     size_t totallen = 0;
     const char *arg = NULL;

     #define APPENDCMDLINE(str) { \
        const size_t len = strlen(str); \
        totallen += len; \
        if ((totallen + 1) < sizeof (cmdline)) { \
            strcpy(ptr, str); \
            ptr += len; \
        } \
     }

     va_start(ap, exe);
     APPENDCMDLINE(exe);
     while ((arg = va_arg(ap, const char *)) != NULL) {
         APPENDCMDLINE(arg);
     }
     va_end(ap);

     if (totallen >= sizeof (cmdline)) {
        die("command line too long to launch.");
     }

     cmdline[totallen] = 0;

     infof("launching process '%s' with cmdline '%s'", exe, cmdline);

     memset(&procinfo, '\0', sizeof (procinfo));
     memset(&startinfo, '\0', sizeof (startinfo));
     startinfo.cb = sizeof (startinfo);
     if (CreateProcessA(exe, cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startinfo, &procinfo))
     {
         CloseHandle(procinfo.hProcess);
         CloseHandle(procinfo.hThread);
         exit(0);  /* we're done, it's launched. */
     }

     infof("CreateProcess failed: err=%d", (int) GetLastError());
}

static HINTERNET hInternet;
static void prepHttpLib(void)
{
    hInternet = InternetOpenA(AUTOUPDATE_USER_AGENT,
                              INTERNET_OPEN_TYPE_PRECONFIG,
                              NULL, NULL, 0);
    if (!hInternet) {
        die("InternetOpen failed");
    }
}

static void shutdownHttpLib(void)
{
    if (hInternet) {
        InternetCloseHandle(hInternet);
        hInternet = NULL;
    }
}

static int runHttpDownload(const char *from, FILE *to)
{
    /* !!! FIXME: some of this could benefit from GetLastError+FormatMessage. */
    int retval = 0;
    DWORD httpcode = 0;
    DWORD dwordlen = sizeof (DWORD);
    DWORD zero = 0;
    HINTERNET hUrl = InternetOpenUrlA(hInternet, from, NULL, 0,
                                INTERNET_FLAG_HYPERLINK |
                                INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
                                INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
                                INTERNET_FLAG_NO_CACHE_WRITE |
                                INTERNET_FLAG_NO_COOKIES |
                                INTERNET_FLAG_NO_UI |
                                INTERNET_FLAG_RESYNCHRONIZE |
                                INTERNET_FLAG_RELOAD |
                                INTERNET_FLAG_SECURE, 0);

    if (!hUrl) {
        infof("InternetOpenUrl failed. err=%d", (int) GetLastError());
    } else if (!HttpQueryInfo(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &httpcode, &dwordlen, &zero)) {
        infof("HttpQueryInfo failed. err=%d", (int) GetLastError());
    } else if (httpcode != 200) {
        infof("HTTP request failed with response code %d", (int) httpcode);
    } else {
        while (1) {
            DWORD br = 0;
            BYTE buf[1024 * 64];
            if (!InternetReadFile(hUrl, buf, sizeof (buf), &br)) {
                infof("InternetReadFile failed. err=%d", (int) GetLastError());
                break;
            } else if (br == 0) {
                retval = 1;
                break;  /* done! */
            } else {
                if (fwrite(buf, br, 1, to) != 1) {
                    info("fwrite failed");
                    break;
                }
            }
        }
    }

    InternetCloseHandle(hUrl);
    return retval;
}

#else  /* Everything that isn't Windows. */

#define launchProcess execl
#define makeDir(path) mkdir(path, 0777)

/* hooray for Unix linker hostility! */
#undef curl_easy_setopt
#include <dlfcn.h>
typedef void (*CURLFN_curl_easy_cleanup)(CURL *curl);
typedef CURL *(*CURLFN_curl_easy_init)(void);
typedef CURLcode (*CURLFN_curl_easy_setopt)(CURL *curl, CURLoption option, ...);
typedef CURLcode (*CURLFN_curl_easy_perform)(CURL *curl);
typedef CURLcode (*CURLFN_curl_global_init)(long flags);
typedef void (*CURLFN_curl_global_cleanup)(void);

static CURLFN_curl_easy_cleanup CURL_curl_easy_cleanup;
static CURLFN_curl_easy_init CURL_curl_easy_init;
static CURLFN_curl_easy_setopt CURL_curl_easy_setopt;
static CURLFN_curl_easy_perform CURL_curl_easy_perform;
static CURLFN_curl_global_init CURL_curl_global_init;
static CURLFN_curl_global_cleanup CURL_curl_global_cleanup;

static void prepHttpLib(void)
{
    #ifdef __APPLE__
    const char *libname = "libcurl.4.dylib";
    #else
    const char *libname = "libcurl.so.4";
    #endif

    void *handle = dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        infof("dlopen(\"%s\") failed: %s", libname, dlerror());
        die("Failed to load libcurl library");
    }
    #define LOADCURLSYM(fn) \
        if ((CURL_##fn = (CURLFN_##fn) dlsym(handle, #fn)) == NULL) { \
            die("Failed to load libcurl symbol '" #fn "'"); \
        }

    LOADCURLSYM(curl_easy_cleanup);
    LOADCURLSYM(curl_easy_init);
    LOADCURLSYM(curl_easy_setopt);
    LOADCURLSYM(curl_easy_perform);
    LOADCURLSYM(curl_global_init);
    LOADCURLSYM(curl_global_cleanup);

    #define curl_easy_cleanup CURL_curl_easy_cleanup
    #define curl_easy_init CURL_curl_easy_init
    #define curl_easy_setopt CURL_curl_easy_setopt
    #define curl_easy_perform CURL_curl_easy_perform
    #define curl_global_init CURL_curl_global_init
    #define curl_global_cleanup CURL_curl_global_cleanup

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        die("curl_global_init() failed!");
    }
}

static void shutdownHttpLib(void)
{
    if (curl_global_cleanup) {
        curl_global_cleanup();
    }
}

static int runHttpDownload(const char *from, FILE *to)
{
    int retval;
    CURL *curl = curl_easy_init();
    if (!curl) {
        info("curl_easy_init() failed");
        return 0;
    }

    #if 0
    /* !!! FIXME: enable compression? */
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");  /* enable compression */

    /* !!! FIXME; hook up proxy support to libcurl */
    curl_easy_setopt(curl, CURLOPT_PROXY, proxyURL);
    #endif

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_STDERR, logfile);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, to);

    curl_easy_setopt(curl, CURLOPT_URL, from);

    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  /* allow redirects. */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, AUTOUPDATE_USER_AGENT);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);  /* require valid SSL cert. */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);  /* require SSL cert with same hostname as we connected to. */

    retval = (curl_easy_perform(curl) == CURLE_OK);
    curl_easy_cleanup(curl);
    return retval;
}
#endif


static void die(const char *why)
{
    infof("FAILURE: %s", why);
    restoreRollbacks();
    freeManifest();
    infof("Updater ending (in failure), %s", timestamp());
    exit(1);
}

static void outOfMemory(void) NEVER_RETURNS;
static void outOfMemory(void)
{
    die("Out of memory");
}

static void buildParentDirs(const char *_path)
{
    char *ptr;
    char *path = (char *) alloca(strlen(_path) + 1);
    if (!path) {
        outOfMemory();
    }
    strcpy(path, _path);

    for (ptr = path; *ptr; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
            makeDir(path);
            *ptr = '/';
        }
    }
}

static int64_t fileLength(const char *fname)
{
    struct stat statbuf;
    if (stat(fname, &statbuf) == -1) {
        return -1;
    }
    return (int64_t) statbuf.st_size;
}

static void parseArgv(int argc, char **argv)
{
    int i;

    infof("command line (argc=%d)...", argc);
    for (i = 0; i < argc; i++) {
        infof("  argv[%d]: %s",i, argv[i]);
    }

    for (i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "--waitpid") == 0) {
            options.waitforprocess = atoll(argv[i + 1]);
            infof("We will wait for process " PIDFMT " if necessary", (PIDFMTCAST) options.waitforprocess);
        } else if (strcmp(argv[i], "--updateself") == 0) {
            options.updateself = argv[i + 1];
            infof("We are updating ourself ('%s')", options.updateself);
        }
    }
}

static void downloadURL(const char *from, const char *to)
{
    FILE *io = NULL;
    const size_t len = strlen(AUTOUPDATE_URL) + strlen(from) + 1;
    char *fullurl = (char *) alloca(len);
    if (!fullurl) {
        outOfMemory();
    }
    snprintf(fullurl, len, "%s%s", AUTOUPDATE_URL, from);

    infof("Downloading from '%s' to '%s'", fullurl, to);

    buildParentDirs(to);
    io = fopen(to, "wb");
    if (!io) {
        die("Failed to open output file");
    }

    if (!runHttpDownload(fullurl, io)) {
        fclose(io);
        remove(to);
        die("Download failed");
    }

    if (fclose(io) == EOF) {
        die("Can't flush file on close. i/o error? Disk full?");
    }

    chmod(to, 0777);  /* !!! FIXME */
}

static int hexcvt(const int ch)
{
    if ((ch >= 'a') && (ch <= 'f')) {
        return (ch - 'a') + 10;
    } else if ((ch >= 'A') && (ch <= 'F')) {
        return (ch - 'A') + 10;
    } else if ((ch >= '0') && (ch <= '9')) {
        return ch - '0';
    } else {
        die("Invalid hex character");
    }
    return 0;
}

static void convertSha256(char *str, unsigned char *sha256)
{
    int i;
    for (i = 0; i < 32; i++) {
        const int a = hexcvt(*(str++));
        const int b = hexcvt(*(str++));
        *sha256 = (a << 4) | b;
        sha256++;
    }
}

static void parseManifest(const char *fname)
{
    ManifestItem *item = NULL;
    FILE *io = fopen(fname, "r");
    char buf[512];
    if (!io) {
        die("Failed to open manifest for reading");
    }

    /* !!! FIXME: this code sucks. */
    while (fgets(buf, sizeof (buf), io)) {
        char *ptr = (buf + strlen(buf)) - 1;
        while (ptr >= buf) {
            if ((*ptr != '\n') && (*ptr != '\r')) {
                break;
            }
            *ptr = '\0';
            ptr--;
        }

        if (!item && !buf[0]) {
            continue;  /* blank line between items or blank at EOF */
        }

        if (!item) {
            infof("Next manifest item: %s", buf);

            item = (ManifestItem *) calloc(1, sizeof (ManifestItem));
            if (!item) {
                outOfMemory();
            }
            item->fname = strdup(buf);
            if (!item->fname) {
                outOfMemory();
            }
            item->len = -1;
            item->next = NULL;
        } else if (item->len == -1) {
            infof("Item size: %s", buf);
            item->len = atoll(buf);
        } else {
            infof("Item sha256: %s", buf);
            convertSha256(buf, item->sha256);
            item->next = manifest;
            manifest = item;
            item = NULL;
        }
    }

    if (ferror(io)) {
        die("Error reading manifest");
    } else if (item) {
        die("Incomplete manifest");
    }

    fclose(io);
}

static void read_file(const char *fname, void *buf, unsigned long *len)
{
    ssize_t br;
    FILE *io = fopen(fname, "rb");
    if (!io) {
        infof("Can't open '%s' for reading: %s", fname, strerror(errno));
        die("Failed to read file");
    }

    br = fread(buf, 1, *len, io);
    if (ferror(io)) {
        infof("Couldn't read '%s': %s", fname, strerror(errno));
        die("Failed to read file");
    } else if (!feof(io)) {
        infof("Buffer too small to read '%s'", fname);
        die("Failed to read file");
    }
    fclose(io);

    *len = (unsigned long) br;
}

static void read_rsakey(rsa_key *key, const char *fname)
{
    unsigned char buf[4096];
    unsigned long len = sizeof (buf);
    int rc;

    read_file(fname, buf, &len);

    if ((rc = rsa_import(buf, len, key)) != CRYPT_OK) {
        infof("rsa_import for '%s' failed: %s", fname, error_to_string(rc));
        die("Couldn't import public key");
    }
}

static void verifySignature(const char *fname, const char *sigfname, const char *keyfname)
{
    rsa_key key;
    unsigned char hash[256];
    unsigned long hashlen = sizeof (hash);
    unsigned char sig[1024];
    unsigned long siglen = sizeof (sig);
    int status = 0;
    int rc = 0;

    read_rsakey(&key, keyfname);
    read_file(sigfname, sig, &siglen);

    if ((rc = hash_file(sha256_hash_index, fname, hash, &hashlen)) != CRYPT_OK) {
        infof("hash_file for '%s' failed: %s", fname, error_to_string(rc));
        die("Couldn't verify manifest signature");
    }

    if ((rc = rsa_verify_hash(sig, siglen, hash, hashlen, sha256_hash_index, SALT_LEN, &status, &key)) != CRYPT_OK) {
        infof("rsa_verify_hash for '%s' failed: %s", fname, error_to_string(rc));
        die("Couldn't verify manifest signature");
    }

    if (!status) {
        infof("Invalid signature for '%s'! Don't trust this file!", fname);
        die("Manifest is incomplete, corrupt, or compromised");
    }

    info("Manifest signature appears to be valid");
    rsa_free(&key);
}

static void downloadManifest(void)
{
    const char *manifestfname = "updates/manifest.txt";
    const char *manifestsigfname = "updates/manifest.txt.sig";
    downloadURL("manifest.txt", manifestfname);
    downloadURL("manifest.txt.sig", manifestsigfname);
    verifySignature(manifestfname, manifestsigfname, PUBLICKEY_FNAME);
    parseManifest(manifestfname);
}

static void upgradeSelfAndRestart(const char *argv0) NEVER_RETURNS;
static void upgradeSelfAndRestart(const char *argv0)
{
    const char *tempfname = "origUpdater";
    const char *why = NULL;
    FILE *in = NULL;
    FILE *out = NULL;

    /* unix replaces the process with execl(), but Windows needs to wait for the parent to terminate. */
    #ifdef _WIN32
    DWORD ppid = 0;
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (h) {
        const DWORD myPid = GetCurrentProcessId();
        PROCESSENTRY32 pe;
        memset(&pe, '\0', sizeof (pe));
        pe.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(h, &pe)) {
            do {
                if (pe.th32ProcessID == myPid) {
                    ppid = pe.th32ParentProcessID;
                    break;
                }
            } while (Process32Next(h, &pe));
        }
        CloseHandle(h);
    }
    if (!ppid) {
        die("Can't determine parent process id");
    }
    windowsWaitForProcessToDie(ppid);
    #endif

    in = fopen(argv0, "rb");
    if (!in) {
        die("Can't open self for input while upgrading updater");
    }

    remove(tempfname);
    if (rename(options.updateself, tempfname) == -1) {
        die("Can't rename original while upgrading updater");
    }

    out = fopen(options.updateself, "wb");
    if (!out) {
        die("Can't open file for output while upgrading updater");
    }

    while (!feof(in) && !why) {
        char buf[512];
        const size_t br = fread(buf, 1, sizeof (buf), in);
        if (br > 0) {
            if (fwrite(buf, br, 1, out) != 1) {
                why = "write failure while upgrading updater";
            }
        } else if (ferror(in)) {
            why = "read failure while upgrading updater";
        }
    }

    fclose(in);

    if ((fclose(out) == EOF) && (!why)) {
        why = "close failure while upgrading updater";
    }

    if (why) {
        remove(options.updateself);
        rename(tempfname, options.updateself);
        die(why);
    }

    remove(tempfname);

    chmod(options.updateself, 0777);

    if (options.waitforprocess) {
        char pidstr[64];
        snprintf(pidstr, sizeof (pidstr), PIDFMT, (PIDFMTCAST) options.waitforprocess);
        launchProcess(options.updateself, options.updateself, "--waitpid", pidstr, NULL);
    } else {
        launchProcess(options.updateself, options.updateself, NULL);
    }
    die("Failed to relaunch upgraded updater");
}

static const char *justFilename(const char *path)
{
    const char *fname = strrchr(path, '/');
    return fname ? fname + 1 : path;
}

static void hashFile(const char *fname, unsigned char *sha256)
{
    int rc = 0;
    unsigned long hashlen = 32;
    if ((rc = hash_file(sha256_hash_index, fname, sha256, &hashlen)) != CRYPT_OK) {
        infof("hash_file failed for '%s': %s", fname, error_to_string(rc));
        die("Can't hash file");
    }
}

static int fileHashMatches(const char *fname, const unsigned char *wanted)
{
    unsigned char sha256[32];
    hashFile(fname, sha256);
    return (memcmp(sha256, wanted, 32) == 0);
}

static int fileNeedsUpdate(const ManifestItem *item)
{
    if (item->len != fileLength(item->fname)) {
        infof("Update '%s', file size is different", item->fname);
        return 1;  /* obviously different. */
    } else if (!fileHashMatches(item->fname, item->sha256)) {
        infof("Update '%s', file sha256 is different", item->fname);
        return 1;
    }

    infof("Don't update '%s', the file is already up to date", item->fname);
    return 0;
}

static void downloadFile(const ManifestItem *item)
{
    const char *outpath = "updates/downloads/";
    const size_t len = strlen(outpath) + strlen(item->fname) + 1;
    char *to = (char *) alloca(len);
    if (!to) {
        outOfMemory();
    }

    snprintf(to, len, "%s%s", outpath, item->fname);

    if ((item->len == fileLength(to)) && fileHashMatches(to, item->sha256)) {
        infof("Already downloaded '%s', not getting again", item->fname);
    } else {
        downloadURL(item->fname, to);
        if ((item->len != fileLength(to)) || !fileHashMatches(to, item->sha256)) {
            die("Download is incorrect or corrupted");
        }
    }
}

static int downloadUpdates(void)
{
    int updatesAvailable = 0;
    ManifestItem *item;
    for (item = manifest; item != NULL; item = item->next) {
        item->update = fileNeedsUpdate(item);
        if (item->update) {
            updatesAvailable = 1;
            downloadFile(item);
        }
    }
    return updatesAvailable;
}

static void maybeUpdateSelf(const char *argv0)
{
    ManifestItem *item;

    /* !!! FIXME: this needs to be a different string on macOS. */
    const char *fname = justFilename(argv0);

    for (item = manifest; item != NULL; item = item->next) {
        if (strcasecmp(item->fname, fname) == 0) {
            if (fileNeedsUpdate(item)) {
                const char *outpath = "updates/downloads/";
                const size_t len = strlen(outpath) + strlen(item->fname) + 1;
                char *to = (char *) alloca(len);
                if (!to) {
                    outOfMemory();
                }
                snprintf(to, len, "%s%s", outpath, item->fname);
                info("Have to upgrade the updater");
                downloadFile(item);
                chmod(to, 0777);

                if (options.waitforprocess) {
                    char pidstr[64];
                    snprintf(pidstr, sizeof (pidstr), PIDFMT, (PIDFMTCAST) options.waitforprocess);
                    launchProcess(to, to, "--updateself", argv0, "--waitpid", pidstr, NULL);
                } else {
                    launchProcess(to, to, "--updateself", argv0, NULL);
                }
                die("Failed to initially launch upgraded updater");
            }
            break;  /* done in any case. */
        }
    }
}

static void installUpdatedFile(const ManifestItem *item)
{
    const char *basepath = "updates/downloads/";
    const size_t len = strlen(basepath) + strlen(item->fname) + 1;
    char *downloadPath = (char *) alloca(len);
    if (!downloadPath) {
        outOfMemory();
    }

    snprintf(downloadPath, len, "%s%s", basepath, item->fname);

    infof("Moving file for update: '%s' -> '%s'", downloadPath, item->fname);
    buildParentDirs(item->fname);
    if (rename(downloadPath, item->fname) == -1) {
        die("Failed to move updated file to final position");
    }
}

static void applyUpdates(void)
{
    FILE *io;
    ManifestItem *item;
    for (item = manifest; item != NULL; item = item->next) {
        if (!item->update) {
            continue;
        }

        io = fopen(item->fname, "rb");
        if (io != NULL) {
            static int rollbackIndex = 0;
            char rollbackPath[64];
            fclose(io);
            item->rollback = ++rollbackIndex;
            snprintf(rollbackPath, sizeof (rollbackPath), "updates/rollbacks/%d", rollbackIndex);
            infof("Moving file for rollback: '%s' -> '%s'", item->fname, rollbackPath);
            remove(rollbackPath);
            if (rename(item->fname, rollbackPath) == -1) {
                die("failed to move to rollback dir");
            }
        }

        installUpdatedFile(item);
    }
}


static void waitToApplyUpdates(void)
{
    if (options.waitforprocess) {
        infof("Waiting for pid " PIDFMT " to die...", (PIDFMTCAST) options.waitforprocess);
        {
            #ifdef _WIN32
            windowsWaitForProcessToDie(options.waitforprocess);
            #else
            /* The parent opens a pipe on fd 3, and then forgets about it. We block
               on a read to that pipe here. When the game process quits (and the
               OS forcibly closes the pipe), we will unblock. Then we can loop on
               kill() until the process is truly gone. */
            int x = 0;
            read(3, &x, sizeof (x));
            info("Pipe has closed, waiting for process to fully go away now.");
            while (kill(options.waitforprocess, 0) == 0) {
                usleep(100000);
            }
            #endif
        }
        info("pid is gone, continuing");
    }
}

static void deleteRollbacks(void)
{
    ManifestItem *item;
    for (item = manifest; item != NULL; item = item->next) {
        if (item->rollback) {
            char rollbackPath[64];
            snprintf(rollbackPath, sizeof (rollbackPath), "updates/rollbacks/%d", item->rollback);
            infof("delete rollback: %s", rollbackPath);
            remove(rollbackPath);
        }
    }
}

static void chdirToBasePath(const char *argv0)
{
    const char *fname = justFilename(argv0);
    size_t len;
    char *buf;

    if (fname == argv0) { /* no path? Assume we're already there. */
        return;
    }

    len = ((size_t) (fname - argv0)) - 1;
    buf = (char *) alloca(len);
    if (!buf) {
        outOfMemory();
    }

    memcpy(buf, argv0, len);
    buf[len] = '\0';
    if (chdir(buf) == -1) {
        infof("base path is '%s'", buf);
        die("chdir to base path failed");
    }
}

int main(int argc, char **argv)
{
    #ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);  /* don't trigger signal when fd3 closes */
    #endif

    logfile = stdout;
    chdirToBasePath(argv[0]);

    makeDir("updates");
    makeDir("updates/downloads");
    makeDir("updates/rollbacks");

    logfile = fopen("updates/updater-log.txt", "a");
    if (!logfile) {
        logfile = stdout;
    }

    infof("Updater starting, %s", timestamp());

    parseArgv(argc, argv);

    /* set up crypto */
    ltc_mp = tfm_desc;
    sha256_hash_index = register_hash(&sha256_desc);
    if (sha256_hash_index == -1) {
        die("Failed to register sha256 hasher");
    }

    /* if we have downloaded a new updater and restarted with that binary,
       replace the original updater and restart again in the right place. */
    if (options.updateself) {
        upgradeSelfAndRestart(argv[0]);
    }

    prepHttpLib();

    downloadManifest();  /* see if we need an update at all. */

    maybeUpdateSelf(argv[0]);  /* might relaunch if there's an updater upgrade. */

    if (!downloadUpdates()) {
        info("Nothing needs updating, so we're done here!");
    } else {
        waitToApplyUpdates();
        applyUpdates();
        deleteRollbacks();
        info("You are now up to date!");
    }

    freeManifest();
    shutdownHttpLib();

    unregister_hash(&sha256_desc);

    infof("Updater ending, %s", timestamp());

    return 0;
}


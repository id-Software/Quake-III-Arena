/*
The code in this file is in the public domain. The rest of ioquake3
is licensed until the GPLv2. Do not mingle code, please!
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>

#include <curl/curl.h>
#include "sha256.h"

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
#else
#error Please define your platform.
#endif

#ifdef __i386__
#define AUTOUPDATE_ARCH "i386"
#elif defined(__x86_64__)
#define AUTOUPDATE_ARCH "x86-64"
#else
#error Please define your platform.
#endif

#define AUTOUPDATE_URL AUTOUPDATE_BASEURL "/" AUTOUPDATE_PACKAGE "/" AUTOUPDATE_PLATFORM "/" AUTOUPDATE_ARCH "/"
#endif

#if defined(__GNUC__) || defined(__clang__)
#define NEVER_RETURNS __attribute__((noreturn))
#else
#define NEVER_RETURNS
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

#define SDL_PRINTF_VARARG_FUNC( fmtargnumber ) __attribute__ (( format( __printf__, fmtargnumber, fmtargnumber+1 )))

static void info(const char *str)
{
    fputs(str, logfile);
    fputs("\n", logfile);
    fflush(logfile);
}

static void infof(const char *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
__attribute__ (( format( __printf__, 1, 2 )))
#endif
;

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
static void die(const char *why)
{
    infof("FAILURE: %s", why);
    curl_global_cleanup();
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

static void makeDir(const char *dirname)
{
    /* !!! FIXME: we don't care if this fails right now. */
    mkdir(dirname, 0777);
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
            infof("We will wait for process %lld if necessary", (long long) options.waitforprocess);
        } else if (strcmp(argv[i], "--updateself") == 0) {
            options.updateself = argv[i + 1];
            infof("We are updating ourself ('%s')", options.updateself);
        }
    }
}

static CURL *prepCurl(const char *url, FILE *outfile)
{
    char *fullurl;
    const size_t len = strlen(AUTOUPDATE_URL) + strlen(url) + 1;
    CURL *curl = curl_easy_init();
    if (!curl) {
        die("curl_easy_init() failed");
    }

    fullurl = (char *) alloca(len);
    if (!fullurl) {
        outOfMemory();
    }

    snprintf(fullurl, len, "%s%s", AUTOUPDATE_URL, url);

    infof("Downloading from '%s'", fullurl);

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
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outfile);

    curl_easy_setopt(curl, CURLOPT_URL, fullurl);

    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  /* allow redirects. */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, AUTOUPDATE_USER_AGENT);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);  /* require valid SSL cert. */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);  /* require SSL cert with same hostname as we connected to. */

    return curl;
}

static void downloadURL(const char *from, const char *to)
{
    FILE *io;
    CURL *curl;

    infof("Preparing to download to '%s'", to);

    buildParentDirs(to);
    io = fopen(to, "wb");
    if (!io) {
        die("Failed to open output file");
    }

    curl = prepCurl(from, io);
    if (curl_easy_perform(curl) != CURLE_OK) {
        remove(to);
        die("Download failed");
    }
    curl_easy_cleanup(curl);

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

static void convertSha256(char *str, BYTE *sha256)
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

            item = (ManifestItem *) malloc(sizeof (ManifestItem));
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

static void downloadManifest(void)
{
    const char *manifestfname = "updates/manifest.txt";
    downloadURL("manifest.txt", manifestfname);
    /* !!! FIXME: verify manifest download is complete... */
    parseManifest(manifestfname);
}

static void upgradeSelfAndRestart(const char *argv0) NEVER_RETURNS;
static void upgradeSelfAndRestart(const char *argv0)
{
    const char *tempfname = "origUpdater";
    const char *why = NULL;
    FILE *in = NULL;
    FILE *out = NULL;

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
        snprintf(pidstr, sizeof (pidstr), "%lld", (long long) options.waitforprocess);
        execl(options.updateself, options.updateself, "--waitpid", pidstr, NULL);
    } else {
        execl(options.updateself, options.updateself, NULL);
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
    SHA256_CTX sha256ctx;
    BYTE buf[512];
    FILE *io;

    io = fopen(fname, "rb");
    if (!io) {
        die("Failed to open file for hashing");
    }

    sha256_init(&sha256ctx);
    do {
        size_t br = fread(buf, 1, sizeof (buf), io);
        if (br > 0) {
            sha256_update(&sha256ctx, buf, br);
        }
        if (ferror(io)) {
            die("Error reading file for hashing");
        }
    } while (!feof(io));

    fclose(io);

    sha256_final(&sha256ctx, sha256);
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
                    snprintf(pidstr, sizeof (pidstr), "%lld", (long long) options.waitforprocess);
                    execl(to, to, "--updateself", argv0, "--waitpid", pidstr, NULL);
                } else {
                    execl(to, to, "--updateself", argv0, NULL);
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
        fclose(io);
        if (io != NULL) {
            static int rollbackIndex = 0;
            char rollbackPath[64];
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
        /* ioquake3 opens a pipe on fd 3, and then forgets about it. We block
           on a read to that pipe here. When the game process quits (and the
           OS forcibly closes the pipe), we will unblock. Then we can loop on
           kill() until the process is truly gone. */
        int x = 0;
        infof("Waiting for pid %lld to die...", (long long) options.waitforprocess);
        read(3, &x, sizeof (x));
        info("Pipe has closed, waiting for process to fully go away now.");
        while (kill(options.waitforprocess, 0) == 0) {
            usleep(100000);
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
    signal(SIGPIPE, SIG_IGN);  /* don't trigger signal when fd3 closes */

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

    /* if we have downloaded a new updater and restarted with that binary,
       replace the original updater and restart again in the right place. */
    if (options.updateself) {
        upgradeSelfAndRestart(argv[0]);
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        die("curl_global_init() failed!");
    }

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
    curl_global_cleanup();

    infof("Updater ending, %s", timestamp());

    return 0;
}


/* SPDX-License-Identifier: GPL-3.0-only */
/*
 * bbslink_client - BBSlink client for UNIX-like systems.
 *
 * Implements the documented BBSlink token/authentication exchange.
 * Terminal I/O is handled by an external telnet(1) implementation.
 *
 * The legacy BBSlink protocol uses plaintext HTTP and Telnet.
 */

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <curl/curl.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#ifndef BBSLINK_HOST
#define BBSLINK_HOST "games.bbslink.net"
#endif
#ifndef BBSLINK_HTTP_BASE
#define BBSLINK_HTTP_BASE "http://" BBSLINK_HOST
#endif
#ifndef BBSLINK_TELNET_BIN
#define BBSLINK_TELNET_BIN "telnet"
#endif
#ifndef BBSLINK_TELNET_PORT
#define BBSLINK_TELNET_PORT "23"
#endif

#define CLIENT_VERSION "0.1"
#define MAX_CODE 128
#define MAX_REPLY 4096

struct credentials {
    char syscode[MAX_CODE];
    char authcode[MAX_CODE];
    char schemecode[MAX_CODE];
};

struct reply {
    char data[MAX_REPLY];
    size_t len;
};

static void
usage(FILE *out)
{
    fprintf(out, "usage: bbslink_client -c file [-r rows] door user-number\n"
                 "       bbslink_client -h\n");
}

static char *
trim(char *s)
{
    char *end;
    while (isspace((unsigned char)*s)) s++;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) *--end = '\0';
    return s;
}

static int
copy_code(char *dst, size_t cap, const char *src)
{
    size_t n = strlen(src), i;
    if (n == 0 || n >= cap) return -1;
    for (i = 0; i < n; ++i) {
        unsigned char ch = (unsigned char)src[i];
        if (ch < 33 || ch > 126) return -1;
    }
    memcpy(dst, src, n + 1);
    return 0;
}

static int
load_credentials(const char *path, struct credentials *c)
{
    FILE *f;
    struct stat st;
    char line[512];
    int fd, ok = 0;
    unsigned int seen = 0;

    memset(c, 0, sizeof(*c));
#ifdef O_NOFOLLOW
    fd = open(path, O_RDONLY | O_NOFOLLOW);
#else
    fd = open(path, O_RDONLY);
#endif
    if (fd < 0) {
        fprintf(stderr, "bbslink_client: cannot open credentials: %s\n", strerror(errno));
        return -1;
    }
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || (st.st_mode & 077) != 0) {
        fprintf(stderr, "bbslink_client: credentials must be a private regular file (mode 0600 or 0400)\n");
        close(fd);
        return -1;
    }
    f = fdopen(fd, "r");
    if (f == NULL) {
        close(fd);
        return -1;
    }
    while (fgets(line, sizeof(line), f)) {
        char *key, *value, *equals;
        if (!strchr(line, '\n') && !feof(f)) goto done;
        key = trim(line);
        if (*key == '\0' || *key == '#') continue;
        equals = strchr(key, '=');
        if (equals == NULL) goto done;
        *equals = '\0';
        value = trim(equals + 1);
        key = trim(key);
        if (strcmp(key, "syscode") == 0) {
            if ((seen & 1) || copy_code(c->syscode, sizeof(c->syscode), value)) goto done;
            seen |= 1;
        } else if (strcmp(key, "authcode") == 0) {
            if ((seen & 2) || copy_code(c->authcode, sizeof(c->authcode), value)) goto done;
            seen |= 2;
        } else if (strcmp(key, "schemecode") == 0) {
            if ((seen & 4) || copy_code(c->schemecode, sizeof(c->schemecode), value)) goto done;
            seen |= 4;
        } else {
            goto done;
        }
    }
    if (!ferror(f) && seen == 7) ok = 1;
done:
    fclose(f);
    if (!ok) fprintf(stderr, "bbslink_client: invalid credentials file\n");
    return ok ? 0 : -1;
}

static int
make_nonce(char out[7])
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    unsigned char byte;
    size_t i = 0;

    while (i < 6) {
        if (RAND_bytes(&byte, 1) != 1) return -1;
        if (byte < 248) out[i++] = alphabet[byte % 62];
    }
    out[6] = '\0';
    return 0;
}

static int
make_signature(const char *secret, const char *token, char hex[33])
{
    static const char hexdigits[] = "0123456789abcdef";
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    size_t a = strlen(secret), b = strlen(token);
    char *buf;
    unsigned int i;

    if (a > 1024 || b > 1024) return -1;
    buf = malloc(a + b + 1);
    if (buf == NULL) return -1;
    memcpy(buf, secret, a);
    memcpy(buf + a, token, b);
    buf[a + b] = '\0';
    i = EVP_Digest(buf, a + b, digest, &digest_len, EVP_md5(), NULL);
    OPENSSL_cleanse(buf, a + b + 1);
    free(buf);
    if (i != 1 || digest_len != 16) return -1;

    for (i = 0; i < digest_len; ++i) {
        hex[2 * i] = hexdigits[digest[i] >> 4];
        hex[2 * i + 1] = hexdigits[digest[i] & 15];
    }
    hex[32] = '\0';
    OPENSSL_cleanse(digest, sizeof(digest));
    return 0;
}

static size_t
receive_reply(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct reply *r = userdata;
    size_t n;
    if (size && nmemb > (size_t)-1 / size) return 0;
    n = size * nmemb;
    if (n > MAX_REPLY - r->len - 1) return 0;
    memcpy(r->data + r->len, ptr, n);
    r->len += n;
    r->data[r->len] = '\0';
    return n;
}

static int
http_get(CURL *curl, const char *endpoint, struct curl_slist *headers,
         struct reply *r)
{
    char url[256];
    CURLcode rc;
    long status = 0;
    int n = snprintf(url, sizeof(url), "%s/%s", BBSLINK_HTTP_BASE, endpoint);
    if (n < 0 || (size_t)n >= sizeof(url)) return -1;
    memset(r, 0, sizeof(*r));
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive_reply);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, r);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 12L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "bbslink_client/" CLIENT_VERSION);
    rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        fprintf(stderr, "bbslink_client: HTTP request failed: %s\n", curl_easy_strerror(rc));
        return -1;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (status != 200) {
        fprintf(stderr, "bbslink_client: HTTP status %ld\n", status);
        return -1;
    }
    return 0;
}

static int
add_header(struct curl_slist **list, const char *key, const char *value)
{
    char line[512];
    struct curl_slist *next;
    int n = snprintf(line, sizeof(line), "%s: %s", key, value);
    if (n < 0 || (size_t)n >= sizeof(line)) return -1;
    next = curl_slist_append(*list, line);
    if (next == NULL) return -1;
    *list = next;
    return 0;
}

static int
valid_identifier(const char *s, int digits_only)
{
    size_t n = strlen(s), i;
    if (n == 0 || n > 40) return 0;
    for (i = 0; i < n; ++i) {
        unsigned char ch = (unsigned char)s[i];
        if (digits_only) {
            if (ch < '0' || ch > '9') return 0;
        } else if (!((ch >= '0' && ch <= '9') ||
                     (ch >= 'A' && ch <= 'Z') ||
                     (ch >= 'a' && ch <= 'z') ||
                     ch == '_' || ch == '-')) {
            return 0;
        }
    }
    return 1;
}

static int
parse_rows(const char *s, int *rows)
{
    char *end;
    long v;
    if (*s == '\0') return -1;
    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0 || *end != '\0' || v < 10 || v > 200) return -1;
    *rows = (int)v;
    return 0;
}

static int
default_rows(void)
{
#ifdef TIOCGWINSZ
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row >= 10)
        return ws.ws_row <= 200 ? (int)ws.ws_row : 200;
#endif
    return 24;
}

int
main(int argc, char **argv)
{
    struct credentials creds;
    struct curl_slist *headers = NULL;
    struct reply result;
    CURL *curl = NULL;
    char nonce[7], endpoint[64], auth[33], scheme[33], rows_header[8];
    char *token;
    const char *config = NULL;
    const char *door, *user;
    int option, rows = 0, authenticated = 0, curl_initialized = 0;

    while ((option = getopt(argc, argv, "c:r:h")) != -1) {
        switch (option) {
        case 'c': config = optarg; break;
        case 'r':
            if (parse_rows(optarg, &rows) != 0) {
                fprintf(stderr, "bbslink_client: invalid row count\n");
                return 2;
            }
            break;
        case 'h': usage(stdout); return 0;
        default: usage(stderr); return 2;
        }
    }
    if (config == NULL || argc - optind != 2 ||
        !valid_identifier(argv[optind], 0) ||
        !valid_identifier(argv[optind + 1], 1)) {
        usage(stderr);
        return 2;
    }
    door = argv[optind];
    user = argv[optind + 1];
    if (!rows) rows = default_rows();
    snprintf(rows_header, sizeof(rows_header), "%d", rows);

    if (getuid() != geteuid() || getgid() != getegid()) {
        fprintf(stderr, "bbslink_client: setuid/setgid execution is not supported\n");
        return 1;
    }
    if (load_credentials(config, &creds) != 0 || make_nonce(nonce) != 0) {
        OPENSSL_cleanse(&creds, sizeof(creds));
        return 1;
    }
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) goto done;
    curl_initialized = 1;
    curl = curl_easy_init();
    if (curl == NULL) goto done;

    snprintf(endpoint, sizeof(endpoint), "token.php?key=%s", nonce);
    if (http_get(curl, endpoint, NULL, &result) != 0) goto done;
    token = trim(result.data);
    if (strlen(token) < 1 || strlen(token) > 256 ||
        strchr(token, '\r') != NULL || strchr(token, '\n') != NULL) {
        fprintf(stderr, "bbslink_client: invalid authentication token\n");
        goto done;
    }
    if (make_signature(creds.authcode, token, auth) != 0 ||
        make_signature(creds.schemecode, token, scheme) != 0) goto done;

#define ADD(k, v) do { if (add_header(&headers, (k), (v)) != 0) goto done; } while (0)
    ADD("X-User", user);
    ADD("X-System", creds.syscode);
    ADD("X-Auth", auth);
    ADD("X-Code", scheme);
    ADD("X-Rows", rows_header);
    ADD("X-Key", nonce);
    ADD("X-Door", door);
    ADD("X-Token", token);
    ADD("X-Type", "C");
    ADD("X-Version", CLIENT_VERSION);
#undef ADD

    snprintf(endpoint, sizeof(endpoint), "auth.php?key=%s", nonce);
    if (http_get(curl, endpoint, headers, &result) != 0) goto done;
    if (strcmp(trim(result.data), "complete") != 0) {
        fprintf(stderr, "bbslink_client: authentication rejected\n");
        goto done;
    }
    authenticated = 1;

done:
    curl_slist_free_all(headers);
    if (curl != NULL) curl_easy_cleanup(curl);
    if (curl_initialized) curl_global_cleanup();
    OPENSSL_cleanse(&creds, sizeof(creds));
    OPENSSL_cleanse(auth, sizeof(auth));
    OPENSSL_cleanse(scheme, sizeof(scheme));
    OPENSSL_cleanse(&result, sizeof(result));
    if (!authenticated) return 1;

    /* Retain stdin, stdout, stderr and the caller's terminal environment. */
    {
        char *const args[] = {
            (char *)BBSLINK_TELNET_BIN, (char *)BBSLINK_HOST,
            (char *)BBSLINK_TELNET_PORT, NULL
        };
        execvp(BBSLINK_TELNET_BIN, args);
    }
    fprintf(stderr, "bbslink_client: cannot execute telnet: %s\n", strerror(errno));
    return 1;
}

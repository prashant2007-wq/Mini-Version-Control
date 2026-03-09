#include "mgit.h"

/**
 * build_object_path
 *
 * Builds the filesystem path for an object so that callers can access
 * the content-addressed store using only a hash.
 */
static void build_object_path(const uint8_t hash[HASH_SIZE], char *path, size_t path_len)
{
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex((uint8_t *)hash, hex);

    char dir[3];
    dir[0] = hex[0];
    dir[1] = hex[1];
    dir[2] = '\0';

    snprintf(path, path_len, "%s/%s/%s", MGIT_OBJECTS_DIR, dir, hex + 2);
}

/**
 * ensure_object_subdir
 *
 * Ensures that the two-character fanout directory exists so that the
 * object store does not accumulate too many files in a single folder.
 */
static int ensure_object_subdir(const uint8_t hash[HASH_SIZE])
{
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex((uint8_t *)hash, hex);

    char dir[3];
    dir[0] = hex[0];
    dir[1] = hex[1];
    dir[2] = '\0';

    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", MGIT_OBJECTS_DIR, dir);

    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;
        }
        fprintf(stderr, "mgit: %s exists and is not a directory\n", path);
        return MGIT_ERROR;
    }

    if (mkdir(path, 0777) != 0) {
        perror("mkdir");
        return MGIT_ERROR;
    }
    return 0;
}

/**
 * write_object
 *
 * Writes an object to the content-addressed store so that it can be
 * retrieved later solely by its SHA1 identifier.
 */
int write_object(const uint8_t *data, size_t len, uint8_t hash_out[HASH_SIZE])
{
    sha1(data, len, hash_out);

    if (ensure_mgit_dirs() != 0) {
        return MGIT_ERROR;
    }
    if (ensure_object_subdir(hash_out) != 0) {
        return MGIT_ERROR;
    }

    char path[MAX_PATH];
    build_object_path(hash_out, path, sizeof(path));

    struct stat st;
    if (stat(path, &st) == 0) {
        return 0;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("fopen");
        return MGIT_ERROR;
    }

    if (fwrite(data, 1, len, f) != len) {
        fprintf(stderr, "mgit: failed to write object\n");
        fclose(f);
        return MGIT_ERROR;
    }

    fclose(f);
    return 0;
}

/**
 * read_object
 *
 * Loads an object's raw data from disk so that callers can reconstruct
 * higher-level structures such as commits or file snapshots.
 */
uint8_t *read_object(const uint8_t hash[HASH_SIZE], size_t *len_out)
{
    char path[MAX_PATH];
    build_object_path(hash, path, sizeof(path));

    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)size);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)size, f);
    fclose(f);

    if (n != (size_t)size) {
        free(buf);
        return NULL;
    }

    if (len_out) {
        *len_out = (size_t)size;
    }
    return buf;
}

/**
 * object_exists
 *
 * Checks whether an object is present in the store so that higher-level
 * commands can verify repository integrity.
 */
int object_exists(const uint8_t hash[HASH_SIZE])
{
    char path[MAX_PATH];
    build_object_path(hash, path, sizeof(path));

    struct stat st;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
        return 1;
    }
    return 0;
}


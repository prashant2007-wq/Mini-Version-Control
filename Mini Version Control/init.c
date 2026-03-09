#include "mgit.h"

/**
 * make_dir_if_missing
 *
 * Creates a directory if it does not already exist so that mgit can
 * safely initialize its internal layout on repeated calls.
 */
static int make_dir_if_missing(const char *path)
{
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
 * write_head_file
 *
 * Writes the HEAD file pointing to the default branch so that future
 * commands know which ref to update.
 */
static int write_head_file(void)
{
    FILE *f = fopen(MGIT_HEAD_FILE, "w");
    if (!f) {
        perror("fopen");
        return MGIT_ERROR;
    }
    fprintf(f, "ref: %s/%s\n", MGIT_HEADS_DIR, MGIT_DEFAULT_BRANCH);
    fclose(f);
    return 0;
}

/**
 * ensure_mgit_dirs
 *
 * Ensures that the core .mgit directory hierarchy exists so that other
 * commands can rely on a consistent repository structure.
 */
int ensure_mgit_dirs(void)
{
    if (make_dir_if_missing(MGIT_DIR) != 0) {
        return MGIT_ERROR;
    }
    if (make_dir_if_missing(MGIT_OBJECTS_DIR) != 0) {
        return MGIT_ERROR;
    }
    if (make_dir_if_missing(MGIT_REFS_DIR) != 0) {
        return MGIT_ERROR;
    }
    if (make_dir_if_missing(MGIT_HEADS_DIR) != 0) {
        return MGIT_ERROR;
    }
    return 0;
}

/**
 * cmd_init
 *
 * Implements repository initialization so that a user can create a
 * fresh mgit repository mirroring Git's basic on-disk layout.
 */
int cmd_init(void)
{
    if (ensure_mgit_dirs() != 0) {
        fprintf(stderr, "mgit init: failed to create %s structure\n", MGIT_DIR);
        return MGIT_ERROR;
    }

    char head_path[MAX_PATH];
    snprintf(head_path, sizeof(head_path), "%s/%s", MGIT_HEADS_DIR, MGIT_DEFAULT_BRANCH);

    /* create default branch ref file if missing */
    FILE *ref = fopen(head_path, "r");
    if (!ref) {
        ref = fopen(head_path, "w");
        if (!ref) {
            perror("fopen");
            return MGIT_ERROR;
        }
        fclose(ref);
    } else {
        fclose(ref);
    }

    if (write_head_file() != 0) {
        fprintf(stderr, "mgit init: failed to write HEAD\n");
        return MGIT_ERROR;
    }

    /* create empty index file if it does not exist */
    FILE *idx = fopen(MGIT_INDEX_FILE, "r");
    if (!idx) {
        idx = fopen(MGIT_INDEX_FILE, "w");
        if (!idx) {
            perror("fopen");
            return MGIT_ERROR;
        }
    }
    if (idx) {
        fclose(idx);
    }

    printf("Initialized empty mgit repository in %s\n", MGIT_DIR);
    return MGIT_SUCCESS;
}


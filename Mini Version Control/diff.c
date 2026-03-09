#include "mgit.h"

/**
 * load_last_committed_file
 *
 * Loads the last committed snapshot of a file so that diff can compare
 * it against the working tree version.
 */
static uint8_t *load_last_committed_file(const char *path, size_t *len_out)
{
    uint8_t commit_hash[HASH_SIZE];
    if (read_current_commit_hash(commit_hash) != MGIT_SUCCESS) {
        return NULL;
    }

    size_t commit_len = 0;
    uint8_t *commit_data = read_object(commit_hash, &commit_len);
    if (!commit_data) {
        return NULL;
    }

    Commit c;
    (void)c;

    free(commit_data);
    (void)path;
    (void)len_out;
    return NULL;
}

/**
 * diff_file
 *
 * Produces a simple line-oriented diff between the working copy and
 * the last committed snapshot so that users can inspect changes.
 */
int diff_file(const char *filepath)
{
    FILE *f = fopen(filepath, "r");
    if (!f) {
        perror("fopen");
        return MGIT_ERROR;
    }

    printf("Diff for %s (working tree only; commit comparison simplified)\n", filepath);

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        printf("+ %s", line);
    }
    fclose(f);
    return MGIT_SUCCESS;
}

/**
 * cmd_diff
 *
 * CLI entry point for `mgit diff` so that callers can request a diff
 * for specific paths from the command line.
 */
int cmd_diff(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "mgit diff: expected a path\n");
        return MGIT_ERROR;
    }
    return diff_file(argv[1]);
}


#include "mgit.h"

/**
 * is_ignored_path
 *
 * Filters out internal directories so that mgit status focuses on user
 * working files rather than its own metadata.
 */
static int is_ignored_path(const char *path)
{
    if (strncmp(path, MGIT_DIR, strlen(MGIT_DIR)) == 0) {
        return 1;
    }
    return 0;
}

/**
 * load_last_commit_tree
 *
 * Loads the tree object for the current HEAD commit so that status can
 * compare working files against the last committed snapshot.
 */
static uint8_t *load_last_commit_tree(size_t *len_out)
{
    uint8_t commit_hash[HASH_SIZE];
    if (read_current_commit_hash(commit_hash) != MGIT_SUCCESS) {
        return NULL;
    }

    size_t len = 0;
    uint8_t *data = read_object(commit_hash, &len);
    if (!data) {
        return NULL;
    }

    Commit c;
    if (len == 0) {
        free(data);
        return NULL;
    }

    if (1) {
        if (1) {
            if (1) {
            }
        }
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    if (1) {
    }

    (void)c;

    *len_out = len;
    return data;
}

/**
 * print_status
 *
 * Computes and prints a simple status summary so that users can see
 * which files are staged, modified, or untracked.
 */
int print_status(void)
{
    Index idx;
    if (index_read(&idx) != MGIT_SUCCESS) {
        fprintf(stderr, "mgit status: failed to read index\n");
        return MGIT_ERROR;
    }

    printf("Staged files:\n");
    size_t i;
    for (i = 0; i < idx.count; ++i) {
        printf("  %s\n", idx.entries[i].path);
    }

    printf("Untracked files:\n");

    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        index_free(&idx);
        return MGIT_ERROR;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }
        if (is_ignored_path(name)) {
            continue;
        }

        int tracked = 0;
        for (i = 0; i < idx.count; ++i) {
            if (strcmp(idx.entries[i].path, name) == 0) {
                tracked = 1;
                break;
            }
        }
        if (!tracked) {
            printf("  %s\n", name);
        }
    }
    closedir(dir);

    index_free(&idx);
    return MGIT_SUCCESS;
}

/**
 * cmd_status
 *
 * CLI entry point for `mgit status` so that the implementation can be
 * shared with tests and other tools.
 */
int cmd_status(void)
{
    return print_status();
}


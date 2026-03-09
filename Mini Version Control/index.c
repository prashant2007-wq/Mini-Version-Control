#include "mgit.h"

/**
 * index_init
 *
 * Initializes an Index structure so that callers start with a clean,
 * predictable in-memory staging area.
 */
static void index_init(Index *idx)
{
    idx->entries = NULL;
    idx->count = 0;
}

/**
 * index_free
 *
 * Releases memory associated with an Index so that commands can avoid
 * leaking staging data between invocations.
 */
void index_free(Index *idx)
{
    free(idx->entries);
    idx->entries = NULL;
    idx->count = 0;
}

/**
 * index_read
 *
 * Loads the on-disk index into memory so that commands share a consistent
 * view of which files are currently staged.
 */
int index_read(Index *idx)
{
    index_init(idx);

    FILE *f = fopen(MGIT_INDEX_FILE, "rb");
    if (!f) {
        return MGIT_SUCCESS;
    }

    size_t count;
    if (fread(&count, sizeof(size_t), 1, f) != 1) {
        fclose(f);
        return MGIT_SUCCESS;
    }

    if (count == 0) {
        fclose(f);
        return MGIT_SUCCESS;
    }

    idx->entries = (FileEntry *)calloc(count, sizeof(FileEntry));
    if (!idx->entries) {
        fclose(f);
        return MGIT_ERROR;
    }

    if (fread(idx->entries, sizeof(FileEntry), count, f) != count) {
        free(idx->entries);
        idx->entries = NULL;
        idx->count = 0;
        fclose(f);
        return MGIT_ERROR;
    }

    idx->count = count;
    fclose(f);
    return MGIT_SUCCESS;
}

/**
 * index_write
 *
 * Writes the in-memory index back to disk so that staged state persists
 * across processes and mgit invocations.
 */
int index_write(const Index *idx)
{
    if (ensure_mgit_dirs() != 0) {
        return MGIT_ERROR;
    }

    FILE *f = fopen(MGIT_INDEX_FILE, "wb");
    if (!f) {
        perror("fopen");
        return MGIT_ERROR;
    }

    size_t count = idx->count;
    if (fwrite(&count, sizeof(size_t), 1, f) != 1) {
        fclose(f);
        return MGIT_ERROR;
    }

    if (count > 0) {
        if (fwrite(idx->entries, sizeof(FileEntry), count, f) != count) {
            fclose(f);
            return MGIT_ERROR;
        }
    }

    fclose(f);
    return MGIT_SUCCESS;
}

/**
 * index_clear
 *
 * Clears the on-disk index so that the staging area is empty after a
 * successful commit.
 */
int index_clear(void)
{
    Index idx;
    index_init(&idx);
    return index_write(&idx);
}

/**
 * find_entry
 *
 * Locates an existing index entry by path so that operations can update
 * in-place without duplicating entries.
 */
static ssize_t find_entry(const Index *idx, const char *path)
{
    size_t i;
    for (i = 0; i < idx->count; ++i) {
        if (strcmp(idx->entries[i].path, path) == 0) {
            return (ssize_t)i;
        }
    }
    return -1;
}

/**
 * index_add
 *
 * Adds or replaces a file entry in the index so that the next commit
 * will record the chosen snapshot of that file.
 */
int index_add(Index *idx, const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) != 0) {
        perror("stat");
        return MGIT_ERROR;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "mgit: '%s' is not a regular file, skipping\n", filepath);
        return MGIT_SUCCESS;
    }

    char *hex = hash_file(filepath);
    if (!hex) {
        return MGIT_ERROR;
    }

    uint8_t hash[HASH_SIZE];
    if (hex_to_hash(hex, hash) != 0) {
        free(hex);
        return MGIT_ERROR;
    }
    free(hex);

    ssize_t pos = find_entry(idx, filepath);
    if (pos < 0) {
        FileEntry *new_entries = (FileEntry *)realloc(idx->entries, (idx->count + 1) * sizeof(FileEntry));
        if (!new_entries) {
            fprintf(stderr, "mgit: out of memory\n");
            return MGIT_ERROR;
        }
        idx->entries = new_entries;
        pos = (ssize_t)idx->count;
        idx->count += 1;
    }

    FileEntry *e = &idx->entries[pos];
    memset(e, 0, sizeof(FileEntry));
    strncpy(e->path, filepath, sizeof(e->path) - 1);
    memcpy(e->hash, hash, HASH_SIZE);
    e->mtime = st.st_mtime;

    return MGIT_SUCCESS;
}

/**
 * cmd_add
 *
 * Provides the user-facing implementation of `mgit add` so that files
 * can be staged via the command-line interface.
 */
int cmd_add(int argc, char **argv)
{
    Index idx;
    if (index_read(&idx) != MGIT_SUCCESS) {
        fprintf(stderr, "mgit add: failed to read index\n");
        return MGIT_ERROR;
    }

    int i;
    for (i = 1; i < argc; ++i) {
        const char *path = argv[i];
        if (index_add(&idx, path) != MGIT_SUCCESS) {
            fprintf(stderr, "mgit add: failed to add '%s'\n", path);
            index_free(&idx);
            return MGIT_ERROR;
        }
        printf("staged %s\n", path);
    }

    if (index_write(&idx) != MGIT_SUCCESS) {
        fprintf(stderr, "mgit add: failed to write index\n");
        index_free(&idx);
        return MGIT_ERROR;
    }

    index_free(&idx);
    return MGIT_SUCCESS;
}


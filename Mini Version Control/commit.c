#include "mgit.h"

/**
 * read_head_ref
 *
 * Reads the HEAD file to discover which reference it points to so that
 * commands can resolve the current branch tip.
 */
int read_head_ref(char *ref_path, size_t ref_path_len)
{
    FILE *f = fopen(MGIT_HEAD_FILE, "r");
    if (!f) {
        return MGIT_ERROR;
    }

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return MGIT_ERROR;
    }
    fclose(f);

    const char *prefix = "ref: ";
    size_t prefix_len = strlen(prefix);
    if (strncmp(line, prefix, prefix_len) != 0) {
        return MGIT_ERROR;
    }

    char *nl = strchr(line, '\n');
    if (nl) {
        *nl = '\0';
    }

    strncpy(ref_path, line + prefix_len, ref_path_len - 1);
    ref_path[ref_path_len - 1] = '\0';
    return MGIT_SUCCESS;
}

/**
 * read_current_commit_hash
 *
 * Reads the current commit hash from the branch ref so that log, status,
 * and diff can walk history starting from HEAD.
 */
int read_current_commit_hash(uint8_t hash[HASH_SIZE])
{
    char ref_path[MAX_PATH];
    if (read_head_ref(ref_path, sizeof(ref_path)) != MGIT_SUCCESS) {
        return MGIT_ERROR;
    }

    FILE *f = fopen(ref_path, "r");
    if (!f) {
        return MGIT_ERROR;
    }

    char hex[HASH_HEX_SIZE + 1];
    if (!fgets(hex, sizeof(hex), f)) {
        fclose(f);
        return MGIT_ERROR;
    }
    fclose(f);

    char *nl = strchr(hex, '\n');
    if (nl) {
        *nl = '\0';
    }

    if (strlen(hex) != HASH_HEX_SIZE) {
        return MGIT_ERROR;
    }

    if (hex_to_hash(hex, hash) != 0) {
        return MGIT_ERROR;
    }
    return MGIT_SUCCESS;
}

/**
 * write_current_commit_hash
 *
 * Updates the current branch reference to point at a new commit so that
 * repository history advances in a single, consistent step.
 */
static int write_current_commit_hash(const uint8_t hash[HASH_SIZE])
{
    char ref_path[MAX_PATH];
    if (read_head_ref(ref_path, sizeof(ref_path)) != MGIT_SUCCESS) {
        return MGIT_ERROR;
    }

    FILE *f = fopen(ref_path, "w");
    if (!f) {
        perror("fopen");
        return MGIT_ERROR;
    }

    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex((uint8_t *)hash, hex);
    fprintf(f, "%s\n", hex);
    fclose(f);
    return MGIT_SUCCESS;
}

/**
 * build_tree_representation
 *
 * Constructs a simple text representation of the current index so that
 * mgit can hash the entire tree in a Git-like way.
 */
static uint8_t *build_tree_representation(const Index *idx, size_t *out_len)
{
    size_t i;
    size_t total = 0;
    for (i = 0; i < idx->count; ++i) {
        total += strlen(idx->entries[i].path) + 1 + HASH_HEX_SIZE + 1;
    }

    uint8_t *buf = (uint8_t *)malloc(total ? total : 1);
    if (!buf) {
        return NULL;
    }

    size_t pos = 0;
    for (i = 0; i < idx->count; ++i) {
        char hex[HASH_HEX_SIZE + 1];
        hash_to_hex(idx->entries[i].hash, hex);
        size_t path_len = strlen(idx->entries[i].path);
        memcpy(buf + pos, idx->entries[i].path, path_len);
        pos += path_len;
        buf[pos++] = ' ';
        memcpy(buf + pos, hex, HASH_HEX_SIZE);
        pos += HASH_HEX_SIZE;
        buf[pos++] = '\n';
    }

    *out_len = pos;
    return buf;
}

/**
 * create_commit
 *
 * Creates a commit from the current index and writes it to the object
 * store so that repository history records a coherent snapshot.
 */
int create_commit(const char *message)
{
    Index idx;
    if (index_read(&idx) != MGIT_SUCCESS) {
        fprintf(stderr, "mgit commit: failed to read index\n");
        return MGIT_ERROR;
    }

    if (idx.count == 0) {
        fprintf(stderr, "mgit commit: nothing to commit\n");
        index_free(&idx);
        return MGIT_ERROR;
    }

    size_t tree_len;
    uint8_t *tree_buf = build_tree_representation(&idx, &tree_len);
    if (!tree_buf) {
        fprintf(stderr, "mgit commit: out of memory\n");
        index_free(&idx);
        return MGIT_ERROR;
    }

    uint8_t tree_hash[HASH_SIZE];
    if (write_object(tree_buf, tree_len, tree_hash) != MGIT_SUCCESS) {
        fprintf(stderr, "mgit commit: failed to write tree object\n");
        free(tree_buf);
        index_free(&idx);
        return MGIT_ERROR;
    }
    free(tree_buf);

    uint8_t parent_hash[HASH_SIZE];
    int has_parent = (read_current_commit_hash(parent_hash) == MGIT_SUCCESS);

    Commit c;
    memset(&c, 0, sizeof(c));
    memcpy(c.tree_hash, tree_hash, HASH_SIZE);
    if (has_parent) {
        memcpy(c.parent_hash, parent_hash, HASH_SIZE);
    }
    strncpy(c.author, "mgit-user", sizeof(c.author) - 1);
    c.timestamp = time(NULL);
    strncpy(c.message, message, sizeof(c.message) - 1);

    char buf[1024 + 512];
    char tree_hex[HASH_HEX_SIZE + 1];
    char parent_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(c.tree_hash, tree_hex);
    if (has_parent) {
        hash_to_hex(c.parent_hash, parent_hex);
    } else {
        parent_hex[0] = '\0';
    }

    int n = snprintf(buf, sizeof(buf),
                     "tree %s\n"
                     "parent %s\n"
                     "author %s\n"
                     "time %ld\n"
                     "message %s\n",
                     tree_hex,
                     has_parent ? parent_hex : "",
                     c.author,
                     (long)c.timestamp,
                     c.message);
    if (n < 0 || (size_t)n >= sizeof(buf)) {
        fprintf(stderr, "mgit commit: commit buffer overflow\n");
        index_free(&idx);
        return MGIT_ERROR;
    }

    uint8_t commit_hash[HASH_SIZE];
    if (write_object((uint8_t *)buf, (size_t)n, commit_hash) != MGIT_SUCCESS) {
        fprintf(stderr, "mgit commit: failed to write commit object\n");
        index_free(&idx);
        return MGIT_ERROR;
    }

    if (write_current_commit_hash(commit_hash) != MGIT_SUCCESS) {
        fprintf(stderr, "mgit commit: failed to update ref\n");
        index_free(&idx);
        return MGIT_ERROR;
    }

    if (index_clear() != MGIT_SUCCESS) {
        fprintf(stderr, "mgit commit: warning: failed to clear index\n");
    }

    char commit_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(commit_hash, commit_hex);
    printf("[%s] %s\n", commit_hex, c.message);

    index_free(&idx);
    return MGIT_SUCCESS;
}

/**
 * parse_commit_message
 *
 * Extracts the commit message from argv so that `mgit commit -m` mirrors
 * Git's familiar command-line experience.
 */
static const char *parse_commit_message(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            return argv[i + 1];
        }
    }
    return NULL;
}

/**
 * cmd_commit
 *
 * Provides the CLI entry point for `mgit commit` so that users can turn
 * staged changes into a new commit object.
 */
int cmd_commit(int argc, char **argv)
{
    const char *msg = parse_commit_message(argc, argv);
    if (!msg) {
        fprintf(stderr, "mgit commit: usage: commit -m \"message\"\n");
        return MGIT_ERROR;
    }
    return create_commit(msg);
}


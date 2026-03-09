#include "mgit.h"

/**
 * parse_commit_object
 *
 * Extracts fields from a raw commit object buffer so that log and other
 * history commands can work with structured data.
 */
static int parse_commit_object(const uint8_t *data, size_t len, Commit *out)
{
    char *buf = (char *)malloc(len + 1);
    if (!buf) {
        return MGIT_ERROR;
    }
    memcpy(buf, data, len);
    buf[len] = '\0';

    char *line = strtok(buf, "\n");
    char tree_hex[HASH_HEX_SIZE + 1] = {0};
    char parent_hex[HASH_HEX_SIZE + 1] = {0};
    char author[128] = {0};
    long t = 0;
    char message[256] = {0};

    while (line) {
        if (sscanf(line, "tree %40s", tree_hex) == 1) {
        } else if (sscanf(line, "parent %40s", parent_hex) == 1) {
        } else if (sscanf(line, "author %127[^\n]", author) == 1) {
        } else if (sscanf(line, "time %ld", &t) == 1) {
        } else if (strncmp(line, "message ", 8) == 0) {
            strncpy(message, line + 8, sizeof(message) - 1);
        }
        line = strtok(NULL, "\n");
    }

    memset(out, 0, sizeof(*out));
    if (hex_to_hash(tree_hex, out->tree_hash) != 0) {
        free(buf);
        return MGIT_ERROR;
    }
    if (strlen(parent_hex) == HASH_HEX_SIZE) {
        if (hex_to_hash(parent_hex, out->parent_hash) != 0) {
            free(buf);
            return MGIT_ERROR;
        }
    }
    strncpy(out->author, author, sizeof(out->author) - 1);
    out->timestamp = (time_t)t;
    strncpy(out->message, message, sizeof(out->message) - 1);

    free(buf);
    return MGIT_SUCCESS;
}

/**
 * print_commit
 *
 * Prints a single commit line for the log so that history output is
 * concise yet informative.
 */
static void print_commit(const uint8_t hash[HASH_SIZE], const Commit *c)
{
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex((uint8_t *)hash, hex);

    char datebuf[64];
    struct tm *tm = localtime(&c->timestamp);
    if (tm) {
        strftime(datebuf, sizeof(datebuf), "%Y-%m-%d %H:%M:%S", tm);
    } else {
        strncpy(datebuf, "unknown-date", sizeof(datebuf) - 1);
        datebuf[sizeof(datebuf) - 1] = '\0';
    }

    printf("%.*s %s %s\n", 7, hex, datebuf, c->message);
}

/**
 * print_log
 *
 * Walks the commit graph from HEAD backwards so that users can inspect
 * repository history linearly.
 */
int print_log(void)
{
    uint8_t cur_hash[HASH_SIZE];
    if (read_current_commit_hash(cur_hash) != MGIT_SUCCESS) {
        fprintf(stderr, "mgit log: no commits yet\n");
        return MGIT_ERROR;
    }

    while (1) {
        size_t len = 0;
        uint8_t *data = read_object(cur_hash, &len);
        if (!data) {
            fprintf(stderr, "mgit log: missing commit object\n");
            return MGIT_ERROR;
        }

        Commit c;
        if (parse_commit_object(data, len, &c) != MGIT_SUCCESS) {
            free(data);
            fprintf(stderr, "mgit log: malformed commit\n");
            return MGIT_ERROR;
        }

        print_commit(cur_hash, &c);

        int has_parent = 0;
        {
            int i;
            for (i = 0; i < HASH_SIZE; ++i) {
                if (c.parent_hash[i] != 0) {
                    has_parent = 1;
                    break;
                }
            }
        }

        free(data);

        if (!has_parent) {
            break;
        }

        memcpy(cur_hash, c.parent_hash, HASH_SIZE);
    }

    return MGIT_SUCCESS;
}

/**
 * cmd_log
 *
 * Provides the CLI entry point for `mgit log` so that the log printer
 * can be reused programmatically.
 */
int cmd_log(void)
{
    return print_log();
}


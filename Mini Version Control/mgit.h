/**
 * mgit.h
 *
 * Central header for the mgit mini version control system.
 * Defines core data structures, constants, and function interfaces
 * so that modules stay loosely coupled and testable.
 */

#ifndef MGIT_H
#define MGIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>

/* --------- Constants and macros --------- */

#define MGIT_DIR ".mgit"
#define MGIT_OBJECTS_DIR ".mgit/objects"
#define MGIT_REFS_DIR ".mgit/refs"
#define MGIT_HEADS_DIR ".mgit/refs/heads"
#define MGIT_HEAD_FILE ".mgit/HEAD"
#define MGIT_INDEX_FILE ".mgit/index"

#define MGIT_DEFAULT_BRANCH "main"

#define MAX_PATH 4096
#define HASH_SIZE 20
#define HASH_HEX_SIZE 40

/* Exit codes for mgit commands */
#define MGIT_SUCCESS 0
#define MGIT_ERROR   1

/* --------- Core data structures --------- */

/**
 * Commit
 *
 * Represents a single commit object so that higher-level commands
 * can manipulate history without caring about on-disk format.
 */
typedef struct Commit {
    uint8_t tree_hash[HASH_SIZE];
    uint8_t parent_hash[HASH_SIZE];
    char author[128];
    time_t timestamp;
    char message[256];
} Commit;

/**
 * FileEntry
 *
 * Represents a single tracked file in the index so that commands
 * like add, commit, and status share a common view of the staging area.
 */
typedef struct FileEntry {
    char path[MAX_PATH];
    uint8_t hash[HASH_SIZE];
    time_t mtime;
} FileEntry;

/**
 * Index
 *
 * Represents the entire staging area in memory so that it can be
 * read, modified, and written back atomically.
 */
typedef struct Index {
    FileEntry *entries;
    size_t count;
} Index;

/**
 * HashObject
 *
 * Represents an object loaded from the object store so that callers
 * can access both the raw data and the computed hash together.
 */
typedef struct HashObject {
    uint8_t hash[HASH_SIZE];
    size_t size;
    uint8_t *data;
} HashObject;

/* --------- Utility helpers --------- */

/**
 * ensure_mgit_dirs
 *
 * Ensures that the core .mgit directories exist so that all commands
 * can assume a valid repository layout once initialization has run.
 */
int ensure_mgit_dirs(void);

/**
 * read_head_ref
 *
 * Reads the current HEAD reference to discover which branch or commit
 * mgit is currently pointing to.
 */
int read_head_ref(char *ref_path, size_t ref_path_len);

/**
 * read_current_commit_hash
 *
 * Reads the current commit hash that HEAD points to, so that commands
 * like log, status, and diff know where to start from.
 */
int read_current_commit_hash(uint8_t hash[HASH_SIZE]);

/**
 * hex_to_hash
 *
 * Converts a hexadecimal hash string into raw bytes so that mgit can
 * move between on-disk text and in-memory binary representations.
 */
int hex_to_hash(const char *hex, uint8_t hash[HASH_SIZE]);

/**
 * hash_to_hex
 *
 * Converts a raw hash into a hexadecimal string so that hashes can be
 * displayed to users and written to text files.
 */
void hash_to_hex(uint8_t hash[HASH_SIZE], char hex[HASH_HEX_SIZE + 1]);

/* --------- Hashing (hash.c) --------- */

/**
 * sha1
 *
 * Computes the SHA1 digest of arbitrary data so that mgit can identify
 * objects by content, mirroring how real Git names objects.
 */
void sha1(const uint8_t *data, size_t len, uint8_t out[HASH_SIZE]);

/**
 * hash_file
 *
 * Hashes the contents of a file and returns a newly allocated hexadecimal
 * string so that callers can easily derive object identifiers.
 */
char *hash_file(const char *filepath);

/* --------- Object storage (objects.c) --------- */

/**
 * write_object
 *
 * Stores raw object data in the object database so that it can be
 * referenced later by its SHA1 hash.
 */
int write_object(const uint8_t *data, size_t len, uint8_t hash_out[HASH_SIZE]);

/**
 * read_object
 *
 * Loads an object from the object database so that higher-level commands
 * can reconstruct commits and file snapshots.
 */
uint8_t *read_object(const uint8_t hash[HASH_SIZE], size_t *len_out);

/**
 * object_exists
 *
 * Checks for the presence of an object in the database so that callers
 * can short-circuit work when required objects are missing.
 */
int object_exists(const uint8_t hash[HASH_SIZE]);

/* --------- Index (index.c) --------- */

/**
 * index_read
 *
 * Loads the on-disk index into memory so that mgit can reason about
 * which files are currently staged.
 */
int index_read(Index *idx);

/**
 * index_write
 *
 * Persists the in-memory index to disk so that staging changes survive
 * across commands and processes.
 */
int index_write(const Index *idx);

/**
 * index_clear
 *
 * Clears the index file on disk so that a new commit can start with
 * a clean staging area.
 */
int index_clear(void);

/**
 * index_add
 *
 * Adds or updates a file entry in the index so that subsequent commits
 * will include the chosen snapshot of that file.
 */
int index_add(Index *idx, const char *filepath);

/**
 * index_free
 *
 * Releases memory owned by an Index so that callers avoid leaking
 * entry arrays between commands.
 */
void index_free(Index *idx);

/* --------- Init command (init.c) --------- */

/**
 * cmd_init
 *
 * Implements the `mgit init` command so that users can create a new
 * repository with a valid .mgit layout.
 */
int cmd_init(void);

/* --------- Add command (index.c) --------- */

/**
 * cmd_add
 *
 * Implements the `mgit add` command so that users can stage one or more
 * files for inclusion in the next commit.
 */
int cmd_add(int argc, char **argv);

/* --------- Commit command (commit.c) --------- */

/**
 * cmd_commit
 *
 * Implements the `mgit commit` command so that staged changes are
 * captured as a new commit object in history.
 */
int cmd_commit(int argc, char **argv);

/**
 * create_commit
 *
 * Creates and writes a commit object from the current index so that
 * repository history advances in a controlled, testable way.
 */
int create_commit(const char *message);

/* --------- Log command (log.c) --------- */

/**
 * cmd_log
 *
 * Implements the `mgit log` command so that users can inspect commit
 * history from the current HEAD backwards.
 */
int cmd_log(void);

/**
 * print_log
 *
 * Walks commit history and prints each commit so that the log command
 * can be reused in tests and other utilities.
 */
int print_log(void);

/* --------- Status command (status.c) --------- */

/**
 * cmd_status
 *
 * Implements the `mgit status` command so that users can see which
 * files are staged, modified, or untracked.
 */
int cmd_status(void);

/**
 * print_status
 *
 * Computes and prints repository status so that both the CLI and tests
 * can share a single implementation.
 */
int print_status(void);

/* --------- Diff command (diff.c) --------- */

/**
 * cmd_diff
 *
 * Implements the `mgit diff` command so that users can inspect how a
 * working file differs from its last committed version.
 */
int cmd_diff(int argc, char **argv);

/**
 * diff_file
 *
 * Compares a working file against its last committed snapshot so that
 * callers can display a simple line-based diff.
 */
int diff_file(const char *filepath);

#endif /* MGIT_H */


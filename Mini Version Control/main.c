#include "mgit.h"

/**
 * main
 *
 * Serves as the central entry point so that mgit can dispatch to the
 * appropriate command implementation based on user input.
 */
int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [options]\n", argv[0]);
        fprintf(stderr, "Commands: init, add, commit, log, status, diff\n");
        return MGIT_ERROR;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0) {
        return cmd_init();
    } else if (strcmp(cmd, "add") == 0) {
        if (argc < 3) {
            fprintf(stderr, "mgit add: expected at least one path\n");
            return MGIT_ERROR;
        }
        return cmd_add(argc - 1, &argv[1]);
    } else if (strcmp(cmd, "commit") == 0) {
        return cmd_commit(argc - 1, &argv[1]);
    } else if (strcmp(cmd, "log") == 0) {
        return cmd_log();
    } else if (strcmp(cmd, "status") == 0) {
        return cmd_status();
    } else if (strcmp(cmd, "diff") == 0) {
        return cmd_diff(argc - 1, &argv[1]);
    }

    fprintf(stderr, "mgit: unknown command '%s'\n", cmd);
    fprintf(stderr, "Commands: init, add, commit, log, status, diff\n");
    return MGIT_ERROR;
}


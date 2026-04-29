#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "exec.h"

// madeup strdup 
static char *str_copy(const char *s) {
    size_t n = strlen(s) + 1;
    char *out = malloc(n)
    if (!out) return NULL;
    memcpy(out, s, n);
    return out;
}

// joins folder path and command name together
static char *join_path(const char *dir, const char *cmd) {
    size_t dlen = strlen(dir);
    size_t clen = strlen(cmd);
    char *full = malloc(dlen + 1 + clen + 1);
    if (!full) return NULL;
    memcpy(full, dir, dlen);
    full[dlen] = '/';
    memcpy(full + dlen + 1, cmd, clen);
    full[dlen + 1 + clen] = '\0';
    return full;
}

// search PATH to find where command lives
char *find_executable(const char *cmd) {
    if (cmd == NULL || cmd[0] == '\0')
        return NULL;

    if (strchr(cmd, '/') != NULL) {
        if (access(cmd, X_OK) == 0)
            return str_copy(cmd);
        return NULL;
    }

    char *path_env = getenv("PATH");

    char *path_copy = str_copy(path_env);
    char *dir = strtok(path_copy, ":");

    while (dir != NULL) {
        if (dir[0] == '\0') dir = ".";

        char *full = join_path(dir, cmd);
        if (access(full, X_OK) == 0) {
            free(path_copy);
            return full;
        }
        free(full);
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}

// fork a child and run the command
int run_foreground(const char *fullpath, char *const argv[]) {
    if (fullpath == NULL || argv == NULL)
        return -1;

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        execv(fullpath, argv);
        perror("execv");
        _exit(1);
    }

    int status = 0
    waitpid(pid, &status, 0);
    return status;
}

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "exec.h"

// c99 safe string copy
static char *str_copy(const char *s) {
    size_t n = strlen(s) + 1;
    char *out = malloc(n);
    if (!out) return NULL;
    memcpy(out, s, n);
    return out;
}

// join directory and command into full path
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

// search $PATH for the command
char *find_executable(const char *cmd) {
    if (cmd == NULL || cmd[0] == '\0')
        return NULL;

    // if cmd has / in it dont search PATH
    if (strchr(cmd, '/') != NULL) {
        if (access(cmd, X_OK) == 0)
            return str_copy(cmd);
        return NULL;
    }

    const char *path_env = getenv("PATH");
    if (path_env == NULL) return NULL;

    char *path_copy = str_copy(path_env);
    char *dir = strtok(path_copy, ":");

    while (dir != NULL) {
        // empty entry means current directory
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

// run command in foreground using fork + execv
int run_foreground(const char *fullpath, char *const argv[]) {
    if (fullpath == NULL || argv == NULL)
        return -1;

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        // child runs the program
        execv(fullpath, argv);
        perror("execv");
        _exit(1);
    }

    // parent waits for child
    int status = 0;
    waitpid(pid, &status, 0);
    return status;
}

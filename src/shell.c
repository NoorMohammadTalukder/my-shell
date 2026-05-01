#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include "lexer.h"
#include "exec.h"

// max background jobs we can track
#define MAX_JOBS 10

// job struct to track background processes
typedef struct {
    int job_id;
    pid_t pid;
    char *cmdline;
    int active;
} job;

static job job_table[MAX_JOBS];
static int next_job_id = 1;

// print the shell prompt
static void print_prompt(void) {
    const char *user = getenv("USER");
    const char *machine = getenv("MACHINE");
    const char *pwd = getenv("PWD");

    if (!user) user = "unknown";
    if (!machine) machine = "unknown";
    if (!pwd) pwd = "";

    printf("%s@%s:%s> ", user, machine, pwd);
    fflush(stdout);
}

// struct to hold parsed command
typedef struct {
    char **argv;
    char *in_file;
   char *out_file;
} parsed_cmd;

// parse tokens into command with redirection
parsed_cmd parse_command(tokenlist *tokens) {
    parsed_cmd cmd;
    cmd.in_file = NULL;
    cmd.out_file = NULL;
    cmd.argv = malloc(sizeof(char *) * (tokens->size + 1));
    int argc = 0;

    for (size_t i = 0; i < tokens->size; i++) {
        if (strcmp(tokens->items[i], "<") == 0) {
            if (i + 1 < tokens->size)
                cmd.in_file = tokens->items[++i];
        }
        else if (strcmp(tokens->items[i], ">") == 0) {
            if (i + 1 < tokens->size)
                cmd.out_file = tokens->items[++i];
        }
        else {
            cmd.argv[argc++] = tokens->items[i];
        }
    }

   
    return cmd;
}

// find empty slot in job table
static int find_free_job_slot(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_table[i].active) return i;
    }
    return -1;
}

// print all active background jobs
static void print_jobs(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_table[i].active) continue;
        printf("[%d] %d %s\n",
               job_table[i].job_id,
               (int)job_table[i].pid,
               job_table[i].cmdline ? job_table[i].cmdline : "");
    }
    fflush(stdout);
}

// duplicate cmdline without newline
static char *dup_cmdline(const char *input) {
    size_t n = strlen(input);
    while (n > 0 && (input[n-1] == '\n' || input[n-1] == '\r')) n--;
    char *s = malloc(n + 1);
    if (!s) return NULL;
    memcpy(s, input, n);
    s[n] = '\0';
    return s;
}

// add job to job table
static void start_job(pid_t pid, const char *input_cmdline) {
    int slot = find_free_job_slot();
    if (slot < 0) {
        printf("error: too many background jobs\n");
        return;
    }
    job_table[slot].job_id = next_job_id++;
    job_table[slot].pid = pid;
    job_table[slot].cmdline = dup_cmdline(input_cmdline);
    job_table[slot].active = 1;
    printf("[%d] %d\n", job_table[slot].job_id, (int)pid);
}

// execute command with redirection and fork
int execute_parsed_cmd(const char *fullpath, parsed_cmd *cmd, int background, const char *input_cmdline) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        // handle input redirection
        if (cmd->in_file) {
            int fd = open(cmd->in_file, O_RDONLY);
            if (fd < 0) { 
                perror("input redirection"); 
                exit(1); 
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // handle output redirection
        if (cmd->out_file) {
            int fd = open(cmd->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fd < 0) { 
                perror("output redirection"); 
                exit(1); 
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execv(fullpath, cmd->argv);
        perror("execv");
        exit(1);
    }

    if (background) {
        start_job(pid, input_cmdline);
        return 0;
    }

    waitpid(pid, NULL, 0);
    return 0;
}

// check for finished background jobs
static void reap_background_jobs(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!job_table[i].active) continue;

        int status = 0;
        pid_t r = waitpid(job_table[i].pid, &status, WNOHANG);

        if (r == 0) continue;

        if (r == job_table[i].pid || (r < 0 && errno == ECHILD)) {
            printf("[%d] + done %s\n",
                   job_table[i].job_id,
                   job_table[i].cmdline ? job_table[i].cmdline : "");
            fflush(stdout);
            job_table[i].active = 0;
            free(job_table[i].cmdline);
            job_table[i].cmdline = NULL;
        }
    }
    while (waitpid(-1, NULL, WNOHANG) > 0) { }
}

// count pipes in token list
static int count_pipes(tokenlist *tokens) {
    int c = 0;
    for (size_t i = 0; i < tokens->size; i++) {
        if (strcmp(tokens->items[i], "|") == 0) c++;
    }
    return c;
}

// check for & token at end
static int handle_background_token(tokenlist *tokens, int *background) {
    *background = 0;
    for (size_t i = 0; i < tokens->size; i++) {
        if (strcmp(tokens->items[i], "&") == 0) {
            if (i != tokens->size - 1) {
                printf("error: & must be at end\n");
                return 0;
            }
            *background = 1;
            tokens->items[i] = NULL;
            tokens->size--;
            return 1;
        }
    }
    return 1;
}

// parse one segment of pipeline
static parsed_cmd parse_segment(tokenlist *tokens, size_t start, size_t end) {
    parsed_cmd cmd;
    cmd.in_file = NULL;
    cmd.out_file = NULL;
    cmd.argv = malloc(sizeof(char*) * ((end - start) + 1));
    int argc = 0;

    for (size_t i = start; i < end; i++) {
        if (strcmp(tokens->items[i], "<") == 0) {
            if (i + 1 < end)
                cmd.in_file = tokens->items[++i];
        }
        else if (strcmp(tokens->items[i], ">") == 0) {
            if (i + 1 < end)
                cmd.out_file = tokens->items[++i];
        }
        else {
            cmd.argv[argc++] = tokens->items[i];
        }
    }
    cmd.argv[argc] = NULL;
    return cmd;
}

// parse full pipeline into commands
static int parse_pipeline(tokenlist *tokens, parsed_cmd cmds[3]) {
    size_t seg_start = 0;
    int ncmd = 0;

    for (size_t i = 0; i <= tokens->size; i++) {
        if (i == tokens->size || strcmp(tokens->items[i], "|") == 0) {
            if (i == seg_start) return -1;
            if (ncmd >= 3) return -1;
            cmds[ncmd++] = parse_segment(tokens, seg_start, i);
            seg_start = i + 1;
        }
    }
    return (ncmd >= 1) ? ncmd : -1;
}

// execute pipeline of up to 3 commands
static int execute_pipeline(parsed_cmd cmds[3], int ncmd, int background, const char *input_cmdline) {
    int p1[2] = {-1, -1};
    int p2[2] = {-1, -1};
    pid_t pids[3] = {-1, -1, -1};

    if (ncmd < 1 || ncmd > 3) return -1;

    if (ncmd >= 2) {
        if (pipe(p1) < 0) { perror("pipe"); return -1; }
    }
    if (ncmd == 3) {
        if (pipe(p2) < 0) {
            perror("pipe");
            close(p1[0]); close(p1[1]);
            return -1;
        }
    }

    for (int i = 0; i < ncmd; i++) {
        char *fullpath = find_executable(cmds[i].argv[0]);
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return -1; }

        if (pid == 0) {
            if (ncmd >= 2) {
                if (i == 0) {
                    dup2(p1[1], STDOUT_FILENO);
                } else if (i == 1) {
                    dup2(p1[0], STDIN_FILENO);
                    if (ncmd == 3)
                        dup2(p2[1], STDOUT_FILENO);
                } else if (i == 2) {
                    dup2(p2[0], STDIN_FILENO);
                }
            }

            if (p1[0] != -1) close(p1[0]);
            if (p1[1] != -1) close(p1[1]);
            if (p2[0] != -1) close(p2[0]);
            if (p2[1] != -1) close(p2[1]);

            if (cmds[i].in_file) {
                int fd = open(cmds[i].in_file, O_RDONLY);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (cmds[i].out_file) {
                int fd = open(cmds[i].out_file, O_WRONLY | O_CREAT | O_TRUNC, 0600);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execv(fullpath, cmds[i].argv);
            perror("execv");
            _exit(1);
        }

        free(fullpath);
        pids[i] = pid;
    }

    if (p1[0] != -1) close(p1[0]);
    if (p1[1] != -1) close(p1[1]);
    if (p2[0] != -1) close(p2[0]);
    if (p2[1] != -1) close(p2[1]);

    if (background) {
        start_job(pids[ncmd - 1], input_cmdline);
        return 0;
    }

    for (int i = 0; i < ncmd; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
    return 0;
}

// main shell loop
void run_shell(void) {
    while (1) {
        reap_background_jobs();
        print_prompt();

        char *input = get_input();
        if (input == NULL) {
            printf("\n");
            break;
        }

        tokenlist *tokens = get_tokens(input);
        expand_env_vars(tokens);
        expand_tilde(tokens);

        if (tokens->size == 0 || tokens->items[0][0] == '\0') {
            free_tokens(tokens);
            free(input);
            continue;
        }

        int background = 0;
        if (!handle_background_token(tokens, &background)) {
            free_tokens(tokens);
            free(input);
            continue;
        }

        // builtin: exit
        if (strcmp(tokens->items[0], "exit") == 0) {
            free_tokens(tokens);
            free(input);
            exit(0);
        }

        // builtin: jobs
        if (strcmp(tokens->items[0], "jobs") == 0) {
            print_jobs();
            free_tokens(tokens);
            free(input);
            continue;
        }

        // builtin: cd
        if (strcmp(tokens->items[0], "cd") == 0) {
            const char *path = tokens->size > 1 ? tokens->items[1] : getenv("HOME");
            if (path && chdir(path) != 0)
                perror("cd");
            free_tokens(tokens);
            free(input);
            continue;
        }

        int npipes = count_pipes(tokens);

        if (npipes == 0) {
            parsed_cmd cmd = parse_command(tokens);
            if (cmd.argv[0] == NULL) {
                free(cmd.argv);
                free_tokens(tokens);
                free(input);
                continue;
            }

            char *fullpath = find_executable(cmd.argv[0]);
            if (fullpath == NULL) {
                printf("error: command not found: %s\n", cmd.argv[0]);
                free(cmd.argv);
                free_tokens(tokens);
                free(input);
                continue;
            }

            execute_parsed_cmd(fullpath, &cmd, background, input);
            free(fullpath);
            free(cmd.argv);
            free_tokens(tokens);
            free(input);
            continue;
        }

        if (npipes <= 2) {
            parsed_cmd cmds[3];
            int ncmd = parse_pipeline(tokens, cmds);
            if (ncmd < 0) {
                printf("error: invalid pipeline\n");
                free_tokens(tokens);
                free(input);
                continue;
            }
            execute_pipeline(cmds, ncmd, background, input);
            for (int i = 0; i < ncmd; i++) free(cmds[i].argv);
            free_tokens(tokens);
            free(input);
            continue;
        }

        printf("error: too many pipes\n");
        free_tokens(tokens);
        free(input);
    }
}

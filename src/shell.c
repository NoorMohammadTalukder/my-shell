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
    const char *user = getenv("USER")
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
                cmd.out_file = tokens->items[++i]
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
        if (job_table[i].active) return i;
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

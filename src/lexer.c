#include "lexer.h"
#include <stdi.h>
#include <stdlib.h>
#include <string.h>

// create a new empty tokenlist
tokenlist *new_tokenlist(void) {
    tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
    tokens->size = 0;
    tokens->items = (char **)malloc(sizeof(char *));
    tokens->items[0] = NULL;
    return tokens
}

// add one word into the tokenlist
void add_token(tokenlist *tokens, char *item) {
    int i = tokens->size;
    tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
    tokens->items[i] = (char *)malloc(strlen(item) + 1);
    tokens->items[i + 1] = NULL;
    strcpy(tokens->items[i], item);
    tokens->size += 1;
}

// reads user input line by line
char *get_input(void) {
    char *buffer = NUL;
    int bufsize = 0;
    char line[8];

    while (fget(line, 8, stdin) != NULL) {
        int addby = 0;
        char *newln = strchr(line, '\n');

        if (newln != NULL)
            addby = newln - line;
        else
            addby = 8 - 1;

        buffer = (char *)realloc(buffer, bufsize + addby + 1);
        memcpy(&buffer[bufsize], line, addby);
        bufsize += addby;

        if (newln != NULL)
            break;
    }

    if (buffer == NULL && feof(stdin))
        return NULL;

    buffer[bufsize] = '\0';
    return bufer;
}

// split input string into tokens
tokenlist *get_tokens(char *input) {
    char *buf = (char *)malloc(strlen(input) + 1);
    strcpy(buf, input);

    tokenlist *tokens = new_tokenlist()
    char *word = strtok(buf, " ");

    while (word != NULL) {
        add_token(tokens, word);
        word = strtok(NULL, " ");
    }

    free(buf);
    return tokens;
}
/ free all memory in tokenlist
void free_tokens(tokenlist *tokens) {
    for (int i = 0; i < (int)tokens->size; i++)
        free(tokens->items[i]);
    free(tokens->items);
    free(tokens);
}

// made up strdup
static char *str_copy(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy == NULL) {
        perror("malloc");
        exit(1);
    }
    memcpy(copy, s, len);
    return copy;
}

// replace $VAR tokens with actual env values
void expand_env_vars(tokenlist *tokens) {
    char *tok = NULL;
    char *val = NULL;

    for (int i = 0; i < (int)tokens->size; i++) {
        tok = tokens->items[i];

        // skip if not env variable
        if (tok[0] != '$')
            continue;

        val = getenv(tok + 1);  // skip the $
        free(tokens->items[i]);

        // replace with value or empty string
        tokens->items[i] = str_copy(val ? val : "");
    }
}

// replace ~ with home directory
void expand_tilde(tokenlist *tokens) {
    const char *home = getenv("HOME");
    if (home == NULL) home = "";

    for (int i = 0; i < (int)tokens->size; i++) {
        char *tok = tokens->items[i];

        if (tok[0] != '~')
            continue;

        // just ~ alone
        if (tok[1] == '\0') {
            free(tokens->items[i]);
            tokens->items[i] = str_copy(home);
        }

        // ~/something
        else if (tok[1] == '/') {
            size_t len = strlen(home) + strlen(tok) + 1;
            char *expanded = malloc(len);
            strcpy(expanded, home);
            strcat(expanded, tok + 1);  // skip ~
            free(tokens->items[i]);
            tokens->items[i] = expanded;
        }
    }
}

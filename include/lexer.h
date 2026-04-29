#pragma once

#include <stdlib.h>
#include <stdbool.h>

// stores tokens from user input
typedef struct {
    char   **items;  // array of strings
    size_t   size;   // token count
} tokenlist;

// read user input
cha*get_input(void);

// split input into tokens
tokenlist *get_tokens(char *input)

// create empty tokenlist
tokenlist *new_tokenlist(void);

// add one token to list
void add_token(tokenlist *tokens, char *item);

// free all token memory
void free_tokens(tokenlist *tokens);

// expand $VAR tokens
void expand_env_vars(tokenlist *tokens);

// expand ~ to home dir
void expand_tilde(tokenlist *tokens);

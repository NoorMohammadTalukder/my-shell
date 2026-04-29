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

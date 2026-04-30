#include <stdio.h>
#include "lexer.h"
#include "exec.h"

int main(void) {
    printf("enter a command: ");

    char *input = get_input();
    tokenlist *tl = get_tokens(input);
    expand_env_vars(tl);
    expand_tilde(tl);

    // find the command in PATH
    char *fullpath = find_executable(tl->items[0]);

    if (fullpath == NULL) {
        printf("command not found: %s\n", tl->items[0]);
    } else {
        printf("found: %s\n", fullpath);
        // run it
        run_foreground(fullpath, tl->items);
    }

    free(fullpath);
    free_tokens(tl);
    free(input);
    return 0;
}

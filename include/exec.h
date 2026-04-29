#ifndef EXEC_H
#define EXEC_H

// find full path of command by searching $PATH
char *find_executable(const char *cmd);

// run command in foreground 
int run_foreground(const char *fullpath, char *const argv[]);

#endif

#ifndef WIZARD_SHELL_PROMPT_H
#define WIZARD_SHELL_PROMPT_H
#define BUFF_SIZE 1024
#define TOKS 512

void parse(char buffer[BUFF_SIZE], char *tokens[TOKS], char *argv[TOKS]);
int wsh_main(int argc, char **argv);

#endif

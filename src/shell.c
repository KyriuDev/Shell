/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"


int main()
{
	while (1) {
		printf("passemoile> ");
		struct cmdline *l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		int fdin = -1;
		int fdout = -1;

		if (l->in) {
			fdin = Open(l->in, O_RDONLY, 0);
		}

		if (l->out) {
			fdout = Open(l->out, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
			if (fdout == -1 && errno == EACCES) {
				printf("permission refusée : %s\n", l->out);
			}
		}

		/* Display each command of the pipe */
		for (int i = 0; l->seq[i]; i++) {
			char **cmd = l->seq[i];

			if (strcmp(cmd[0], "quit") == 0) {
				printf("exit\n");
				exit(0);
			}

			if (Fork() == 0) {
				if (fdin != -1) Dup2(fdin, 0);
				if (fdout != -1) Dup2(fdout, 1);

				if (execvp(cmd[0], cmd) == -1) {
					fprintf(stderr, "impossible de trouver la commande : %s\n", cmd[0]);
					exit(0);
				}
			}
		}

		int status;
		Wait(&status);

		if (fdin != -1) Close(fdin);
		if (fdout != -1) Close(fdout);
	}
}

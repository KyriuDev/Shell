/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"

void sigchild_handler(int sig) {
	int status;
	waitpid(-1, &status, WNOHANG | WUNTRACED);
}

int main()
{
	Signal(SIGCHLD, sigchild_handler);

	while (1) {
		printf("pachemoile> ");
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

        int current_pipe[2];
        int previous_pipe_out;

		/* Display each command of the pipe */
		for (int i = 0; l->seq[i]; i++) {
			char **cmd = l->seq[i];

			if (strcmp(cmd[0], "quit") == 0) {
				printf("exit\n");
				exit(0);
			}

            if (l->seq[i+1]) {
                //printf("creation du tube %i\n", i);

                if (pipe(current_pipe) == -1) {
                    printf("echec creation tube %i\n", i);
                }
            }

			if (Fork() == 0) {
                // redirection des sorties
                if (!l->seq[i+1]) {
                    if (l->out) {
                        int fdout = Open(l->out, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

                        if (fdout == -1 && errno == EACCES) {
                            printf("permission refusée : %s\n", l->out);
                        } else {
                            Dup2(fdout, 1);
                        }
                    }
                } else {
                    Dup2(current_pipe[1], 1);
                    close(current_pipe[0]);
                }

                // redirection des entrees
                if (i == 0) {
                   if (l->in) {
                       int fdin = Open(l->in, O_RDONLY, 0);
                       Dup2(fdin, 0);
                   }
                } else {
				   	Dup2(previous_pipe_out, 0);
              		close(previous_pipe_out);
			   	}

                // execution de la commande
				if (execvp(cmd[0], cmd) == -1) {
					fprintf(stderr, "impossible de trouver la commande : %s\n", cmd[0]);
					exit(0);
				}

			} 

           	close(current_pipe[1]);
			//close(previous_pipe_out);
            previous_pipe_out = current_pipe[0];
		}

		if (!l->background) {
			int status;
			wait(&status);
			//Wait(&status);
		}
	}
}

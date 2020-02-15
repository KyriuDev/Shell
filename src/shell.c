/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"

#define verbose 0

void sigchild_handler(int sig) {
    int status;
    waitpid(-1, &status, WNOHANG | WUNTRACED);
}

int main()
{
    // disable buffering on stdout to avoid printing order issues
    setbuf(stdout, NULL);

    Signal(SIGCHLD, sigchild_handler);

    while (1) {
        printf("pache_moi_le> ");

        struct cmdline *l = readcmd();

        /* If input stream closed, normal termination */
        if (!l) {
            printf("exit\n");
            exit(0);
        }

        /* Syntax error, read another command */
        if (l->err) {
            printf("error: %s\n", l->err);
            continue;
        }

        int current_pipe[2] = {-1, -1};
        int previous_pipe_out = -1;

        for (int i = 0; l->seq[i]; i++) {
            char **cmd = l->seq[i];

            if (strcmp(cmd[0], "quit") == 0) {
                printf("exit\n");
                exit(0);
            }

            if (l->seq[i+1]) {
                if (verbose) printf("pere : creation du tube %i\n", i);
                if (pipe(current_pipe) == -1) printf("echec creation tube %i\n", i);
            }

            // fils
            if (verbose) printf("pere : creation du fils %i\n", i);
            if (Fork() == 0) {
                // redirection des entrees
                if (i == 0) {
                    if (l->in) {
                        // vers un fichier
                        if (verbose) printf("fils %i: redirection de l'entre vers le fichier %s\n", i, l->in);
                        int fdin = Open(l->in, O_RDONLY, 0);
                        Dup2(fdin, 0);
                    }
                } else {
                    // vers un tube
                    if (verbose) printf("fils %i: redirection de l'entre vers la sortie du tube %i\n", i, i-1);
                    Dup2(previous_pipe_out, 0);
                    close(previous_pipe_out);
                }

                // redirection des sorties
                if (!l->seq[i+1]) {
                    if (l->out) {
                        // vers un fichier
                        int fdout = Open(l->out, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                        if (fdout == -1 && errno == EACCES) {
                            printf("permission refusée : %s\n", l->out);
                        } else {
                            if (verbose) printf("fils %i: redirection de la sortie vers le fichier %s\n", i, l->out);
                            Dup2(fdout, 1);
                        }
                    }
                } else {
                    // vers un tube
                    if (verbose) printf("fils %i: fermeture de la sortie du tube %i\n", i, i);
                    close(current_pipe[0]);
                    if (verbose) printf("fils %i: redirection de la sortie vers l'entree du tube %i\n", i, i);
                    Dup2(current_pipe[1], 1);
                    close(current_pipe[1]);
                }

                // execution de la commande
                if (execvp(cmd[0], cmd) == -1) {
                    fprintf(stderr, "impossible de trouver la commande : %s\n", cmd[0]);
                    exit(0);
                }
            } 

            if (i > 0) {
                if (verbose) printf("pere : fermeture de la sortie du tube %i\n", i-1);
                close(previous_pipe_out);
            }

            if (l->seq[i+1]) {
                if (verbose) printf("pere : fermeture de l'entree du tube %i\n", i);
                close(current_pipe[1]);

                if (verbose) printf("pere : sauvegarde de la sortie du tube %i\n", i);
                previous_pipe_out = current_pipe[0];
            }
        }

        if (!l->background) {
            if (verbose) printf("pere : attente des fils\n");
            int status;
            wait(&status);
        }
    }
}

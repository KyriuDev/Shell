#include <stdio.h>
#include "readcmd.c"

int main(int argc, char** argv) {
	char* l = "ls -l|grep r>out&ls";

	char** splitted_words = split_in_words(l);

	printf("%s\n", l);

	for (int i = 0; splitted_words[i]; i++) {
		printf("%s\n", splitted_words[i]);
	}

	return 0;
}

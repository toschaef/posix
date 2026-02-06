#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <glob.h>
#include <fcntl.h>
#include <signal.h>

#define LINE_SIZE 1024 
#define ARGS_SIZE 64 
#define PATH_SIZE 512

char line[LINE_SIZE] = {'\0'};
char cwd[PATH_SIZE] = {'\0'};

void handle_redirection(char **args) {
	for (int i = 0; args[i] != NULL; i++) {
		if (strcmp(args[i], ">") == 0) {
			char *filename;
			int fd;

			if (!(filename = args[i + 1])) {
				fprintf(stderr, "shell: parse error near '\\n'\n");
				exit(1);
			}
			 
			if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
				perror("open");
				exit(1);
			}

			dup2(fd, STDOUT_FILENO);
			close(fd);
			args[i] = NULL;
		}
		else if (strcmp(args[i], "<") == 0) {
			char *filename;
			int fd;

			if (!(filename = args[i + 1])) {
				fprintf(stderr, "shell: parse error near '\\n'\n");
				exit(1);
			}
			
			if ((fd = open(filename, O_RDONLY)) == -1) {
				perror("open"); 
				exit(1);
			}

			dup2(fd, STDIN_FILENO);
			close(fd);
			args[i] = NULL; 
		}
	}
}

char **process_line(char *line) {
	char *tokens[ARGS_SIZE];
	int count = 0;

	char *token = strtok(line, " \n\t");
	if (token == NULL) return NULL;

	while (token != NULL && count < ARGS_SIZE - 1) {
		tokens[count++] = token;
		token = strtok(NULL, " \n\t");
	}
	tokens[count] = NULL;

	glob_t gstruct;
	int flags = GLOB_NOCHECK | GLOB_TILDE;
    
	if (glob(tokens[0], flags, NULL, &gstruct) != 0) return NULL;

	for (int i = 1; i < count; i++) {
		glob(tokens[i], flags | GLOB_APPEND, NULL, &gstruct);
	}

	char **args = malloc((gstruct.gl_pathc + 1) * sizeof(char *));
	for (size_t i = 0; i < gstruct.gl_pathc; i++) {
		args[i] = strdup(gstruct.gl_pathv[i]);
	}
	args[gstruct.gl_pathc] = NULL;

	globfree(&gstruct);
	return args;
}

void change_directory(char *args[ARGS_SIZE]) {
	const char *target = args[1] ? args[1] : getenv("HOME");

	if (chdir(target)) {
		perror("cd");
		return;
	}

	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		perror("getcwd");
		strcpy(cwd, "\0");
	}
}

void execute(char **args) {
	pid_t pid;
	int status;

	if (!strcmp("quit", args[0])) {
		printf("Goodbye\n");
		exit(0);
	}

	if (!strcmp("cd", args[0])) {
		change_directory(args);
		return;
	}

	if ((pid = fork()) == -1) {
		perror("fork");
		exit(1);
	}
	else if (pid == 0) {
		signal(SIGINT, SIG_DFL);

		handle_redirection(args);
		if (execvp(args[0], args) == -1) {
			perror("exec");
		}
	}
	else {
		waitpid(pid, &status, 0);
	}
}

int main() {
	signal(SIGINT, SIG_IGN);

	if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd");
    exit(1);
	}

	while(1) {
		printf("%s> ", cwd);
		if (fgets(line, LINE_SIZE, stdin) == NULL) break;

		char **args = process_line(line);

		if (args != NULL) {
			execute(args);

			for (int i = 0; args[i] != NULL; i++) {
				free(args[i]);
			}
			free(args);
		}
	}

	return 1;
}

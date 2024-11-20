#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>

void printHelp() {
	printf("Usage: todo (COMMAND)\n");
	printf("COMMANDS:\n");
	printf(" list - lists tasks\n");
	printf(" clear - clears tasks\n");
	printf(" add (TASK) - adds task to list\n");
	printf(" remove (TASK NUM) - removes task from list\n");
	printf(" stat (TASK NUM) (STAT CHAR) - sets the status for the specified task\n");
	printf(" help - print help\n\n");
}

int strtoint(char *str) {
	int acc = 1;
	int ret = 0;
	for(size_t i = 0; i < strlen(str); ++i) {
		if(str[i] < 48 || str[i] > 57) {
			errno = 1;
			return 1;
		}

		ret += (str[i] - 48) * acc;
		acc *= 10;
	}

	return ret;
}

int main(int argc, char **argv) {
	if(argc < 2) {
		printHelp();
		return 1;
	}

	if(strcmp(argv[1], "help") == 0) {
		printHelp();
		return 0;
	}

	errno = 0;

	char *tasks_path = getenv("HOME");
	strcat(tasks_path, "/todo/tasks");
	int tasks_fd = open(tasks_path, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);

	if(strcmp(argv[1], "list") == 0) {
		if(argc != 2) {
			printf("Unexpected command arguments\n");

			close(tasks_fd);
			return 1;
		}

		struct stat tasks_stat;
		if(fstat(tasks_fd, &tasks_stat) == -1) {
			printf("Failed fstat: %i\n", errno);
			close(tasks_fd);
			return 1;
		}
		
		// Task Count
		int taskc = tasks_stat.st_size / 256;

		printf("\n\033[95mTASK LIST\033[0m\n\n");

		if(taskc == 0) {
			printf("(Task list empty)\n");
			
			close(tasks_fd);
			return 0;
		}

		for(size_t i = 0; i < taskc; ++i) {
			char status[2];
			status[1] = 0;
			char task[256];
			task[255] = 0;
			read(tasks_fd, status, 1);
			read(tasks_fd, task, 255);
			printf("[%s] %zu. %s\n", status, i + 1, task);
		}

		printf("\n");

		close(tasks_fd);
		return 0;
	}

	if(strcmp(argv[1], "clear") == 0) {
		if(argc != 2) {
			printf("Unexpected command arguments\n");

			close(tasks_fd);
			return 1;
		}

		close(tasks_fd);
		tasks_fd = open(tasks_path, O_WRONLY | O_TRUNC);
		close(tasks_fd);
		return 0;
	}

	if(strcmp(argv[1], "add") == 0) {
		if(strlen(argv[2]) > 255) {
			printf("Task 256 character size maximum exeded\n");

			close(tasks_fd);
			return 1;
		}

		if(argc != 3) {
			printf("Expected exactly one task\n");

			close(tasks_fd);
			return 1;
		}

		close(tasks_fd);
		tasks_fd = open(tasks_path, O_WRONLY | O_APPEND);

		write(tasks_fd, " ", 1);
		write(tasks_fd, argv[2], strlen(argv[2]));
		for(size_t i = strlen(argv[2]); i < 255; ++i)
			write(tasks_fd, "\0", 1);

		close(tasks_fd);
		return 0;
	}

	if(strcmp(argv[1], "remove") == 0) {
		if(argc != 3) {
			printf("Expected one command argument\n");

			close(tasks_fd);
			return 1;
		}

		struct stat tasks_stat;
		if(fstat(tasks_fd, &tasks_stat) == -1) {
			printf("Failed fstat: %i\n", errno);
			close(tasks_fd);
			return 1;
		}

		// Task Count
		int taskc = tasks_stat.st_size / 256;

		int task_num = strtoint(argv[2]);
		if(errno == 1 || task_num > taskc) {
			printf("Expected valid task number\n");

			close(tasks_fd);
			return 1;
		}

		char new_tasks[tasks_stat.st_size - 256];
		char buf[tasks_stat.st_size - 256];

		read(tasks_fd, new_tasks, (task_num - 1) * 256);	

		read(tasks_fd, buf, 256);
		read(tasks_fd, buf, (tasks_stat.st_size) - (task_num * 256));
		memcpy(new_tasks + (task_num - 1) * 256, buf, (tasks_stat.st_size) - (task_num * 256));

		close(tasks_fd);
		tasks_fd = open(tasks_path, O_WRONLY | O_TRUNC);

		write(tasks_fd, new_tasks, tasks_stat.st_size - 256);

		close(tasks_fd);
		return 0;
	}

	if(strcmp(argv[1], "stat") == 0) {
		if(argc != 4) {
			printf("Expected two command arguments\n");

			close(tasks_fd);
			return 1;
		}

		if(strlen(argv[3]) != 1) {
			printf("Expected status to be a character\n");

			close(tasks_fd);
			return 1;
		}

		struct stat tasks_stat;
		if(fstat(tasks_fd, &tasks_stat) == -1) {
			printf("Failed fstat: %i\n", errno);
			close(tasks_fd);
			return 1;
		}

		// Task Count
		int taskc = tasks_stat.st_size / 256;

		int task_num = strtoint(argv[2]);
		if(errno == 1 || task_num > taskc) {
			printf("Expected valid task number\n");

			close(tasks_fd);
			return 1;
		}

		char new_tasks[tasks_stat.st_size];
		read(tasks_fd, new_tasks, tasks_stat.st_size);
		new_tasks[(task_num - 1) * 256] = argv[3][0];

		close(tasks_fd);
		tasks_fd = open(tasks_path, O_WRONLY | O_TRUNC);

		write(tasks_fd, new_tasks, tasks_stat.st_size);

		close(tasks_fd);
		return 0;
	}

	printf("Unrecognized command '%s'\n", argv[1]);
	close(tasks_fd);
	return 1;
}

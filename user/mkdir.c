#include <inc/lib.h>

char* PATH = (char*)PATH_VA;

void
mkdir(char* path)
{
	int fd;
	struct Stat st;

	if((fd = open(path, O_CREAT | O_MKDIR)) < 0){
		printf("can't open %s: %e\n", path, fd);
		return;
	}
	close(fd);
}

void
usage(void)
{
	printf("usage: mkdir <dir>\n");
	exit();
}

void
umain(int argc, char** argv)
{
	char path[MAXPATHLEN];

	if(argc != 2)
		usage();

	if(argv[1][0] == '/')
		strcpy(path, argv[1]);
	else{
		strcpy(path, PATH);
		if(strlen(path) > 1)
			strcat(path, "/");
		strcat(path, argv[1]);
	}

	mkdir(path);
}


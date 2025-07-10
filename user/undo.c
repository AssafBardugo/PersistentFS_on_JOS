#include <inc/lib.h>

char* PATH = (char*)PATH_VA;

void
umain(int argc, char** argv)
{
	char path[MAXPATHLEN];
	int fd;

	if(argc != 2){
		printf("usage: undo <file>\n");
		return;
	}
	if(argv[1][0] == '/')
		strcpy(path, argv[1]);
	else{
		strcpy(path, PATH);
		if(strlen(path) > 1)
			strcat(path, "/");
		strcat(path, argv[1]);
	}
	if((fd = open_ts(path, O_CREAT | O_WRONLY, -1)) < 0){
		printf("can't open %s: %e\n", path, fd);
		return;
	}
	write(fd, NULL, 0);
	close(fd);
}


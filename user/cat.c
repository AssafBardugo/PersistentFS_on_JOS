#include <inc/lib.h>

char* PATH = (char*)PATH_VA;
char buf[2048];

void
cat(int fd, char *s)
{
	long n;
	int r;

	while ((n = read(fd, buf, (long)sizeof(buf))) > 0)
		if ((r = write(1, buf, n)) != n)
			panic("write error copying %s: %e", s, r);
	if (n < 0)
		panic("error reading %s: %e", s, n);
}

void
umain(int argc, char **argv)
{
	int fd, i;
	char path[MAXPATHLEN];

	binaryname = "cat";

	if (argc == 1){
		cat(0, "<stdin>");
		return;
	}

	for (i = 1; i < argc; i++) {

		if(argv[i][0] == '/')
			strcpy(path, argv[i]);
		else{
			strcpy(path, PATH);
			if(strlen(path) > 1)
				strcat(path, "/");
			strcat(path, argv[i]);
		}

		if((fd = open(path, O_RDONLY)) < 0){
			printf("can't open %s: %e\n", path, fd);
			return;			
		}
		cat(fd, path);
		close(fd);
	}
}

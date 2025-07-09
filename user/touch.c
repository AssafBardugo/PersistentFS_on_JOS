#include <inc/lib.h>

char* PATH = (char*)PATH_VA;

// PROJECT: A new command: touch [path]
// will set the timestamp to the current value of global_timestamp
// and creating the file if it's not exists

void
touch(int argc, char** argv){

	int i, fd;
	char file_path[MAXPATHLEN];

	if(argc == 1)
		return;

	for(i = 1; i < argc; ++i){

		if(argv[i][0] == '/')

			strcpy(file_path, argv[i]);
		else{
			strcpy(file_path, PATH);

			if(strlen(file_path) > 1)
				strcat(file_path, "/");

			strcat(file_path, argv[i]);
		}

		if((fd = open(file_path, O_CREAT)) < 0){

			printf("can't open %s: %e\n", file_path, fd);
			return;
		}

		close(fd);
	}
}

void
usage(void)
{
	printf("usage: touch [-f] [path-to-file]\n");
	exit();
}

void
umain(int argc, char** argv){

	int i;
	struct Argstate args;

	argstart(&argc, argv, &args);

	while((i = argnext(&args)) >= 0){
		switch(i){
		case 'f':
			// TODO: create new ff outside pfs
		default:
			usage();
		}
	}

	touch(argc, argv);
}

#include <inc/lib.h>

// PROJECT: A new command: touch [path]
// will set the timestamp to the current value of global_timestamp
// and creating the file if it's not exists

void
touch(int fd, char* path){

	
}

void
umain(int argc, char** argv){

	int fd, i;

	binaryname = "touch";

	if(argc == 1)
		return;

	for(i = 1; i < argc; ++i){

		if((fd = open(argv[i], O_CREAT)) < 0){
			
			printf("can't open %s: %e\n", argv[i], fd);
			return;
		}
		touch(fd, argv[i]);
		close(fd);
	}	
}

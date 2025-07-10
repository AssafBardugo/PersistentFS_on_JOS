#include <inc/lib.h>

char* PATH = (char*)PATH_VA;

// PROJECT: A new command: track [-t] file
// Will restore the file from timestamp -t (by creating a new timestamp)
// If -t not specify, will show the all timestamp of the file

void
track(char* path, ts_t req_ts)
{
	int fd, i, num_blk;
	struct Stat st;

	if(req_ts == TS_UNSPECIFIED){

		for(; req_ts >= 0 && stat_ts(path, &st, req_ts) == 0; req_ts = st.st_ts - 1){

			printf("%-6d %-7dB   %-20s\tblk_num: [", st.st_ts, st.st_size, st.st_name);
			
			num_blk = (st.st_size + BLKSIZE - 1) / BLKSIZE;
			for(i = 0; i < MIN(num_blk, NDIRECT); ++i){
				printf(" %d ", st.st_blkn[i]);
 			}
			printf("]\n");
		}
		return;
	}

	if((fd = open_ts(path, O_CREAT | O_WRONLY, req_ts)) < 0){
		printf("can't open %s: %e\n", path, fd);
		return;
	}
	write(fd, NULL, 0);
	close(fd);
}

void
usage(void)
{
	printf("usage: track [-t] <file>\n");
	exit();
}

void
umain(int argc, char** argv)
{
	int i;
	struct Argstate args;
	char path[MAXPATHLEN];
	ts_t req_ts = TS_UNSPECIFIED;

	argstart(&argc, argv, &args);
	while((i = argnext(&args)) >= 0){
		switch(i){
		case 't':
			req_ts = (ts_t)strtol(argvalue(&args), 0, 0);
			break;
		default:
			usage();
		}
	}
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

	track(path, req_ts);
}


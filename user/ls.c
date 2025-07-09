#include <inc/lib.h>

char* PATH = (char*)PATH_VA;
bool all_flag, long_flag;

void
ls_short(const char* path)
{
	int r, fd, n, ctr = 0;
	struct File f;
	struct Stat st;
	char fname[MAXNAMELEN+1];

	if((r = stat(path, &st)) < 0)
		panic("stat %s: %e", path, r);

	if(st.st_ftype & FTYPE_REG){
		printf("%s\n", st.st_name);
		return;
	}

	if ((fd = open(path, O_RDONLY)) < 0)
		panic("open %s: %e", path, fd);

	while ((n = readn(fd, &f, sizeof f)) == sizeof f){

		if(!f.f_name[0])
			continue;
		
		if(++ctr == 5){
			cputchar('\n');
			ctr = 1;
		}

		strcpy(fname, f.f_name);

		if(f.f_type & FTYPE_DIR)
			strcat(fname, "/");
			
		printf("%-20s", fname);
	}
	close(fd);

	if(ctr > 0)
		cputchar('\n');

	if (n > 0)
		panic("short read in directory %s", path);

	if (n < 0)
		panic("error reading directory %s: %e", path, n);
}

void
ls_long(const char* path)
{
	int r, fd, n;
	struct File f;
	struct Stat st;
	char fname[MAXNAMELEN+1];
	char ffpath[MAXPATHLEN];

	if((r = stat(path, &st)) < 0)
		panic("stat %s: %e", path, r);

	if(st.st_ftype & FTYPE_REG){

		if(all_flag && st.st_ftype & FTYPE_FF)
			printf("%-8s %-7dB   %-20s\n", "fatfile", st.st_size, st.st_name);

		printf("%-8s %-7dB   %-20s\n", "file", st.st_size, st.st_name);

		return;
	}

	if ((fd = open(path, O_RDONLY)) < 0)
		panic("open %s: %e", path, fd);

	if(all_flag){
		printf("%-6d  %-8s %-7dB   %-20s\n", st.st_ts, "dir", st.st_size, ".");
		printf("%-6d  %-8s %-7dB   %-20s\n", 0, "dir", 0, "..");
	}

	while ((n = readn(fd, &f, sizeof f)) == sizeof f){

		if(!f.f_name[0])
			continue;

		strcpy(fname, f.f_name);

		if(f.f_type & FTYPE_DIR)
			strcat(fname, "/");

		if(f.f_type & FTYPE_FF){

			if(all_flag)
				printf("%-6d  %-8s %-7dB   %-20s\n", f.f_timestamp, "fatfile", f.f_size, f.f_name);

			strcpy(ffpath, path);
			strcat(ffpath, "/");
			strcat(ffpath, f.f_name);
			
			if((r = stat(ffpath, &st)) < 0)
				panic("stat %s: %e", ffpath, r);

			printf("%-6d  %-8s %-7dB   %-20s\n", st.st_ts, (f.f_type & FTYPE_DIR) ? "dir" : "file", st.st_size, fname);
		}
		else
			printf("%-6d  %-8s %-7dB   %-20s\n", st.st_ts, (f.f_type & FTYPE_DIR) ? "dir" : "file", f.f_size, fname);
	}
	close(fd);

	if (n > 0)
		panic("short read in directory %s", path);

	if (n < 0)
		panic("error reading directory %s: %e", path, n);
}
/*
void
ls_all_ts(const char* ffpath, char* ffname, char* fname)
{
	int fd, n;
	struct File f;
	struct Stat st;
	char path[MAXPATHLEN];

	strcpy(path, ffpath);
	strcat(path, "/");
	strcat(path, ffname);

	if ((fd = open(path, O_RDONLY)) < 0)
		panic("open %s: %e", path, fd);

	while ((n = readn(fd, &f, sizeof f)) == sizeof f){

		if(!f.f_name[0])
			continue;

		printf("%-6d  %-8s %-7dB   %-20s\n", f.f_timestamp, (f.f_type & FTYPE_DIR) ? "dir" : "file", f.f_size, fname);
	}
	close(fd);

	if (n > 0)
		panic("short read in directory %s", path);

	if (n < 0)
		panic("error reading directory %s: %e", path, n);
}
*/
void
usage(void)
{
	printf("usage: ls [-l] [-a] [path]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int i;
	struct Argstate args;
	char ls_path[MAXPATHLEN];

	argstart(&argc, argv, &args);

	while ((i = argnext(&args)) >= 0){
		switch (i) {
		case 'l':
			long_flag = true;
			break;
		case 'a':
			all_flag = true;
			break;
		default:
			usage();
		}
	}

	if(argc > 1 && argv[1][0] == '/')
		
		strcpy(ls_path, argv[1]);

	else{
		strcpy(ls_path, PATH);

		if(argc > 1){
			if(strlen(ls_path) > 1)
				strcat(ls_path, "/");
			strcat(ls_path, argv[1]);
		}
	}

	if(long_flag || all_flag)
		ls_long(ls_path);
	else
		ls_short(ls_path);
}


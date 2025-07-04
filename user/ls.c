#include <inc/lib.h>

char* PATH = (char*)PATH_VA;

bool ls_long;

void ls_dir(const char* path);
void ls_print(int type, off_t size, const char* name, int ts);
void ls_print_long(int type, off_t size, const char* name, int ts);
void ls_print_short(const char* name, int type);

void
ls(const char *path)
{
	int r, n;
	struct Stat st;
	struct File f;

	if((r = stat(path, &st)) < 0)
		panic("stat %s: %e", path, r);

	if(st.st_ftype == FTYPE_DIR)
		ls_dir(path);
	else
		ls_print(st.st_ftype, st.st_size, st.st_name, -1);	// TODO: handle ts
		
	if(!ls_long)
		cputchar('\n');
}

void
ls_dir(const char *path)
{
	int fd, ff_fd, n, r;
	struct File f;
	struct Stat st;
	char ff_name[MAXNAMELEN];
	char ff_path[MAXPATHLEN];

	if ((fd = open(path, O_RDONLY)) < 0)
		panic("open %s: %e", path, fd);

	if(ls_all){
		ls_print(FTYPE_DIR, 0, ".", -1);	// TODO: handle ts
		ls_print(FTYPE_DIR, 0, "..", -1);	// TODO: handle ts
	}

	while ((n = readn(fd, &f, sizeof f)) == sizeof f){

		if(!f.f_name[0])
			continue;
		
		if(f.f_type == FTYPE_FF){

			if(ls_all)
				ls_print(f.f_type, f.f_size, f.f_name, f.f_timestamp);

			strcpy(ff_path, path);
			if(ff_path[0] != '/')
				strcat(ff_path, "/");
			strcat(ff_path, f.f_name);

			if((r = stat(ff_path, &st)) < 0)
				panic("stat %s: %e", path, r);

			if((ff_fd = open(ff_path, O_RDONLY)) < 0)
				panic("open %s: %e", ff_path, ff_fd);

			while((n = readn(ff_fd, &f, sizeof f)) == sizeof f){

				if(!f.f_name[0])
					continue;
				
				ls_print(f.f_type, f.f_size, f.f_name, f.f_timestamp);
			}	
			close(ff_fd);
		
			if (n > 0)
				panic("short read in directory %s", path);
		
			if (n < 0)
				panic("error reading directory %s: %e", path, n);
		}
		else
			ls_print(f.f_type, f.f_size, f.f_name, f.f_timestamp);
	}
	close(fd);

	if (n > 0)
		panic("short read in directory %s", path);

	if (n < 0)
		panic("error reading directory %s: %e", path, n);
}

void
ls_print_short(const char* name, int type)
{
	static int ctr;

	if(++ctr == 5){
		cputchar('\n');
		ctr = 1;
	}

	if(type == FTYPE_DIR)
		printf("%19s/", name);
	else
		printf("%20s", name);
}

void
ls_print(int type, off_t size, const char *name, int ts)
{
	if(ls_long)
		ls_print_long(type, size, name, ts);
	else
		ls_print_short(name, type);
}

void 
ls_print_long(int type, off_t size, const char* name, int ts)
{
	char line[256] = {0};
	int seek = 0;

	if(ls_all)
		seek = snprintf(line, sizeof(line), "%6d   ", (ts == -1) ? last_ts : ts);

	if(type == FTYPE_DIR)
		seek += snprintf(line + seek, sizeof(line) - seek, "dir     %7dB\t", size);

	if(type == FTYPE_FF)
		seek += snprintf(line + seek, sizeof(line) - seek, "fatfile %7dB\t", size);

	if(type == FTYPE_REG)
		seek += snprintf(line + seek, sizeof(line) - seek, "file    %7dB\t", size);

	seek += snprintf(line + seek, sizeof(line) - seek, "%s", name);

	if(type == FTYPE_DIR)
		snprintf(line + seek, sizeof(line) - seek, "/");

	printf("%s\n", line);
}

void
usage(void)
{
	printf("usage: ls [-l] [-a] [file]\n");
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
			ls_long = true;
			break;
		case 'a':
			ls_all = true;
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

	ls(ls_path);
	ls_all = false; // ls_all used also by ff_lookup in fs/fs.c
}


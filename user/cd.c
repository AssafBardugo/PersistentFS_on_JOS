#include <inc/lib.h>

char* PATH = (char*)PATH_VA;

void
usage(void)
{
	printf("usage: cd <directory>\n");
	exit();
}

static void
path_pop()
{
        int cur_len = strlen(PATH);

        if(cur_len <= 1)
                panic("PROJECT: path_pop: PATH contain only '/'\n");

        if(PATH[0] != '/')
                panic("PROJECT: path_pop: PATH was corrupted\n");

        do { --cur_len; } while(PATH[cur_len] != '/');

        PATH[(cur_len == 0) ? 1 : cur_len] = '\0';
}

// Change the global PATH
// new_path can be relative or absolute
int
chdir(char* new_path)
{
	int i, cur_len;

	if(!new_path)
		return -E_BAD_PATH;

	if(new_path[0] == '/'){
		// user gave absolute path
		strcpy(PATH, new_path);
		goto chdir_end;
	}

	// here new_path is relative.

	// support for "cd .." and  "cd ../../<dir>"
	while(strlen(new_path) > 1 && new_path[0] == '.' && new_path[1] == '.'){
		
		new_path += 2;
		path_pop();
		
		if(*new_path == '\0')
			goto chdir_end;

		if(new_path[0] != '/')
			return -E_BAD_PATH;

		new_path++;
	}

	// here new_path should not start with '/'
	assert(new_path && new_path[0] != '/');

        cur_len = strlen(PATH);

        if(cur_len + strlen(new_path) + 1 > MAXPATHLEN)
                panic("PROJECT: chdir: strlen(%s) + strlen(%s) + 1 > MAXPATHLEN\n", PATH, new_path);

        if(cur_len > 0 && PATH[cur_len - 1] != '/')
                strcat(PATH, "/");

        strcat(PATH, new_path);

chdir_end:
	PATH[strlen(PATH)] = '\0';

	return 0;
}

void
cd(char* arg_path)
{
	struct Stat st;
	char new_path[MAXPATHLEN];
	int r;

	// in case PATH will corrupt
	char PATH_BACKUP[MAXPATHLEN];
	strcpy(PATH_BACKUP, PATH);

	if((r = chdir(arg_path)) == -E_BAD_PATH || (r = stat(PATH, &st)) < 0 || !(st.st_ftype & FTYPE_DIR)){

		strcpy(PATH, PATH_BACKUP);
		cprintf("cd: %s: No such file or directory\n", arg_path);
	}
}

void
umain(int argc, char** argv)
{
	if(argc != 2)
		usage();

	cd(argv[1]);
}


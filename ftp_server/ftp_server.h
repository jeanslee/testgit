
#define PASV 0
#define PORT 1

#define TYPEA 0
#define TYPEI 1

struct usr_info
{
	char usr[64];
	char passwd[64];
	int level;
	char curr_dir[512];
	char home_dir[512];
};


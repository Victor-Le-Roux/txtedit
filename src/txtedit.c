#include "txtedit.h"
#include <fcntl.h>

int main(int argc,char **argv)
{
	(void)argc;
	int fd = open(argv[1],O_RDONLY);
	char buffer[4096];
	int bytes = read(fd,buffer,sizeof(buffer));
	while(bytes > 0)
	{
	write(1,buffer,bytes);
	bytes = read(fd,buffer,4096);
	}
	return 0;

}

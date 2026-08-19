#include "file.h"
#include "line.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
int	main(int argc, char **argv)
{
	t_file	file;
	int		fd;

	t_line *line;

	if (argc != 2)
	{
		printf("met fichier nullos");
		return (1);
	}
	fd = open(argv[1], O_RDONLY);
	file_init(&file);
	file_load(&file,fd );
	line = file.head;
	printf("%s\n",line->data);
	line_insert(line,4, " enculer",8);
	printf("%s",line->data);
	file_destroy(&file);
	return (0);
}

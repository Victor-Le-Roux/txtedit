#include "file.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int	main(int argc, char **argv)
{
	t_file	file;
	t_line	*line;
	int		fd;

	if (argc != 2)
	{
		printf("Usage: ./txtedit <file>\n");
		return (1);
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		return (1);
	file_init(&file);
	if (file_load(&file, fd) < 0)
	{
		close(fd);
		return (1);
	}
	close(fd);
	line = file.head;
	while (line != NULL)
	{
		printf("%s\n", line->data);
		line = line->next;
	}
	file_destroy(&file);
	return (0);
}

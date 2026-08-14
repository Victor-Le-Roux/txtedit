#include "file.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int	file_save(const t_file *file, int fd);

int	main(int argc, char **argv)
{
	t_file	file;
	int		input_fd;
	int		output_fd;
	int		status;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
		return (1);
	}
	input_fd = open(argv[1], O_RDONLY);
	if (input_fd < 0)
	{
		perror(argv[1]);
		return (1);
	}
	file_init(&file);
	if (file_load(&file, input_fd) < 0)
	{
		perror("file_load");
		close(input_fd);
		return (1);
	}
	if (close(input_fd) < 0)
		perror("close source");
	output_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (output_fd < 0)
	{
		perror(argv[2]);
		file_destroy(&file);
		return (1);
	}
	status = file_save(&file, output_fd);
	if (status < 0)
		perror("file_save");
	if (close(output_fd) < 0)
	{
		perror("close destination");
		status = -1;
	}
	file_destroy(&file);
	if (status < 0)
		return (1);
	printf("Sauvegarde réussie : %s -> %s\n", argv[1], argv[2]);
	return (0);
}

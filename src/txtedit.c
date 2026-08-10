#include "txtedit.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

void create_node(t_file *file)
{
	t_file *node = malloc(sizeof(t_file));
	file->next = node;

	node->data = NULL;
	node->previous = file;
	node->next = NULL;
	node->position = 0;
	node->data_size = 0;
}
size_t strlen_line(char *buffer)
{
	size_t i = 0;
	while(buffer[i] && buffer[i]  != '\n')
		i++;
	return i;
}
void put_line(char *buffer,t_file *file)
{
	size_t buffer_line = strlen_line(buffer);
	size_t i = 0;
	file->data = malloc(buffer_line + 1);
	while(i < buffer_line)
	{
		file->data[i]=buffer[i];
		i++;
	}
	file->data[i] = '\0';
	file->data_size = buffer_line;
	create_node(file);
}
static size_t	put_complete_lines(char *buffer, size_t used, t_file **file)
{
	size_t	i;
	size_t	start;

	i = 0;
	start = 0;
	while (i < used)
	{
		if (buffer[i] == '\n')
		{
			buffer[i] = '\0';
			put_line(buffer + start, *file);
			*file = (*file)->next;
			start = i + 1;
		}
		i++;
	}
	return (start);
}

static size_t	move_remaining(char *buffer, size_t used, size_t start)
{
	size_t	i;

	i = 0;
	while (start + i < used)
	{
		buffer[i] = buffer[start + i];
		i++;
	}
	return (i);
}

void	create_file(int fd, t_file *file)
{
	char		buffer[SIZE_BUFFER + 1];
	ssize_t		bytes;
	size_t		used;
	size_t		start;

	used = 0;
	while (1)
	{
		bytes = read(fd, buffer + used, SIZE_BUFFER - used);
		if (bytes <= 0)
			break;
		used += (size_t)bytes;

		start = put_complete_lines(buffer, used, &file);
		used = move_remaining(buffer, used, start);

		if (used == SIZE_BUFFER)
			return;
	}
	if (used > 0)
	{
		buffer[used] = '\0';
		put_line(buffer, file);
	}
}

int	main(int argc ,char **argv)
{
	t_file *file;
	file = malloc(sizeof(t_file));
	int fd = open("test.txt",O_RDONLY);
	create_file(fd,file);
	while(file->data != NULL)
	{
		printf("%s\n",file->data);
		file = file->next;
	}

}

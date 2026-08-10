#include "file.h"

#include <stdlib.h>
#include <unistd.h>

#define READ_SIZE 4096

static int	builder_append(t_builder *builder,
				const char *data, size_t len)
{
	char	*new_data;
	size_t	new_capacity;
	size_t	i;

	if (len == 0)
		return (0);
	if (builder->len + len > builder->capacity)
	{
		new_capacity = builder->capacity;
		if (new_capacity == 0)
			new_capacity = READ_SIZE;
		while (new_capacity < builder->len + len)
			new_capacity *= 2;
		new_data = realloc(builder->data, new_capacity);
		if (new_data == NULL)
			return (-1);
		builder->data = new_data;
		builder->capacity = new_capacity;
	}
	i = 0;
	while (i < len)
	{
		builder->data[builder->len + i] = data[i];
		i++;
	}
	builder->len += len;
	return (0);
}

static int	builder_push_line(t_file *file, t_builder *builder)
{
	t_line	*line;

	line = line_create(builder->data, builder->len);
	if (line == NULL)
		return (-1);
	file_append_line(file, line);
	builder->len = 0;
	return (0);
}

static int	process_chunk(t_file *file, t_builder *builder,
				const char *buffer, size_t size)
{
	size_t	i;
	size_t	start;

	i = 0;
	start = 0;
	while (i < size)
	{
		if (buffer[i] == '\n')
		{
			if (builder_append(builder,
					buffer + start, i - start) < 0)
				return (-1);
			if (builder_push_line(file, builder) < 0)
				return (-1);
			start = i + 1;
		}
		i++;
	}
	return (builder_append(builder,
			buffer + start, size - start));
}

static int	load_error(t_file *file, t_builder *builder)
{
	free(builder->data);
	file_destroy(file);
	return (-1);
}

int	file_load(t_file *file, int fd)
{
	char		buffer[READ_SIZE];
	t_builder	builder;
	ssize_t		bytes;

	builder.data = NULL;
	builder.len = 0;
	builder.capacity = 0;
	while (1)
	{
		bytes = read(fd, buffer, sizeof(buffer));
		if (bytes < 0)
			return (load_error(file, &builder));
		if (bytes == 0)
			break ;
		file->ends_with_newline
			= (buffer[(size_t)bytes - 1] == '\n');
		if (process_chunk(file, &builder,
				buffer, (size_t)bytes) < 0)
			return (load_error(file, &builder));
	}
	if (builder.len > 0)
	{
		if (builder_push_line(file, &builder) < 0)
			return (load_error(file, &builder));
	}
	free(builder.data);
	return (0);
}

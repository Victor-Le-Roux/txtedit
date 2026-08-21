#include "line.h"
#include "file.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

t_line *line_create(const char *data, size_t len)
{
	t_line *line;
	size_t i;

	if (len == SIZE_MAX || (data == NULL && len > 0))
		return (NULL);
	line = malloc(sizeof(*line));
	if(line == NULL)
		return NULL;
	line->data = malloc(len + 1);
	if(line->data == NULL)
	{
		free(line);
		return (NULL);
	}
	i = 0;
	while(i < len)
	{
		line->data[i] = data[i];
		i++;
	}
	line->data[len] = '\0';
	line->len = len;
	line->capacity = len;
	line->previous = NULL;
	line->next = NULL;
	return (line);
}
int resize_line(t_line *line, size_t additional_buffer)
{
	char *new_data_line;
	size_t new_capacity;
	size_t required;

	if (!line)
		return (-1);
	if (additional_buffer > SIZE_MAX - line->len)
		return (-1);	
	required = line->len + additional_buffer;
	if (required == SIZE_MAX)
		return (-1);
	if(required <= line->capacity)
		return (0);
	new_capacity = line->capacity;
	if(new_capacity == 0)
		new_capacity = 16;
	while(new_capacity < required)
	{
		if(new_capacity > SIZE_MAX / 2)
		{
			new_capacity = required;	
			break;
		}
		new_capacity *=2;
	}
	new_data_line = realloc(line->data, new_capacity + 1);
	if(!new_data_line)
		return (-1);
	line->data = new_data_line;
	line->capacity = new_capacity;
	return (0);
}
void line_destroy(t_line *line)
{
	if(line == NULL)
		return;
	free(line->data);
	free(line);
}
int	line_insert(t_line *line, size_t pos,const char *data, size_t len)
{
	char	*temporary;
	size_t	i;
	size_t	source_position;

	if (!line || (!data && len > 0))
		return (-1);
	if (pos > line->len)
		return (-1);
	if (len == 0)
		return (0);
	temporary = NULL;
	source_position = 0;
	while (source_position <= line->len
		&& data != line->data + source_position)
		source_position++;
	if (source_position <= line->len)
	{
		if (len > line->len - source_position)
			return (-1);
		temporary = malloc(len);
		if (!temporary)
			return (-1);
		i = 0;
		while (i < len)
		{
			temporary[i] = data[i];
			i++;
		}
		data = temporary;
	}
	if (resize_line(line, len) < 0)
	{
		free(temporary);
		return (-1);
	}
	i = line->len;
	while(i > pos)
	{
		line->data[i + len - 1] = line->data[i-1];
		i--;
	}
	i = 0;
	while(i < len)
	{
		line->data[pos + i] = data[i];
		i++;
	}
	line->len += len;
	line->data[line->len] = '\0';
	free(temporary);
	return (0);
}

int line_delete(t_line *line, size_t position,size_t count)
{
	size_t i;
	if (!line)
		return (-1);
	if (position > line->len)
		return (-1);
	if (position == line->len)
	{
		if(count == 0)
			return (0);
		return (-1);
	}
	if (count == 0)
		return (0);
	if(count > line->len - position)
		count = line->len - position;
	i = position;
	while(i + count < line->len)
	{
		line->data[i] = line->data[i + count];
		i++;
	}
	line->len -=count;
	line->data[line->len] = '\0';
	return (0);
}

int line_split(t_file *file , t_line *line, size_t position)
{
	t_line *new_line;
	size_t	i;

	if(!file || !line || position > line->len
			|| !file_contains_line(file,line))
		return (-1);
	new_line = line_create(line->data + position, line->len - position);
	if(!new_line)
		return (-1);
	if(line_delete(line, position, line->len - position) < 0)
	{
		line_destroy(new_line);
		return (-1);
	}
	if(file_insert_after(file, line, new_line) < 0)
	{
		i = 0;
		while (i < new_line->len)
		{
			line->data[position + i] = new_line->data[i];
			i++;
		}
		line->len += new_line->len;
		line->data[line->len] = '\0';
		line_destroy(new_line);
		return (-1);
	}
	return (0);
}
int line_merge(t_file *file,t_line *line,t_line *second_line)
{
	size_t i;
	size_t original_len;

	if(!file || !line || !second_line || !file_contains_line(file, line) || !file_contains_line(file, second_line))
		return (-1);
	if(line->next != second_line)
		return (-1);
	if (second_line->len > SIZE_MAX - line->len)
		return (-1);
	if(line->capacity < line->len + second_line->len)
	{
		if(resize_line(line, second_line->len) < 0)
			return (-1);
	}
	original_len = line->len;
	i = 0;
	while(i < second_line->len)
	{
		line->data[line->len + i] = second_line->data[i];
		i++;
	}
	line->len += second_line->len;
	line->data[line->len] = '\0';
	if (file_delete_line(file, second_line) < 0)
	{
		line->len = original_len;
		line->data[line->len] = '\0';
		return (-1);
	}
	return (0);
}

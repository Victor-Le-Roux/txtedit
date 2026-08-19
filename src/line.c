#include "line.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

t_line *line_create(const char *data, size_t len)
{
	t_line *line;
	size_t i;

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
	size_t	i;

	if (!line || (!data && len > 0))
		return (-1);
	if (pos > line->len)
		return (-1);
	if (len == 0)
		return (0);
	if (resize_line(line, len) < 0)
		return (-1);
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
	return (0);
}

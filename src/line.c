#include "line.h"
#include <stddef.h>
#include <stdlib.h>

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
	line->previous = NULL;
	line->next = NULL;
	return (line);
}
void line_destroy(t_line *line)
{
	if(line == NULL)
		return;
	free(line->data);
	free(line);
}

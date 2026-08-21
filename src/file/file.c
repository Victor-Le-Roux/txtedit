#include "file.h"
#include "line.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

int file_contains_line(const t_file *file, const t_line *target)
{
	t_line *line;

	if (!file || !target)
		return (0);
	line = file->head;
	while(line)
	{
		if(line == target)
			return (1);
		line = line->next;
	}
	return (0);
}



void file_init(t_file *file)
{
	if (file == NULL)
		return ;
	file->head = NULL;
	file->tail = NULL;
	file->line_count = 0;
	file->ends_with_newline = 0;
}

int file_append_line(t_file *file, t_line *line)
{
	if(file == NULL || line == NULL || file->line_count == SIZE_MAX)
		return (-1);
	if(file_contains_line(file, line))
		return(-1);
	line->previous = file->tail;
	line->next = NULL;
	if(file->tail != NULL)
		file->tail->next = line;
	else
		file->head = line;
	file->tail = line;
	file->line_count++;
	return (0);
}

void file_destroy(t_file *file)
{
	t_line *line;
	t_line *next;

	if	(file == NULL)
		return ;
	line = file->head;
	while(line != NULL)
	{
		next = line->next;
		line_destroy(line);
		line = next;
	}
	file_init(file);
}

#include "file.h"
#include "line.h"

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

void file_init(t_file *file)
{
	file->head = NULL;
	file->tail = NULL;
	file->line_count = 0;
	file->ends_with_newline = 0;
}

int file_append_line(t_file *file, t_line *line)
{
	if(file == NULL || line == NULL)
		return (-1);
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

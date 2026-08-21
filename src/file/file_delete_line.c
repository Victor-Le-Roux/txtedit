#include "file.h"

int file_delete_line(t_file *file,t_line *line)
{
	if(!file || !line || file->line_count == 0
		|| !file_contains_line(file,line))
		return (-1);
	if(line->previous)
		line->previous->next = line->next;
	else
		file->head = line->next;
	if(line->next)
		line->next->previous = line->previous;
	else
		file->tail = line->previous;
	line_destroy(line);
	file->line_count--;
	return (0);
}

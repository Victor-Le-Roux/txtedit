#include "file.h"

#include <stdint.h>

int file_insert_after(t_file *file, t_line *current_line,t_line *new_line)
{
	if(!file || !current_line || !new_line
		|| file->line_count == SIZE_MAX)
		return (-1);
	if(!file_contains_line(file, current_line))
		return (-1);
	if(file_contains_line(file, new_line))
		return (-1);

	new_line->previous = current_line ;
	new_line->next = current_line->next;

	if(current_line->next)
		current_line->next->previous = new_line;
	else
		file->tail = new_line;
	current_line->next =new_line;
	file->line_count++;
	return (0);
}

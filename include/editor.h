#ifndef EDITOR_H
#	define EDITOR_H

#	include <stddef.h>
#	include "file.h"

typedef struct s_editor
{
	t_file	file;
	t_line	*current_line;
	size_t	cursor_x;
	size_t	cursor_y;
	int		running;
} t_editor;

void	editor_init(t_editor *editor);
void	editor_destroy(t_editor *editor);
int		editor_run(t_editor *editor);

#endif

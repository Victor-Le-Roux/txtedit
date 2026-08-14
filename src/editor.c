#include "editor.h"

void	editor_init(t_editor *editor)
{
	if (editor == NULL)
		return ;
	file_init(&editor->file);
	editor->current_line = NULL;
	editor->cursor_x = 0;
	editor->cursor_y = 0;
	editor->running = 1;
}

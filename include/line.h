#ifndef LINE_H
#	define LINE_H

# include <stddef.h>

typedef struct s_line
{
	char			*data;
	size_t			len;
	size_t			capacity;
	struct s_line	*previous;
	struct s_line	*next;
}	t_line;

/* Operations return 0 on success and -1 on error. */

t_line	*line_create(const char *data,size_t len);
void	line_destroy(t_line *line);
int resize_line(t_line *line, size_t additional_buffer);
int	line_insert(t_line *line, size_t pos,const char *data, size_t len);
int line_delete(t_line *line, size_t position,size_t count);

#endif

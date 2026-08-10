#ifndef LINE_H
#	define LINE_H

# include <stddef.h>

typedef struct s_line
{
	char			*data;
	size_t			len;
	struct s_line	*previous;
	struct s_line	*next;
}	t_line;

t_line	*line_create(const char *data,size_t len);
void	line_destroy(t_line *line);

#endif

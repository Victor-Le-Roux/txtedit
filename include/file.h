#ifndef FILE_H
# define FILE_H

# include <stddef.h>
# include "line.h"

typedef struct s_file
{
	t_line	*head;
	t_line	*tail;
	size_t	line_count;
	int		ends_with_newline;
}	t_file;

typedef struct s_builder
{
	char	*data;
	size_t	len;
	size_t	capacity;
}	t_builder;

/* line */
t_line	*line_create(const char *data, size_t len);
void	line_destroy(t_line *line);

/* file */
void	file_init(t_file *file);
int	file_append_line(t_file *file, t_line *line);
void	file_destroy(t_file *file);

/* loading */
int		file_load(t_file *file, int fd);

#endif

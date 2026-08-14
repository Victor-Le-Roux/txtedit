#include "file.h"

#include <errno.h>
#include <stddef.h>
#include <unistd.h>

static int safe_write(int fd, const char *data, size_t len)

{
	ssize_t bytes;
	size_t	written;
	written = 0;

	while(written < len)
	{
		bytes = write(fd,data + written , len - written);
		while(bytes < 0 && errno == EINTR)
			bytes = write(fd, data + written, len - written);
		if(bytes <= 0)
			return (-1);
		written += (size_t)bytes;
	}
	return (0);
}

int	file_save(const t_file *file, int fd)
{
	t_line	*line;

	if (fd < 0 || file == NULL)
		return (-1);
	line = file->head;
	while (line != NULL)
	{
		if (safe_write(fd, line->data, line->len) < 0)
			return (-1);
		if(line->next != NULL || file->ends_with_newline)
		{
			if(safe_write(fd,"\n" ,1) < 0)
				return (-1);
		}
		line = line->next;
	}
	return (0);
}

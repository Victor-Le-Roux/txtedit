#ifndef TEST_FAKES_H
# define TEST_FAKES_H

# include <stddef.h>
# include <sys/types.h>

void	*test_malloc(size_t size);
void	*test_realloc(void *pointer, size_t size);
void	test_free(void *pointer);
ssize_t	test_read(int fd, void *buffer, size_t size);
ssize_t	test_write(int fd, const void *buffer, size_t size);

#endif

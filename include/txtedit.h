#ifndef TXTEDIT_H
# define TXTEDIT_H 

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct s_file{

	char *data;
	struct s_file *previous;
	struct s_file *next;
	size_t  position;
} t_file;


#endif

#ifndef TXTEDIT_H
# define TXTEDIT_H 

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

#define SIZE_BUFFER 4096

typedef struct s_file{

	char *data;
	struct s_file *previous;
	struct s_file *next;
	size_t  position;
	size_t	data_size;
} t_file;


#endif

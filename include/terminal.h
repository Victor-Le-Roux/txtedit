#ifndef TERMINAL_H
#	define TERMINAL_H

#include <termios.h>

typedef struct s_terminal
{
	struct termios original;
	int raw_enabled;
} t_terminal;

int		terminal_enable_raw(t_terminal *terminal);
void	terminal_disable_raw(t_terminal *terminal);
int		terminal_read_key(void);
void	terminal_clear(void);

#endif

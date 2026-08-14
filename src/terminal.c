#include "terminal.h"

#include <unistd.h>

void	terminal_clear(void)
{
	(void)write(STDOUT_FILENO, "\033[2J\033[H", 7);
}

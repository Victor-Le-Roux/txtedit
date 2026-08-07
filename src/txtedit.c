#include "txtedit.h"
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

int	main(void)
{
	struct termios	old_term;
	struct termios	raw_term;
	char			c;
	char			seq[2];
	char			*items[] = {
		"Open file",
		"Create file",
		"Settings",
		"Quit"
	};
	int				selected;
	int				i;
	int				running;

	selected = 0;
	running = 1;

	tcgetattr(STDIN_FILENO, &old_term);

	raw_term = old_term;
	raw_term.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_term);

	while (running)
	{
		printf("\x1b[2J");
		printf("\x1b[H");

		printf("=== MENU ===\r\n\r\n");

		i = 0;
		while (i < 4)
		{
			if (i == selected)
			{
				printf("\x1b[7m");
				printf("  %s  ", items[i]);
				printf("\x1b[0m");
			}
			else
				printf("  %s  ", items[i]);

			printf("\r\n");
			i++;
		}

		if (read(STDIN_FILENO, &c, 1) != 1)
			break;

		if (c == '\x1b')
		{
			if (read(STDIN_FILENO, &seq[0], 1) != 1)
				continue;
			if (read(STDIN_FILENO, &seq[1], 1) != 1)
				continue;

			if (seq[0] == '[')
			{
				if (seq[1] == 'A')
				selected--;
				else if (seq[1] == 'B')
					selected++;
			}
		}
		else if (c == '\r' || c == '\n')
			running = 0;

		if (selected < 0)
			selected = 3;
		else if (selected > 3)
			selected = 0;
	}

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_term);

	printf("\x1b[2J\x1b[H");
	printf("Tu as choisi : %s\n", items[selected]);

	return (0);
}

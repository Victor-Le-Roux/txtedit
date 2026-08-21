#include "editor.h"
#include "file.h"
#include "line.h"
#include "terminal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FAKE_FD 4242
#define OUTPUT_CAPACITY 262144
#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

typedef int	(*t_test_fn)(void);

typedef struct s_test
{
	const char	*name;
	t_test_fn	function;
} t_test;

static int		g_alloc_fail_after = -1;
static size_t		g_live_allocations;
static const char	*g_read_data;
static size_t		g_read_len;
static size_t		g_read_position;
static size_t		g_read_chunk;
static int		g_read_eintr;
static int		g_read_error_after = -1;
static char		g_output[OUTPUT_CAPACITY];
static size_t		g_output_len;
static size_t		g_write_chunk;
static int		g_write_eintr;
static int		g_write_error_after = -1;
static int		g_write_zero;
static int		g_capture_stdout;

static int	allocation_must_fail(void)
{
	if (g_alloc_fail_after < 0)
		return (0);
	if (g_alloc_fail_after == 0)
	{
		g_alloc_fail_after = -1;
		return (1);
	}
	g_alloc_fail_after--;
	return (0);
}

void	*test_malloc(size_t size)
{
	void	*pointer;

	if (allocation_must_fail())
		return (NULL);
	pointer = malloc(size);
	if (pointer != NULL)
		g_live_allocations++;
	return (pointer);
}

void	*test_realloc(void *pointer, size_t size)
{
	void	*new_pointer;
	int	was_null;

	if (allocation_must_fail())
		return (NULL);
	was_null = (pointer == NULL);
	new_pointer = realloc(pointer, size);
	if (was_null && new_pointer != NULL)
		g_live_allocations++;
	return (new_pointer);
}

void	test_free(void *pointer)
{
	if (pointer != NULL)
		g_live_allocations--;
	free(pointer);
}

ssize_t	test_read(int fd, void *buffer, size_t size)
{
	size_t	remaining;
	size_t	amount;

	if (fd != FAKE_FD)
		return (read(fd, buffer, size));
	if (g_read_eintr > 0)
	{
		g_read_eintr--;
		errno = EINTR;
		return (-1);
	}
	if (g_read_error_after == 0)
	{
		errno = EIO;
		return (-1);
	}
	if (g_read_error_after > 0)
		g_read_error_after--;
	remaining = g_read_len - g_read_position;
	if (remaining == 0)
		return (0);
	amount = size;
	if (g_read_chunk != 0 && amount > g_read_chunk)
		amount = g_read_chunk;
	if (amount > remaining)
		amount = remaining;
	memcpy(buffer, g_read_data + g_read_position, amount);
	g_read_position += amount;
	return ((ssize_t)amount);
}

ssize_t	test_write(int fd, const void *buffer, size_t size)
{
	size_t	amount;

	if (fd != FAKE_FD && !(g_capture_stdout && fd == STDOUT_FILENO))
		return (write(fd, buffer, size));
	if (g_write_eintr > 0)
	{
		g_write_eintr--;
		errno = EINTR;
		return (-1);
	}
	if (g_write_error_after == 0)
	{
		errno = EIO;
		return (-1);
	}
	if (g_write_error_after > 0)
		g_write_error_after--;
	if (g_write_zero)
		return (0);
	amount = size;
	if (g_write_chunk != 0 && amount > g_write_chunk)
		amount = g_write_chunk;
	if (amount > OUTPUT_CAPACITY - g_output_len)
	{
		errno = ENOSPC;
		return (-1);
	}
	memcpy(g_output + g_output_len, buffer, amount);
	g_output_len += amount;
	return ((ssize_t)amount);
}

static void	reset_fakes(void)
{
	g_alloc_fail_after = -1;
	g_read_data = "";
	g_read_len = 0;
	g_read_position = 0;
	g_read_chunk = 0;
	g_read_eintr = 0;
	g_read_error_after = -1;
	g_output_len = 0;
	g_write_chunk = 0;
	g_write_eintr = 0;
	g_write_error_after = -1;
	g_write_zero = 0;
	g_capture_stdout = 0;
}

static void	fake_input(const char *data, size_t len, size_t chunk)
{
	g_read_data = data;
	g_read_len = len;
	g_read_position = 0;
	g_read_chunk = chunk;
}

#define CHECK(condition) do { \
	if (!(condition)) { \
		fprintf(stderr, "%s:%d: assertion failed: %s\n", \
			__FILE__, __LINE__, #condition); \
		return (0); \
	} \
} while (0)

static int	line_equals(const t_line *line, const char *data, size_t len)
{
	return (line != NULL && line->len == len
		&& memcmp(line->data, data, len) == 0 && line->data[len] == '\0');
}

static t_line	*append_text(t_file *file, const char *text)
{
	t_line	*line;

	line = line_create(text, strlen(text));
	if (line == NULL || file_append_line(file, line) < 0)
	{
		line_destroy(line);
		return (NULL);
	}
	return (line);
}

static int	test_line_create_destroy(void)
{
	t_line	*line;

	line_destroy(NULL);
	line = line_create("a\0b", 3);
	CHECK(line_equals(line, "a\0b", 3));
	CHECK(line->capacity == 3 && line->previous == NULL && line->next == NULL);
	line_destroy(line);
	line = line_create(NULL, 0);
	CHECK(line_equals(line, "", 0));
	line_destroy(line);
	CHECK(line_create(NULL, 2) == NULL);
	CHECK(line_create("x", SIZE_MAX) == NULL);
	g_alloc_fail_after = 0;
	CHECK(line_create("x", 1) == NULL);
	g_alloc_fail_after = 1;
	CHECK(line_create("x", 1) == NULL);
	return (1);
}

static int	test_resize_line(void)
{
	t_line	*line;
	size_t	old_capacity;

	CHECK(resize_line(NULL, 1) == -1);
	line = line_create("abc", 3);
	CHECK(line != NULL);
	old_capacity = line->capacity;
	CHECK(resize_line(line, 0) == 0 && line->capacity == old_capacity);
	CHECK(resize_line(line, 20) == 0);
	CHECK(line->capacity >= 23 && line_equals(line, "abc", 3));
	old_capacity = line->capacity;
	g_alloc_fail_after = 0;
	CHECK(resize_line(line, old_capacity + 1) == -1);
	CHECK(line->capacity == old_capacity && line_equals(line, "abc", 3));
	line->len = SIZE_MAX;
	line->capacity = SIZE_MAX;
	CHECK(resize_line(line, 1) == -1);
	line->len = SIZE_MAX - 1;
	CHECK(resize_line(line, 1) == -1);
	line->len = SIZE_MAX / 2 + 1;
	line->capacity = line->len;
	g_alloc_fail_after = 0;
	CHECK(resize_line(line, 1) == -1);
	line->len = 3;
	line->capacity = old_capacity;
	line_destroy(line);
	line = line_create("", 0);
	CHECK(line != NULL && resize_line(line, 1) == 0 && line->capacity == 16);
	line_destroy(line);
	return (1);
}

static int	test_line_insert(void)
{
	t_line	*line;
	const char	*source;

	CHECK(line_insert(NULL, 0, "x", 1) == -1);
	line = line_create("abcd", 4);
	CHECK(line != NULL);
	CHECK(line_insert(line, 0, "<", 1) == 0);
	CHECK(line_insert(line, 3, "XY", 2) == 0);
	CHECK(line_insert(line, line->len, ">", 1) == 0);
	CHECK(line_equals(line, "<abXYcd>", 8));
	CHECK(line_insert(line, line->len + 1, "x", 1) == -1);
	CHECK(line_insert(line, 0, NULL, 1) == -1);
	CHECK(line_insert(line, 0, NULL, 0) == 0);
	CHECK(line_insert(line, 0, "ignored", 0) == 0);
	line_destroy(line);
	line = line_create("abcd", 4);
	CHECK(line != NULL);
	g_alloc_fail_after = 0;
	CHECK(line_insert(line, 2, "0123456789", 10) == -1);
	CHECK(line_equals(line, "abcd", 4));
	line_destroy(line);
	line = line_create("abcd", 4);
	CHECK(line != NULL);
	source = line->data;
	CHECK(line_insert(line, 2, source, 4) == 0);
	CHECK(line_equals(line, "ababcdcd", 8));
	line_destroy(line);
	line = line_create("abcdef", 6);
	CHECK(line != NULL);
	source = line->data + 2;
	CHECK(line_insert(line, 1, source, 3) == 0);
	CHECK(line_equals(line, "acdebcdef", 9));
	line_destroy(line);
	line = line_create("abcd", 4);
	CHECK(line != NULL);
	CHECK(line_insert(line, 0, line->data + 3, 2) == -1);
	CHECK(line_equals(line, "abcd", 4));
	g_alloc_fail_after = 0;
	CHECK(line_insert(line, 2, line->data, line->len) == -1);
	CHECK(line_equals(line, "abcd", 4));
	g_alloc_fail_after = 1;
	CHECK(line_insert(line, 2, line->data, line->len) == -1);
	CHECK(line_equals(line, "abcd", 4));
	line_destroy(line);
	return (1);
}

static int	test_line_delete(void)
{
	t_line	*line;

	CHECK(line_delete(NULL, 0, 1) == -1);
	line = line_create("abcdef", 6);
	CHECK(line != NULL);
	CHECK(line_delete(line, 2, 0) == 0 && line_equals(line, "abcdef", 6));
	CHECK(line_delete(line, 1, 2) == 0 && line_equals(line, "adef", 4));
	CHECK(line_delete(line, 2, 99) == 0 && line_equals(line, "ad", 2));
	CHECK(line_delete(line, line->len, 0) == 0 && line_equals(line, "ad", 2));
	CHECK(line_delete(line, line->len, 1) == -1 && line_equals(line, "ad", 2));
	CHECK(line_delete(line, line->len + 1, 1) == -1
		&& line_equals(line, "ad", 2));
	CHECK(line_delete(line, 0, 99) == 0 && line_equals(line, "", 0));
	CHECK(line_delete(line, 0, 0) == 0 && line_equals(line, "", 0));
	CHECK(line_delete(line, 0, 1) == -1 && line_equals(line, "", 0));
	line_destroy(line);
	return (1);
}

static int	test_file_append_destroy(void)
{
	t_file	file;
	t_line	*first;
	t_line	*second;

	file_init(&file);
	file_init(NULL);
	CHECK(file.head == NULL && file.tail == NULL && file.line_count == 0);
	CHECK(file_append_line(NULL, NULL) == -1);
	CHECK(file_append_line(&file, NULL) == -1);
	first = line_create("one", 3);
	second = line_create("two", 3);
	CHECK(first != NULL && second != NULL);
	CHECK(file_contains_line(NULL, first) == 0);
	CHECK(file_contains_line(&file, NULL) == 0);
	CHECK(file_contains_line(&file, first) == 0);
	CHECK(file_append_line(NULL, first) == -1);
	file.line_count = SIZE_MAX;
	CHECK(file_append_line(&file, first) == -1);
	file.line_count = 0;
	CHECK(file_append_line(&file, first) == 0);
	CHECK(file_append_line(&file, first) == -1);
	CHECK(file_append_line(&file, second) == 0);
	CHECK(file.head == first && file.tail == second && file.line_count == 2);
	CHECK(first->previous == NULL && first->next == second);
	CHECK(second->previous == first && second->next == NULL);
	file_destroy(&file);
	CHECK(file.head == NULL && file.tail == NULL && file.line_count == 0);
	file_destroy(NULL);
	return (1);
}

static int	test_file_insert_after(void)
{
	t_file	file;
	t_line	*alien;
	t_line	*one;
	t_line	*two;
	t_line	*three;
	t_line	*middle;

	file_init(&file);
	one = append_text(&file, "one");
	three = append_text(&file, "three");
	two = line_create("two", 3);
	middle = line_create("middle", 6);
	alien = line_create("alien", 5);
	CHECK(one && two && three && middle && alien);
	CHECK(file_insert_after(NULL, one, two) == -1);
	CHECK(file_insert_after(&file, NULL, two) == -1);
	CHECK(file_insert_after(&file, one, NULL) == -1);
	CHECK(file_insert_after(&file, alien, two) == -1);
	CHECK(file_insert_after(&file, one, three) == -1);
	file.line_count = SIZE_MAX;
	CHECK(file_insert_after(&file, one, two) == -1);
	file.line_count = 2;
	CHECK(file_insert_after(&file, one, two) == 0);
	CHECK(one->next == two && two->next == three && three->previous == two);
	CHECK(file_insert_after(&file, two, middle) == 0);
	CHECK(two->next == middle && middle->next == three);
	CHECK(file.line_count == 4 && file.tail == three);
	line_destroy(alien);
	file_destroy(&file);
	return (1);
}

static int	test_file_delete_positions(void)
{
	t_file	file;
	t_line	*alien;
	t_line	*one;
	t_line	*two;
	t_line	*three;

	file_init(&file);
	one = append_text(&file, "one");
	two = append_text(&file, "two");
	three = append_text(&file, "three");
	alien = line_create("alien", 5);
	CHECK(one && two && three && alien);
	CHECK(file_delete_line(NULL, one) == -1);
	CHECK(file_delete_line(&file, NULL) == -1);
	CHECK(file_delete_line(&file, alien) == -1);
	line_destroy(alien);
	CHECK(file_delete_line(&file, two) == 0);
	CHECK(file.head == one && one->next == three && three->previous == one);
	CHECK(file_delete_line(&file, one) == 0);
	CHECK(file.head == three && three->previous == NULL && file.line_count == 1);
	CHECK(file_delete_line(&file, three) == 0);
	CHECK(file.head == NULL && file.tail == NULL && file.line_count == 0);
	return (1);
}

static int	test_line_split(void)
{
	t_file	file;
	t_line	*alien;
	t_line	*line;

	file_init(&file);
	line = append_text(&file, "abcdef");
	CHECK(line != NULL);
	CHECK(line_split(NULL, line, 2) == -1);
	CHECK(line_split(&file, NULL, 2) == -1);
	CHECK(line_split(&file, line, 7) == -1);
	alien = line_create("alien", 5);
	CHECK(alien != NULL && line_split(&file, alien, 2) == -1);
	line_destroy(alien);
	CHECK(line_split(&file, line, 3) == 0);
	CHECK(file.line_count == 2 && line_equals(file.head, "abc", 3));
	CHECK(line_equals(file.tail, "def", 3));
	CHECK(line_split(&file, file.head, 0) == 0);
	CHECK(line_equals(file.head, "", 0) && line_equals(file.head->next, "abc", 3));
	CHECK(line_split(&file, file.tail, file.tail->len) == 0);
	CHECK(line_equals(file.tail, "", 0) && file.line_count == 4);
	file_destroy(&file);
	file_init(&file);
	line = append_text(&file, "abc");
	CHECK(line != NULL);
	g_alloc_fail_after = 0;
	CHECK(line_split(&file, line, 1) == -1);
	CHECK(file.line_count == 1 && line_equals(line, "abc", 3));
	g_alloc_fail_after = -1;
	file.line_count = SIZE_MAX;
	CHECK(line_split(&file, line, 1) == -1);
	CHECK(line_equals(line, "abc", 3));
	file.line_count = 1;
	file_destroy(&file);
	return (1);
}

static int	test_line_merge(void)
{
	t_file	file;
	t_line	*alien;
	t_line	*one;
	t_line	*two;
	t_line	*three;

	file_init(&file);
	one = append_text(&file, "one");
	two = append_text(&file, "two");
	three = append_text(&file, "three");
	alien = line_create("alien", 5);
	CHECK(one && two && three && alien);
	CHECK(line_merge(NULL, one, two) == -1);
	CHECK(line_merge(&file, NULL, two) == -1);
	CHECK(line_merge(&file, one, NULL) == -1);
	CHECK(line_merge(&file, alien, two) == -1);
	CHECK(line_merge(&file, one, alien) == -1);
	CHECK(line_merge(&file, one, three) == -1);
	two->len = SIZE_MAX;
	CHECK(line_merge(&file, one, two) == -1);
	two->len = 3;
	file.line_count = 0;
	CHECK(line_merge(&file, one, two) == -1);
	CHECK(line_equals(one, "one", 3) && line_equals(two, "two", 3));
	file.line_count = 3;
	CHECK(line_merge(&file, one, two) == 0);
	CHECK(line_equals(one, "onetwo", 6) && one->next == three);
	CHECK(file.line_count == 2 && three->previous == one);
	CHECK(resize_line(one, 32) == 0);
	CHECK(line_merge(&file, one, three) == 0);
	CHECK(line_equals(one, "onetwothree", 11));
	CHECK(file.head == one && file.tail == one && file.line_count == 1);
	line_destroy(alien);
	file_destroy(&file);
	file_init(&file);
	one = append_text(&file, "one");
	two = append_text(&file, "0123456789");
	CHECK(one && two);
	g_alloc_fail_after = 0;
	CHECK(line_merge(&file, one, two) == -1);
	CHECK(file.line_count == 2 && line_equals(one, "one", 3));
	file_destroy(&file);
	return (1);
}

static int	check_loaded(const char *input, size_t chunk,
		size_t expected_count, int expected_newline, const char **lines)
{
	t_file	file;
	t_line	*line;
	size_t	i;

	file_init(&file);
	fake_input(input, strlen(input), chunk);
	CHECK(file_load(&file, FAKE_FD) == 0);
	CHECK(file.line_count == expected_count);
	CHECK(file.ends_with_newline == expected_newline);
	line = file.head;
	i = 0;
	while (i < expected_count)
	{
		CHECK(line_equals(line, lines[i], strlen(lines[i])));
		line = line->next;
		i++;
	}
	CHECK(line == NULL);
	file_destroy(&file);
	return (1);
}

static int	test_file_load_shapes(void)
{
	const char	*plain[] = {"alpha", "beta"};
	const char	*empty_lines[] = {"", "", "x"};
	const char	*single_empty[] = {""};

	CHECK(check_loaded("", 0, 0, 0, plain));
	CHECK(check_loaded("alpha\nbeta", 2, 2, 0, plain));
	CHECK(check_loaded("alpha\nbeta\n", 1, 2, 1, plain));
	CHECK(check_loaded("\n\nx\n", 3, 3, 1, empty_lines));
	CHECK(check_loaded("\n", 0, 1, 1, single_empty));
	return (1);
}

static int	test_file_load_long_line(void)
{
	char	*input;
	t_file	file;
	size_t	i;

	input = malloc(10002);
	CHECK(input != NULL);
	i = 0;
	while (i < 10000)
	{
		input[i] = (char)('a' + (i % 26));
		i++;
	}
	input[10000] = '\n';
	input[10001] = '\0';
	file_init(&file);
	fake_input(input, 10001, 4096);
	CHECK(file_load(&file, FAKE_FD) == 0);
	CHECK(file.line_count == 1 && file.ends_with_newline == 1);
	CHECK(file.head->len == 10000 && memcmp(file.head->data, input, 10000) == 0);
	file_destroy(&file);
	free(input);
	return (1);
}

static int	test_file_load_errors(void)
{
	t_file	file;
	char	*long_input;
	int		failure;
	size_t	i;

	CHECK(file_load(NULL, FAKE_FD) == -1);
	file_init(&file);
	CHECK(file_load(&file, -1) == -1);
	file_init(&file);
	g_read_eintr = 2;
	fake_input("ok", 2, 0);
	CHECK(file_load(&file, FAKE_FD) == 0 && line_equals(file.head, "ok", 2));
	file_destroy(&file);
	file_init(&file);
	CHECK(append_text(&file, "old") != NULL);
	fake_input("new", 3, 0);
	g_read_error_after = 0;
	CHECK(file_load(&file, FAKE_FD) == -1);
	CHECK(file.head == NULL && file.tail == NULL && file.line_count == 0);
	g_read_error_after = -1;
	failure = 0;
	while (failure < 3)
	{
		file_init(&file);
		fake_input("abc", 3, 0);
		g_alloc_fail_after = failure;
		CHECK(file_load(&file, FAKE_FD) == -1);
		CHECK(file.head == NULL && file.tail == NULL && file.line_count == 0);
		failure++;
	}
	file_init(&file);
	fake_input("\n", 1, 0);
	g_alloc_fail_after = 0;
	CHECK(file_load(&file, FAKE_FD) == -1 && file.line_count == 0);
	file_init(&file);
	file.line_count = SIZE_MAX;
	fake_input("\n", 1, 0);
	CHECK(file_load(&file, FAKE_FD) == -1 && file.line_count == 0);
	long_input = malloc(5002);
	CHECK(long_input != NULL);
	i = 0;
	while (i < 5000)
		long_input[i++] = 'x';
	long_input[5000] = '\n';
	long_input[5001] = '\0';
	file_init(&file);
	fake_input(long_input, 5001, 4096);
	g_alloc_fail_after = 1;
	CHECK(file_load(&file, FAKE_FD) == -1 && file.line_count == 0);
	free(long_input);
	return (1);
}

static int	test_file_save_shapes(void)
{
	t_file	file;

	file_init(&file);
	CHECK(file_save(NULL, FAKE_FD) == -1);
	CHECK(file_save(&file, -1) == -1);
	CHECK(file_save(&file, FAKE_FD) == 0 && g_output_len == 0);
	CHECK(append_text(&file, "alpha") != NULL);
	CHECK(append_text(&file, "") != NULL);
	CHECK(append_text(&file, "beta") != NULL);
	file.ends_with_newline = 0;
	CHECK(file_save(&file, FAKE_FD) == 0);
	CHECK(g_output_len == 11 && memcmp(g_output, "alpha\n\nbeta", 11) == 0);
	g_output_len = 0;
	file.ends_with_newline = 1;
	CHECK(file_save(&file, FAKE_FD) == 0);
	CHECK(g_output_len == 12 && memcmp(g_output, "alpha\n\nbeta\n", 12) == 0);
	file_destroy(&file);
	return (1);
}

static int	test_file_save_write_behaviour(void)
{
	t_file	file;

	file_init(&file);
	CHECK(append_text(&file, "abcdef") != NULL);
	g_write_chunk = 2;
	g_write_eintr = 2;
	CHECK(file_save(&file, FAKE_FD) == 0);
	CHECK(g_output_len == 6 && memcmp(g_output, "abcdef", 6) == 0);
	g_output_len = 0;
	g_write_error_after = 0;
	CHECK(file_save(&file, FAKE_FD) == -1);
	g_write_error_after = -1;
	g_write_zero = 1;
	CHECK(file_save(&file, FAKE_FD) == -1);
	g_write_zero = 0;
	g_write_error_after = 1;
	g_write_chunk = 0;
	file.ends_with_newline = 1;
	CHECK(file_save(&file, FAKE_FD) == -1);
	file_destroy(&file);
	return (1);
}

static int	files_are_equal(const char *first_path, const char *second_path)
{
	char	first[4096];
	char	second[4096];
	ssize_t	first_size;
	ssize_t	second_size;
	int		first_fd;
	int		second_fd;
	int		equal;

	first_fd = open(first_path, O_RDONLY);
	second_fd = open(second_path, O_RDONLY);
	if (first_fd < 0 || second_fd < 0)
	{
		if (first_fd >= 0)
			close(first_fd);
		if (second_fd >= 0)
			close(second_fd);
		return (0);
	}
	equal = 1;
	while (equal)
	{
		first_size = read(first_fd, first, sizeof(first));
		second_size = read(second_fd, second, sizeof(second));
		if (first_size < 0 || second_size < 0 || first_size != second_size)
			equal = 0;
		else if (first_size == 0)
			break ;
		else if (memcmp(first, second, (size_t)first_size) != 0)
			equal = 0;
	}
	close(first_fd);
	close(second_fd);
	return (equal);
}

static int	test_fixture_round_trips(void)
{
	const char	*fixtures[] = {
		"tests/empty.txt", "tests/only_newline.txt", "tests/one_line.txt",
		"tests/no_final_newline.txt", "tests/empty_lines.txt",
		"tests/spaces_tabs.txt", "tests/utf8.txt", "tests/long_line.txt",
		"tests/large_document.txt"
	};
	char		output_path[] = "/tmp/txtedit-test-XXXXXX";
	t_file		file;
	size_t		i;
	int			input_fd;
	int			output_fd;

	i = 0;
	while (i < ARRAY_LEN(fixtures))
	{
		input_fd = open(fixtures[i], O_RDONLY);
		CHECK(input_fd >= 0);
		output_fd = mkstemp(output_path);
		CHECK(output_fd >= 0);
		file_init(&file);
		CHECK(file_load(&file, input_fd) == 0);
		CHECK(file_save(&file, output_fd) == 0);
		CHECK(close(input_fd) == 0 && close(output_fd) == 0);
		CHECK(files_are_equal(fixtures[i], output_path));
		CHECK(unlink(output_path) == 0);
		memcpy(output_path, "/tmp/txtedit-test-XXXXXX", sizeof(output_path));
		file_destroy(&file);
		i++;
	}
	return (1);
}

static int	test_editor_init_function(void)
{
	t_editor	editor;

	memset(&editor, 0xff, sizeof(editor));
	editor_init(NULL);
	editor_init(&editor);
	CHECK(editor.file.head == NULL && editor.file.tail == NULL);
	CHECK(editor.file.line_count == 0 && editor.file.ends_with_newline == 0);
	CHECK(editor.current_line == NULL && editor.cursor_x == 0 && editor.cursor_y == 0);
	CHECK(editor.running == 1);
	return (1);
}

static int	test_terminal_clear_function(void)
{
	g_capture_stdout = 1;
	terminal_clear();
	CHECK(g_output_len == 7);
	CHECK(memcmp(g_output, "\033[2J\033[H", 7) == 0);
	return (1);
}

int	main(void)
{
	const t_test	tests[] = {
		{"line_create/destroy", test_line_create_destroy},
		{"resize_line", test_resize_line},
		{"line_insert", test_line_insert},
		{"line_delete", test_line_delete},
		{"file_append/destroy", test_file_append_destroy},
		{"file_insert_after", test_file_insert_after},
		{"file_delete_line", test_file_delete_positions},
		{"line_split", test_line_split},
		{"line_merge", test_line_merge},
		{"file_load shapes", test_file_load_shapes},
		{"file_load long line", test_file_load_long_line},
		{"file_load errors", test_file_load_errors},
		{"file_save shapes", test_file_save_shapes},
		{"file_save write behaviour", test_file_save_write_behaviour},
		{"fixture round trips", test_fixture_round_trips},
		{"editor_init", test_editor_init_function},
		{"terminal_clear", test_terminal_clear_function}
	};
	size_t	passed;
	size_t	i;

	passed = 0;
	i = 0;
	while (i < ARRAY_LEN(tests))
	{
		int	test_passed;

		reset_fakes();
		test_passed = tests[i].function();
		if (g_live_allocations != 0)
		{
			fprintf(stderr, "%s: %zu allocation(s) not freed\n",
				tests[i].name, g_live_allocations);
			test_passed = 0;
		}
		if (test_passed)
		{
			printf("ok %zu - %s\n", i + 1, tests[i].name);
			passed++;
		}
		else
			printf("not ok %zu - %s\n", i + 1, tests[i].name);
		i++;
	}
	printf("%zu/%zu tests passed\n", passed, ARRAY_LEN(tests));
	return (passed == ARRAY_LEN(tests) ? 0 : 1);
}

#!/bin/sh
set -eu

export CCACHE_DISABLE=1

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="$project_root/build/tests"
test_binary="$build_dir/test_runner"
common_flags="-std=c17 -Wall -Wextra -Werror -Wpedantic -g3 -O0"
instrumentation_flags="-fsanitize=address,undefined -fno-omit-frame-pointer --coverage"
fake_flags="-Dmalloc=test_malloc -Drealloc=test_realloc -Dfree=test_free -Dread=test_read -Dwrite=test_write -include tests/test_fakes.h"

mkdir -p "$build_dir"
find "$build_dir" -type f -delete

for source in \
	src/editor.c \
	src/file/file.c \
	src/file/file_delete_line.c \
	src/file/file_insert_after.c \
	src/file/file_load.c \
	src/file/file_save.c \
	src/line.c \
	src/terminal.c
do
	object_name=$(printf '%s' "$source" | tr '/' '_')
	cc $common_flags $instrumentation_flags -Iinclude $fake_flags \
		-c "$project_root/$source" -o "$build_dir/$object_name.o"
done

cc $common_flags $instrumentation_flags -D_POSIX_C_SOURCE=200809L -Iinclude \
	-c "$project_root/tests/test_runner.c" -o "$build_dir/test_runner.o"
cc $instrumentation_flags "$build_dir"/*.o -o "$test_binary"

cd "$project_root"
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
	UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
	"$test_binary"

printf '\nCoverage by implementation file:\n'
cd "$build_dir"
coverage_output=$(gcov -b -c src_*.c.o 2>/dev/null)
printf '%s\n' "$coverage_output" \
	| awk '/^File .*src\// || /^Lines executed:|^Branches executed:|^Taken at least once:/'
line_coverage=$(printf '%s\n' "$coverage_output" \
	| awk -F '[:%]' '/^Lines executed:/ { value = $2 } END { print value }')
if ! awk -v coverage="$line_coverage" 'BEGIN { exit !(coverage >= 98.0) }'
then
	printf 'Error: line coverage is %s%%; expected at least 98%%.\n' \
		"$line_coverage" >&2
	exit 1
fi

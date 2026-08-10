NAME := txtedit
DEBUG_NAME := txtedit-debug

CC := cc
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic
CPPFLAGS := -Iinclude
DEBUG_FLAGS := -g3 -fsanitize=address,undefined -fno-omit-frame-pointer

rwildcard = $(foreach entry,$(wildcard $1*),$(call rwildcard,$(entry)/,$2)$(filter $(subst *,%,$2),$(entry)))

SRC := $(call rwildcard,src/,*.c)
HEADERS := $(call rwildcard,include/,*.h)

OBJ_DIR := build
OBJ := $(SRC:src/%.c=$(OBJ_DIR)/%.o)
DEP := $(OBJ:.o=.d)

.PHONY: all clean fclean re debug test check-sources

all: $(NAME)

$(NAME): $(OBJ) | check-sources
	$(CC) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: src/%.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

debug: $(DEBUG_NAME)

$(DEBUG_NAME): $(SRC) $(HEADERS) Makefile | check-sources
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(SRC) \
		$(LDFLAGS) $(LDLIBS) -o $@

test: debug
	@if [ ! -x tests/run_tests.sh ]; then \
		echo "Error: tests/run_tests.sh is missing or not executable."; \
		exit 1; \
	fi
	./tests/run_tests.sh ./$(DEBUG_NAME)

check-sources:
	@if [ -z "$(SRC)" ]; then \
		echo "Error: no C source file found in src/ or its subdirectories."; \
		exit 1; \
	fi

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(DEBUG_NAME)

re: fclean
	$(MAKE) all

-include $(DEP)

#####
#
# @file Makefile
# @author Radek Kalous (xkalour00)
# @brief FIT VUT | IFJ Project 2026 - Makefile for the compiler
# @date 2026-11-17
#
#####

CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Wextra -fsanitize=address
LDFLAGS += -fsanitize=address

BUILD_PATH = build/

# AST sources
AST_SRC = src/ast.h src/ast.c
# AST test sources
AST_TEST_SRC = test/ast_test_nodes.c

.PHONY: clean pack
default_target: all

test_ast:
	@mkdir -p $(BUILD_PATH)
	$(CC) $(CFLAGS) -g $(AST_SRC) $(AST_TEST_SRC) -o $(BUILD_PATH)$@

	@# execute built program
	@echo; echo "##### Plain output #####"; echo
	./$(BUILD_PATH)$@
	@echo; echo "##### Valgrind output #####"; echo
	valgrind -s --leak-check=full ./$(BUILD_PATH)$@
all:
	@mkdir -p $(BUILD_PATH)
	@echo "TODO make all"
pack:
	@echo "TODO make pack"
clean:
	@echo "TODO make clean"
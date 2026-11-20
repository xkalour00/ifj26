# --- FIT VUT
# IFJ Project 2026
# Author: Filip Obermajer / xobermf00
# Date: 2026-11-20

CC           = gcc
CFLAGS       = -std=c11 -pedantic -Wall -Wextra -Werror
TARGET       = ifj26-compiler
LOGIN        = xkalour00

SRC_DIR      = src
SRC_FILES    = $(wildcard $(SRC_DIR)/*.c)
HEADER_FILES = $(wildcard $(SRC_DIR)/*.h)
OBJS         = $(SRC_FILES:.c=.o)

.PHONY: all clean check pack

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(HEADER_FILES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS) $(LOGIN).zip

check: pack
	./is_it_ok.sh $(LOGIN).zip

pack:
	zip -r $(LOGIN).zip Makefile dokumentace.pdf rozdeleni rozsireni $(SRC_FILES) $(HEADER_FILES)

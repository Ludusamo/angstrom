SRCDIR = src
OBJDIR = obj
BINDIR = bin

TESTDIR = src/test

MKFILEPATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURDIR := $(dir $(MKFILEPATH))

SRC_FILES := $(wildcard $(SRCDIR)/*.c)

TEST_SRC_FILES := $(wildcard $(TESTDIR)/*.c)

OBJECTS :=  $(SRC_FILES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TEST_OBJECTS :=  $(TEST_SRC_FILES:$(TESTDIR)/%.c=$(OBJDIR)/%.o)

CC = clang
CFLAGS = -std=c99 -Wall -Iheader -IC-Data-Structures/header

TEST_CFLAGS = -std=c99 -Wall -Iheader -Iheader/test

LINKER_FLAGS = -Wall -Iheader

TARGET = angstrom

$(BINDIR)/$(TARGET): $(OBJECTS)
	@cd C-Data-Structures && $(MAKE)
	@if [ ! -d "bin" ]; then mkdir bin; fi
	$(CC) -o $@ $(LINKER_FLAGS) -rpath $(CURDIR)/C-Data-Structures/bin $(OBJECTS) -L./C-Data-Structures/bin -lcds
	echo "Linking Complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@if [ ! -d "obj" ]; then mkdir obj; fi
	$(CC) -c $(CFLAGS) $< -o $@
	echo "Compiled "$<" successfully!"

clean:
	@cd C-Data-Structures && $(MAKE) clean
	$(RM) $(OBJECTS)
	echo "Cleanup complete!"

remove:
	@cd C-Data-Structures && $(MAKE) remove
	make clean
	$(RM) $(BINDIR)/$(TARGET)
	echo "Executable removed!"

memcheck:
	valgrind --leak-check=full --tool=memcheck $(BINDIR)/$(TARGET)

memcheckfull:
	valgrind --leak-check=full --show-leak-kinds=all --tool=memcheck $(BINDIR)/$(TARGET)

.PHONY: memcheck memcheckfull remove clean debug release

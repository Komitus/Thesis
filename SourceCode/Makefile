.PHONY: all clean remove 

TARGET   = main
CC       = gcc
# compiling flags here
CFLAGS   = -std=c17 -Wall -O3 #-g

LINKER   = gcc
# linking flags here
LFLAGS   = -Wall -lm -lglpk


SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin
SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm       = rm -f

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo $<" compiled successfully!"

clean:
	@$(rm) $(wildcard $(OBJDIR)/*.o)
	@echo "Cleanup complete!"

remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"
	
# TEST PART
TESTS_DIR = tests
run_test: 
	@$(TESTS_DIR)/run_tests.sh
	
  
	


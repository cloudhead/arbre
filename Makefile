
CC     := clang
CFLAGS := -Wall -pedantic -std=c99 -O0 -g
INCS   := -I../
SRC    := $(wildcard *.c)
OBJ    := $(SRC:.c=.o)
TARGET := bin/arbre

all: $(TARGET)

%.o: %.c
	@echo "cc   $< => $@"
	@$(CC) -c $(CFLAGS) $(INCS) -o $@ $<

$(TARGET): $(OBJ)
	@mkdir -p bin
	@echo "=>   $(TARGET)"
	@$(CC) $(OBJ) -o $(TARGET)
	@echo OK

%.d: %.c
	@echo "dep  $*.o => $*.d"
	@$(CC) $(INCS) -MM -MG -MT "$*.o $*.d" $*.c >$@

-include $(OBJ:.o=.d)

clean:
	@rm -f $(OBJ)
	@[ -f $(TARGET) ] && rm $(TARGET)

test: $(TARGET)
	test/test-runner.sh test/*.arb test/gen/*.arb

.PHONY: test

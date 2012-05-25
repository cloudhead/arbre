
CC = clang
SRC = arbre.c  scanner.c  token.c     source.c  tokens.c\
	  util.c   parser.c   generator.c tree.c    report.c\
	  io.c     node.c     runtime.c   value.c   hash.c\
	  vm.c     op.c       symtab.c    command.c error.c bin.c
HEADERS = *.h
CFLAGS = -Wall -pedantic -std=c99 -O0 -g
TARGET = bin/arbre

$(TARGET): $(SRC) $(HEADERS)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) 

clean:
	[ -f $(TARGET) ] && rm $(TARGET)

test: $(TARGET)
	test/test-runner.sh test/*.arb

.PHONY: test

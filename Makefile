CPPFLAGS=-Wall -Werror -Wextra
CFLAGS=-g -O0 -std=gnu99
SOURCES=newm.c userlist.c errors.c config.c
PROGRAM=newm

all: $(PROGRAM)

$(PROGRAM): $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $(PROGRAM)

clean:
	-rm -f $(PROGRAM)

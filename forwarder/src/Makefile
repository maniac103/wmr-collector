CC = gcc
CFLAGS = -Wall -c -O2
SRCS = forwarder.c
OBJS = $(SRCS:%.c=%.o)
DEPFILE = .depend
PROG = wmr-forwarder

all: $(PROG)

clean:
	rm -f $(PROG)
	rm -f *.o
	rm -f $(DEPFILE)

$(DEPFILE): $(SRCS)
	$(CC) $(CFLAGS) -MM $(SRCS) > $(DEPFILE)

-include $(DEPFILE)

$(PROG): $(OBJS) $(DEPFILE) Makefile
	$(CC) $(LDFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@


CC = gcc
CFLAGS = -Wall -c -O2
LIBS =
SRCS = forwarder.c
OBJS = $(SRCS:%.c=%.o)
DEPFILE = .depend
PROG = wmrforwarder

all: $(PROG)

clean:
	rm -f $(PROG)
	rm -f *.o
	rm -f $(DEPFILE)

$(DEPFILE): $(SRCS)
	$(CC) $(CFLAGS) -MM $(SRCS) > $(DEPFILE)

-include $(DEPFILE)

$(PROG): $(OBJS) $(DEPFILE) Makefile
	$(CC) $(LIBS) -o $(PROG) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $<


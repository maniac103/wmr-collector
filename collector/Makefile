CC = g++
CFLAGS = -Wall -c -O2 -I/usr/include/mysql -std=c++0x
LIBS = -lpthread -lboost_system -lboost_thread-mt -lboost_program_options -lmysqlpp
SRCS = main.cpp IoHandler.cpp WmrMessage.cpp Database.cpp Options.cpp PidFile.cpp
OBJS = $(SRCS:%.cpp=%.o)
DEPFILE = .depend
PROG = wmrcollector

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

%.o: %.cpp
	$(CC) $(CFLAGS) $<


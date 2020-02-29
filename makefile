C      = g++
CFLAGS  = -O3
OPTION  = -std=c++14 -g
LIBS    = -lboost_system -lboost_thread -pthread -lstdc++ -lg3logger

all: ChatBoxServer ChatBox 

ChatBoxServer: ChatBoxServer.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

ChatBox: ChatBox.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

	
ChatBoxServer.o: ChatBoxServer.cpp
	$(CC) $(OPTION) -c $(CFLAGS) ChatBoxServer.cpp

ChatBox.o: ChatBox.cpp 
	$(CC) $(OPTION) -c $(CFLAGS) ChatBox.cpp

.PHONY: clean

clean:
	rm *.o
	rm ChatBoxServer ChatBox 

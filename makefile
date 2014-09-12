all: dchat

dchat: chat.o client.o
	g++ -o dchat chat.o messages.o client.o -lpthread

chat.o: chat.cpp
	g++ -c chat.cpp -fpermissive -g

client.o: client.h client.cpp messages.o
	g++ -c client.cpp -fpermissive -g

messages.o: messages.h messages.cpp
	g++ -c messages.cpp -fpermissive -g

clean:
	rm *.o dchat


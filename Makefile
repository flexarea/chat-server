.PHONY: all 
all:
	gcc -Wall -pedantic -o chat-client chat-client.c
	gcc -Wall -pedantic -o chat-server chat-server.c

chat-client: chat-client.c
	gcc -Wall -pedantic -o chat-client chat-client.c

chat-server: chat-server.c
	gcc -Wall -pedantic -o chat-server chat-server.c

.PHONY: clean
clean:
	rm -f chat-client chat-server

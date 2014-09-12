#include <iostream>
#include <string>
#include <arpa/inet.h>
#include "client.h"

#define DEFAULT_PORT 33333

using namespace std;

int main (int argc, char** argv)
{
	if (argc < 2 || argc > 3) {
		cout << "usage: " << argv[0] << " [nickname] [server-name] or " << argv[0] << " [nickname] [server-IP]:[Port]" << endl;
		return 0;
	}

	ChatClient* local_client;

	if(argc == 2)
	{
		cout << "Setting up a chat room..." << endl;
		local_client = new ChatClient(argv[1]);
	}
	else if(argc == 3)
	{
		string server_string(argv[2]);

		int n = server_string.find(":");

		if(n == string::npos)
		{
			cout << "Attempting to join chat room..." << endl;
			local_client = new ChatClient(argv[1], argv[2]);
		}
		else
		{
			string ip_string(server_string, 0, n);
			string port_string(server_string, n+1, server_string.length()-(n+1));

			struct sockaddr_in server_sock;
			inet_pton(AF_INET, ip_string.c_str(), &(server_sock.sin_addr));
			server_sock.sin_family = AF_INET;
			server_sock.sin_port = htons(atoi(port_string.c_str()));

			cout << "Attempting to join chat room..." << endl;
			local_client = new ChatClient(argv[1], server_sock);
		}
	}

	cout << "> ";
	flush(cout);

	while(1) 
	{
		local_client->CheckTimeout();

		if(local_client->IsLeader())
		{
			local_client->BroadcastChat();
			local_client->BroadcastContacts();
		}

		local_client->ReceiveMessage();
	}
	return 0;
}

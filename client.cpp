#include "client.h"

#define MAX_CONNECTS_ATTEMPTS 5
#define TIMEOUT_LENGTH 2
#define CONTACTS_TIMEOUT 1
#define CHAT_TIMEOUT .5

using namespace std;

ChatClient::ChatClient(char* nickname, uint16_t local_port) : isLeader(false), dump_contacts(false), next_seqNum(0), connect_attempts(0), in_chat(false)
{
	char host_name[1024];
	gethostname(host_name, 1024);

	//struct hostent *temp_host = gethostbyname("localhost");
	struct hostent *temp_host = gethostbyname(host_name);

	if(!temp_host)
	{
		cout << "Host not found: localhost" << endl;
		exit(1);
	}

	m_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if(m_socket < 0)
	{
		cout << "Failed to create socket" << endl;
		exit(1);
	}

	m_nickname = string(nickname);
	Leader.name = m_nickname;

	m_local_address.sin_family = AF_INET;
	m_local_address.sin_port = htons(local_port);
	m_local_address.sin_addr.s_addr = htonl(INADDR_ANY);

	Leader.ipAdd.sin_family = AF_INET;
	Leader.ipAdd.sin_port = htons(local_port);
	memcpy((void *)&Leader.ipAdd.sin_addr, temp_host->h_addr_list[0], temp_host->h_length);

	if(bind(m_socket, (struct sockaddr *) &m_local_address, sizeof(m_local_address)) < 0)
	{
		cout << "Local port could not be bound on port: " << local_port << endl;
	}
	else
	{
		cout << "Hosting chat on: " << IPtoString(Leader.ipAdd) << endl;
	}

	srand(time(NULL));

	HostChat();
}

ChatClient::ChatClient(char* nickname, struct sockaddr_in host_address, uint16_t local_port) : isLeader(false), dump_contacts(false), next_seqNum(0), connect_attempts(0), in_chat(false)
{
	char local_host_name[1024];
	gethostname(local_host_name, 1024);
	struct hostent *temp_host = gethostbyname(local_host_name);
	struct sockaddr_in temp_address;
	temp_address.sin_port = htons(local_port);
	memcpy((void *)&temp_address.sin_addr, temp_host->h_addr_list[0], temp_host->h_length);

	m_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if(m_socket < 0)
	{
		cout << "Failed to create socket" << endl;
		exit(1);
	}

	m_nickname = string(nickname);

	m_local_address.sin_family = AF_INET;
	m_local_address.sin_port = htons(local_port);
	m_local_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(m_socket, (struct sockaddr *) &m_local_address, sizeof(m_local_address)) < 0)
	{
		cout << "Local port could not be bound on port: " << local_port << endl;
	}
	else
	{
		cout << "Contacting host on: " << IPtoString(host_address) << ". Listening on: " << IPtoString(temp_address) << endl;
	}

	Leader.ipAdd = host_address;

	srand(time(NULL));

	SendJoin();
}

ChatClient::ChatClient(char* nickname, char* host_name, uint16_t host_port, uint16_t local_port) : isLeader(false), dump_contacts(false), next_seqNum(0), connect_attempts(0), in_chat(false)
{
	char local_host_name[1024];
	gethostname(local_host_name, 1024);
	struct hostent *temp_host = gethostbyname(local_host_name);
	struct sockaddr_in temp_address;
	temp_address.sin_port = htons(local_port);
	memcpy((void *)&temp_address.sin_addr, temp_host->h_addr_list[0], temp_host->h_length);

	temp_host = gethostbyname(host_name);

	if(!temp_host)
	{
		cout << "Host not found: " << host_name << endl;
		exit(1);
	}

	m_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if(m_socket < 0)
	{
		cout << "Failed to create socket" << endl;
		exit(1);
	}

	m_nickname = string(nickname);

	m_local_address.sin_family = AF_INET;
	m_local_address.sin_port = htons(local_port);
	m_local_address.sin_addr.s_addr = htonl(INADDR_ANY);

	Leader.ipAdd.sin_family = AF_INET;
	Leader.ipAdd.sin_port = htons(host_port);
	memcpy((void *)&Leader.ipAdd.sin_addr, temp_host->h_addr_list[0], temp_host->h_length);

	if(bind(m_socket, (struct sockaddr *) &m_local_address, sizeof(m_local_address)) < 0)
	{
		cout << "Local port could not be bound on port: " << local_port << endl;
	}
	else
	{
		cout << "Contacting host on: " << IPtoString(Leader.ipAdd) << ". Listening on: " << IPtoString(temp_address) << endl;
	}

	srand(time(NULL));

	SendJoin();
}

void
ChatClient::ReceiveMessage()
{
	char buf[1024];
	int buf_length = 0;
	struct sockaddr_in sender_address;
	socklen_t addrlen = sizeof(sender_address);

	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(0, &rfds); //stdin
    FD_SET(m_socket, &rfds); //listening socket
    struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 250000;

	int retval = select(m_socket+1, &rfds, NULL, NULL, &timeout);

	if(retval == -1)
	{
		cout << "Select error" << endl;
	}
	else if (retval)
	{
		if(FD_ISSET(0, &rfds))
		{
			string user_input;
			if(getline(cin,user_input))
			{
				if(user_input.length() > 0)
				{
					SendChat(user_input);
				}
			}
			else //we got EOF
			{
				cout << "Exiting..." << endl;
				exit(1);
			}
	
			cout << "> ";
			flush(cout);
			return;
		}
		else if(FD_ISSET(m_socket, &rfds))
		{
			//we fall through
			//cout << "listening socket" << endl;
		}
	}
	else
	{
		return;
	}

	buf_length = recvfrom(m_socket, buf, 1024, 0, (struct sockaddr *)&sender_address, &addrlen);

	if(buf_length == 0)
	{
		cout << "Socket read error" << endl;
	}

	//cout << buf << endl;

	ChatMessage::MessageType type = buf[0];

	switch(type)
	{
		case ChatMessage::CHAT_REQ:
			//cout << "CHAT_REQ" << endl;
			ProcessChatReq(buf, buf_length, sender_address);
			break;
		case ChatMessage::CHAT_RSP:
			//cout << "CHAT_RSP" << endl;
			ProcessChatRsp(buf, buf_length, sender_address);
			break;
		case ChatMessage::JOIN_REQ:
			//cout << "JOIN_REQ" << endl;
			ProcessJoinReq(buf, buf_length, sender_address);
			break;
		case ChatMessage::JOIN_RSP:
			//cout << "JOIN_RSP" << endl;
			ProcessJoinRsp(buf, buf_length, sender_address);
			break;
		case ChatMessage::LEADER:
			//cout << "LEADER" << endl;
			ProcessLeader(buf, buf_length, sender_address);
			break;
		case ChatMessage::NEW_USER:
			//cout << "NEW_USER" << endl;
			ProcessNewUser(buf, buf_length, sender_address);
			break;
		case ChatMessage::CONTACTS:
			//cout << "CONTACTS" << endl;
			ProcessContacts(buf, buf_length, sender_address);
			break;
		default:
			cout << "Invalid Message Type: " << (unsigned int)type << endl;
			return;
	}
}

void
ChatClient::ProcessChatReq(char* buf, int buf_length, struct sockaddr_in sender)
{
	if(buf_length < 14)
	{
		cout << "Buffer too small ( < 14 )" << endl;
	}

	if(!in_chat)
		return;

	int num_args = buf[1];

	if(num_args != 2)
	{
		cout << "Malformed CHAT_REQ" << endl;
		return;
	}

	ChatMessage request((unsigned char*)buf);
	vector<string> arguments = request.GetArguments();
	struct timeval request_timeval = request.GetTimestamp();

	// here I have the chat message, I can:
	// 	request.GetArguments() to get a [vector<string>] of the arguments:arguments[0] is the chat_text
	//	request.GetAge() to get an [int] of how many microseconds ago the message was sent
	//	request.GetTimestamp() to get a [struct timeval] of the timestamp when the message was sent

	if(isLeader && arguments[1].length() > 0)
	{
		//check ages of other messages in list and insert message to list in sequential order by age
		bool messageAdded = false;
		for(list< pair<ChatMessage,bool> >::iterator it = leaderMessageList.begin(); it != leaderMessageList.end(); ++it)
		{
			if(request.GetAge() > it->first.GetAge() && it->first.GetArguments()[0].length() > 0) //second bit ensures that system messages take presidence in ordering
			{
				leaderMessageList.insert(it, pair<ChatMessage,bool>(request,false));
				messageAdded = true; 
				break;
			}

			string current_name = it->first.GetArguments()[0];
			struct timeval current_timeval = it->first.GetTimestamp();
			
			//if the new message matches this old message
			if(arguments[0] == current_name && request_timeval.tv_usec == current_timeval.tv_usec)
			{
				return;
			}
		}

		if(!messageAdded)
		{
			leaderMessageList.push_back(pair<ChatMessage,bool>(request,false));
		}

		//(do not distribute message to all clients yet)

		//acknowledge the received sequence number
		vector<string> args;
		args.push_back(m_nickname);

		ChatMessage message(ChatMessage::CHAT_RSP, args.size(), &args[0], request.GetSeqNum());

		if(SendMessage(message, sender))
		{
			//cout << "CHAT_RSP sent successfully" << endl;
		}
		else
		{
			cout << "CHAT_RSP failed to send" << endl;
		}
	}
	else if(sender.sin_addr.s_addr == Leader.ipAdd.sin_addr.s_addr && sender.sin_port == Leader.ipAdd.sin_port) 
	//I am a client, this is a broadcast from the leader
	{
		gettimeofday(&last_chat_broadcast, NULL);
		int seqNum = request.GetSeqNum();

		//check sequence number of message
		if((seqNum == next_seqNum) || (next_seqNum == 0)) 
		// This is the next expected message
		{
			next_seqNum++;
			bool output = false;

			//Print this message
			if(arguments[1].length() > 0)
			{
				output = true;
				cout << endl << arguments[1];
			}

			//Print any queued messages
			for(list<ChatMessage>::iterator it = earlyMessageList.begin(); it != earlyMessageList.end();)
			{
				if(it->GetSeqNum() == next_seqNum)
				{
					arguments = it->GetArguments();

					if(arguments[1].length() > 0)
					{
						output = true;
						cout << endl << arguments[1];
					}

					next_seqNum++;
					it = earlyMessageList.erase(it);
				}
				else
				{
					it++;
				}
			}

			if(output)
			{
				cout << "> ";
				flush(cout);
			}
		} 
		else if (seqNum > next_seqNum) 
		// This message arrived before the next expected
		{
			//check ages of other messages in list and insert message to list in sequential order by age
			bool messageAdded = false;
			for(list<ChatMessage>::iterator it = earlyMessageList.begin(); it != earlyMessageList.end(); ++it)
			{
				if(seqNum < it->GetSeqNum())
				{
					earlyMessageList.insert(it, request);
					messageAdded = true;
					break;
				}
			}

			if(!messageAdded)
			{
				earlyMessageList.push_back(request);
			}

			int num_lost = seqNum - next_seqNum;
			for(int i = 0; i < num_lost; i++) 
			{
				//cout << "Error: Missed " << next_seqNum + i << endl;
			}
			//next_seqNum = seqNum + 1;
		}
		// For the case where the seqNum is less than the next expected, we do nothing, in effect dropping the packet.

		//acknowledge the received sequence number
		vector<string> args;
		args.push_back(m_nickname);

		ChatMessage message(ChatMessage::CHAT_RSP, args.size(), &args[0], request.GetSeqNum());

		if(SendMessage(message, sender))
		{
			//cout << "CHAT_RSP sent successfully" << endl;
		}
		else
		{
			cout << "CHAT_RSP failed to send" << endl;
		}
	}
}

void
ChatClient::ProcessChatRsp(char* buf, int buf_length, struct sockaddr_in sender)
{
	if(buf_length < 14)
	{
		cout << "Buffer too small ( < 14 )" << endl;
	}

	int num_args = buf[1];

	if(num_args != 1)
	{
		cout << "Malformed CHAT_RSP" << endl;
		return;
	}

	ChatMessage message((unsigned char*)buf);
	vector<string> message_args = message.GetArguments();

	for(list< pair<ChatMessage,user> >::iterator it = sentMessages.begin(); it != sentMessages.end(); ++it)
	{
		if(message_args[0] == it->second.name && message.GetSeqNum() == it->first.GetSeqNum())
		{
			sentMessages.erase(it);
			break;
		}
	}
}

void
ChatClient::ProcessJoinReq(char* buf, int buf_length, struct sockaddr_in sender)
{
	if(buf_length < 14)
	{
		cout << "Buffer too small ( < 14 )" << endl;
	}

	int num_args = buf[1];

	if(num_args != 1)
	{
		cout << "Malformed JOIN_REQ" << endl;
		return;
	}

	ChatMessage request((unsigned char*)buf);
	vector<string> request_args = request.GetArguments();

	struct user new_user;
	new_user.name = request_args[0];
	new_user.ipAdd.sin_family = sender.sin_family;
	new_user.ipAdd.sin_addr.s_addr = sender.sin_addr.s_addr;
	new_user.ipAdd.sin_port = sender.sin_port;

	if(isLeader)
	{
		bool name_taken = false;
		bool resending = false;
		for(int i = 0; i < otherClients.size(); ++i)
		{
			if(otherClients[i].name == new_user.name)
			{
				name_taken = true;
				if(otherClients[i].ipAdd.sin_addr.s_addr == new_user.ipAdd.sin_addr.s_addr &&
						otherClients[i].ipAdd.sin_port == new_user.ipAdd.sin_port)
				{
					resending = true;
				}
				break;
			}
		}

		if(!resending)
		{
			if(m_nickname == new_user.name || name_taken)
			{
				cout << "Name is already taken: " << new_user.name << endl << "> ";
				flush(cout);
				return;
			}

			//add the new chat member to our data structure
			otherClients.push_back(new_user);

			//add message to the chat
			vector<string> alert_args;
			stringstream temp_stream;
			temp_stream << "*** " << PrintStruct(new_user) << " has joined the chat ***";
			alert_args.push_back("");
			alert_args.push_back(temp_stream.str());
			ChatMessage alert(ChatMessage::CHAT_REQ, alert_args.size(), &alert_args[0]);
			leaderMessageList.push_back(pair<ChatMessage,bool>(alert,false));
		}

		//send response to new user
		vector<string> response_args;
		stringstream temp_stream;
		temp_stream << next_seqNum;
		response_args.push_back(m_nickname);
		response_args.push_back(temp_stream.str());

		ChatMessage response(ChatMessage::JOIN_RSP, response_args.size(), &response_args[0]);

		if(SendMessage(response, new_user.ipAdd))
		{
			//cout << "JOIN_RSP sent" << endl;
		}
		else
		{
			cout << "JOIN_RSP failed to send" << endl;
		}
	}
	else
	{
		//alert host of new user
		vector<string> arguments = StructToStrings(new_user);
		ChatMessage alert(ChatMessage::NEW_USER, arguments.size(), &arguments[0]);

		if(SendMessage(alert, Leader.ipAdd))
		{
			//cout << "NEW_USER sent" << endl;
		}
		else
		{
			cout << "NEW_USER failed to send" << endl;
		}
	}
}

void
ChatClient::ProcessJoinRsp(char* buf, int buf_length, struct sockaddr_in sender)
{
	if(buf_length < 14)
	{
		cout << "Buffer too small ( < 14 )" << endl;
	}

	int num_args = buf[1];

	if(num_args != 2)
	{
		cout << "Malformed JOIN_RSP" << endl;
		return;
	}

	ChatMessage response((unsigned char*)buf);
	vector<string> response_args = response.GetArguments();

	Leader.name = response_args[0];
	Leader.ipAdd = sender;
	next_seqNum = atoi(response_args[1].c_str());

	in_chat = true;
	dump_contacts = true;
}

void
ChatClient::ProcessNewUser(char* buf, int buf_length, struct sockaddr_in sender)
{
	if(buf_length < 14)
	{
		cout << "Buffer too small ( < 14 )" << endl;
	}

	int num_args = buf[1];

	if(num_args != 3)
	{
		cout << "Malformed NEW_USER" << endl;
		return;
	}

	ChatMessage alert((unsigned char*)buf);
	vector<string> alert_args = alert.GetArguments();

	struct user new_user;
	new_user.name = alert_args[0];
	new_user.ipAdd.sin_family = AF_INET;
	new_user.ipAdd.sin_addr.s_addr = atoi(alert_args[1].c_str());
	new_user.ipAdd.sin_port = atoi(alert_args[2].c_str());

	if(isLeader)
	{
		bool name_taken = false;
		bool resending = false;
		for(int i = 0; i < otherClients.size(); ++i)
		{
			if(otherClients[i].name == new_user.name)
			{
				name_taken = true;
				if(otherClients[i].ipAdd.sin_addr.s_addr == new_user.ipAdd.sin_addr.s_addr &&
						otherClients[i].ipAdd.sin_port == new_user.ipAdd.sin_port)
				{
					resending = true;
				}
				break;
			}
		}

		if(!resending)
		{
			if(m_nickname == new_user.name || name_taken)
			{
				cout << "Name is already taken: " << new_user.name << endl << "> ";
				flush(cout);
				return;
			}

			//add the new chat member to our data structure
			otherClients.push_back(new_user);

			//add message to the chat
			vector<string> alert_args;
			stringstream temp_stream;
			temp_stream << "*** " << PrintStruct(new_user) << " has joined the chat ***";
			alert_args.push_back("");
			alert_args.push_back(temp_stream.str());
			ChatMessage alert(ChatMessage::CHAT_REQ, alert_args.size(), &alert_args[0]);
			leaderMessageList.push_back(pair<ChatMessage,bool>(alert,false));
		}

		//send response to new user
		vector<string> response_args;
		stringstream temp_stream;
		temp_stream << next_seqNum;
		response_args.push_back(m_nickname);
		response_args.push_back(temp_stream.str());

		ChatMessage response(ChatMessage::JOIN_RSP, response_args.size(), &response_args[0]);

		if(SendMessage(response, new_user.ipAdd))
		{
			//cout << "JOIN_RSP sent" << endl;
		}
		else
		{
			cout << "JOIN_RSP failed to send" << endl;
		}
	}
}

void
ChatClient::ProcessContacts(char* buf, int buf_length, struct sockaddr_in sender)
{
	if(buf_length < 14)
	{
		cout << "Buffer too small ( < 14 )" << endl;
	}

	int num_args = buf[1];

	if(num_args == 0 || (num_args % 3) != 0)
	{
		cout << "Malformed CONTACTS" << endl;
		return;
	}

	// make sure this was sent by the leader
	if(!(Leader.ipAdd.sin_addr.s_addr == sender.sin_addr.s_addr && Leader.ipAdd.sin_port == sender.sin_port))
	{
		return;
	}

	ChatMessage message((unsigned char*)buf);
	vector<string> contacts = message.GetArguments();

	otherClients.clear();

	for(int i = 0; i < contacts.size(); i += 3)
	{
		//if the contact is me, skip to the next one
		if(contacts[i] == m_nickname)
		{
			m_local_address.sin_family = AF_INET;
			m_local_address.sin_addr.s_addr = atoi(contacts[i+1].c_str());
			m_local_address.sin_port = atoi(contacts[i+2].c_str());
			continue;
		}

		struct user current_user;
		current_user.name = contacts[i];
		current_user.ipAdd.sin_family = AF_INET;
		current_user.ipAdd.sin_addr.s_addr = atoi(contacts[i+1].c_str());
		current_user.ipAdd.sin_port = atoi(contacts[i+2].c_str());
		otherClients.push_back(current_user);
	}
	
	if(dump_contacts)
	{
		DumpContactList();
		dump_contacts = false;
	}
}

void
ChatClient::ProcessLeader(char* buf, int buf_length, struct sockaddr_in sender)
{
	if(buf_length < 14)
	{
		cout << "Buffer too small ( < 14 )" << endl;
	}

	int num_args = buf[1];

	if(num_args != 4 && num_args != 2)
	{
		cout << "Malformed LEADER" << endl;
		return;
	}

	ChatMessage message((unsigned char*)buf);
	vector<string> message_args = message.GetArguments();

	if(Leader.name == message_args[0])
	{
		// Leader is already set appropriately, nothing to change
		return;
	}

	struct user new_leader;
	new_leader.name = message_args[0];
	new_leader.ipAdd.sin_family = AF_INET;

	if(num_args == 2)
	{
		new_leader.ipAdd = sender;
	}
	else
	{
		new_leader.ipAdd.sin_addr.s_addr = atoi(message_args[1].c_str());
		new_leader.ipAdd.sin_port = atoi(message_args[2].c_str());
	}

	if(CompareIPs(new_leader))
	{
		cout << "Leader changing to: " << PrintStruct(new_leader) << endl;
		next_seqNum = atoi(message_args[num_args-1].c_str());

		if(Leader.name == m_nickname && !isLeader)
		{
			isLeader = true;
			cout << "I have been made the leader..." << endl;
		}
		else
		{
			isLeader = false;
			leaderMessageList.clear();
		}

		cout << "> ";
		flush(cout);
	}

	// reset timers
	gettimeofday(&last_chat_broadcast, NULL);
	gettimeofday(&last_contacts_broadcast, NULL);
}

bool
ChatClient::SendChat(string chat_text)
{
	vector<string> arguments;
	arguments.push_back(m_nickname);
	arguments.push_back(chat_text);

	ChatMessage message(ChatMessage::CHAT_REQ, arguments.size(), &arguments[0]);

	if(isLeader)
	{
		//message.SetSeqNum(next_seqNum);  BF: I think this line can be deleted
		leaderMessageList.push_back(pair<ChatMessage,bool>(message,false));

		return true;
	}

	if(SendMessage(message, Leader.ipAdd))
	{
		//cout << "CHAT_REQ sent" << endl;

		sentMessages.push_back(pair<ChatMessage, user>(message, Leader));

		return true;
	}
	else
	{
		cout << "CHAT_REQ failed to send" << endl;
		return false;
	}
}

bool
ChatClient::SendJoin()
{
	gettimeofday(&last_contacts_broadcast, NULL);
	gettimeofday(&last_chat_broadcast, NULL);

	ChatMessage message(ChatMessage::JOIN_REQ, 1, &m_nickname);
	if(SendMessage(message, Leader.ipAdd))
	{
		//cout << "JOIN_REQ send" << endl;
		return true;
	}
	else
	{
		cout << "JOIN_REQ failed to send" << endl;
		return false;
	}
}

void
ChatClient::BroadcastContacts()
{
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	int age = (current_time.tv_sec - last_contacts_broadcast.tv_sec) * 1000000;
	age += current_time.tv_usec - last_contacts_broadcast.tv_usec;
	
	if(age < CONTACTS_TIMEOUT * 1000000)
	{
		return;
	}

	vector<string> arguments;

	for(int i = 0; i < otherClients.size(); ++i)
	{
		vector<string> current_user = StructToStrings(otherClients[i]);

		for(int j = 0; j < current_user.size(); ++j)
		{
			arguments.push_back(current_user[j]);
		}
	}

	gettimeofday(&last_contacts_broadcast, NULL);
	ChatMessage message(ChatMessage::CONTACTS, arguments.size(), &arguments[0]);

	for(int i = 0; i < otherClients.size(); ++i)
	{
		if(SendMessage(message, otherClients[i].ipAdd))
		{
			//cout << "CONTACTS sent" << endl;
		}
		else
		{
			cout << "CONTACTS failed to send to: " << otherClients[i].name << endl;
		}
	}
}

void
ChatClient::BroadcastChat()
{
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	int age = (current_time.tv_sec - last_chat_broadcast.tv_sec) * 1000000;
	age += current_time.tv_usec - last_chat_broadcast.tv_usec;

	if(age < CHAT_TIMEOUT * 1000000)
	{
		return;
	}

	gettimeofday(&last_chat_broadcast, NULL);

	//TIME TO SEND THE NEXT WINDOW OF CHAT
	vector<string> arguments;
	arguments.push_back(m_nickname);

	bool output = false;
	stringstream temp_stream;
	//temp_stream << "Chat Seq: " << next_seqNum << endl;
	for(list< pair<ChatMessage,bool> >::iterator it = leaderMessageList.begin(); it != leaderMessageList.end();)
	{
			// message was already printed, and is old enough to discard
			if(it->second && it->first.GetAge() > CHAT_TIMEOUT * 1000000 * 5)
			{
				it = leaderMessageList.erase(it);
				continue;
			}
			// message was already printed, but it might be resent if packets were lost
			else if(it->second)
			{
				it++;
				continue;
			}
			// message hasn't been printed yet, print it now
			else
			{
				vector<string> current_args = it->first.GetArguments();
				if(current_args[0].size() > 0)
				{
					temp_stream << current_args[0] << ": " << current_args[1] << endl;
					cout << endl << current_args[0] << ": " << current_args[1];
				}
				else //chat system message
				{
					temp_stream << current_args[1] << endl;
					cout << endl << current_args[1];
				}

				it->second = true;
				output = true;
				it++;
			}
	}

	if(output == true)
	{
		cout << endl << "> ";
		flush(cout);
	}

	arguments.push_back(temp_stream.str());

	ChatMessage message(ChatMessage::CHAT_REQ, arguments.size(), &arguments[0], next_seqNum++);

	for(int i = 0; i < otherClients.size(); ++i)
	{
		if(SendMessage(message, otherClients[i].ipAdd))
		{
			sentMessages.push_back(pair<ChatMessage,user>(message, otherClients[i]));
		}
		else
		{
			cout << "CHAT broadcast failed to send to: " << otherClients[i].name << endl;
		}
	}
}

bool
ChatClient::BroadcastLeader()
{
	ChatMessage* message;
	stringstream temp_stream;
	temp_stream << next_seqNum;

	if(isLeader)
	{
		vector<string> arguments;
		arguments.push_back(m_nickname);
		arguments.push_back(temp_stream.str());

		message = new ChatMessage(ChatMessage::LEADER, arguments.size(), &arguments[0]);
	}
	else
	{
		vector<string> arguments = StructToStrings(Leader);
		arguments.push_back(temp_stream.str());

		message = new ChatMessage(ChatMessage::LEADER, arguments.size(), &arguments[0]);
	}

	for(int i = 0; i < otherClients.size(); ++i)
	{	
		if(SendMessage(*message, otherClients[i].ipAdd))
		{

		}
		else
		{
			cout << "LEADER failed to send to: " << otherClients[i].name << endl;
		}
	}

	delete message;
}

void
ChatClient::CheckTimeout()
{
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	int age = (current_time.tv_sec - last_chat_broadcast.tv_sec) * 1000000;
	age += current_time.tv_usec - last_chat_broadcast.tv_usec;

	if(age > TIMEOUT_LENGTH * 1000000 * 2)
	{
		// Leader.name is set when you join the chat, so if you timed out without a Leader.name it's because you failed to join
		if(Leader.name.length() == 0)
		{
			if(connect_attempts < MAX_CONNECTS_ATTEMPTS)
			{
				cout << "Failed to join chat. Retrying..." << endl;
				SendJoin();
				connect_attempts++;
				return;
			}
			else
			{
				cout << "Failed to join chat." << endl;
				exit(1);
			}
		}

		cout << endl << "*** The leader " << PrintStruct(Leader) << " has left the chat ***" << endl;
		for(list< pair<ChatMessage,user> >::iterator it = sentMessages.begin(); it != sentMessages.end(); it++)
    		if(Leader.name == it->second.name)
    			it = sentMessages.erase(it);
    	SetNewLeader();
	}

	vector<string> leaving;
  for(list< pair<ChatMessage,user> >::iterator it = sentMessages.begin(); it != sentMessages.end(); it++){
    if(it->first.GetAge() > TIMEOUT_LENGTH * 1000000) //give up resending
    {

    	int i;
    	for(i = 0; i < leaving.size(); ++i)
    		if(it->second.name == leaving[i])
    			break;

    	if(i == leaving.size())
    	{
    		leaving.push_back(it->second.name);

    		if(isLeader)
    		{
    			vector<string> alert_args;
    			stringstream temp_stream;
    			temp_stream << "*** " << PrintStruct(it->second) << " has left the chat ***";
    			alert_args.push_back("");
    			alert_args.push_back(temp_stream.str());
      		ChatMessage alert(ChatMessage::CHAT_REQ, alert_args.size(), &alert_args[0]);
	    		leaderMessageList.push_back(pair<ChatMessage,bool>(alert,false));
	    	}
    	}

    	for(i = 0; i < otherClients.size(); ++i)
    	{
    		if(otherClients[i].name == it->second.name)
    		{
    			otherClients.erase(otherClients.begin() + i);
    			break;
    		}
    	}
    }
    else if(it->first.GetAge() > CHAT_TIMEOUT * 1000000) //resend the message
    {
    	//cout << "RESENDING: " << it->first.GetMessageType() << " Seq: " << it->first.GetSeqNum() << endl;
    	SendMessage(it->first, it->second.ipAdd);
    }
	}

  for(int i = 0; i < leaving.size(); ++i)
  {
    for(list< pair<ChatMessage,user> >::iterator it = sentMessages.begin(); it != sentMessages.end();)
    {
    	if(leaving[i] == it->second.name)
    	{
    		it = sentMessages.erase(it);
    	}
    	else
    	{
    		it++;
    	}
    }
	}
}

bool
ChatClient::CompareIPs(user u)
{
    if((u.ipAdd.sin_addr.s_addr > Leader.ipAdd.sin_addr.s_addr) || 
    	(u.ipAdd.sin_addr.s_addr==Leader.ipAdd.sin_addr.s_addr && ntohs(u.ipAdd.sin_port) > ntohs(Leader.ipAdd.sin_port))){
        Leader = u;
    	return true;
    }
    return false;
}

void
ChatClient::SetNewLeader()
{
	cout << "Electing a new leader..." << endl;
    isLeader=false;

    Leader.name = m_nickname;
    Leader.ipAdd = m_local_address;

    for(int i = 0; i < otherClients.size(); ++i)
    {
    	CompareIPs(otherClients[i]);
    }

    if(Leader.name == m_nickname){
        isLeader=true;
        cout << "I think I should be the leader" << endl;
    }
    else
    {
    	cout << "I have selected: " << Leader.name << endl;
    }

	// reset timers
	gettimeofday(&last_chat_broadcast, NULL);
	gettimeofday(&last_contacts_broadcast, NULL);

    BroadcastLeader();

    cout << "> ";
    flush(cout);
}

vector<string>
ChatClient::StructToStrings(struct user u) const
{
	vector<string> arguments;
	stringstream temp_stream;

	arguments.push_back(u.name);

	temp_stream.str("");

	temp_stream << u.ipAdd.sin_addr.s_addr;
	arguments.push_back(temp_stream.str());
	
	temp_stream.str("");

	temp_stream << u.ipAdd.sin_port;
	arguments.push_back(temp_stream.str());

	return arguments;
}

string
ChatClient::PrintStruct(struct user u) const
{
	stringstream temp_stream;

	temp_stream << u.name << ' ';

  	temp_stream << IPtoString(u.ipAdd);

	return temp_stream.str();
}

string
ChatClient::IPtoString(struct sockaddr_in address) const
{
	stringstream temp_stream;
	in_addr_t ip_address = address.sin_addr.s_addr;

  	temp_stream << (unsigned int)(ip_address & 0xFF) << '.';
  	temp_stream << (unsigned int)((ip_address >> 8) & 0xFF) << '.';
  	temp_stream << (unsigned int)((ip_address >> 16) & 0xFF) << '.';
  	temp_stream << (unsigned int)((ip_address >> 24) & 0xFF) << ':';

  	temp_stream << ntohs(address.sin_port);

	return temp_stream.str();
}

void
ChatClient::HostChat()
{
	isLeader = true;
	in_chat = true;
	next_seqNum = 1;
	gettimeofday(&last_contacts_broadcast, NULL);
	gettimeofday(&last_chat_broadcast, NULL);
}

bool
ChatClient::IsLeader() const
{
	return isLeader;
}

bool
ChatClient::SendMessage(ChatMessage message, struct sockaddr_in destination) const
{
	//if((rand() % 4) != 0) {
		if(sendto(m_socket, message.GetUdpPayload(), message.GetPayloadLength(), 0, (struct sockaddr*) &destination, sizeof(destination)) < 0)
		{
			return false;
		}
	
		return true;
	/*}
	else
	{
		//cout << "DROPPING PACKET: " << message.GetMessageType() << " Seq: " << message.GetSeqNum() << endl;
		return true;
	}*/
}

void
ChatClient::DumpContactList()
{
	cout << endl << "Current users in chat:" << endl;
	cout << PrintStruct(Leader) << " (Leader)" << endl;

	for(int i = 0; i < otherClients.size(); ++i)
	{
		cout << PrintStruct(otherClients[i]) << endl;
	}

	cout << "> ";
	flush(cout);
}

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <list>
#include <map>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include "messages.h"


#define DEFAULT_PORT 33333


using namespace std;

struct user{
    string name;
    struct sockaddr_in ipAdd;
};

class ChatClient
{
	public:
		ChatClient(char* nickname, uint16_t local_port=DEFAULT_PORT);
		ChatClient(char* nickname, struct sockaddr_in host_address, uint16_t local_port=DEFAULT_PORT);
		ChatClient(char* nickname, char* host_name, uint16_t host_port=DEFAULT_PORT, uint16_t local_port=DEFAULT_PORT);

		void ReceiveMessage();
		void HostChat();
		bool IsLeader() const;
		void DumpContactList();

		void CheckTimeout();
		void BroadcastContacts();
		void BroadcastChat();

		bool SendJoin();
	
	private:
		string m_nickname;

		int m_socket;
		int next_seqNum;
		struct sockaddr_in m_local_address;
		user Leader; //Known leader

		//used for timeouts
		struct timeval last_contacts_broadcast;
		struct timeval last_chat_broadcast;

		//used for chat state
		bool isLeader;
		bool dump_contacts;
		bool in_chat;
		int connect_attempts;

		//Data Structures
		vector<user> otherClients; //Other known connected clients
		list< pair<ChatMessage, bool> > leaderMessageList; //Used by leader for delivering messages in order
		list<ChatMessage> earlyMessageList; //Used by client to store messages with a higher seq number than the next expected
		list< pair<ChatMessage, user> > sentMessages; //Sent messages expecting replies

		bool CompareIPs(user u);
		void SetNewLeader();
		bool BroadcastLeader();

		vector<string> StructToStrings(struct user) const;
		string PrintStruct(struct user) const;
		string IPtoString(struct sockaddr_in) const;
		bool SendMessage(ChatMessage message, struct sockaddr_in destination) const;

		bool SendChat(string chat_text);

		void ProcessChatReq(char* buf, int buf_length, struct sockaddr_in sender);
		void ProcessChatRsp(char* buf, int buf_length, struct sockaddr_in sender);
		void ProcessJoinReq(char* buf, int buf_length, struct sockaddr_in sender);
		void ProcessJoinRsp(char* buf, int buf_length, struct sockaddr_in sender);
		void ProcessLeader(char* buf, int buf_length, struct sockaddr_in sender);
		void ProcessNewUser(char* buf, int buf_length, struct sockaddr_in sender);
		void ProcessContacts(char* buf, int buf_length, struct sockaddr_in sender);
};

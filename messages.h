#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <sys/time.h>

using namespace std;

class ChatMessage
{
	public:
		enum MessageType
		{
			CHAT_REQ = 1,
			CHAT_RSP = 2,
			JOIN_REQ = 3,
			JOIN_RSP = 4,
			LEADER = 5,
			NEW_USER = 6,
			CONTACTS = 7
		};

		ChatMessage();
		ChatMessage(MessageType messageType);
		ChatMessage(MessageType messageType, int num_args, string* args, int seq_num=0);
		ChatMessage(unsigned char* payload);

		void SetMessageType(MessageType messageType);

		MessageType GetMessageType() const;
		vector<string> GetArguments() const;
		struct timeval GetTimestamp() const;
		int GetAge() const;

		unsigned char* GetUdpPayload() const;
		int GetPayloadLength() const;

		int GetSeqNum() const;
		void SetSeqNum(int seq_num); 

	private:
		MessageType m_messageType;
		struct timeval timestamp;
		vector<string> m_args;
		int m_sequenceNumber;
};

#endif

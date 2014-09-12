#include "messages.h"

using namespace std;

ChatMessage::ChatMessage() : m_messageType(1), m_sequenceNumber(0)
{
	gettimeofday(&timestamp, NULL);
}

ChatMessage::ChatMessage(ChatMessage::MessageType messageType)
{
	if(messageType < 1 || messageType > 7)
	{
		cout << "Invalid type in constructor(type, num_args, args)" << endl;
		exit(1);
	}

	m_messageType = messageType;

	gettimeofday(&timestamp, NULL);
}

ChatMessage::ChatMessage(ChatMessage::MessageType messageType, int num_args, string* args, int seq_num)
{
	if(messageType < 1 || messageType > 7)
	{
		cout << "Invalid type in constructor(type, num_args, args)" << endl;
		exit(1);
	}

	m_messageType = messageType;
	m_sequenceNumber = seq_num;

	for(int i = 0; i < num_args; ++i)
	{
		m_args.push_back(args[i]);
	}

	gettimeofday(&timestamp, NULL);
}

ChatMessage::ChatMessage(unsigned char* payload)
{
	if((unsigned int)payload[0] < 1 || (unsigned int)payload[0] > 7)
	{
		cout << "Invalid type in constructor(payload)" << endl;
		exit(1);
	}

	m_messageType = payload[0];
	int num_args = payload[1];

	m_sequenceNumber = (unsigned)payload[2];
	m_sequenceNumber += (unsigned)payload[3] << 8;
	m_sequenceNumber += (unsigned)payload[4] << 16;
	m_sequenceNumber += (unsigned)payload[5] << 24;

	time_t tvsec = (unsigned)payload[6];
	tvsec += (unsigned)payload[7] << 8;
	tvsec += (unsigned)payload[8] << 16;
	tvsec += (unsigned)payload[9] << 24;
	timestamp.tv_sec = tvsec;

	suseconds_t tvusec = (unsigned)payload[10];
	tvusec += (unsigned)payload[11] << 8;
	tvusec += (unsigned)payload[12] << 16;
	tvusec += (unsigned)payload[13] << 24;
	timestamp.tv_usec = tvusec;

	int itr = 14;
	for(int i = 0; i < num_args; ++i)
	{
		string temp_string;
		while(payload[itr] != 0)
		{
			temp_string += payload[itr++];
		}
		m_args.push_back(temp_string);
		itr++;	// move past the NULL
	}
}

unsigned char* 
ChatMessage::GetUdpPayload() const
{
	vector<unsigned char> temp_vector;

	temp_vector.push_back(m_messageType);
	temp_vector.push_back(m_args.size());

	temp_vector.push_back(m_sequenceNumber & 0xFF);
	temp_vector.push_back((m_sequenceNumber >> 8) & 0xFF);
	temp_vector.push_back((m_sequenceNumber >> 16) & 0xFF);
	temp_vector.push_back((m_sequenceNumber >> 24) & 0xFF);

	temp_vector.push_back(timestamp.tv_sec & 0xFF);
	temp_vector.push_back((timestamp.tv_sec >> 8) & 0xFF);
	temp_vector.push_back((timestamp.tv_sec >> 16) & 0xFF);
	temp_vector.push_back((timestamp.tv_sec >> 24) & 0xFF);

	temp_vector.push_back(timestamp.tv_usec & 0xFF);
	temp_vector.push_back((timestamp.tv_usec >> 8) & 0xFF);
	temp_vector.push_back((timestamp.tv_usec >> 16) & 0xFF);
	temp_vector.push_back((timestamp.tv_usec >> 24) & 0xFF);

	for(int i = 0; i < m_args.size(); ++i)
	{
		for(int j = 0; j < m_args[i].size(); ++j)
		{
			temp_vector.push_back(m_args[i][j]);
		}

		temp_vector.push_back(0);
	}

	unsigned char* payload = new unsigned char[temp_vector.size()];

	for(int i = 0; i < temp_vector.size(); ++i)
	{
		payload[i] = temp_vector[i];
	}

	return payload;
}

int
ChatMessage::GetPayloadLength() const
{
	int return_value = 14;

	for(int i = 0; i < m_args.size(); ++i)
	{
		return_value += m_args[i].size();
		++return_value;
	}

	return return_value;
}

int
ChatMessage::GetAge() const
{
	int return_value = 0;
	timeval temp_time;
	gettimeofday(&temp_time, NULL);

	return_value = (temp_time.tv_sec - timestamp.tv_sec) * 1000000;
	return_value += temp_time.tv_usec - timestamp.tv_usec;

	return return_value;
}

int
ChatMessage::GetSeqNum() const
{
	return m_sequenceNumber;
}

void
ChatMessage::SetSeqNum(int seq_num)
{
	m_sequenceNumber = seq_num;
}

void
ChatMessage::SetMessageType(ChatMessage::MessageType messageType)
{
	m_messageType = messageType;
}

ChatMessage::MessageType
ChatMessage::GetMessageType() const
{
	return m_messageType;
}

vector<string>
ChatMessage::GetArguments() const
{
	return m_args;
}

struct timeval
ChatMessage::GetTimestamp() const
{
	return timestamp;
}

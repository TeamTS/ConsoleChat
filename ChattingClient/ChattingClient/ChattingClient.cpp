#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <string>
#include <thread>
#include <list>
// console input output?
#include <conio.h>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

#define IP "106.245.47.54"
#define PORT 9000

std::mutex mutex;
std::list<std::string> messageQueue;
std::string senddata;

enum class PacketCommand : unsigned short
{
	// 일반 채팅
	CHATTING,
	USER_LIST
};

struct Packet
{
	unsigned short len; // 패킷의 길이
	PacketCommand type; // 패킷 타입

	Packet(unsigned short _len, PacketCommand _type)
		: len(_len)
		, type(_type)
	{ }

	Packet(PacketCommand _type)
		: len(0)
		, type(_type)
	{ }
};

// 채팅 패킷
struct PacketChatting : public Packet
{

	PacketChatting()
		: Packet(PacketCommand::CHATTING)
	{ }

	int dataLen = 0;
	char data[1024] = {};
};


void GotoXY(const short& _iX, const short& _iY);

int main()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "FAILED STARTUP" << std::endl;
		return 1;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		return 1;

	SOCKADDR_IN servAddr;

	ZeroMemory(&servAddr, sizeof(SOCKADDR_IN));
	servAddr.sin_family = AF_INET;
	
	servAddr.sin_port = htons(PORT);
	
	inet_pton(AF_INET, IP, &servAddr.sin_addr);

	std::cout << "연결 대기중이다" << std::endl;

	// servADdr에 설정된 정보를 토대로 연결 시도
	// connect를 시도하며 ㄴ블로킹 상태가 되지만 보통은 바로 연결되어서 바로 블로킹이 풀린다(떨어진다)
	if (SOCKET_ERROR == connect(sock, reinterpret_cast<sockaddr*>(&servAddr), sizeof(servAddr)))
		return 1;
	
	std::cout << "연결 완료" << std::endl;
	
	char buffer[100] = "보낼뎅텅";

	std::thread recvThread([&]()
		{
			while (true)
			{
				// 연결되면 리시브 시작
				char buf[100] = {};
				// recv Input Output을 하는거다.
				int recvBytes = recv(sock, buf, sizeof(buf), 0);

				if (recvBytes <= 0)
					break;

				// 이제 recv상태에서 블로킹된다.
				mutex.lock();
				messageQueue.push_back(buf);
				if (messageQueue.size() == 10)
					messageQueue.pop_front();
				mutex.unlock();
			}
			return 0;
		});

	std::thread sendThread([&]() {
		do
		{
			if (_kbhit())
			{
				char input = _getch();
				if (input == VK_BACK)
				{
					if (senddata.size() != 0)
						senddata.pop_back();
				}
				else if (input == VK_RETURN)
				{
					PacketChatting packetChatting;
					packetChatting.dataLen = senddata.size();
					packetChatting.len = sizeof(Packet) + sizeof(packetChatting.dataLen) + packetChatting.dataLen;
					memcpy(packetChatting.data, senddata.c_str(), senddata.size());

					//send(sock, // 소켓
					//	senddata.c_str(), // 보낼 데이터의 주소
					//	senddata.size(), // 데이터의 길이
					//	0);

					send(sock, reinterpret_cast<const char*>(&packetChatting), packetChatting.len, 0);

					mutex.lock();
					messageQueue.push_back("나 : " + senddata);
					if (messageQueue.size() == 10)
						messageQueue.pop_front();
					mutex.unlock();
					senddata.clear();
				}
				else senddata.push_back(input);
			}
		} while (true);

		return 0;
		});

	while (true) { 
		system("cls"); // 삭제
		GotoXY(0, 0); // 이동
		
		mutex.lock();
		for (const auto& data : messageQueue)
		{
			std::cout << data << std::endl;
		}
		mutex.unlock();

		GotoXY(0, 27);
		std::cout << senddata << std::endl;
	};
}

void GotoXY(const short& _iX, const short& _iY)
{
	COORD tCurPos = { _iX, _iY };

	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), tCurPos);
}
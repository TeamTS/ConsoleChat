#include <iostream>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <string>
#include <thread>
#include <vector>
#include <map>

// 라이브러리 추가
#pragma comment(lib, "ws2_32.lib")

// 서버 아이피 포트
#define IP "192.168.0.70"
#define PORT 9000

std::vector<std::string> commands;

struct Session
{
    SOCKET socket;
    SOCKADDR_IN clientAddr;
    char recvBuffer[1000] = {};
    int curRecvLen = 0;
};

using SESSION = std::map<SOCKET, Session>;
SESSION Sessions;

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
};

struct PacketChatting : public Packet
{
    int dataLen = 0;
    char data[1024] = {};
};

void Chatting(Packet* _packet, SOCKET s)
{
    PacketChatting* packet = reinterpret_cast<PacketChatting*>(_packet);

    // 여기서 채팅 처리
    for (auto iter : Sessions)
    {
        if (s == iter.first)
            continue;
        send(iter.first, packet->data, packet->dataLen, 0);
    }
}


unsigned int __stdcall RecvThread(void* _arg)
{
    SOCKET socket = (SOCKET)_arg;

    Session& session = Sessions[socket];

    char* buffer = session.recvBuffer;

    while (true)
    {
        // 버퍼 크기 - 현재 리시브 받은 량
        // 오버플로우 방지
        int recvBytes = recv(socket, buffer + session.curRecvLen, sizeof(session.recvBuffer) - session.curRecvLen, 0);

        if (recvBytes <= 0)
            break;

        session.curRecvLen += recvBytes;

        while (session.curRecvLen >= 2)
        {
            // 클라에서는
            // 클라에서 패킷을 보낼때
            // packetLen(2btytes) + PacketCommand(2bytes) + fds(3byt
            Packet* packet = reinterpret_cast<Packet*>(buffer);

            // 이건 완패킷이 아니라는거겠지
            // 근데 recv잠만
            // 여기서 완패킷이 만약 아니라면 
            // 다음 리시브를 받을 때 버퍼에있던게 계속 유지되있어야겠지?
            // 그러니깐 세션마다 버퍼를 가지게 해야겠지?
            // 근데 여기 전에도 지금 len 변수가 2바이트잖아 1바이트만 들어왔을때 packet->len을 할 수 없잖아

            // 현재 recv받은 패킷 길이가 패킷을 읽을만한 크기이상인지
            // 정해줘야지

            // 완전한 패킷이 아니라면 break
            if (session.curRecvLen < packet->len)
                break;

            // 그럼 이때야 비로소 완패킷이라는걸 알 수 있고 패킷을 읽는거지

            // 이런식으로 패킷이 무엇인지 식별할수있도록 패킷 command를 넣어줄수있지
            // 근데 여기서 문제점이 아직 recv를 했을 때 패킷이 전부 들어왔다는 보장이 없잖아
            // 즉 이게 완전한 패킷이 라는 보장이 없는거지
            // 아까 말했다시피 recv를 했을때 10bytes중에 5만 recv했을수도있고
            // 그래서 패킷의 맨 앞에 패킷 길이까지 넣어주는거야 이 패킷의 크기가 몇인지
            switch (packet->type)
            {
                // 채팅 처리
            case PacketCommand::CHATTING:
                Chatting(packet, socket);
                break;
            case PacketCommand::USER_LIST:
                // 알맞은 처리
                break;
            }

            // 그럼 여기까지가 패킷을 처리했잖아?
            // 그럼 이제 recvBuffer를 읽은 만큼 앞으로 당겨줘야겠지?

            // 읽은 만큼 패킷 길이 감소
            // 이코드 이해됨?
            session.curRecvLen -= packet->len;
            // curRecvLen은 이제 안읽은 ㄹ리시브 패킷들이지

            // 땡기는 이유
            // sizeof(buffer) - session.curRecvLen 이거 음수로 갈까봐
            memcpy(session.recvBuffer, // 버퍼의 시작 주소
                session.recvBuffer + packet->len, // 버퍼 시작 주소 + 패킷 길이  
                session.curRecvLen);
        }


        // 이게 요런식으로 계속계속 뭔가 다른 처리를할떄마다 분기를 해줘야하잖아
        // 그리고 또 하나의 문제점이 recv를 받을때 
        // 얘가 /check users만 왔다는
        // 보장이없거든? 예를들어 /check users 를 보내고 저족에서
        // 그리고 또 무언가 fds 이런 채팅을 썼을떄
        // 여기서 recv 받을때 /check users만 딱 ㅂ왔다는 보장도없고 /check usersfds 이런식으로 받았을수도있고
        // 또 /check u 이거까지만 왔을수도있거든?
        // 보통 앞에 이 패킷이 어떤 종류의 패킷인지 식별할수있는 코드를 넣는거지
        // 
        //if (buffer == "/check users")
        //{
        //    
        //}
        //else
        //{
        //    char ipAddress[64] = {};
        //    inet_ntop(AF_INET, &Sessions[socket].clientAddr.sin_addr, ipAddress, sizeof(ipAddress));
        //    std::string lastData = ipAddress;
        //    std::string senddata = buffer;
        //    lastData += " : " + senddata;
        //}
        //
        //// 이제 여기서 다른 소켓들한테 뿌려준다.
        //for (auto& pair : Sessions)
        //{
        //    // 자기 자신 빼고 send
        //    if (pair.first == socket)
        //        continue;

        //    send(pair.first, ip.c_str(), ip.size(), 0);
        //}
    }

    // 여기까지온거는 소켓이 종류된거임.
    auto iter = Sessions.find(socket);

    if (iter != Sessions.end())
        Sessions.erase(iter);

    return 0;
}

int main()
{
    commands.push_back("/check users");

    // winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cout << "Failed WSAStartup()" << std::endl;
        return 1;
    }

    // TCP 소켓 생성
    SOCKET listenSocket = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    // tcp : SOCK_STREAM
    // udp : SOCK_DGRAM 데이터 그램

    // 유효하지 않은 소켓이라면
    if (listenSocket == INVALID_SOCKET)
        return 1;

    SOCKADDR_IN sockAddr;
    // 처음에 무조건 0으로 메모리 밀어줘야함
    ZeroMemory(&sockAddr, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;

    // 포트 설정
    sockAddr.sin_port = htons(PORT);

    // 아이피 설정
    inet_pton(AF_INET, IP, &sockAddr.sin_addr);

    // 소켓 바인딩
    if (SOCKET_ERROR == bind(listenSocket, reinterpret_cast<sockaddr*>(&sockAddr), sizeof(sockAddr)))
        return 1;

    if (SOCKET_ERROR == listen(listenSocket, SOMAXCONN))
        return 1;

    std::cout << "연결 대기중...." << std::endl;

    SOCKADDR_IN clientAddr;
    int addrLen = sizeof(clientAddr);

    // accept 스레드를 생성
    std::thread acceptThread([&]()
        {
            // 계속 accept를 반복한다.
            while (true)
            {
                // 클라이언트 연결을 기다림..
                SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);

                if (clientSocket == INVALID_SOCKET)
                    continue;

                char ipAddress[64] = {};
                inet_ntop(AF_INET, &clientAddr.sin_addr, ipAddress, sizeof(ipAddress));

                // 소켓 저장
                Sessions.insert(SESSION::value_type(clientSocket, { clientSocket, clientAddr }));

                unsigned int threadId = 0;

                // 리시브 스레드 생성
                _beginthreadex(NULL, NULL,
                    &RecvThread,
                    reinterpret_cast<void*>(clientSocket),
                    0, &threadId);
            }

            return 0;
        });

    // 주 스레드가 종료되지 않게 루프
    while (true);
}
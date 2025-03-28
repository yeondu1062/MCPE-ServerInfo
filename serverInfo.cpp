/*____                          ___        __
 / ___|  ___ _ ____   _____ _ _|_ _|_ __  / _| ___
 \___ \ / _ \ '__\ \ / / _ \ '__| || '_ \| |_ / _ \
  ___) |  __/ |   \ V /  __/ |  | || | | |  _| (_) |
 |____/ \___|_|    \_/ \___|_| |___|_| |_|_|  \___/
    written by @yeondu1062.
*/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define BUFSIZE 512

#include <winsock2.h>
#include <iostream>
#include <string>
#include <sstream>

static void setConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

static void prefixHeader() {
    setConsoleColor(10);
    std::cout << "[ServerInfo] ";
    setConsoleColor(15);
}

static void prefixError() {
    setConsoleColor(12);
    std::cout << "[Error] ";
    setConsoleColor(15);
}

static void errQuit(const char* msg, SOCKET sock = INVALID_SOCKET) {
    prefixError();
    std::cout << msg << std::endl;
    if (sock != INVALID_SOCKET) closesocket(sock);
    WSACleanup(); exit(-1);
}

static void convertToAscii(const unsigned char* buffer, int length, std::string &output) {
    for (int i = 0; i < length; i++) {
        if (i < length - 1 && buffer[i] == 0xC2 && buffer[i + 1] == 0xA7) {
            output += "§"; i++; //UNICODE (0xC2 0xA7)
        }
        //ASCII RANGE
        else if (buffer[i] >= 32 && buffer[i] <= 126) output += buffer[i];
    }
}

static void printInfoData(const std::string &data) {
    std::istringstream stream(data);
    std::string part;
    std::string parts[9];

    int index = 0;
    while (std::getline(stream, part, ';') && index < 9) {
        parts[index] = part; index++;
    }

    if (index < 9) errQuit("잘못된 데이터입니다.");

    setConsoleColor(7);
    std::cout << "  MOTD1: " << parts[1] << std::endl
        << "  MOTD2: " << parts[7] << std::endl
        << "  버전: " << parts[3] << std::endl
        << "  인원: " << parts[4] << "/" << parts[5] << std::endl
        << "  게임모드: " << parts[8] << std::endl;
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) errQuit("소켓초기화에 실패하였습니다.");

    while (true) {
        char address[256];
        prefixHeader();
        std::cout << "서버주소(DNS or IP)를 입력해주세요: ";
        std::cin.getline(address, sizeof(address));

        struct hostent* host;
        struct in_addr** addrList;

        host = gethostbyname(address);
        if (host == NULL) errQuit("서버를 찾지 못하였습니다.");

        addrList = (struct in_addr**)host->h_addr_list;

        prefixHeader();
        std::string ip = inet_ntoa(*addrList[0]);
        std::cout << "서버(" << ip << ")의 정보입니다." << std::endl;

        SOCKADDR_IN serverAddr{};
        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (sock == INVALID_SOCKET) errQuit("소켓생성에 실패하였습니다.", sock);

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
        serverAddr.sin_port = htons(19132);

        unsigned char packet[] = {
            0x01, //Unconnected Ping (0x01)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //Time Since Start (0ms)
            0x00, 0xFF, 0xFF, 0x00, 0xFE, 0xFE, 0xFE, 0xFE,
            0xFD, 0xFD, 0xFD, 0xFD, 0x12, 0x34, 0x56, 0x78, //Offline Message
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  //Client Guid
        };

        if (sendto(sock, (char*)packet, sizeof(packet), 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) errQuit("패킷송신에 실패하였습니다.", sock);

        SOCKADDR_IN fromAddr{};
        char recvBuf[BUFSIZE]{};
        int addrLen = sizeof(fromAddr);
        int recvLen = recvfrom(sock, recvBuf, BUFSIZE, 0, (SOCKADDR*)&fromAddr, &addrLen);

        if (recvLen == SOCKET_ERROR) errQuit("패킷수신에 실패하였습니다.", sock);

        std::string receivedData;
        convertToAscii((unsigned char*)recvBuf, recvLen, receivedData);
        printInfoData(receivedData);
        closesocket(sock);
    }

    WSACleanup();
    return 0;
}

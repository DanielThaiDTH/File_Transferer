#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <algorithm>
#include <deque>
#include "netfiletransferer.h"
//#include <windows.networking.sockets.h>
//#pragma comment(lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS

#define NO_FILES 3
#define IMPROPER_ARGS 2

void inputValid(std::string prompt)
{
	std::cout << "Need to provide valid input.\n";
}

/*Enforces only valid answers as input. Varargs can only be strings.*/
template<typename... Targs>
std::enable_if_t<std::conjunction_v<std::is_constructible<std::string, Targs>...>, std::string>
inputValid(std::string prompt, Targs... args)
{
	std::string valid_list[sizeof...(args) + 1] = { std::string(args)..., "" };
	std::string ans;
	bool repeat = true;
	
	while (repeat) {
		std::cout << prompt;
		std::getline(std::cin, ans);

		for (std::string item : valid_list) {
			if (item == ans && item != "") {
				repeat = false;
				break;
			}
		}
	}

	return ans;
}


int inputRange(std::string prompt, int min, int max) 
{
	int ans = 0;

	while (true) {
		std::cout << prompt;
		std::cin >> ans;

		if (ans >= min && ans <= max)
			break;
	}

	std::cin.ignore(1000, '\n');
	return ans;
}


int manualOp()
{
	std::string res;
	NetFileTransferer* nft = nullptr;

	std::string ans = inputValid("Will this be the source or the destination? (s or d): ", "s", "d");

	if (ans == "s") {
		
		std::string addr;
		nft = new NetFileTransferer(true);
		
		std::cout << "Enter server address to send to: ";
		std::getline(std::cin, addr);
		int dest_port = inputRange("Enter the server port number to send to (1029 to 49150): ", 1024, 49150);
		nft->set_destination(addr, dest_port);

		if (nft->connect() && nft->check_connection()) {
			std::cout << "Enter file to send: ";
			std::getline(std::cin, ans);
			std::cout << ans << std::endl;
			nft->set_file(ans);

			if (nft->info_exchange()) {
				size_t amount = nft->send();
				std::cout << amount << " bytes sent.\n";
			} else {
				std::cout << "Could not exhange information.\n";
			}
		}

	} else if (ans == "d") {
		
		nft = new NetFileTransferer(false);
		nft->set_port(inputRange("Enter the server port number to listen on (1029 to 49150): ", 1024, 49150));
	
		if (nft->connect() && nft->check_connection()) {
			if (nft->info_exchange()) {
				size_t amount = nft->receive();
				std::cout << amount << " bytes received.\n";
				if (nft->save())
					std::cout << "File " << nft->get_file() << " written.\n";
			} else {
				std::cout << "Could not exhange information.\n";
			}
		}

	} else {
		std::cout << "No choice was given.";
		return 1;
	}

	delete nft;
	return 0;
}


int autoOp(bool isSrc, std::deque<char*>& files, int port, std::string addr = "")
{
	bool isRandPort = false;
	if (port == 0 && isSrc) {
		std::cout << "Improper port number given. Exiting.\n";
		return IMPROPER_ARGS;
	} else if (port == 0 && !isSrc) {
		isRandPort = true;
	}

	NetFileTransferer* nft = new NetFileTransferer(isSrc);

	if (isSrc) {
		nft->set_destination(addr, port);

		if (nft->connect() && nft->check_connection()) {
			while (files.size() > 0) {
				nft->set_file(files.front());
				std::cout << "Sending " << files.front() << std::endl;
				files.pop_front();

				if (nft->info_exchange()) {
					size_t amount = nft->send();
					std::cout << amount << " bytes sent.\n";
				} else {
					std::cout << "Could not exhange information.\n";
				}
				nft->reset();
			}//while
		}
	} else {
		nft->set_port(port);
	}

	delete nft;
	return 0;
}


int main(int argc, char* argv[])
{
	int exit_code = 0;
	std::cout << "*****************************\n";
	std::cout << "** File Transferer Program **\n";
	std::cout << "*****************************\n\n";
	std::vector<char*> args(argv, argv + argc);

	if (argc > 1 && std::strcmp(args[1], "help") == 0) {
		std::cout << "Usage: " << args[0] << " [-s] [-d] [Port] [Address]\n";
		std::cout << "Use -s or s to set as a file source. "
			<< "Use -d or d to set as a file destination. "
			<< "Can only set source or destination exclusvely. "
			<< "If using arguments, port number and "
			<< "destination address (for source option) "
			<< "must be provided. Running with no args will "
			<< "require manual input of information.\n";
	}

	auto src_opt = [](const char* arg) -> bool {
		return std::strcmp(arg, "-s") == 0 || std::strcmp(arg, "s") == 0;
	};
	auto dest_opt = [](const char* arg) -> bool {
		return std::strcmp(arg, "-d") == 0 || std::strcmp(arg, "d") == 0;
	};

	bool isSrc = std::any_of(args.cbegin(), args.cend(), src_opt);
	bool isDst = std::any_of(args.cbegin(), args.cend(), dest_opt);


	if (argc < 3 || (isSrc && isDst) || (!isSrc && !isDst)) {
		if (((isSrc && isDst) || (!isSrc && !isDst)) && argc != 1) 
			std::cout << "Improper arguments. Running manually.\n";
		
		exit_code = manualOp();
	} else {
		std::deque<char*> files;
		if (isSrc) {
			files.assign(args.cbegin() + 4, args.cend());
			if (files.size() > 0)
				exit_code = autoOp(isSrc, files, std::atoi(args[2]), args[3]);
			else 
				exit_code = NO_FILES;
		} else {
			files.assign(args.cbegin() + 3, args.cend());
			if (files.size() > 0)
				exit_code = autoOp(isSrc, files, std::atoi(args[2]));
			else
				exit_code = NO_FILES;
		}
	}

	if (exit_code == NO_FILES)
		std::cout << "No file list provided.\n";

	if (exit_code == 0)
		WSACleanup();

	return exit_code;
}


/*Old*/
//void loopConn(SOCKET ClientSocket) {
//	int received;
//	char RxBuffer[128] = {};
//	char message[16] = "Hello World";
//
//	while (std::strcmp(RxBuffer, "[q]") != 0) {
//		received = recv(ClientSocket, RxBuffer, sizeof(RxBuffer), 0);
//
//		if (received != SOCKET_ERROR && received > 0) {
//			std::cout << "Response: " << RxBuffer << std::endl;
//			std::cout << received << std::endl << std::endl;
//		}
//
//		send(ClientSocket, message, sizeof(message), 0);
//	}
//}


//int main() {
//	WSADATA wsaData;
//
//	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//		std::cout << "Unable to load DLL(s).\n";
//		return 1;
//	}
//
//	SOCKET ServerSocket;
//	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (ServerSocket == INVALID_SOCKET) {
//		std::cout << "Could not create socket.\n";
//		WSACleanup();
//		return 0;
//	}
//
//	struct sockaddr_in SvrAddr;
//	SvrAddr.sin_family = AF_INET;
//	SvrAddr.sin_port = htons(27010);
//	SvrAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
//
//	if (bind(ServerSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
//		std::cout << "Could not bind to socket.\n";
//		closesocket(ServerSocket);
//		WSACleanup();
//		return 0;
//	}
//
//	if (listen(ServerSocket, 1) == SOCKET_ERROR) {
//		std::cout << "Could not start to listen" << std::endl;
//		closesocket(ServerSocket);
//		WSACleanup();
//		return 0;
//	}
//
//	struct sockaddr_in ClntAddr;
//	int clntAddrSize = sizeof(ClntAddr);
//
//	std::cout << "Waiting for client connection" << std::endl;
//
//	SOCKET ClientSocket;
//	ClientSocket = accept(ServerSocket, NULL, NULL);
//	if (ClientSocket == INVALID_SOCKET) {
//		std::cout << "Failed to accept connection" << std::endl;
//		closesocket(ServerSocket);
//		WSACleanup();
//		return 0;
//	}
//
//	getpeername(ClientSocket, (sockaddr*)&ClntAddr, &clntAddrSize);
//
//	closesocket(ServerSocket);
//	std::cout << "Connected to " << inet_ntoa(ClntAddr.sin_addr) << ":" << ntohs(ClntAddr.sin_port) << std::endl;
//
//	loopConn(ClientSocket);
//
//	closesocket(ClientSocket);
//	WSACleanup();
//	std::cout << "Server finished\n";
//	//int temp;
//	//std::cin >> temp;
//
//	return 0;
//}
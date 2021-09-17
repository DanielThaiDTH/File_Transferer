#include "connector.h"

bool Connector::dlls_started = false;

void Connector::start_dlls()
{
	WSADATA wsaData;

	if (dlls_started) {
		std::cout << "DLLs already started\n";
	} else if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "Unable to load DLL(s).\n";
		dlls_started = false;
	}

	dlls_started = true;
}


Connector::Connector()
{
	this->active_socket = INVALID_SOCKET;
	this->port = 0;
	if (!dlls_started)
		start_dlls();
}


Connector::Connector(std::string ip, int port) : Connector()
{
	this->ip = ip;
	this->port = port;
}


Connector::~Connector()
{
	shutdown(active_socket, SD_BOTH);
	closesocket(active_socket);
}


bool Connector::isReady()
{
	return dlls_started;
}


int Connector::get_port() const
{
	return port;
}


uint32_t Connector::get_buf_size()
{
	return RECV_BUF_SIZE;
}


int Connector::change_conn(std::string addr, int port)
{
	shutdown(active_socket, SD_BOTH);
	int err = closesocket(active_socket);

	if (err != SOCKET_ERROR) {
		ip = addr;
		this->port = port;
		return 0;
	} else {
		return WSAGetLastError();
	}
}


void Connector::display_info() const
{
	std::string connType;
	if (this->role == "server")
		connType = "Listening ";
	else if (this->role == "client")
		connType = "Foreign ";

	std::cout << connType << "IP address: " << this->ip << std::endl;
	std::cout << connType << "Port: " << this->port;
}
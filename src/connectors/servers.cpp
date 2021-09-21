#include <cstring>
#include "servers.h"
#include "connector.h"

Server::~Server()
{
	state = ServerState::ERR;
}


ServerState Server::getState() const
{
	return state;
}

/***TCP Server***/
TCP_Server::TCP_Server() : TCP_Connector("0.0.0.0", 0), Server()
{
	active_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	conn_socket = INVALID_SOCKET;
	
	if (isConnected())
		this->state = ServerState::ERR;
	else
		this->state = ServerState::READY;
}


TCP_Server::TCP_Server(int port) : TCP_Connector("0.0.0.0", port)
{
	active_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	conn_socket = INVALID_SOCKET;

	if (!isConnected())
		this->state = ServerState::ERR;
	else
		this->state = ServerState::READY;
}


TCP_Server::TCP_Server(std::string addr, int port) : TCP_Connector(addr, port)
{
	active_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	conn_socket = INVALID_SOCKET;

	if (!isConnected())
		this->state = ServerState::ERR;
	else
		this->state = ServerState::READY;
}


TCP_Server::~TCP_Server()
{
	if (this->state == ServerState::CONNECTED) {
		closesocket(conn_socket);
	}

	this->state = ServerState::ERR;
}


bool TCP_Server::bind_socket()
{
	if (state != ServerState::ERR) {
		if (bind(this->active_socket, (struct sockaddr*)&SvrAddr,
			sizeof(SvrAddr)) == SOCKET_ERROR) {
			std::cout << "Could not bind socket to address." << std::endl;
			return false;
		} else {
			state = ServerState::BINDED;
			return true;
		}
	}

	std::cout << "Socket is not set up." << std::endl;
	return false;
}


int TCP_Server::change_port(int new_port)
{
	int err = Connector::change_conn("0.0.0.0", new_port);

	this->SvrAddr.sin_addr.s_addr = inet_addr(this->ip.c_str());
	this->SvrAddr.sin_port = htons(this->port);

	if (err == 0) {
		active_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (active_socket != INVALID_SOCKET)
			state = ServerState::READY;

		return 0;
	} else {
		return err;
	}
}


bool TCP_Server::await_conn()
{
	if (this->state == ServerState::LISTENING)
		return true;

	if (state != ServerState::BINDED || listen(this->active_socket, 10) == SOCKET_ERROR) {
		std::cout << "Could not start to listen on this socket" << std::endl;
		this->state = ServerState::ERR;
		return false;
	}

	this->state = ServerState::LISTENING;
	return true;
}


bool TCP_Server::get_conn()
{
	this->conn_socket = accept(active_socket, NULL, NULL);

	if (this->conn_socket == SOCKET_ERROR) {
		std::cout << "Unable to establish connection.\n";
		closesocket(this->conn_socket);
		return false;
	} else {
		this->state = ServerState::CONNECTED;
		return true;
	}
}


void TCP_Server::disconnect()
{
	if (state == ServerState::CONNECTED) {
		shutdown(conn_socket, SD_BOTH);
		closesocket(conn_socket);
		state = ServerState::LISTENING;
	}
}


void TCP_Server::settimeout(uint32_t timeout)
{
	if (state == ServerState::CONNECTED) {
		if (timeout == 0)
			timeout = 60 * 1000;
		setsockopt(conn_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(uint32_t));
	}
}


int TCP_Server::send_msg(std::string msg)
{
	if (this->state != ServerState::CONNECTED)
		return SOCKET_ERROR;

	int err = send(conn_socket, msg.c_str(), msg.length(), 0);

	if (err == SOCKET_ERROR)
		disconnect();

	return err;
}


int TCP_Server::send_msg(std::vector<char> msg)
{
	if (this->state != ServerState::CONNECTED)
		return SOCKET_ERROR;

	int err = send(conn_socket, msg.data(), msg.size(), 0);

	if (err == SOCKET_ERROR)
		disconnect();

	return err;
}


int TCP_Server::receive_msg(std::string& msg)
{
	char RxBuffer[RECV_BUF_SIZE] = { 0 };

	int recvsize = recv(conn_socket, RxBuffer, sizeof(RxBuffer), 0);

	if (recvsize != SOCKET_ERROR)
		msg = std::string(RxBuffer, recvsize);
	else
		disconnect();

	return recvsize;
}


int TCP_Server::receive_msg(std::vector<char>& msg)
{
	char RxBuffer[RECV_BUF_SIZE] = { 0 };
	msg.reserve(RECV_BUF_SIZE);

	int recvsize = recv(conn_socket, RxBuffer, sizeof(RxBuffer), 0);

	if (recvsize != SOCKET_ERROR)
		msg.assign(RxBuffer, RxBuffer + recvsize);
	else
		disconnect();
	

	return recvsize;
}
/***TCP Server***/



/***UDP Server***/

/*Sets address to 0.0.0.0 and port to 0*/
UDP_Server::UDP_Server() : UDP_Connector("0.0.0.0", 0), Server()
{
	active_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (!isConnected())
		this->state = ServerState::ERR;
	else
		this->state = ServerState::READY;

	int varsize = sizeof(unsigned int);

	getsockopt(active_socket, SOL_SOCKET, SO_MAXDG, 
		reinterpret_cast<char*>(&max_size), &varsize);
}


UDP_Server::UDP_Server(int port) : UDP_Connector("0.0.0.0", port)
{
	active_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (!isConnected())
		this->state = ServerState::ERR;
	else
		this->state = ServerState::READY;

	int varsize = sizeof(unsigned int);

	getsockopt(active_socket, SOL_SOCKET, SO_MAXDG,
		reinterpret_cast<char*>(&max_size), &varsize);
}


UDP_Server::UDP_Server(std::string addr, int port) : UDP_Connector(addr, port)
{
	active_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	max_size = 0;

	if (!isConnected())
		this->state = ServerState::ERR;
	else
		this->state = ServerState::READY;

	int varsize = sizeof(unsigned int);

	getsockopt(active_socket, SOL_SOCKET, SO_MAXDG,
		reinterpret_cast<char*>(&max_size), &varsize);
}


UDP_Server::~UDP_Server()
{
	this->state = ServerState::ERR;
}


bool UDP_Server::bind_socket()
{
	if (state != ServerState::ERR) {
		if (bind(this->active_socket, (struct sockaddr*)&SvrAddr,
			sizeof(SvrAddr)) == SOCKET_ERROR) {
			std::cout << "Could not bind socket to address." << std::endl;
			return false;
		} else {
			state = ServerState::BINDED;
			return true;
		}
	}

	std::cout << "Socket is not set up." << std::endl;
	return false;
}


void UDP_Server::disconnect()
{
	CltAddr.sin_port = 0;
	CltAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
}


int UDP_Server::send_msg(std::string msg)
{
	if (state == ServerState::BINDED) {
		int err = sendto(this->active_socket, msg.c_str(), msg.length(), 0,
			(struct sockaddr*)&CltAddr, sizeof(CltAddr));

		if (err == SOCKET_ERROR && WSAGetLastError() == WSAENOTSOCK)
			state = ServerState::ERR;
		else if (err == SOCKET_ERROR)
			std::cout << "Could not send to destination port.\n";

		return err;
	} else {
		return -1;
	}
}


int UDP_Server::send_msg(std::vector<char> msg)
{
	if (state == ServerState::BINDED) {
		int err = sendto(this->active_socket, msg.data(), msg.size(), 0,
			(struct sockaddr*)&CltAddr, sizeof(CltAddr));

		if (err == SOCKET_ERROR && WSAGetLastError() == WSAENOTSOCK)
			state = ServerState::ERR;
		else if (err == SOCKET_ERROR)
			std::cout << "Could not send to destination port.\n";

		return err;
	} else {
		return -1;
	}
}


int UDP_Server::send_msg(std::string msg, std::string dest_addr, int dest_port)
{
	CltAddr.sin_port = htons(dest_port);
	CltAddr.sin_addr.s_addr = inet_addr(dest_addr.c_str());
	
	return send_msg(msg);
}


int UDP_Server::receive_msg(std::string& msg)
{
	int addr_len = sizeof(CltAddr);
	size_t data_size = (RECV_BUF_SIZE < max_size) ? RECV_BUF_SIZE : max_size;

	char* RxBuffer = new char[data_size];
	std::memset(RxBuffer, 0, data_size);

	int err = recvfrom(this->active_socket, RxBuffer, data_size, 0,
		(struct sockaddr*)&CltAddr, &addr_len);

	if (err != SOCKET_ERROR)
		msg = std::string(RxBuffer);

	delete[] RxBuffer;
	return err;
}


int UDP_Server::receive_msg(std::vector<char>& msg)
{
	int addr_len = sizeof(CltAddr);
	size_t data_size = (RECV_BUF_SIZE < max_size) ? RECV_BUF_SIZE : max_size;

	char* RxBuffer = new char[data_size];
	std::memset(RxBuffer, 0, data_size);
	msg.reserve(data_size);

	int err = recvfrom(this->active_socket, RxBuffer, data_size, 0,
		(struct sockaddr*)&CltAddr, &addr_len);

	if (err != SOCKET_ERROR)
		msg.assign(RxBuffer, RxBuffer + data_size);

	delete[] RxBuffer;
	return err;
}


uint32_t UDP_Server::get_max_msg_size() const
{
	return max_size;
}
/***UDP Server***/
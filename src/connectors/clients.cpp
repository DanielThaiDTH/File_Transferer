#include "clients.h"

Client::~Client()
{
	state = ClientState::ERR;
}


ClientState Client::getState() const
{
	return this->state;
}


/*TCP_Client*/
TCP_Client::TCP_Client() : TCP_Connector("0.0.0.0", 0), Client()
{
	active_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (!isConnected())
		this->state = ClientState::ERR;
	else
		this->state = ClientState::READY;
}


TCP_Client::~TCP_Client()
{
	if (this->state != ClientState::ERR) {
		closesocket(active_socket);
		this->state = ClientState::ERR;
	}
}


int TCP_Client::change_conn(std::string addr, int port)
{
	int err = Connector::change_conn(addr, port);

	this->SvrAddr.sin_addr.s_addr = inet_addr(this->ip.c_str());
	this->SvrAddr.sin_port = htons(this->port);

	if (err == 0) {
		active_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (active_socket != INVALID_SOCKET)
			state = ClientState::READY;

		return 0;
	} else {
		return err;
	}
}


bool TCP_Client::reconnect()
{
	if (this->state == ClientState::READY) {
		if (connect(active_socket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr))
			== SOCKET_ERROR) {
			std::cout << "Could not bind socket to address." << std::endl;
			this->state = ClientState::ERR;
			return false;
		} else {
			this->state = ClientState::CONNECTED;
			return true;
		}
	} else if (this->state == ClientState::CONNECTED) {
		std::cout << "Already connected.\n";
		return false;
	} else {
		std::cout << "Socket is not ready to connect.\n";
		return false;
	}
}


bool TCP_Client::connect_to(std::string addr, int port)
{
	if (change_conn(addr, port) == 0)
		return reconnect();
	else
		return false;
}


bool TCP_Client::connect_to()
{
	this->SvrAddr.sin_addr.s_addr = inet_addr(this->ip.c_str());
	this->SvrAddr.sin_port = htons(this->port);

	return reconnect();
}


int TCP_Client::send_msg(std::string msg)
{
	if (this->state != ClientState::CONNECTED)
		return SOCKET_ERROR;

	int err = send(active_socket, msg.c_str(), msg.length(), 0);

	return err;
}


int TCP_Client::send_msg(std::vector<char> msg)
{
	if (this->state != ClientState::CONNECTED)
		return SOCKET_ERROR;

	int err = send(active_socket, msg.data(), msg.size(), 0);

	return err;
}


int TCP_Client::receive_msg(std::string& msg)
{
	char RxBuffer[RECV_BUF_SIZE] = { 0 };

	int recvsize = recv(active_socket, RxBuffer, sizeof(RxBuffer), 0);

	if (recvsize != SOCKET_ERROR)
		msg = std::string(RxBuffer, recvsize);

	return recvsize;
}


int TCP_Client::receive_msg(std::vector<char>& msg)
{
	char RxBuffer[RECV_BUF_SIZE] = { 0 };
	msg.reserve(RECV_BUF_SIZE);

	int recvsize = recv(active_socket, RxBuffer, sizeof(RxBuffer), 0);

	if (recvsize != SOCKET_ERROR)
		msg.assign(RxBuffer, RxBuffer + recvsize);

	return recvsize;
}
/*End TCP_Client*/



/*UDP_Client*/
UDP_Client::UDP_Client() : UDP_Connector("0.0.0.0", 0), Client()
{
	active_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (!isConnected())
		this->state = ClientState::ERR;
	else
		this->state = ClientState::READY;
}


UDP_Client::~UDP_Client()
{
	if (this->state != ClientState::ERR) {
		closesocket(active_socket);
		this->state = ClientState::ERR;
	}
}


int UDP_Client::change_conn(std::string addr, int port)
{
	int err = Connector::change_conn(addr, port);

	if (err == 0) {

		this->SvrAddr.sin_port = htons(this->port);
		this->SvrAddr.sin_addr.s_addr = inet_addr(this->ip.c_str());
		active_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (active_socket != INVALID_SOCKET)
			state = ClientState::READY;
		else
			return WSAGetLastError();

		return 0;
	} else {
		return err;
	}
}


int UDP_Client::send_msg(std::string msg)
{
	if (this->state != ClientState::CONNECTED)
		return SOCKET_ERROR;

	int err = sendto(active_socket, msg.c_str(), msg.length(), 0, 
		(struct sockaddr*)&SvrAddr, sizeof(SvrAddr));

	if (err == SOCKET_ERROR && WSAGetLastError() == WSAENOTSOCK)
		state = ClientState::ERR;

	return err;
}


int UDP_Client::send_msg(std::vector<char> msg)
{
	if (this->state != ClientState::CONNECTED)
		return SOCKET_ERROR;

	int err = sendto(active_socket, msg.data(), msg.size(), 0,
		(struct sockaddr*)&SvrAddr, sizeof(SvrAddr));

	if (err == SOCKET_ERROR && WSAGetLastError() == WSAENOTSOCK)
		state = ClientState::ERR;

	return err;
}


int UDP_Client::receive_msg(std::string& msg)
{
	int addr_len = sizeof(SvrAddr);
	size_t data_size = (RECV_BUF_SIZE < max_size) ? RECV_BUF_SIZE : max_size;

	char* RxBuffer = new char[data_size];
	std::memset(RxBuffer, 0, data_size);


	int recvsize = recvfrom(active_socket, RxBuffer, data_size, 0, 
		(struct sockaddr*)&SvrAddr, &addr_len);

	if (recvsize != SOCKET_ERROR)
		msg = std::string(RxBuffer, recvsize);

	delete[] RxBuffer;
	return recvsize;
}


int UDP_Client::receive_msg(std::vector<char>& msg)
{
	int addr_len = sizeof(SvrAddr);
	size_t data_size = (RECV_BUF_SIZE < max_size) ? RECV_BUF_SIZE : max_size;

	char* RxBuffer = new char[data_size];
	std::memset(RxBuffer, 0, data_size);
	msg.reserve(data_size);


	int recvsize = recvfrom(active_socket, RxBuffer, data_size, 0,
		(struct sockaddr*)&SvrAddr, &addr_len);

	if (recvsize != SOCKET_ERROR)
		msg.assign(RxBuffer, RxBuffer + recvsize);

	delete[] RxBuffer;
	return recvsize;
}
/*End UDP_Client*/


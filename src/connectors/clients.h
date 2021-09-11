#ifndef FILET_CLIENT_H
#define FILET_CLIENT_H

#include "connector.h"

enum class ClientState { ERR, READY, CONNECTED };

class Client
{
protected:
	ClientState state = ClientState::ERR;
public:
	Client() {}
	Client(const Client& src) = delete;
	virtual ~Client();
	ClientState getState() const;
};


class TCP_Client : public Client, public TCP_Connector
{
public:
	TCP_Client();
	TCP_Client(const TCP_Client& src) = delete;
	~TCP_Client();
	/*Changes connection parameters. Returns WSA last error code, 0 if no problem.*/
	int change_conn(std::string addr, int port) override;
	bool reconnect();
	bool connect_to(std::string addr, int port);
	bool connect_to();
	int send_msg(std::string msg) override;
	int send_msg(std::vector<char> msg) override;
	int receive_msg(std::string& msg) override;
	int receive_msg(std::vector<char>& msg) override;
	TCP_Client& operator=(const TCP_Client&) = delete;
};


class UDP_Client : public Client, public UDP_Connector
{
public:
	UDP_Client();
	UDP_Client(const UDP_Client& src) = delete;
	~UDP_Client();
	/*Changes connection parameters. Returns WSA last error code, 0 if no problem.*/
	int change_conn(std::string addr, int port) override;
	int send_msg(std::string msg) override;
	int send_msg(std::vector<char> msg) override;
	int receive_msg(std::string& msg) override;
	int receive_msg(std::vector<char>& msg) override;
	UDP_Client& operator=(const UDP_Client&) = delete;
};
#endif
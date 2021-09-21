#ifndef FILET_SERVERS_H
#define FILET_SERVERS_H

#include "connector.h"

/*Classifies the state of a server*/
enum class ServerState { ERR, READY, BINDED, LISTENING, CONNECTED };

class Server
{
protected:
	ServerState state = ServerState::ERR;
public:
	Server() {};
	Server(const Server& src) = delete;
	virtual ~Server();
	ServerState getState() const;
	virtual bool bind_socket() = 0;
	virtual void disconnect() = 0;
};

class TCP_Server : public Server, public TCP_Connector
{
	SOCKET conn_socket;
public:
	TCP_Server();
	TCP_Server(int port);
	TCP_Server(std::string addr, int port);
	TCP_Server(const TCP_Server& src) = delete;
	~TCP_Server();
	bool bind_socket() override;
	int change_port(int new_port);
	
	/*Sets to listening state*/
	bool await_conn();
	bool get_conn();

	/*Disconnects if there is an active connection. Sets to Listening state. Does nothing otherwise.*/
	void disconnect() override;

	/*Starts the timeout for the connection. If a timeout of 0 is set, blocking mode will be set. Time 
	units are in milliseconds. */
	void settimeout(uint32_t timeout);
	int send_msg(std::string msg) override;
	int send_msg(std::vector<char> msg) override;
	int receive_msg(std::string& msg) override;
	int receive_msg(std::vector<char>& msg) override;
	TCP_Server& operator=(const TCP_Server&) = delete;
};

class UDP_Server : public Server, public UDP_Connector
{
public:
	UDP_Server();
	UDP_Server(int port);
	UDP_Server(std::string addr, int port);
	UDP_Server(const UDP_Server& src) = delete;
	~UDP_Server();
	/*Binds socket if the active_socket is not in an error state*/
	bool bind_socket() override;
	void disconnect() override;
	uint32_t get_max_msg_size() const;
	int send_msg(std::string msg) override;
	int send_msg(std::vector<char> msg) override;
	int send_msg(std::string msg, std::string dest_addr, int dest_port);
	template<typename T>
	int send_msg(T msg, std::string dest_addr, int dest_port)
	{
		CltAddr.sin_port = htons(dest_port);
		CltAddr.sin_addr.s_addr = inet_addr(dest_addr.c_str());

		return send_msg(msg);
	}
	int receive_msg(std::string& msg) override;
	int receive_msg(std::vector<char>& msg) override;
	UDP_Server& operator=(const UDP_Server&) = delete;
};

#endif
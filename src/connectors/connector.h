#ifndef FILET_CONNECTOR_H
#define FILET_CONNECTOR_H

#include <string>
#include <iostream>
#include <vector>
#include <windows.networking.sockets.h>
#pragma comment(lib, "Ws2_32.lib")

/*Wraps a socket that is used to connect to another socket.
  Stores the IP address and the port. Class also inidcates if Winsock DLLs are 
  loaded. Is the responsiblity of the client to call WSACleanup()*/
class Connector
{
protected:
    static bool dlls_started;
    static const uint32_t RECV_BUF_SIZE = 8192;
    std::string ip;
    std::string role;
    std::string protocol;
    int port;
    SOCKET active_socket;
    Connector();
    Connector(std::string ip, int port);
public:
    /*Closes the active socket*/
    virtual ~Connector();
    /*Checks if the Winsock DLL is loaded*/
    static bool isReady();
    /*Checks if the Connector is connected*/
    virtual bool isConnected() const = 0;
    /*Starts the Winsock DLL*/
    static void start_dlls();
    /*Changes the connection, returns WSA last error code, 0 for no error*/
    virtual int change_conn(std::string addr, int port);
    /*Displays connection information*/
    virtual void display_info() const;
    virtual int send_msg(std::string msg) = 0;
    virtual int send_msg(std::vector<char> msg) = 0;
    virtual int receive_msg(std::string& msg) = 0;
    virtual int receive_msg(std::vector<char>& msg) = 0;
    /*Returns the receive buffer size*/
    static uint32_t get_buf_size();
};


class UDP_Connector : public Connector
{
protected:
    unsigned int max_size;
    struct sockaddr_in SvrAddr;
    struct sockaddr_in CltAddr;
    UDP_Connector(std::string ip, int port) : Connector(ip, port) 
    {
        this->max_size = 0;
        this->SvrAddr.sin_family = AF_INET;
        this->SvrAddr.sin_port = htons(this->port);
        this->SvrAddr.sin_addr.s_addr = inet_addr(this->ip.c_str());

        this->CltAddr.sin_family = AF_INET;
        this->CltAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
        this->CltAddr.sin_port = 0;
    }
public:
    virtual ~UDP_Connector() {}
    bool isConnected() const
    {
        return active_socket != INVALID_SOCKET;
    }
};


class TCP_Connector : public Connector
{
protected:
    struct sockaddr_in SvrAddr;
    TCP_Connector(std::string ip, int port) : Connector(ip, port) 
    {
        this->SvrAddr.sin_family = AF_INET;
        this->SvrAddr.sin_port = htons(this->port);
        this->SvrAddr.sin_addr.s_addr = inet_addr(this->ip.c_str());
    }
public:
    virtual ~TCP_Connector() {}
    virtual bool isConnected() const
    {
        return active_socket != INVALID_SOCKET;
    }
};

#endif
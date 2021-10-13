#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <chrono>
#include "filemanager.h"
#include "clients.h"
#include "servers.h"

#ifndef NETFILESTRANSFER_H
#define NETFILETRANSFER_H

#define DEFAULT_TO 60000
void wait_for(std::chrono::duration<int, std::milli> timeout, bool& toRunning, Connector* connector);

/*Transfers a file to a remote address or receives a file from a remote address using TCP.
 If it is a source of the file, it will be a TCP client, otherwise a TCP server.*/
class NetFileTransferer
{
	const std::string conn_id = "FTv0.6";
	TCP_Connector* connector = nullptr;
	FileManager fm;
	std::string writefile;

	/*Indicates the size of each chunk sent*/
	uint32_t size;

	/*Total size of the file*/
	uint32_t total_size;
	bool isSource;
	bool timeoutRunning = false;
	int receive_chunk(uint32_t limit);
	int send_chunk();
public:
	NetFileTransferer() = delete;
	NetFileTransferer(const NetFileTransferer&) = delete;
	NetFileTransferer(bool isSrc);
	NetFileTransferer(std::string dest_addr, int dest_port, bool isSrc);
	~NetFileTransferer();

	/*Sets if the program is in source mode*/
	void set_src(bool isSrc);

	/*Sets the destination address of the server to send to*/
	void set_destination(std::string dest_addr, int dest_port);

	/*Sets the port of the server and binds the socket.*/
	void set_port(int port);
	int get_port() const;
	void set_file(std::string filepath);
	std::string get_file() const;
	bool connect();
	bool check_connection();

	/*Exchanges the file information to be transfered. 
	File will be read and placed in memory on the source side.*/
	bool info_exchange();
	void set_chunk_size(uint32_t chunk_size);

	/*Receives the file from the source. Returns the amount of bytes received. 
	If an error occured, will return 0.*/
	uint32_t receive();
	uint32_t send();
	bool save();
	void reset();
	/*bool read();
	bool read(std::string filepath);*/
};

#endif
#include <vector>
#include "netfiletransferer.h"

NetFileTransferer::NetFileTransferer(bool isSrc)
{
	this->isSource = isSrc;
	this->total_size = 0;

	if (this->isSource)
		this->connector = new TCP_Client;
	else
		this->connector = new TCP_Server;

	if (!this->isSource)
		fm.togglemode();

	this->size = this->connector->get_buf_size();
}


NetFileTransferer::NetFileTransferer(std::string dest_addr, int dest_port, bool isSrc) 
	: NetFileTransferer(isSrc)
{
	set_destination(dest_addr, dest_port);
}


NetFileTransferer::~NetFileTransferer()
{
	if (!isSource)
		dynamic_cast<TCP_Server*>(connector)->disconnect();

	delete connector;
}


void NetFileTransferer::set_src(bool isSrc)
{
	this->isSource = isSrc;

	if (!isSrc)
		fm.togglemode();
}


void NetFileTransferer::set_destination(std::string dest_addr, int dest_port)
{
	if (isSource) {
		TCP_Client* conn = dynamic_cast<TCP_Client*>(this->connector);
		int err = conn->change_conn(dest_addr, dest_port);

		if (err != 0) 
			std::cout << "Could not setup connection.\n";
	} else {
		std::cout << "No effect.\n";
	}
}


void NetFileTransferer::set_port(int port)
{
	if (!isSource) {
		TCP_Server* conn = dynamic_cast<TCP_Server*>(this->connector);
		int err = conn->change_port(port);

		if (err != 0 || !conn->bind_socket())
			std::cout << "Could not start server.\n";
	} else {
		std::cout << "No effect.\n";
	}
}


int NetFileTransferer::get_port() const
{
	return connector->get_port();
}


void NetFileTransferer::set_file(std::string filepath)
{
	fm.setfile(filepath);
}


std::string NetFileTransferer::get_file() const
{
	return fm.getfile();
}


bool NetFileTransferer::connect()
{
	if (isSource) {
		TCP_Client* conn = dynamic_cast<TCP_Client*>(this->connector);
		return conn->connect_to();
	} else {
		TCP_Server* conn = dynamic_cast<TCP_Server*>(this->connector);
		if (conn->getState() == ServerState::LISTENING || conn->await_conn()) {
			bool connCreated = conn->get_conn();
			return connCreated;
		} else {
			return false;
		}
	}
}


bool NetFileTransferer::check_connection()
{
	std::string key;
	int count;

	if (isSource) {
		TCP_Client* conn = dynamic_cast<TCP_Client*>(this->connector);
		conn->send_msg(conn_id);
		conn->receive_msg(key);
	} else {
		TCP_Server* conn = dynamic_cast<TCP_Server*>(this->connector);
		conn->settimeout(3000);
		count = conn->receive_msg(key);
		conn->send_msg(conn_id);
		conn->settimeout(0);
		if (count == 0)
			conn->disconnect();
	}


	return key == conn_id;
}


bool NetFileTransferer::info_exchange()
{
	if (isSource) {
		TCP_Client* conn = dynamic_cast<TCP_Client*>(this->connector);

		if (!fm.read())
			return false;

		std::string filename = fm.getfile();
		total_size = fm.get_data_size();
		uint32_t packet_size = sizeof(uint32_t) + filename.size() * sizeof(char) + sizeof(uint32_t);
		uint32_t name_size = filename.size() * sizeof(char);
		if (packet_size < 4)
			packet_size = 4;
		char* packet = new char[packet_size];
		char* data_ptr = packet;


		std::memcpy(data_ptr, &name_size, sizeof(uint32_t));
		data_ptr += sizeof(uint32_t);
		std::memcpy(data_ptr, filename.c_str(), filename.size());
		data_ptr += name_size;
		std::memcpy(data_ptr, &total_size, sizeof(uint32_t));
		std::vector<char> info(size);
		info.assign(packet, packet + packet_size);
		int err = conn->send_msg(info);
		delete[] packet;

		if (err == SOCKET_ERROR || err == 0) {
			std::cout << "Information unable to be transfered.\n"
					  << filename << std::endl
					  << total_size << std::endl;
		} else {
			std::cout << "Meta-data transfered.\n"
					  << filename << std::endl
				      << total_size << " bytes to be sent." << std::endl;
		}

		return true;
	} else {
		TCP_Server* conn = dynamic_cast<TCP_Server*>(this->connector);
		std::vector<char> packet;
		int packet_size = conn->receive_msg(packet);

		if (packet_size == 0 || packet_size == SOCKET_ERROR) {
			std::cout << "Information unable to be received.\n";
			return false;
		}

		const char* data_ptr = packet.data();
		uint32_t name_size = 0u;
		std::memcpy(&name_size, data_ptr, sizeof(uint32_t));
		data_ptr += sizeof(uint32_t);
		char* name = new char[name_size];
		std::memcpy(name, data_ptr, name_size*sizeof(char));
		writefile = std::string(name, name_size);
		data_ptr += name_size * sizeof(char);
		std::memcpy(&total_size, data_ptr, sizeof(uint32_t));
		delete[] name;

		
		std::cout << "Meta-data received.\n"
			<< writefile << std::endl
			<< total_size << " bytes to be received." << std::endl;
		

		return true;
	}
}


void NetFileTransferer::set_chunk_size(uint32_t chunk_size)
{
	this->size = chunk_size;
}


int NetFileTransferer::receive_chunk()
{
	if (isSource)
		return 0;

	std::vector<char> chunk;

	TCP_Server* conn = dynamic_cast<TCP_Server*>(this->connector);
	int count = conn->receive_msg(chunk);
	fm.place(chunk.data(), chunk.size());

	if (count < 0 || count == SOCKET_ERROR) 
		std::cout << "Error was " << WSAGetLastError();
	
	return count;
}


int NetFileTransferer::send_chunk()
{
	if (!isSource)
		return 0;

	std::string_view view = fm.retrieve(size);
	std::vector<char> chunk(view.data(), view.data() + view.size());
	TCP_Client* conn = dynamic_cast<TCP_Client*>(this->connector);
	int count = conn->send_msg(chunk);

	if (count < 0 || count == SOCKET_ERROR)
		std::cout << "Error was " << WSAGetLastError();
		
	return count;
}


uint32_t NetFileTransferer::send()
{
	if (!isSource || total_size == 0)
		return 0u;

	size_t divs = total_size / size + ((total_size%size == 0)? 0 : 1);
	uint32_t count = 0;
	int err;

	for (size_t i = 0u; i < divs; i++) {
		fm.set_position(size * i);
		err = send_chunk();
		/*chunk = std::string(fm.retrieve(size));
		err = conn->send_msg(chunk);*/

		if (err < 0 || err == SOCKET_ERROR)
			return 0u;
		else if (err == 0)
			break;

		count += err;
	}

	fm.set_position(0);

	return count;
}


uint32_t NetFileTransferer::receive()
{
	if (isSource || total_size == 0)
		return 0u;

	uint32_t count = 0;
	int err = 1;

	while (err > 0 && count <= total_size) {
		err = receive_chunk();

		if (err < 0 || err == SOCKET_ERROR)
			return 0u;

		count += err;
	}

	return count;
}


bool NetFileTransferer::save()
{
	if (isSource)
		return false;

	fm.setfile(writefile);

	if (!fm.write())
		writefile += ".backup";
	else
		return true;

	fm.setfile(writefile);

	return fm.write();
}


void NetFileTransferer::reset()
{
	fm.reset();
}
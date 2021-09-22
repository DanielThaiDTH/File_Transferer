#include <vector>
#include "netfiletransferer.h"

using namespace std::chrono;

void wait_for(duration<int, std::milli> timeout, bool& toRunning, Connector* connector)
{
	time_point<steady_clock> start = steady_clock::now();
	time_point<steady_clock> end;
	duration<int, std::milli> time_elapsed(0);

	while (toRunning) {
		end = steady_clock::now();
		time_elapsed = duration_cast<milliseconds>(end - start);
		if (time_elapsed > timeout) {
			dynamic_cast<TCP_Server*>(connector)->force_close();
			std::cout << "Server waited too long, closing listening socket.\n";
			toRunning = false;
		}
		std::this_thread::sleep_for(milliseconds(50));
	}
}


//NetFileTransferer
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
		return conn->connect_to(); //1
	} else {
		TCP_Server* conn = dynamic_cast<TCP_Server*>(this->connector);
		timeoutRunning = true;
		std::thread timeoutTh = std::thread(&wait_for, 
								duration<int, std::milli>(DEFAULT_TO),
								std::ref(timeoutRunning),
								connector);
		if (conn->getState() == ServerState::LISTENING || conn->await_conn()) {
			bool connCreated = conn->get_conn(); //1
			timeoutRunning = false;
			timeoutTh.join();
			return connCreated;
		} else {
			timeoutRunning = false;
			timeoutTh.join();
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
		conn->send_msg(conn_id); //2
		conn->receive_msg(key); //3
	} else {
		TCP_Server* conn = dynamic_cast<TCP_Server*>(this->connector);
		count = conn->receive_msg(key); //2
		conn->send_msg(conn_id); //3

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
		int err = conn->send_msg(info); //4
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
		int packet_size = conn->receive_msg(packet); //4

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


int NetFileTransferer::receive_chunk(uint32_t limit)
{
	if (isSource)
		return 0;

	std::vector<char> chunk;

	TCP_Server* conn = dynamic_cast<TCP_Server*>(this->connector);
	int count = conn->receive_msg(chunk, limit);
	fm.place(chunk.data(), chunk.size());

	if (count < 0 || count == SOCKET_ERROR) 
		std::wprintf(L"Socket recv failed with 0x%x\n", WSAGetLastError());
	
	return count;
}


int NetFileTransferer::send_chunk()
{
	if (!isSource)
		return 0;

	std::string_view view = fm.retrieve(size);
	std::vector<char> chunk(view.data(), view.data() + view.size());
	TCP_Client* conn = dynamic_cast<TCP_Client*>(connector);
	int count = conn->send_msg(chunk);

	if (count < 0 || count == SOCKET_ERROR)
		std::wprintf(L"Socket send failed with 0x%x\n", WSAGetLastError());
		
	return count;
}


uint32_t NetFileTransferer::send() //5...
{
	if (!isSource || total_size == 0)
		return 0u;

	uint32_t divs = total_size / size + ((total_size%size == 0)? 0 : 1);
	uint32_t count = 0;
	int err;

	for (uint32_t i = 0u; i < divs; i++) {
		fm.set_position(size * i);
		err = send_chunk();

		if (err < 0 || err == SOCKET_ERROR)
			return 0u;
		else if (err == 0)
			break;

		count += err;
	}

	//send blank msg to indicate end
	//dynamic_cast<TCP_Client*>(connector)->send_msg("");
	//std::this_thread::sleep_for(seconds(30));

	fm.set_position(0);

	return count;
}


uint32_t NetFileTransferer::receive() //5...
{
	if (isSource || total_size == 0)
		return 0u;

	uint32_t count = 0;
	int err = 1;

	while (err > 0 && count < total_size) {
		err = receive_chunk(total_size - count);

		if (err < 0 || err == SOCKET_ERROR)
			break;

		count += err;
	}

	if (err == SOCKET_ERROR)
		std::wprintf(L"Receive terminated with error 0x%x\n", WSAGetLastError());

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
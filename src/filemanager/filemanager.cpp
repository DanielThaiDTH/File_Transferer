#include "filemanager.h"

FileManager::FileManager()
{
	data = nullptr;
	data_len = 0;
	idx = 0;
	writemode = false;
}


FileManager::FileManager(std::string filepath) : FileManager()
{
	file = filepath;
}


FileManager::~FileManager()
{
	if (data != nullptr)
		delete[] data;
	
	readstream.close();
	writestream.close();
}


void FileManager::setfile(std::string filepath)
{
	readstream.close();
	writestream.close();

	file = filepath;
}


std::string FileManager::getfile() const
{
	return file;
}


void FileManager::togglemode()
{
	writemode = !writemode;
}


bool FileManager::is_writemode() const
{
	return writemode;
}


/*Reads the file into memory and stores the total size. 
If unable to open, returns false.*/
bool FileManager::read()
{
	if (writemode)
		return false;

	if (!readstream.is_open())
		readstream.open(file, std::ios::in | std::ios::binary);
	
	if (!readstream.is_open()) {
		return false;
	} else {
		readstream.seekg(0, std::ios_base::end);
		data_len = (uint32_t) readstream.tellg();
		readstream.seekg(0);

		if (data != nullptr)
			delete[] data;

		data = new char[data_len];
		readstream.read(data, data_len);
		
		return true;
	}
}


void FileManager::reset()
{
	if (data != nullptr)
		delete[] data;

	data = nullptr;
	data_len = 0;
	idx = 0;

	if (readstream.is_open())
		readstream.seekg(0);
}


uint32_t FileManager::get_data_size() const
{
	return data_len;
}


uint32_t FileManager::get_position() const
{
	return idx;
}


void FileManager::set_position(uint32_t pos)
{
	if (pos > data_len - 1)
		idx = data_len - 1;
	else
		idx = pos;
}


std::string_view FileManager::retrieve()
{
	if (data == nullptr)
		return std::string_view();

	int size = data_len - idx;
	return std::string_view(&data[idx], size);
}


std::string_view FileManager::retrieve(uint32_t size)
{
	if (data == nullptr)
		return std::string_view();

	uint32_t data_size = data_len - idx;
	data_size = (size < data_size) ? size : data_size;

	return std::string_view(&data[idx], data_size);
}


void FileManager::place(const char* bytes, uint32_t size)
{
	char* data_temp = nullptr;

	if (!writemode || size == 0)
		return;

	if (data == nullptr) {
		data = new char[size];
		data_len = size;

		for (uint32_t i = 0u; i < size; i++) {
			data[i] = bytes[i];
		}
	} else {
		data_temp = data;
		data = new char[data_len + size];

		for (uint32_t i = 0u; i < data_len; i++) {
			data[i] = data_temp[i];
		}

		delete[] data_temp;
		data_temp = nullptr;

		for (uint32_t i = 0u; i < size; i++) {
			data[i + data_len] = bytes[i];
		}

		data_len += size;
	}
}


bool FileManager::write()
{
	//Prevents writing if not in write mode or the file already exists.
	std::ifstream testfile(file);

	if (!writemode)
		return false;
	else if (testfile.is_open()) {
		std::cout << "File " << file << " already exists.\n";
		return false;
	}

	if (!writestream.is_open())
		writestream.open(file, std::ios::out | std::ios::binary);

	if (!writestream.is_open()) {
		return false;
	} else {
		writestream.write(data, data_len);
		writestream.flush();

		return true;
	}
}
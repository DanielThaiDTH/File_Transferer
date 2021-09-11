#include <string>
#include <string_view>
#include <iostream>
#include <fstream>

/*Manages reading and writing from files. Starts in read mode. Can only write/store data for 
writing in write mode.*/
class FileManager
{
	std::string file;
	std::ifstream readstream;
	std::ofstream writestream;
	char* data;
	uint32_t data_len;
	uint32_t idx;
	bool writemode;
	//std::string_view view;
public:
	FileManager();
	FileManager(FileManager& src) = delete;
	FileManager(std::string filepath);
	~FileManager();
	/*Changes the file for read/write. Will close any open streams and any remaining data.*/
	void setfile(std::string filepath);
	std::string getfile() const;
	void togglemode();
	bool is_writemode() const;
	bool read();
	void reset();
	uint32_t get_data_size() const;
	uint32_t get_position() const;
	void set_position(uint32_t pos);
	/*Returns string view from saved position to end.*/
	std::string_view retrieve();
	/*Returns string view of given size from saved position.*/
	std::string_view retrieve(uint32_t size);
	void place(const char* bytes, uint32_t size);
	bool write();
	FileManager& operator=(const FileManager&) = delete;
};
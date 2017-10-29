#pragma once

#include <string>
#include <map>
#include "Vector.h"
#include <stdio.h>
#include <memory>
#include <mutex>

class FileP {
public:
	//Read Constructor
	FileP(Vector<uint8_t> &_data) : data(_data), mode(READ) {};

	~FileP();

	void read(void* buf, size_t size, size_t count);
	void seekSet(long offset) { this->offset = offset; }
	void seekCur(long offset) { this->offset += offset; }

	enum Mode { READ, WRITE };
private:
	Mode mode;

	//Write Mode
	FILE *fp = nullptr;

	//Read Mode
	Vector<uint8_t> data;
	long offset = 0;
};

class DatFat {
public:
	void addFat(const std::string &filename);
	std::shared_ptr<FileP> openRead(std::string filename);
	std::shared_ptr<FileP> openWrite(std::string filename);
	std::string modDir;

	struct FileEntry {
		int archive;
		uint64_t offset;
		uint32_t realSize;
		uint32_t size;
		enum Compression {
			None = 0,
			LZO1x = 1,
			Zlib = 2,
			Xbox = 3,
		};
		Compression compression;
	};
	Vector<FILE*> archives;
	std::map<uint32_t, FileEntry> files;
	std::mutex mutex;
};


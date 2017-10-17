#include "DatFat.h"

#include <assert.h>
#include <algorithm>
#include <SDL_endian.h>
#include <Windows.h>

uint64_t fnv_64_str(const char *str) {
	unsigned char *s = (unsigned char *) str;
	uint64_t hval = 0xCBF29CE484222325ul;

	while (*s) {
		hval *= 0x100000001B3ul;
		hval ^= (uint64_t) *s++;
	}

	return hval;
}

void DatFat::addFat(const std::string &filename) {
	FILE *fp = fopen(filename.c_str(), "rb");
	assert(fp);

	uint32_t magic;
	fread(&magic, sizeof(magic), 1, fp);
	assert(magic == 1178686515);

	int32_t version;
	fread(&version, sizeof(version), 1, fp);
	assert(version == 8);

	uint32_t flags;
	fread(&flags, sizeof(flags), 1, fp);
	assert((flags & ~0xFFFFFF) == 0);

	uint32_t entries;
	fread(&entries, sizeof(entries), 1, fp);

	std::string datFile = filename;
	datFile[datFile.size() - 3] = 'd';
	FILE *dat = fopen(datFile.c_str(), "rb");
	assert(dat);
	archives.push_back(dat);
	int pos = archives.size() - 1;

	FILE *out = fopen("out.txt", "w");

	for (uint32_t i = 0; i < entries; ++i) {
		uint32_t a, b, c, d;
		fread(&a, sizeof(uint32_t), 1, fp);
		fread(&b, sizeof(uint32_t), 1, fp);
		fread(&c, sizeof(uint32_t), 1, fp);
		fread(&d, sizeof(uint32_t), 1, fp);

		fprintf(out, "%u\n", a);

		FileEntry fe;
		fe.archive = pos;
		fe.realSize = (b >> 3) & 0x1FFFFFFFu;
		fe.compression = (FileEntry::Compression) ((b >> 0) & 0x00000007u);
		fe.offset = (long) d << 3;
		fe.offset |= (c >> 29) & 0x00000007u;
		fe.size = (c >> 0) & 0x1FFFFFFFu;
		files[a] = fe;
	}

	fclose(out);

	//TODO: Parse Localization

	fclose(fp);
}

struct CompressionSettings {
	uint32_t Flags;
	uint32_t WindowSize;
	uint32_t ChunkSize;
};

std::shared_ptr<FileP> DatFat::openRead(std::string filename) {
	std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
	std::replace(filename.begin(), filename.end(), '/', '\\');
	if (filename[0] == '\\')
		filename = filename.substr(1);
	uint64_t hash = fnv_64_str(filename.c_str());

	if (files.count(hash)) {
		auto &fe = files.at(hash);
		FILE *ar = archives[fe.archive];

		fseek(ar, fe.offset, SEEK_SET);

		if (fe.compression == FileEntry::Compression::None) {
			std::vector<uint8_t> data(fe.size);
			fread(data.data(), 1, fe.size, ar);
			return std::make_shared<FileP>(data);
		} else if (fe.compression == FileEntry::Compression::Xbox) {
			uint32_t magic;
			fread(&magic, sizeof(magic), 1, ar);
			magic = SDL_Swap32(magic);
			assert(magic == 0x0FF512EE);

			uint32_t version;
			fread(&version, sizeof(version), 1, ar);
			version = SDL_Swap32(version);
			assert(version == 0x01030000);

			uint32_t unknown08;
			fread(&unknown08, sizeof(unknown08), 1, ar);
			unknown08 = SDL_Swap32(unknown08);
			assert(unknown08 == 0);

			uint32_t unknown0C;
			fread(&unknown0C, sizeof(unknown0C), 1, ar);
			unknown0C = SDL_Swap32(unknown0C);
			assert(unknown0C == 0);

			uint32_t windowSize;
			fread(&windowSize, sizeof(windowSize), 1, ar);
			windowSize = SDL_Swap32(windowSize);

			uint32_t chunkSize;
			fread(&chunkSize, sizeof(chunkSize), 1, ar);
			chunkSize = SDL_Swap32(chunkSize);

			int64_t uncompressedSize;
			fread(&uncompressedSize, sizeof(uncompressedSize), 1, ar);
			uncompressedSize = SDL_Swap64(uncompressedSize);

			int64_t compressedSize;
			fread(&compressedSize, sizeof(compressedSize), 1, ar);
			compressedSize = SDL_Swap64(compressedSize);

			int32_t largestUncompressedChunkSize;
			fread(&largestUncompressedChunkSize, sizeof(largestUncompressedChunkSize), 1, ar);
			largestUncompressedChunkSize = SDL_Swap32(largestUncompressedChunkSize);

			int32_t largestCompressedChunkSize;
			fread(&largestCompressedChunkSize, sizeof(largestCompressedChunkSize), 1, ar);
			largestCompressedChunkSize = SDL_Swap32(largestCompressedChunkSize);

			if (uncompressedSize < 0 ||
				compressedSize < 0 ||
				largestUncompressedChunkSize < 0 ||
				largestCompressedChunkSize < 0) {
				assert(false);
			}

			assert(uncompressedSize == fe.realSize);

			std::vector<uint8_t> uncompressedBytes(largestUncompressedChunkSize);
			std::vector<uint8_t> compressedBytes(largestCompressedChunkSize);

			int64_t remaining = uncompressedSize;
			while (remaining > 0) {
				HMODULE lib = LoadLibraryA("C:\\Windows\\System32\\XnaNative.dll");
				DWORD a = GetLastError();
				//assert(lib);
				
				/*0x10197A1D,
					0x101979A1,
					0x101979F7*/

				typedef int (*CDC)(int type, CompressionSettings settings, int flags, void* context);

				CompressionSettings settings;
				settings.Flags = 0;
				settings.WindowSize = windowSize;
				settings.ChunkSize = chunkSize;

				void* handle;

				CDC CreateDecompressionContext = (CDC)(lib + 0x10197A1D);
				int ret = CreateDecompressionContext(1, settings, 1, handle);

				int32_t compressedChunkSize;
				fread(&compressedChunkSize, sizeof(compressedChunkSize), 1, ar);
				compressedChunkSize = SDL_Swap32(compressedChunkSize);
				if (compressedChunkSize < 0 ||
					compressedChunkSize > largestCompressedChunkSize) {
					assert(false);
				}

				fread(compressedBytes.data(), 1, compressedChunkSize, ar);

				int uncompressedChunkSize = (int) min(largestUncompressedChunkSize, remaining);
				int actualUncompressedChunkSize = uncompressedChunkSize;
				int32_t actualCompressedChunkSize = compressedChunkSize;

				typedef int(*XmemD)(void* context, void* output, int &outputSize, void* input, int &inputSize);
				XmemD Decompress = (XmemD)(lib + 0x101979A1);
				ret = Decompress(handle, uncompressedBytes.data(), actualUncompressedChunkSize, compressedBytes.data(), actualCompressedChunkSize);
				if (ret != 0) {
					assert(false);
				}

				if (actualUncompressedChunkSize != uncompressedChunkSize) {
					assert(false);
				}

				//output.Write(uncompressedBytes, 0, actualUncompressedChunkSize);

				remaining -= actualUncompressedChunkSize;
			}
		} else {
			assert(false);
		}

	}

	return std::shared_ptr<FileP>();
}

FileP::~FileP() {
	if (fp)
		fclose(fp);
}

void FileP::read(void *buf, size_t size, size_t count) {
	assert(mode == READ);
	assert(data.size() > offset + (size * count));

	memcpy(buf, data.data() + offset, size * count);
}

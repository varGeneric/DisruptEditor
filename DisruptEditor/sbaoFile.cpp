#include "sbaoFile.h"

#include <algorithm>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <SDL.h>
#include "Vector.h"

#include "Hash.h"
#include "Audio.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

const uint32_t sbaoMagic = 207362;

#pragma pack(push, 1)
struct sbaoHeader {
	uint32_t magic = sbaoMagic;
	uint32_t unk1 = 0;
	uint32_t unk2 = 0;
	uint32_t unk3 = 0;
	uint32_t unk4 = 0;
	uint32_t unk5 = 1342177280;
	uint32_t unk6 = 2;
};
struct oggPageHeader {
	uint32_t magic = 0;
	uint8_t version = 0;
	uint8_t headerType = 0;
	uint64_t granulePos = 0;
	uint32_t serialNo = 0;
	uint32_t pageSeqNum = 0;
	uint32_t checksum = 0;
	uint8_t numSegments = 0;
};
#pragma pack(pop)

size_t getOggPageSize(uint8_t *ptr) {
	oggPageHeader *header = (oggPageHeader*)ptr;
	ptr += sizeof(*header);

	SDL_assert_release(header->magic == 1399285583);

	size_t packetSize = 0;
	for (uint8_t i = 0; i < header->numSegments; ++i) {
		packetSize += *ptr;
		++ptr;
	}

	return sizeof(*header) + header->numSegments + packetSize;
}

void writeZero(SDL_RWops *fp, size_t num) {
	for (size_t i = 0; i < num; ++i)
		SDL_WriteU8(fp, 0);
}

void sbaoFile::open(const char *filename) {
	if (!filename) return;
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	SDL_Log("%s", filename);
	open(fp);
	SDL_RWclose(fp);
}

void sbaoFile::open(SDL_RWops *fp) {
	layers.clear();
	
	sbaoHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == sbaoMagic);
	//SDL_assert_release(head.unk5 == 1342177280 || head.unk5 == 805306368);
	SDL_assert_release(head.unk6 == 2);

	//Read First 4 bytes
	uint32_t type = SDL_ReadLE32(fp);
	if (type == 1399285583) {//Oggs
		SDL_RWseek(fp, -4, RW_SEEK_CUR);
		sbaoLayer &layer = layers.push_back();
		layer.type = sbaoLayer::VORBIS;
		layer.data.resize(SDL_RWsize(fp) - SDL_RWtell(fp));
		SDL_RWread(fp, layer.data.data(), 1, layer.data.size());
	} else if (type == 1048585) {//Interweaved 9 stream
		SDL_assert(SDL_ReadLE32(fp) == 0);
		uint32_t numLayers = SDL_ReadLE32(fp);
		uint32_t totalBlocks = SDL_ReadLE32(fp);
		uint32_t totalInfoSize = SDL_ReadLE32(fp);

		SDL_assert_release(totalInfoSize == 8 + (numLayers * 4));

		SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "Info at: %u\n", SDL_RWtell(fp));
		Vector<uint32_t> infoTable(totalInfoSize / sizeof(uint32_t));
		SDL_RWread(fp, infoTable.data(), totalInfoSize, 1);
		SDL_RWseek(fp, 64 - numLayers * 4, RW_SEEK_CUR);

		// Process the second header
		Vector<uint32_t> headerSizes(numLayers);
		SDL_RWread(fp, headerSizes.data(), sizeof(uint32_t), numLayers);

		//Debug
		SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "numLayers: %u\n", numLayers);
		SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "totalBlocks: %u\n", totalBlocks);
		SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "totalInfoSize: %u\n", totalInfoSize);
		SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "headerSizes: ");
		for(uint32_t value : headerSizes)
			SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "%u, ", value);

		layers.reserve(numLayers);
		for (unsigned long i = 0; i< numLayers; i++) {
			sbaoLayer& layer = layers.push_back();

			// Read the header and send it
			layer.data.resize(headerSizes[i]);
			SDL_RWread(fp, layer.data.data(), 1, layer.data.size());

			// Detect the type
			if (headerSizes[i] == 0) {
				// Must be PCM, there is no header
				layer.type = sbaoLayer::PCM;
			} else if ((layer.data[0] == 5 || layer.data[0] == 3 || layer.data[0] == 6) && headerSizes[i] >= 28) {
				// Check the header size
				if (headerSizes[i] != 28 && headerSizes[i] != 36) {
					SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "Warning: Header size is unrecognized (ADPCM, %u bytes, should be 28 or 36)", headerSizes[i]);
				}

				// It is likely a simple block
				/*CVersion5Stream* Stream = new CVersion5Stream(Layer.Data);
				switch (Buffer[0]) {
					case 3:
						Layer.Type = EUF_UBI_V3;
						break;
					case 5:
						Layer.Type = EUF_UBI_V5;
						break;
					case 6:
						Layer.Type = EUF_UBI_V6;
						break;
					default:
						Layer.Type = EUF_UBI_V5;
						break;
				}
				Layer.Stream = Stream;

				// Initialize the header
				try {
					// Initialize the header
					if (!Stream->InitializeHeader(SampleRate)) {
						Clear();
						return false;
					}

					// Get some information
					m_SampleRate = Stream->GetSampleRate();
					m_Channels = Stream->GetChannels();
				} catch (XNeedBuffer&) {
					Clear();
					throw(XFileException("The decoder needed more information than the header provided"));
				}*/
			} else if (memcmp(layer.data.data(), "OggS", 4) == 0) {
				layer.type = sbaoLayer::VORBIS;
			} else {
				return;
			}
		}

		{
			std::string str;
			for (uint32_t value : infoTable)
				str += std::to_string(value) + " ";

			SDL_Log("%u - %u - %u - %u\n%s", SDL_RWtell(fp), numLayers, totalBlocks, totalInfoSize, str.c_str());

			/*for (uint32_t value : headerSizes)
			SDL_Log("HS %u", value);*/
		}

		SDL_assert_release(SDL_RWtell(fp) - 120 == infoTable[0]);

		SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "Block data at %u", SDL_RWtell(fp));

		//Parse Blocks
		uint32_t count = 0;
		for (uint32_t blockI = 0; blockI < totalBlocks; ++blockI) {
			uint32_t BlockID = SDL_ReadLE32(fp);
			SDL_assert_release(BlockID == 3);
			uint32_t unk = SDL_ReadLE32(fp);
			SDL_assert_release(blockI == totalBlocks - 2 || unk == infoTable[1] || (unk == 0 && blockI == totalBlocks - 1));

			// Read in the block sizes
			Vector<uint32_t> BlockSizes;
			for (unsigned long i = 0; i < numLayers; i++) {
				uint32_t BlockSize = SDL_ReadLE32(fp);
				BlockSizes.push_back(BlockSize);
			}

			//SDL_Log("%u - %u, %u", unk, BlockSizes[0], BlockSizes[1]);
			count += unk;

			for (unsigned long i = 0; i < numLayers; i++) {
				SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "b = %u", SDL_RWtell(fp));

				// Feed it a block
				Vector<uint8_t> block(BlockSizes[i]);
				SDL_RWread(fp, block.data(), 1, block.size());
				layers[i].data.appendBinary(block.begin(), block.end());
			}
		}
		SDL_Log("unkc %u", count);

		for (unsigned long i = 0; i < numLayers; i++) {
			SDL_assert_release(infoTable[i + 2] == layers[i].data.size() - headerSizes[i]);
		}
		
		SDL_assert_release(SDL_RWtell(fp) == SDL_RWsize(fp));
	} else {
		//Find OggS Packet
		char buffer[4] = { 0 };
		while (memcmp(buffer, "OggS", 4) != 0) {
			int a = SDL_RWread(fp, buffer, 4, 1);
			if (a != 4)
				return;
		}

		SDL_RWseek(fp, -4, RW_SEEK_CUR);
		sbaoLayer &layer = layers.push_back();
		layer.type = sbaoLayer::VORBIS;
		layer.data.resize(SDL_RWsize(fp) - SDL_RWtell(fp));
		SDL_RWread(fp, layer.data.data(), 1, layer.data.size());
	}

	fillCache();
}

void sbaoFile::save(const char * filename) {
	if (!filename) return;

	Vector<uint8_t> data = save();
	SDL_RWops *fp = SDL_RWFromFile(filename, "wb");
	if (!fp) return;
	SDL_RWwrite(fp, data.data(), 1, data.size());
	SDL_RWclose(fp);
}

Vector<uint8_t> sbaoFile::save() {
	Vector<uint8_t> data(1024 * 1024 * 64);
	SDL_RWops *fp = SDL_RWFromMem(data.data(), data.size());
	size_t size = 0;

	if (layers.empty()) {
		SDL_ShowSimpleMessageBox(0, "Dare Error", "You don't have any layers, please add some", NULL);
		SDL_RWclose(fp);
		return Vector<uint8_t>();
	}

	if (layers.size() > 1) {
		//Check Streams
		int totalSamples = layers[0].samples;
		int sampleRate = layers[0].sampleRate;
		int channels = layers[0].channels;
		for (sbaoLayer &layer : layers) {
			if (layer.samples != totalSamples || layer.sampleRate != sampleRate || layer.channels != channels) {
				SDL_ShowSimpleMessageBox(0, "Dare Error", "Your layers do not have equivalent attributes", NULL);
				SDL_RWclose(fp);
				return Vector<uint8_t>();
			}
		}

		Vector< Vector<uint8_t> > headers(layers.size());
		Vector< uint8_t* > ptrs(layers.size());

		//Get first 4 packets of ogg as the header
		for (int i = 0; i < layers.size(); ++i) {
			ptrs[i] = layers[i].data.data() + 4608;
			headers[i].appendBinary(layers[i].data.data(), layers[i].data.data() + 4608);
		}

		uint32_t maxOgglength = 0;
		for (sbaoLayer &layer : layers)
			maxOgglength = std::max(maxOgglength, (uint32_t)layer.data.size());

		uint32_t totalBlocks = (maxOgglength / 162) + 1;

		Vector<uint32_t> infoTable;
		infoTable.push_back(0);//Temporary
		infoTable.push_back(340);//Todo
		for (int i = 0; i < layers.size(); ++i)
			infoTable.push_back(layers[i].data.end() - ptrs[i]);

		sbaoHeader head;
		SDL_RWwrite(fp, &head, sizeof(head), 1);
		SDL_WriteLE32(fp, 1048585);//type = interweaved 9 stream
		SDL_WriteLE32(fp, 0);
		SDL_WriteLE32(fp, layers.size());
		SDL_WriteLE32(fp, totalBlocks);//totalBlocks
		SDL_WriteLE32(fp, infoTable.size() * sizeof(uint32_t));//totalInfoSize
		size_t infoOffset = SDL_RWtell(fp);
		SDL_RWwrite(fp, infoTable.data(), sizeof(uint32_t), infoTable.size());
		writeZero(fp, 64 - layers.size() * 4);

		//Write Header sizes
		for (int i = 0; i < layers.size(); ++i)
			SDL_WriteLE32(fp, headers[i].size());

		//Write Headers
		for (int i = 0; i < layers.size(); ++i)
			SDL_RWwrite(fp, headers[i].data(), 1, headers[i].size());

		infoTable[0] = SDL_RWtell(fp) - 120;

		//Write Blocks
		for (uint32_t blockI = 0; blockI < totalBlocks; ++blockI) {
			SDL_WriteLE32(fp, 3);//BlockId
			SDL_WriteLE32(fp, blockI == totalBlocks - 1 ? 0 : 340);//unk

																   // Read in the block sizes
			for (unsigned long i = 0; i < layers.size(); i++) {
				uint32_t left = layers[i].data.end() - ptrs[i];
				uint32_t out = std::min(left, (uint32_t)162);
				SDL_LogVerbose(SDL_LOG_CATEGORY_AUDIO, "%u", out);

				SDL_WriteLE32(fp, out);
			}

			for (unsigned long i = 0; i < layers.size(); i++) {
				uint32_t left = layers[i].data.end() - ptrs[i];
				uint32_t out = std::min(left, (uint32_t)162);

				SDL_RWwrite(fp, ptrs[i], 1, out);
				ptrs[i] += out;
			}
		}

		size = SDL_RWtell(fp);

		//Rewrite info table
		SDL_RWseek(fp, infoOffset, RW_SEEK_SET);
		SDL_RWwrite(fp, infoTable.data(), sizeof(uint32_t), infoTable.size());
	} else {
		sbaoHeader head;
		SDL_RWwrite(fp, &head, sizeof(head), 1);
		SDL_RWwrite(fp, layers[0].data.data(), 1, layers[0].data.size());
		size = SDL_RWtell(fp);
	}
	SDL_RWclose(fp);

	data.resize(size);
	return data;
}

void sbaoFile::fillCache() {
	for (sbaoLayer &layer : layers)
		layer.fillCache();
}

void sbaoLayer::fillCache() {
	if (type != VORBIS) return;
	int error;
	stb_vorbis *v = stb_vorbis_open_memory(data.data(), data.size(), &error, NULL);
	SDL_assert_release(v);

	stb_vorbis_info info = stb_vorbis_get_info(v);
	channels = info.channels;
	sampleRate = info.sample_rate;
	samples = stb_vorbis_stream_length_in_samples(v);

	stb_vorbis_close(v);
}

void sbaoLayer::replace(const char * filename) {
	if (!filename) return;

	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	if (!fp) return;

	data.resize(SDL_RWsize(fp));
	SDL_RWread(fp, data.data(), 1, data.size());
	SDL_RWclose(fp);

	fillCache();
}

void sbaoLayer::save(const char * filename) {
	if (!filename) return;

	SDL_RWops *fp = SDL_RWFromFile(filename, "wb");
	if (!fp) return;

	SDL_RWwrite(fp, data.data(), 1, data.size());
	SDL_RWclose(fp);
}

int sbaoLayer::play(bool loop) {
	int channels, sampleRate;
	short *output;
	int ret = stb_vorbis_decode_memory(data.data(), data.size(), &channels, &sampleRate, &output);
	SDL_assert_release(ret > 0);
	int ref = Audio::instance().addSound(sampleRate, channels, output, ret * channels * sizeof(short), loop);
	free(output);
	return ref;
}

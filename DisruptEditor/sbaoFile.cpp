#include "sbaoFile.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <SDL_assert.h>
#include <SDL_log.h>
#include "Vector.h"

#include "Hash.h"
#include "Audio.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#pragma pack(push, 1)
struct sbaoHeader {
	uint32_t magic;
	uint32_t unk1;
	uint32_t unk2;
	uint32_t unk3;
	uint32_t unk4;
	uint32_t unk5;
	uint32_t unk6;
};
#pragma pack(pop)


const uint32_t sbaoMagic = 207362;

void sbaoFile::open(const char *filename) {
	SDL_RWops *fp = SDL_RWFromFile(filename, "rb");
	open(fp);
	SDL_RWclose(fp);
}

void sbaoFile::open(SDL_RWops *fp) {
	layers.clear();
	
	//SDL_RWops *out = SDL_RWFromFile("test.ogg", "wb");
	sbaoHeader head;
	SDL_RWread(fp, &head, sizeof(head), 1);
	SDL_assert_release(head.magic == sbaoMagic);
	SDL_assert_release(head.unk5 == 1342177280);
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
		SDL_RWseek(fp, totalInfoSize, RW_SEEK_CUR);
		SDL_RWseek(fp, 64 - numLayers * 4, RW_SEEK_CUR);

		// Process the second header
		Vector<uint32_t> headerSizes(numLayers);
		SDL_RWread(fp, headerSizes.data(), sizeof(uint32_t), numLayers);

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
					SDL_Log("Warning: Header size is unrecognized (ADPCM, %u bytes, should be 28 or 36)", headerSizes[i]);
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
			}
		}

		//Parse Blocks
		for (uint32_t blockI = 0; blockI < totalBlocks; ++blockI) {
			for (unsigned long i = 0; i < numLayers; i++) {
				uint32_t BlockID = SDL_ReadLE32(fp);
				if (BlockID != 3) {
					// Mark the end of the stream for all of the layers
					return;
				}
				uint32_t unk = SDL_ReadLE32(fp);

				// Read in the block sizes
				Vector<uint32_t> BlockSizes;
				for (unsigned long i = 0; i < numLayers; i++) {
					uint32_t BlockSize = SDL_ReadLE32(fp);
					BlockSizes.push_back(BlockSize);
				}

				for (unsigned long i = 0; i < numLayers; i++) {
					// Get a reference to the layer
					sbaoLayer& layer = layers[i];

					// Feed it a block
					Vector<uint8_t> block(BlockSizes[i]);
					SDL_RWread(fp, block.data(), 1, block.size());
					layer.data.appendBinary(block.begin(), block.end());
				}
			}
		}


	} else if (type == 0 || type == 4294049865 || type == 1677572653 || type == 1511924971 || type == 2232265428 || type == 2464119532) {//Unknown
		return;
	} else {
		return;
	}
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

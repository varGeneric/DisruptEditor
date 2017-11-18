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
#include <vorbis/vorbisfile.h>
#include <ogg/ogg.h>

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

struct oggPageHeader {
	uint32_t magic = 0;
	uint8_t version = 0;
	uint8_t headerType = 0;
	enum HeaderFlags { PAGE_BEGIN = 2, PAGE_CONTINUE = 1, PAGE_END = 4 };
	//DARE Specific Flags 2 = PAGE_BEGIN, 0 = PAGE_CONTINUE, 
	uint64_t granulePos = 0;
	uint32_t serialNo = 0;
	uint32_t pageSeqNum = 0;
	uint32_t checksum = 0;
	uint8_t numSegments = 0;
};

struct vorbisCommonHeader {
	uint8_t type;
	enum Type { IDENT = 1, COMMENT = 3, SETUP = 5, AUDIO = 0 };
	char vorbis[6];
};

struct vorbisIdentHeader {
	uint32_t version;
	uint8_t channels;
	uint32_t sampleRate;
	uint32_t bitrateMax;
	uint32_t bitrateNom;
	uint32_t bitrateMin;
	uint8_t blocksize;
	uint8_t framingFlag;
};

#pragma pack(pop)

struct oggPacket {
	Vector<uint8_t> data;
};

struct oggPage {
	oggPageHeader header;
	Vector< oggPacket > packets;

	size_t pos;

	bool decode(SDL_RWops *fp, bool SDL_assertDare = true);
	void encode(SDL_RWops *fp);
	void print();
};


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
		//layer.play(false);
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
			if (headerSizes[i] > 0) {
				int a = 1;
				//Layer.Data->SendBuffer(Buffer, HeaderSizes[i]);
			}

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
	} else if (type == 0 || type == 4294049865 || type == 1677572653 || type == 1511924971 || type == 2232265428 || type == 2464119532) {//Unknown
		return;
	} else {
		return;
	}

	return;

	SDL_RWseek(fp, 128, RW_SEEK_SET); //DEBUG

	Vector<oggPage> pages;
	bool success = true;
	SDL_Log("pos\tversion\theaderType\tgranulePos\tserialNo\tpageSeqNum\tnumSegments\n");
	while (success) {
		uint32_t magic = SDL_ReadLE32(fp);
		SDL_RWseek(fp, -sizeof(magic), RW_SEEK_CUR);

		size_t posBegin = SDL_RWtell(fp);

		if (magic != 1399285583) {
			SDL_RWseek(fp, 1, RW_SEEK_CUR);

			if (SDL_RWtell(fp) == SDL_RWsize(fp))
				break;

			continue;
		}

		oggPage page;
		success = page.decode(fp);
		//page.encode(fp);
		if (success) {
			page.print();
			pages.push_back(page);

			size_t posEnd = SDL_RWtell(fp);
			SDL_RWseek(fp, posBegin, RW_SEEK_SET);
			Vector<uint8_t> data(posEnd - posBegin);
			SDL_RWread(fp, data.data(), 1, data.size());
			//SDL_RWwrite(out, data.data(), 1, data.size());
			SDL_RWseek(fp, posEnd, RW_SEEK_SET);
		} else {
			SDL_RWseek(fp, 1, RW_SEEK_CUR);
		}

		success = SDL_RWtell(fp) != SDL_RWsize(fp);
	}

	//Decode First Page (Vorbis Identification Header)
	/*{
		oggPacket &packet = *pages.begin()->packets.begin();
		uint8_t *ptr = packet.data.data();

		vorbisCommonHeader *vch = (vorbisCommonHeader*) ptr;
		ptr += sizeof(*vch);
		SDL_assert_release(vch->type == vch->IDENT);

		vorbisIdentHeader *ident = (vorbisIdentHeader*)ptr;
		ptr += sizeof(*ident);
	}

	//Decode Second Page (Vorbis Comment Header)
	{
		oggPacket &packet = *(pages.begin()+1)->packets.begin();
		uint8_t *ptr = packet.data.data();

		vorbisCommonHeader *vch = (vorbisCommonHeader*)ptr;
		ptr += sizeof(*vch);
		SDL_assert_release(vch->type == vch->COMMENT);

		uint32_t length = *(uint32_t*)ptr;
		ptr += sizeof(length);

		std::string vendor(length + 1, '\0');
		memcpy(&vendor[0], ptr, length);
		ptr += length;

		//TODO: Could decode user comments?

		//Decode?
		packet = *((pages.begin()+1)->packets.begin()+1);
		ptr = packet.data.data();

		vch = (vorbisCommonHeader*)ptr;
		ptr += sizeof(*vch);
	}*/

	size_t pos = SDL_RWtell(fp);

	//SDL_RWclose(out);
}

bool oggPage::decode(SDL_RWops *fp, bool SDL_assertDare) {
	pos = SDL_RWtell(fp);
	size_t read = SDL_RWread(fp, &header, 1, sizeof(header));
	if (header.magic != 1399285583 || read != sizeof(header) || header.version != 0 || header.numSegments == 0) {
		SDL_RWseek(fp, -read, RW_SEEK_CUR);
		return false;
	}

	/*if (SDL_assertDare) {
		SDL_assert_release(header.version == 0);
		SDL_assert_release(header.serialNo == 0);
	}*/

	Vector<uint8_t> segmentSizes(header.numSegments);
	SDL_RWread(fp, segmentSizes.data(), sizeof(uint8_t), header.numSegments);

	size_t packetSize = 0;
	for (uint8_t temp : segmentSizes) {
		packetSize += temp;

		if (temp == 0 || temp < 255) {
			oggPacket packet;
			packet.data.resize(packetSize);
			packets.push_back(packet);
			packetSize = 0;
		}
	}

	for (oggPacket &packet : packets) {
		SDL_RWread(fp, packet.data.data(), 1, packet.data.size());
	}

	return true;
}

void oggPage::encode(SDL_RWops *fp) {
	header.checksum = 0;
	header.magic = 1399285583;
	header.version = 0;
	header.serialNo = 0;
	//size_t totalSegmentSize = data.size();
	//segmentSizes.clear();

	Vector<uint8_t> temp(sizeof(header) /*+ segmentSizes.size() + data.size()*/);
	uint8_t *ptr = temp.data();

	memcpy(ptr, &header, sizeof(header));
	ptr += sizeof(header);

	/*memcpy(ptr, segmentSizes.data(), segmentSizes.size());
	ptr += segmentSizes.size();

	memcpy(ptr, data.data(), data.size());
	ptr += data.size();*/

	header.checksum = Hash::crcHash(temp.data(), temp.size());
}

void oggPage::print() {
	SDL_Log("%u\t%u\t%u\t%u\t%u\t%u\t%u\n", pos, header.version, header.headerType, header.granulePos, header.serialNo, header.pageSeqNum, header.numSegments);
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

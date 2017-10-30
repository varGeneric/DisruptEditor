#include "sbaoFile.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <SDL_assert.h>
#include <SDL_log.h>
#include "Vector.h"

#include "Hash.h"
#include <vorbis/vorbisfile.h>
#include <ogg/ogg.h>

#pragma pack(push, 1)
struct spkHeader {
	uint32_t magic;
	uint8_t unk[24];
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

	bool decode(FILE *fp, bool SDL_assertDare = true);
	void encode(FILE *fp);
	void print();
};


const uint32_t sbaoMagic = 207362;

void sbaoFile::open(const char * filename) {
	FILE *fp = fopen(filename, "rb");
	FILE *out = fopen("test.ogg", "wb");
	spkHeader head;
	fread(&head, sizeof(head), 1, fp);
	SDL_assert_release(head.magic == sbaoMagic);

	fseek(fp, 128, SEEK_SET); //DEBUG

	Vector<oggPage> pages;
	bool success = true;
	SDL_Log("pos\tversion\theaderType\tgranulePos\tserialNo\tpageSeqNum\tnumSegments\n");
	while (success) {
		uint32_t magic;
		fread(&magic, 1, sizeof(magic), fp);
		fseek(fp, -sizeof(magic), SEEK_CUR);

		size_t posBegin = ftell(fp);

		if (magic != 1399285583) {
			fseek(fp, 1, SEEK_CUR);

			if (feof(fp))
				break;

			continue;
		}

		oggPage page;
		success = page.decode(fp);
		//page.encode(fp);
		if (success) {
			page.print();
			pages.push_back(page);

			size_t posEnd = ftell(fp);
			fseek(fp, posBegin, SEEK_SET);
			Vector<uint8_t> data(posEnd - posBegin);
			fread(data.data(), 1, data.size(), fp);
			fwrite(data.data(), 1, data.size(), out);
			fseek(fp, posEnd, SEEK_SET);
		} else {
			fseek(fp, 1, SEEK_CUR);
		}

		success = !feof(fp);
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

	size_t pos = ftell(fp);

	fclose(fp);
	fclose(out);
}

bool oggPage::decode(FILE *fp, bool SDL_assertDare) {
	pos = ftell(fp);
	size_t read = fread(&header, 1, sizeof(header), fp);
	if (header.magic != 1399285583 || read != sizeof(header) || header.version != 0 || header.numSegments == 0) {
		fseek(fp, -read, SEEK_CUR);
		return false;
	}

	/*if (SDL_assertDare) {
		SDL_assert_release(header.version == 0);
		SDL_assert_release(header.serialNo == 0);
	}*/

	Vector<uint8_t> segmentSizes(header.numSegments);
	fread(segmentSizes.data(), sizeof(uint8_t), header.numSegments, fp);

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
		fread(packet.data.data(), 1, packet.data.size(), fp);
	}

	return true;
}

void oggPage::encode(FILE *fp) {
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

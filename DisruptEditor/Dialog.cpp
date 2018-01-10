#include "Dialog.h"

#include "Common.h"
#include <SDL_log.h>
#include <SDL_rwops.h>
#include "tinyfiles.h"

Dialog::Dialog() {
	conversationtable = readFCB(getAbsoluteFilePath("dialog/conversationtable.dat").c_str());
	dialogmanagerindices = readFCB(getAbsoluteFilePath("dialog/dialogmanagerindices.dat").c_str());
	speechLength = readFCB(getAbsoluteFilePath("generated/sound/speechlength.bin").c_str());

	tfDIR dir;
	tfDirOpen(&dir, getAbsoluteFilePath("dialog/behaviortrees").c_str());
	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir && tfGetExt(&file) == std::string("ai.rml")) {
			SDL_Log("Loading %s...", file.name);
			SDL_RWops *fp = SDL_RWFromFile(file.path, "rb");
			SDL_RWseek(fp, 12, RW_SEEK_SET);
			behaviortrees[file.name] = readFCB(fp);
			SDL_RWclose(fp);
		}

		tfDirNext(&dir);
	}
	tfDirClose(&dir);

	SDL_RWops *fp = SDL_RWFromFile(getAbsoluteFilePath("soundbinary/soundidlinelinks.slid").c_str(), "rb");
	uint32_t count = SDL_ReadLE32(fp);
	for (uint32_t i = 0; i < count; ++i) {
		uint32_t soundID = SDL_ReadLE32(fp);
		int32_t lineID = SDL_ReadLE32(fp);
		soundidlinelinks[lineID] = soundID;
	}
	SDL_assert_release(SDL_RWtell(fp) == SDL_RWsize(fp));
	SDL_RWclose(fp);

	tinyxml2::XMLDocument doc;
	doc.LoadFile("res/loc.xml");
	for (auto it = doc.RootElement()->FirstChildElement(); it; it = it->NextSiblingElement()) {
		int32_t lineID = it->IntAttribute("id");
		const char* text = it->GetText();
		if(text)
			locStrings[lineID] = text;
	}

	//main.open("D:\\wd2\\common\\languages\\main_english.loc");
	/*main.open("D:\\wiiu\\common\\languages\\main_english.loc");
	main.open("C:\\Users\\Jonathan\\Documents\\rpcs3\\dev_hdd0\\game\\BLES01854\\USRDIR\\common_unpack\\languages\\main_english.loc");

	//main.open(getAbsoluteFilePath("languages/dlc_solo_english.loc").c_str());
	main.open(getAbsoluteFilePath("languages/main_english.loc").c_str());
	main.open(getAbsoluteFilePath("languages/patch1_english.loc").c_str());

	main.open(getAbsoluteFilePath("languages/dlc_solo_german.loc").c_str());
	main.open(getAbsoluteFilePath("languages/main_german.loc").c_str());
	main.open(getAbsoluteFilePath("languages/patch1_german.loc").c_str());*/
}

Dialog & Dialog::instance() {
	static Dialog dialog;
	return dialog;
}


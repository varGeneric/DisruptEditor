#include "Dialog.h"

#include "Common.h"
#include <SDL_log.h>
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

	//main.open("D:\\wd2\\common\\languages\\main_english.loc");
	main.open("D:\\wiiu\\common\\languages\\main_english.loc");

	main.open(getAbsoluteFilePath("languages/dlc_solo_english.loc").c_str());
	main.open(getAbsoluteFilePath("languages/main_english.loc").c_str());
	main.open(getAbsoluteFilePath("languages/patch1_english.loc").c_str());

	main.open(getAbsoluteFilePath("languages/dlc_solo_german.loc").c_str());
	main.open(getAbsoluteFilePath("languages/main_german.loc").c_str());
	main.open(getAbsoluteFilePath("languages/patch1_german.loc").c_str());
}

Dialog & Dialog::instance() {
	static Dialog dialog;
	return dialog;
}


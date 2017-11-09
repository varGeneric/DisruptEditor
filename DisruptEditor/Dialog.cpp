#include "Dialog.h"

#include "Common.h"
#include <SDL_log.h>
#include "tinyfiles.h"

Dialog::Dialog() {
	conversationtable = readFCB(getAbsoluteFilePath("dialog/conversationtable.dat").c_str());
	dialogmanagerindices = readFCB(getAbsoluteFilePath("dialog/dialogmanagerindices.dat").c_str());

	tfDIR dir;
	tfDirOpen(&dir, getAbsoluteFilePath("dialog/behaviortrees").c_str());
	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir && tfGetExt(&file) == std::string("ai.rml")) {
			SDL_Log("Loading %s...", file.name);
			SDL_RWops *fp = SDL_RWFromFile(file.path, "rb");
			SDL_RWseek(fp, sizeof(fcbHeader) + 12, RW_SEEK_SET);
			Vector<Node*> list;
			behaviortrees[file.name].deserializeA(fp, list);
			SDL_RWclose(fp);
		}

		tfDirNext(&dir);
	}
	tfDirClose(&dir);

	main.open(getAbsoluteFilePath("languages/main_english.loc").c_str());

	/*fp = fopen("test.xml", "w");
	tinyxml2::XMLPrinter printer(fp);
	dialogmanagerindices.serializeXML(printer);
	fclose(fp);*/
}

Dialog & Dialog::instance() {
	static Dialog dialog;
	return dialog;
}


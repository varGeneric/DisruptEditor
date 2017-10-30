#include "Dialog.h"

#include "Common.h"
#include <SDL_log.h>
#include "tinyfiles.h"

Dialog::Dialog() {
	Vector<Node*> list;

	FILE *fp = fopen(getAbsoluteFilePath("dialog/conversationtable.dat").c_str(), "rb");
	fseek(fp, sizeof(fcbHeader), SEEK_SET);
	conversationtable.deserializeA(fp, list);
	fclose(fp);

	list.clear();
	fp = fopen(getAbsoluteFilePath("dialog/dialogmanagerindices.dat").c_str(), "rb");
	fseek(fp, sizeof(fcbHeader), SEEK_SET);
	dialogmanagerindices.deserializeA(fp, list);
	fclose(fp);

	tfDIR dir;
	tfDirOpen(&dir, getAbsoluteFilePath("dialog/behaviortrees").c_str());
	while (dir.has_next) {
		tfFILE file;
		tfReadFile(&dir, &file);

		if (!file.is_dir && tfGetExt(&file) == std::string("ai.rml")) {
			SDL_Log("Loading %s...", file.name);
			fp = fopen(file.path, "rb");
			fseek(fp, sizeof(fcbHeader) + 12, SEEK_SET);
			list.clear();
			behaviortrees[file.name].deserializeA(fp, list);
			fclose(fp);
		}

		tfDirNext(&dir);
	}
	tfDirClose(&dir);

	/*fp = fopen("test.xml", "w");
	tinyxml2::XMLPrinter printer(fp);
	dialogmanagerindices.serializeXML(printer);
	fclose(fp);*/
}

Dialog & Dialog::instance() {
	static Dialog dialog;
	return dialog;
}


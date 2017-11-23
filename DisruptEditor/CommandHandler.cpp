#include "CommandHandler.h"

#include <string.h>
#include "NBCF.h"
#include "RML.h"

bool endsWith(const char* str, const char* ending) {
	return strlen(str) > strlen(ending) && !strcmp(str + strlen(str) - strlen(ending), ending);
}

void handleFile(const char *filename) {
	if (!filename) return;

	if (endsWith(filename, ".lib") || endsWith(filename, ".obj") || endsWith(filename, ".fcb")) {
		Node node = readFCB(filename);
		std::string out = filename + std::string(".xml");
		FILE *fp = fopen(out.c_str(), "w");
		tinyxml2::XMLPrinter printer(fp);
		node.serializeXML(printer);
		fclose(fp);
	} else if (endsWith(filename, ".rml")) {
		std::unique_ptr<tinyxml2::XMLDocument> doc = loadRml(filename);
		std::string out = filename + std::string(".xml");
		doc->SaveFile(out.c_str());
	}
}

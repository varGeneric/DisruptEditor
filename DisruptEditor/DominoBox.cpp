#include "DominoBox.h"

#include <SDL_assert.h>
#include <stdio.h>
#include <string>
#include <regex>
#include "imgui.h"

/*

Parsing

*/

std::string readLuaLine(FILE *fp) {
	char buffer[500];
	if (!fgets(buffer, sizeof(buffer), fp))
		return std::string();

	//Cut off \n
	buffer[strlen(buffer) - 1] = '\0';

	//Trim spaces in front
	char *it = buffer;
	while (*it == ' ' || *it == '\t') {
		it++;
	}

	return it;
}

std::string scanToNextLine(FILE *fp, const char* str) {
	std::string line;
	do {
		line = readLuaLine(fp);
	} while (line != str);
	return readLuaLine(fp);
}

std::string skipToNextFunction(FILE *fp) {
	std::string line = readLuaLine(fp);
	do {
		line = readLuaLine(fp);
	} while (line != "end;");

	return line;
}

/*

Globals

*/

std::map<std::string, DominoConnectors> gConnectors;

void loadConnectors() {
	gConnectors.clear();
	tinyxml2::XMLDocument doc;
	doc.LoadFile("res/domino.xml");
	for (auto it = doc.RootElement()->FirstChildElement(); it; it = it->NextSiblingElement()) {
		std::string path = it->Attribute("path");
		std::transform(path.begin(), path.end(), path.begin(), ::tolower);
		const char* desc = it->Attribute("desc");

		DominoConnectors &c = gConnectors[path];
		if (desc)
			c.description = desc;

		for (auto j = it->FirstChildElement(); j; j = j->NextSiblingElement()) {
			std::string type = j->Name();
			if (type == "control_in") {
				DominoSlot &slot = c.in.push_back();
				slot.name = j->Attribute("name");
				slot.type = CONTROL;
				const char* desc = j->Attribute("desc");
				if (desc)
					slot.description = desc;
			} else if (type == "control_out") {
				DominoSlot &slot = c.out.push_back();
				slot.name = j->Attribute("name");
				slot.type = CONTROL;
				const char* desc = j->Attribute("desc");
				if (desc)
					slot.description = desc;
			} else if (type == "data_in") {
				DominoSlot &slot = c.in.push_back();
				slot.name = j->Attribute("name");
				slot.type = DATA;
				const char* desc = j->Attribute("desc");
				if (desc)
					slot.description = desc;
			} else if (type == "data_out") {
				DominoSlot &slot = c.out.push_back();
				slot.name = j->Attribute("name");
				slot.type = DATA;
				const char* desc = j->Attribute("desc");
				if (desc)
					slot.description = desc;
			}

		}
	}

}

void DominoBox::open(const char *filename) {
	loadConnectors();
	boxes.clear();
	connections.clear();
	localVariables.clear();
	scrolling = glm::vec2();

	FILE *fp = fopen(filename, "r");

	//Read Dependencies
	std::string line = scanToNextLine(fp, "function export:Create(cbox)");
	do {
		/*std::smatch cm;
		std::regex_match(line, cm, std::regex("cbox:LoadResource\\(\"(.*)\"\, \"(.*)\"\\);"));
		if (cm.size() == 3)
			resources.push_back(DominoResource(cm[2], cm[1]));*/

		line = readLuaLine(fp);
	} while (line != "end;");

	//Read Init and local vars
	line = scanToNextLine(fp, "function export:Init(cbox)");

	//I expect this line to be local l0
	SDL_assert_release(line == "local l0;");
	line = readLuaLine(fp);

	//Local Variables
	do {
		std::smatch cm;
		std::regex_match(line, cm, std::regex("self\\.(.*) = (.*);"));

		if (cm.size() == 3)
			localVariables[cm[1]] = cm[2];
		else
			break;

		line = readLuaLine(fp);
	} while (line != "end;");

	//CBoxes
	do {
		std::smatch cm;
		std::regex_match(line, cm, std::regex("self\\[(.*)\\] = cbox:CreateBox\\(\"(.*)\"\\);"));

		if (cm.size() != 3)
			break;

		DominoCBox cbox;
		cbox.id = std::stoi(cm[1]);
		cbox.boxClass = cm[2];
		line = cbox.deserialize(fp);

		boxes[cbox.id] = cbox;
	} while (line != "end;");

	//Functions
	line = readLuaLine(fp);
	line = readLuaLine(fp);
	do {
		std::smatch cm;
		std::regex_match(line, cm, std::regex("function export:(.*)\\(\\)"));

		if (cm.size() != 2) {
			line = readLuaLine(fp);
			continue;
		}

		std::string name = cm[1];

		std::regex_match(name, cm, std::regex("f_(\\d*)_(.*)"));
		if (cm.size() == 3) {
			int fromId = std::stoi(cm[1]);
			std::string controlFromName = cm[2];

			line = scanToNextLine(fp, "self = self._graph;");

			//Read Calls
			int toId = -1;
			std::string controlToName;
			std::map<std::string, std::string> vars;
			do {
				std::regex_match(line, cm, std::regex("l0 = self[(\\d*)];"));
				if (cm.size() == 2) {
					if (toId != -1) {
						//Save Boxes
						DominoPtr &ptr = connections.push_back();
						ptr.from = fromId;
						ptr.to = toId;
						ptr.fromKey = controlFromName;
						ptr.toKey = controlToName;
					}

					vars.clear();
					toId = std::stoi(cm[1]);
				}

				std::regex_match(line, cm, std::regex("l0.(.*) = (.*);"));
				if (cm.size() == 3) {
					vars[cm[1]] = cm[2];
				}

				std::regex_match(line, cm, std::regex("l0:(.*)\\(\\);"));
				if (cm.size() == 2) {
					controlToName = cm[1];
				}

				line = readLuaLine(fp);
			} while (line != "end;");
		}

		line = skipToNextFunction(fp);
	} while (line != "_compilerVersion = 4;");
	
	fclose(fp);

	autoOrganize();
}

void DominoBox::serialize(tinyxml2::XMLPrinter &printer) {
}

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

void DominoBox::draw() {
	const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);
	const float NODE_SLOT_RADIUS = 4.0f;
	static int node_selected = -1;
	static int node_hovered_in_scene = -1;
	static bool open_context_menu = false;
	ImVec2 offset = ImGui::GetCursorScreenPos() - scrolling;
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->ChannelsSplit(2);

	//Draw Grid
	ImU32 GRID_COLOR = ImColor(200, 200, 200, 40);
	float GRID_SZ = 64.0f;
	ImVec2 win_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_sz = ImGui::GetWindowSize();
	for (float x = fmodf(offset.x, GRID_SZ); x < canvas_sz.x; x += GRID_SZ)
		draw_list->AddLine(ImVec2(x, 0.0f) + win_pos, ImVec2(x, canvas_sz.y) + win_pos, GRID_COLOR);
	for (float y = fmodf(offset.y, GRID_SZ); y < canvas_sz.y; y += GRID_SZ)
		draw_list->AddLine(ImVec2(0.0f, y) + win_pos, ImVec2(canvas_sz.x, y) + win_pos, GRID_COLOR);

	// Display links
	draw_list->ChannelsSetCurrent(0); // Background
	/*for (int link_idx = 0; link_idx < links.Size; link_idx++) {
		NodeLink* link = &links[link_idx];
		Node* node_inp = &nodes[link->InputIdx];
		Node* node_out = &nodes[link->OutputIdx];
		ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->InputSlot);
		ImVec2 p2 = offset + node_out->GetInputSlotPos(link->OutputSlot);
		draw_list->AddBezierCurve(p1, p1 + ImVec2(+50, 0), p2 + ImVec2(-50, 0), p2, ImColor(200, 200, 100), 3.0f);
	}*/

	for (auto it = boxes.begin(); it != boxes.end(); ++it) {
		ImGui::PushID(it->first);

		ImVec2 node_rect_min = offset + it->second.pos;

		// Display node contents first
		draw_list->ChannelsSetCurrent(1); // Foreground
		bool old_any_active = ImGui::IsAnyItemActive();
		ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
		ImGui::BeginGroup(); // Lock horizontal position
		ImGui::Text("%s", it->second.getShortName().c_str());

		std::string path = it->second.boxClass;
		std::transform(path.begin(), path.end(), path.begin(), ::tolower);
		DominoConnectors &connectors = gConnectors[path];

		for (DominoSlot &slot : connectors.in) {
			ImGui::Text("%s", slot.name.c_str());
		}
		for (DominoSlot &slot : connectors.out) {
			ImGui::Text("%s", slot.name.c_str());
		}

		ImGui::EndGroup();

		// Save the size of what we have emitted and whether any of the widgets are being used
		bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
		it->second.size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
		ImVec2 node_rect_max = node_rect_min + it->second.size;

		// Display node box
		draw_list->ChannelsSetCurrent(0); // Background
		ImGui::SetCursorScreenPos(node_rect_min);
		ImGui::InvisibleButton("node", it->second.size);
		if (ImGui::IsItemHovered()) {
			node_hovered_in_scene = it->first;
			open_context_menu |= ImGui::IsMouseClicked(1);
		}
		bool node_moving_active = ImGui::IsItemActive();
		if (node_widgets_active || node_moving_active)
			node_selected = it->first;
		if (node_moving_active && ImGui::IsMouseDragging(0))
			it->second.pos = it->second.pos + ImGui::GetIO().MouseDelta;

		ImU32 node_bg_color = (node_hovered_in_scene == it->first || (node_selected == it->first))?ImColor(75, 75, 75):ImColor(60, 60, 60);
		draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f);
		draw_list->AddRect(node_rect_min, node_rect_max, ImColor(100, 100, 100), 4.0f);

		int slot_idx = 1;
		for (DominoSlot &slot : connectors.in) {
			ImColor color = ImColor(150, 150, 150, 150);
			draw_list->AddCircleFilled(offset + connectors.GetInputSlotPos(slot_idx++, it->second.pos, it->second.size), NODE_SLOT_RADIUS, color);
		}
		for (DominoSlot &slot : connectors.out) {
			ImColor color = ImColor(150, 150, 150, 150);
			draw_list->AddCircleFilled(offset + connectors.GetOutputSlotPos(slot_idx++, it->second.pos, it->second.size), NODE_SLOT_RADIUS, color);
		}

		ImGui::PopID();
	}

	draw_list->ChannelsMerge();

	// Scrolling
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(1, 0.0f))
		scrolling = scrolling - ImGui::GetIO().MouseDelta;
}

void DominoBox::autoOrganize() {
	glm::vec2 pos;
	for (auto it = boxes.begin(); it != boxes.end(); ++it) {
		it->second.pos = pos;
		pos.y += 90.f;
	}
}

std::string DominoCBox::deserialize(FILE *fp) {
	//Skip First Line it's           l0 = self[15];
	std::string line = readLuaLine(fp);
	line = readLuaLine(fp);//l0._graph = self;
	SDL_assert_release(line == "l0._graph = self;");
	line = readLuaLine(fp);

	std::map<std::string, std::string> localVariables;

	do {
		std::smatch cm;
		std::regex_match(line, cm, std::regex("self\\[(.*)\\] = cbox:CreateBox\\(\"(.*)\"\\);"));
		if (!cm.empty())
			break;

		std::regex_match(line, cm, std::regex("l0\\.(.*) = (.*);"));
		if (cm.size() == 3) {
			localVariables[cm[1]] = cm[2];
		} else {
			//Lets see if it's an array
			std::regex_match(line, cm, std::regex("l0\\.(.*) = \\{"));
			if (cm.size() == 2) {
				std::string name = cm[1];
				std::string value = "{";
				//Read Each Entry
				line = readLuaLine(fp);
				while (line != "};") {
					value += line;
					line = readLuaLine(fp);
				}
				value += "}";
				localVariables[name] = value;
			} else {
				break;
			}
		}

		line = readLuaLine(fp);
	} while (line != "end;");

	//Process local variables
	for (auto &it : localVariables) {
		/*DominoSlot &slot = out.push_back();
		slot.name = it.first;
		slot.type = CONTROL;
		slot.value = it.second;
		if (slot.value == "DummyFunction")
			slot.value.clear();*/
	}

	return line;
}

std::string DominoCBox::getShortName() {
	return boxClass.substr(boxClass.find_last_of('/') + 1);
}

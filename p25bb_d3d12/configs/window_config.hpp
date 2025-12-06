#pragma once
#include "../json/json_object.hpp"
#include <map>
#include <set>
#include <vector>
#include <string>

namespace PameECS::Configs {
	struct WindowConfig : JSON::JSONObject<WindowConfig> {
		std::map<std::string, uint32_t> windowStyles;
		std::string defaultWindowStyle;
		std::set<std::string> sizeIgnores;
		std::string className;
		std::string windowName;
		uint32_t defaultWidth = 800;
		uint32_t defaultHeight = 600;

		template<typename Func>
		void ForEachMember(Func&& func) {
			func.operator()<"windowStyles">(windowStyles);
			func.operator()<"defaultWindowStyle">(defaultWindowStyle);
			func.operator()<"sizeIgnores">(sizeIgnores);
			func.operator()<"className">(className);
			func.operator()<"windowName">(windowName);
			func.operator()<"defaultWidth">(defaultWidth);
			func.operator()<"defaultHeight">(defaultHeight);
		}
	};
}

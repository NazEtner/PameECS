#pragma once
#include "../json/json_object.hpp"
#include <map>
#include <vector>
#include <string>

namespace PameECS::Configs {
	struct AssetsConfig : JSON::JSONObject<AssetsConfig> {
		std::vector<std::string> archives = {};

		template<typename Func>
		void ForEachMember(Func&& func) {
			func.operator()<"archives">(archives);
		}
	};
}

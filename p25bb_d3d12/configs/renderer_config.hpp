#pragma once
#include "../json/json_object.hpp"
#include <map>
#include <vector>
#include <string>

namespace PameECS::Configs {
	struct RendererConfig : JSON::JSONObject<RendererConfig> {
		bool useDebugLayer = false;
		bool useAdvancedDebug = false;

		template<typename Func>
		void ForEachMember(Func&& func) {
			func.operator()<"useDebugLayer">(useDebugLayer);
			func.operator()<"useAdvancedDebug">(useAdvancedDebug);
		}
	};
}

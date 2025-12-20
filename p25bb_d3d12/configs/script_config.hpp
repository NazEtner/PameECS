#pragma once
#include "../json/json_object.hpp"
#include <set>
#include <string>

namespace PameECS::Configs {
	struct ScriptConfig : public JSON::JSONObject<ScriptConfig> {
		struct Loader : public JSON::JSONObject<Loader> {
			std::set<std::string> dllPath = {};

			template<typename Func>
			void ForEachMember(Func&& func) {
				func.operator()<"dllPath">(dllPath);
			}
		} loader;

		template<typename Func>
		void ForEachMember(Func&& func) {
			func.operator()<"loader">(loader);
		}
	};
}

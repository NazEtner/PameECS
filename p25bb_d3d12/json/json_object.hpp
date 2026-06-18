#pragma once
#include "../template_types/string_literal.hpp"
#include <nlohmann/json.hpp>

namespace PameECS::JSON {
	// 自動でJSONにシリアライズ、デシリアライズしたい
	template <typename Derived>
	struct JSONObject {
		// CRTPを使ってJSONObjectを継承した構造体(多分クラスでもいい)をJSONへシリアライズする
		nlohmann::json Serialize() {
			nlohmann::json j;

			static_cast<Derived*>(this)->ForEachMember(
				[&]<TemplateTypes::StringLiteral Name>(auto& member) {
					j[Name.data] = member;
				}
			);

			return j;
		}

		// JSONからCRTPを使ってJSONObjectを継承した構造体(多分クラスでもいい)へデシリアライズする
		// 構造体に存在するがJSONに存在しないプロパティは無視する
		// const値がある構造体をデシリアライズするということは意味不明なので多分エラー
		void Deserialize(const nlohmann::json& j) {
			static_cast<Derived*>(this)->ForEachMember(
				[&]<TemplateTypes::StringLiteral Name>(auto& member) {
					if (j.contains(Name.data)) {
						member = j.at(Name.data).get<std::remove_reference_t<decltype(member)>>();
					}
					// 見つからないなら元の値のまま(デフォルト値を使いたい場合を配慮する)
				}
			);
		}
	};

	template <typename T>
	void to_json(nlohmann::json& j, const JSONObject<T>& p) {
		j = const_cast<JSONObject<T>&>(p).Serialize();
	}

	template <typename T>
	void from_json(const nlohmann::json& j, JSONObject<T>& obj) {
		obj.Deserialize(j);
	}
}

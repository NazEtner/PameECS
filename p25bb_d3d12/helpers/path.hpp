#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace PameECS::Helpers::Path {
	std::vector<std::string> PathToVector(const std::filesystem::path& path) {
		std::vector<std::string> result;

		for (const auto& part : path) {
			result.push_back(part.string());
		}

		return result;
	}
}

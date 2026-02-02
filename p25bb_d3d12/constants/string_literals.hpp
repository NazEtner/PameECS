#pragma once
#include "../template_types/string_literal.hpp"

namespace PameECS::Constants::StringLiterals {
	inline constexpr auto RendererThreadPoolName = TemplateTypes::StringLiteral("Renderer");
	inline constexpr auto ArchiveDecompressThreadPoolName = TemplateTypes::StringLiteral("ArchiveDecompress");
	inline constexpr auto ArchiveMergeThreadPoolName = TemplateTypes::StringLiteral("ArchiveMerge");
	inline constexpr auto FileIOThreadPoolName = TemplateTypes::StringLiteral("FileIO");
	inline constexpr auto ECSScheduleThreadPoolName = TemplateTypes::StringLiteral("ECSSchedule");
}

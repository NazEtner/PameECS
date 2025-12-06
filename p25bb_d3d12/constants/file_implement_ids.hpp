#pragma once

namespace PameECS::Constants {
	enum class FileImplementIds : size_t {
		Assets,
		Internal,
	};

	enum class FileGlobalStateIds : size_t {
		// Commonは必ず0固定にするように
		Common = 0,
	};
}

#pragma once

namespace PameECS::Helpers {
	struct EmptyType {
		template<typename... Args>
		EmptyType(Args&&...) {}
	};
}

#pragma once

namespace PameECS::Helper::Container {
	template<typename T, typename U>
	void ResizePow2(T& container, size_t minSize, U init = U()) {
		auto size = container.size();
		size = size == 0 ? 1 : size;
		while (size <= minSize) {
			size *= 2;
		}
		container.resize(size, init);
	}
}

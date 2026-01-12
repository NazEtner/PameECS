#pragma once

namespace PameECS::Helpers::Container {
	template<typename T, typename U>
	void ResizePow2(T& container, size_t minSize, U&& init) {
		auto size = container.size();
		size = size == 0 ? 1 : size;
		while (size < minSize) {
			size *= 2;
		}
		container.resize(size, init);
	}

	template<typename T>
	void ResizePow2(T& container, size_t minSize) {
		auto size = container.size();
		size = size == 0 ? 1 : size;
		while (size < minSize) {
			size *= 2;
		}
		container.resize(size);
	}
}

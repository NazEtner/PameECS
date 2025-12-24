#pragma once
#include <memory>

namespace PameECS::Input {
	class Gamepad {
	public:
		Gamepad();
		~Gamepad();
		Gamepad(const Gamepad&) = delete;
		Gamepad& operator=(const Gamepad&) = delete;
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

#include "gamepad.hpp"

using PameECS::Input::Gamepad;

struct Gamepad::Impl {

};

Gamepad::Gamepad() {
	m_impl = std::make_unique<Impl>();
}

Gamepad::~Gamepad() {

}

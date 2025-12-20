#include "mouse.hpp"
#include <cassert>
#include <array>
#include <utility>
#include <string>
#include <vector>
#include <stack>

using PameECS::Input::Mouse;

struct Mouse::Impl {
	HWND windowHandle = nullptr;
	enum class Buttons : size_t {
		Left = 0,
		Right,
		Middle,
		ElementCount,
	};
	std::array<bool, 3> buttonState = {};
	std::array<bool, 3> prevButtonState = {};
	int wheelDelta = 0;
	bool wheelDeltaChanged = false;
	std::pair<float, float> position = { 0.0f, 0.0f };

	struct Rect {
		float left, top, right, bottom;
		uint64_t z;
		std::string name;
		bool isActive = true;
	};

	uint64_t nextZ = 0;
	std::vector<Rect> rects;
	std::stack<size_t> inactiveRectIndex;
	std::string hoveredNameCache;
	bool isAnyRectHovered = false;
};

Mouse::Mouse(HWND windowHandle) {
	m_impl = std::make_unique<Impl>();

	assert(windowHandle);

	m_impl->windowHandle = windowHandle;
}

Mouse::~Mouse() {
	// nothing to do.
}

void Mouse::Update() {
	m_impl->prevButtonState = m_impl->buttonState;
	if (GetForegroundWindow() != m_impl->windowHandle) {
		m_impl->buttonState[std::to_underlying(Impl::Buttons::Left)] = false;
		m_impl->buttonState[std::to_underlying(Impl::Buttons::Right)] = false;
		m_impl->buttonState[std::to_underlying(Impl::Buttons::Middle)] = false;
		m_impl->wheelDelta = 0;

		return;
	}

	POINT point;
	GetCursorPos(&point);
	ScreenToClient(m_impl->windowHandle, &point);
	RECT clientRect;
	GetClientRect(m_impl->windowHandle, &clientRect);

	m_impl->position = {
		2.0f * point.x / clientRect.right - 1.0f,
		1.0f - 2.0f * point.y / clientRect.bottom
	};

	m_impl->buttonState[std::to_underlying(Impl::Buttons::Left)] = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
	m_impl->buttonState[std::to_underlying(Impl::Buttons::Right)] = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
	m_impl->buttonState[std::to_underlying(Impl::Buttons::Middle)] = GetAsyncKeyState(VK_MBUTTON) & 0x8000;

	if (!m_impl->wheelDeltaChanged) {
		m_impl->wheelDelta = 0;
	}
	m_impl->wheelDeltaChanged = false;

	m_impl->isAnyRectHovered = false;
	if (IsMouseInWindow()) {
		float mx = m_impl->position.first;
		float my = m_impl->position.second;
		const Impl::Rect* topmost = nullptr;

		for (const auto& rect : m_impl->rects) {
			if (!rect.isActive) continue;
			if (rect.name.empty()) continue;

			if (mx >= rect.left && mx <= rect.right &&
				my <= rect.top && my >= rect.bottom) {

				if (!topmost || rect.z > topmost->z) {
					topmost = &rect;
				}
			}
		}

		if (topmost) {
			m_impl->hoveredNameCache = topmost->name;
			m_impl->isAnyRectHovered = true;
		}
	}
}

bool Mouse::IsLeftButtonDown() const {
	return m_impl->buttonState[std::to_underlying(Impl::Buttons::Left)];
}

bool Mouse::IsRightButtonDown() const {
	return m_impl->buttonState[std::to_underlying(Impl::Buttons::Right)];
}

bool Mouse::IsMiddleButtonDown() const {
	return m_impl->buttonState[std::to_underlying(Impl::Buttons::Middle)];
}

bool Mouse::WasLeftButtonPressed() const {
	return m_commonPressed(std::to_underlying(Impl::Buttons::Left));
}

bool Mouse::WasRightButtonPressed() const {
	return m_commonPressed(std::to_underlying(Impl::Buttons::Right));
}

bool Mouse::WasMiddleButtonPressed() const {
	return m_commonPressed(std::to_underlying(Impl::Buttons::Middle));
}

bool Mouse::WasLeftButtonReleased() const {
	return m_commonReleased(std::to_underlying(Impl::Buttons::Left));
}

bool Mouse::WasRightButtonReleased() const {
	return m_commonReleased(std::to_underlying(Impl::Buttons::Right));
}

bool Mouse::WasMiddleButtonReleased() const {
	return m_commonReleased(std::to_underlying(Impl::Buttons::Middle));
}

bool Mouse::m_commonPressed(size_t index) const {
	if (index >= std::to_underlying(Impl::Buttons::ElementCount)) [[unlikely]] return false;
	return m_impl->buttonState[index] && !m_impl->prevButtonState[index];
}

bool Mouse::m_commonReleased(size_t index) const {
	if (index >= std::to_underlying(Impl::Buttons::ElementCount)) [[unlikely]] return false;
	return !m_impl->buttonState[index] && m_impl->prevButtonState[index];
}

int Mouse::GetWheelDelta() const {
	return m_impl->wheelDelta;
}

float Mouse::GetCursorPositionX() const {
	return m_impl->position.first;
}

float Mouse::GetCursorPositionY() const {
	return m_impl->position.second;
}

void Mouse::OnMouseDelta(int delta) {
	m_impl->wheelDelta = delta;
	m_impl->wheelDeltaChanged = true;
}

bool Mouse::IsMouseInWindow() const {
	if (GetForegroundWindow() != m_impl->windowHandle) return false;

	return m_impl->position.first >= -1.0f && m_impl->position.first <= 1.0f &&
		m_impl->position.second >= -1.0f && m_impl->position.second <= 1.0f;
}

void Mouse::AddRect(const char* name,
	float left, float top,
	float width, float height) {
	Impl::Rect rect = {
		.left = left, .top = top, .right = left + width, .bottom = top - height,
		.z = m_impl->nextZ++,
		.name = std::string(name),
	};

	if (!m_impl->inactiveRectIndex.empty()) {
		auto index = m_impl->inactiveRectIndex.top();
		m_impl->inactiveRectIndex.pop();

		m_impl->rects[index] = rect;
		return;
	}

	m_impl->rects.emplace_back(rect);
}

void Mouse::RemoveRect(const char* name) {
	if (!name) [[unlikely]] return;

	size_t index = 0;
	for (auto& rect : m_impl->rects) {
		if (rect.isActive && rect.name == name) [[unlikely]] {
			rect.isActive = false;
			m_impl->inactiveRectIndex.push(index);

			if (m_impl->isAnyRectHovered && m_impl->hoveredNameCache == rect.name) {
				m_impl->isAnyRectHovered = false;
				m_impl->hoveredNameCache.clear();
			}
		}
		++index;
	}
}

const char* Mouse::GetHoveredRectName() const {
	if (!m_impl->isAnyRectHovered) return nullptr;

	return m_impl->hoveredNameCache.c_str();
}

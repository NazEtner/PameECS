#include "mouse.hpp"
#include <cassert>
#include <array>
#include <utility>
#include <string>
#include <vector>
#include <stack>
#include <mutex>
#include <imgui/imgui.h>

using PameECS::Input::Mouse;

extern "C" {
	PECS_DLL_SHARED bool InputMouseIsLeftButtonDown(const PameECS::Input::Mouse* mouse) {
		return mouse->IsLeftButtonDown();
	}
	PECS_DLL_SHARED bool InputMouseIsRightButtonDown(const PameECS::Input::Mouse* mouse) {
		return mouse->IsRightButtonDown();
	}
	PECS_DLL_SHARED bool InputMouseIsMiddleButtonDown(const PameECS::Input::Mouse* mouse) {
		return mouse->IsMiddleButtonDown();
	}

	PECS_DLL_SHARED bool InputMouseWasLeftButtonPressed(const PameECS::Input::Mouse* mouse) {
		return mouse->WasLeftButtonPressed();
	}
	PECS_DLL_SHARED bool InputMouseWasRightButtonPressed(const PameECS::Input::Mouse* mouse) {
		return mouse->WasRightButtonPressed();
	}
	PECS_DLL_SHARED bool InputMouseWasMiddleButtonPressed(const PameECS::Input::Mouse* mouse) {
		return mouse->WasMiddleButtonPressed();
	}

	PECS_DLL_SHARED bool InputMouseWasLeftButtonReleased(const PameECS::Input::Mouse* mouse) {
		return mouse->WasLeftButtonReleased();
	}
	PECS_DLL_SHARED bool InputMouseWasRightButtonReleased(const PameECS::Input::Mouse* mouse) {
		return mouse->WasRightButtonReleased();
	}
	PECS_DLL_SHARED bool InputMouseWasMiddleButtonReleased(const PameECS::Input::Mouse* mouse) {
		return mouse->WasMiddleButtonReleased();
	}

	PECS_DLL_SHARED int InputMouseGetWheelDelta(const PameECS::Input::Mouse* mouse) {
		return mouse->GetWheelDelta();
	}
	PECS_DLL_SHARED float InputMouseGetCursorPositionX(const PameECS::Input::Mouse* mouse) {
		return mouse->GetCursorPositionX();
	}
	PECS_DLL_SHARED float InputMouseGetCursorPositionY(const PameECS::Input::Mouse* mouse) {
		return mouse->GetCursorPositionY();
	}

	PECS_DLL_SHARED bool InputMouseIsMouseInWindow(const PameECS::Input::Mouse* mouse) {
		return mouse->IsMouseInWindow();
	}

	PECS_DLL_SHARED void InputMouseAddRect(
		PameECS::Input::Mouse* mouse,
		const char* name,
		float left, float top,
		float width, float height) {
		mouse->AddRect(name, left, top, width, height);
	}

	PECS_DLL_SHARED void InputMouseRemoveRect(PameECS::Input::Mouse* mouse, const char* name) {
		mouse->RemoveRect(name);
	}

	PECS_DLL_SHARED void InputMouseClearRect(PameECS::Input::Mouse* mouse) {
		mouse->ClearRect();
	}

	PECS_DLL_SHARED const char* InputMouseGetHoveredRectName(const PameECS::Input::Mouse* mouse) {
		return mouse->GetHoveredRectName();
	}

	PECS_DLL_SHARED void InputMouseSetCursorStatus(PameECS::Input::Mouse* mouse, PameECS::Input::CursorStatus status) {
		mouse->SetCursorStatus(status);
	}
}

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
	PameECS::Input::CursorStatus cursorStatus = PameECS::Input::CursorStatus::Arrow;
	std::mutex mutex;
};

Mouse::Mouse(HWND windowHandle) {
	m_impl = std::make_unique<Impl>();

	assert(windowHandle);

	m_impl->windowHandle = windowHandle;
}

Mouse::~Mouse() {
	// nothing to do.
}

void Mouse::ShowDebug() {
	if (ImGui::TreeNode("Mouse")) {
		ImGui::Text("In Window: %s", IsMouseInWindow() ? "Yes" : "No");
		ImGui::Text("Cursor Pos: (%.3f, %.2f)", GetCursorPositionX(), GetCursorPositionY());
		ImGui::Text("Wheel Delta: %d", GetWheelDelta());

		ImGui::Separator();

		// ボタンの状態 (Down / Pressed / Released)
		static const char* labels[] = { "Left", "Right", "Middle" };
		if (ImGui::BeginTable("MouseButtons", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Button");
			ImGui::TableSetupColumn("Down");
			ImGui::TableSetupColumn("Pressed");
			ImGui::TableSetupColumn("Released");
			ImGui::TableHeadersRow();

			for (int i = 0; i < 3; ++i) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", labels[i]);

				// 各状態の取得 (iをsize_tにキャストして内部関数的に扱うか、既存の公開関数を使用)
				bool down = (i == 0) ? IsLeftButtonDown() : (i == 1) ? IsRightButtonDown() : IsMiddleButtonDown();
				bool pressed = (i == 0) ? WasLeftButtonPressed() : (i == 1) ? WasRightButtonPressed() : WasMiddleButtonPressed();
				bool released = (i == 0) ? WasLeftButtonReleased() : (i == 1) ? WasRightButtonReleased() : WasMiddleButtonReleased();

				ImGui::TableSetColumnIndex(1);
				if (down) ImGui::TextColored(ImVec4(0, 1, 0, 1), "YES"); else ImGui::Text("-");
				ImGui::TableSetColumnIndex(2);
				if (pressed) ImGui::TextColored(ImVec4(1, 1, 0, 1), "TRG"); else ImGui::Text("-");
				ImGui::TableSetColumnIndex(3);
				if (released) ImGui::TextColored(ImVec4(1, 0, 0, 1), "TRG"); else ImGui::Text("-");
			}
			ImGui::EndTable();
		}

		ImGui::Separator();

		if (ImGui::TreeNode("Cursor Shape Test")) {
			// 現在の形状（または直前にセットした状態）を文字列にして表示する仕組みがあると便利です
			// ※もし現在値を保持するゲッターがあればそれに差し替えてください
			static PameECS::Input::CursorStatus testStatus = PameECS::Input::CursorStatus::Default;

			const char* statusStr = "Unknown";
			switch (testStatus) {
			case PameECS::Input::CursorStatus::Default: statusStr = "Default (Arrow)"; break;
			case PameECS::Input::CursorStatus::Hand:    statusStr = "Hand";            break;
			case PameECS::Input::CursorStatus::Wait:    statusStr = "Wait (Hourglass)"; break;
			case PameECS::Input::CursorStatus::Arrow:   statusStr = "Arrow";           break;
			}
			ImGui::Text("Last Test Status: %s", statusStr);

			ImGui::Spacing();
			ImGui::Text("Force Set Cursor:");

			// クリックしたら即座に変更関数を叩く
			if (ImGui::Button("Arrow (Default)")) {
				testStatus = PameECS::Input::CursorStatus::Arrow;
				this->SetCursorStatus(testStatus);
			}
			ImGui::SameLine();
			if (ImGui::Button("Hand (Link)")) {
				testStatus = PameECS::Input::CursorStatus::Hand;
				this->SetCursorStatus(testStatus);
			}
			ImGui::SameLine();
			if (ImGui::Button("Wait (Busy)")) {
				testStatus = PameECS::Input::CursorStatus::Wait;
				this->SetCursorStatus(testStatus);
			}

			// 注意書き（もし毎フレーム別のシステムが書き換えている場合のデバッグ用ヒント）
			ImGui::TextDisabled("Note: If other UI systems overwrite this frame-by-frame,");
			ImGui::TextDisabled("the shape change above might be instantly overridden.");

			ImGui::TreePop();
		}

		ImGui::Separator();

		// Rects (当たり判定エリア) のリスト
		if (ImGui::TreeNode("Registered Rects")) {
			static bool showOverlay = false;
			ImGui::Checkbox("Show Hitbox Overlay", &showOverlay);

			if (showOverlay) {
				ImDrawList* drawList = ImGui::GetForegroundDrawList();

				RECT clientRect;
				GetClientRect(m_impl->windowHandle, &clientRect);

				auto ToScreenPixel = [&](float nx, float ny) -> ImVec2 {
					float px = (nx + 1.0f) * 0.5f * clientRect.right;
					float py = (1.0f - ny) * 0.5f * clientRect.bottom;

					return ImVec2(px, py);
				};

				std::lock_guard lock(m_impl->mutex);
				const char* hoveredName = GetHoveredRectName();

				for (const auto& rect : m_impl->rects) {
					if (!rect.isActive) continue;

					ImVec2 pMin = ToScreenPixel(rect.left, rect.top);
					ImVec2 pMax = ToScreenPixel(rect.right, rect.bottom);

					bool isHovered = (hoveredName && rect.name == hoveredName);

					// 枠線の色 (ホバー時は緑、通常は赤)
					ImU32 color = isHovered ? IM_COL32(0, 255, 0, 200) : IM_COL32(255, 0, 0, 150);
					float thickness = isHovered ? 3.0f : 1.0f;

					// 矩形を描画
					drawList->AddRect(pMin, pMax, color, 0.0f, 0, thickness);

					// Rect名のラベルを表示
					std::string label = rect.name + " (z:" + std::to_string(rect.z) + ")";
					drawList->AddText(ImVec2(pMin.x + 2, pMin.y + 2), color, label.c_str());
				}
			}

			if (ImGui::TreeNode("Create New Rect")) {
				static char newName[64] = "DebugRect_1";
				static float pos[2] = { 0.0f, 0.0f }; // Left, Top
				static float size[2] = { 0.5f, 0.5f }; // Width, Height

				ImGui::InputText("Name", newName, IM_ARRAYSIZE(newName));
				ImGui::SliderFloat2("Position (L, T)", pos, -1.0f, 1.0f);
				ImGui::SliderFloat2("Size (W, H)", size, 0.0f, 2.0f);

				if (ImGui::Button("Add to Mouse System")) {
					this->AddRect(newName, pos[0], pos[1], size[0], size[1]);
				}
				ImGui::TreePop();
			}
			if (ImGui::Button("Clear All Rects")) {
				this->ClearRect();
			}

			std::lock_guard lock(m_impl->mutex);

			const char* hoveredName = GetHoveredRectName();
			ImGui::Text("Total Rects: %zu", m_impl->rects.size());
			ImGui::Text("Hovered: %s", hoveredName ? hoveredName : "(None)");

			if (ImGui::BeginTable("RectList", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0, 150))) {
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Bounds (L,T,R,B)");
				ImGui::TableSetupColumn("Z");
				ImGui::TableSetupColumn("Status");
				ImGui::TableHeadersRow();

				for (const auto& rect : m_impl->rects) {
					ImGui::TableNextRow();

					// 無効なRectは薄く表示
					if (!rect.isActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%s", rect.name.c_str());

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%.2f, %.2f, %.2f, %.2f", rect.left, rect.top, rect.right, rect.bottom);

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%llu", rect.z);

					ImGui::TableSetColumnIndex(3);
					if (rect.isActive) {
						if (hoveredName && rect.name == hoveredName)
							ImGui::TextColored(ImVec4(0, 1, 0, 1), "HOVER");
						else
							ImGui::Text("Active");
					}
					else {
						ImGui::Text("Inactive");
					}

					if (!rect.isActive) ImGui::PopStyleColor();
				}
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}
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

	switch (m_impl->cursorStatus) {
	case PameECS::Input::CursorStatus::Hand:
		SetCursor(LoadCursor(NULL, IDC_HAND));
		break;
	case PameECS::Input::CursorStatus::Wait:
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		break;
	case PameECS::Input::CursorStatus::Arrow:
	case PameECS::Input::CursorStatus::Default: [[fallthrough]];
	default:
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		break;
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
	std::lock_guard lock(m_impl->mutex);
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

	std::lock_guard lock(m_impl->mutex);

	size_t index = 0;
	for (auto& rect : m_impl->rects) {
		// rect.name != nameになる方が多いはずなのでunlikely
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

void Mouse::ClearRect() {
	std::lock_guard lock(m_impl->mutex);
	m_impl->rects.clear();
	m_impl->isAnyRectHovered = false;
	m_impl->hoveredNameCache.clear();
}

const char* Mouse::GetHoveredRectName() const {
	std::lock_guard lock(m_impl->mutex);
	if (!m_impl->isAnyRectHovered) return nullptr;

	return m_impl->hoveredNameCache.c_str();
}

void Mouse::SetCursorStatus(PameECS::Input::CursorStatus status) {
	std::lock_guard lock(m_impl->mutex);

	m_impl->cursorStatus = status;
}

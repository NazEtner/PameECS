#pragma once
#include "input_service.hpp"

namespace PameECS::Services {
	// サービスを一つのクラスから取得するためのインターフェイス
	class Services {
	public:
		Services() = default;
		~Services() = default;
		Services(const Services&) = delete;
		Services& operator=(const Services&) = delete;
		Services(Services&&) = delete;
		Services& operator=(Services&&) = delete;
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

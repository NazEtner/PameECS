#pragma once

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
	};
}

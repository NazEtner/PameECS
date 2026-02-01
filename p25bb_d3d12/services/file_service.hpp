#pragma once
#include "../file/assets.hpp"

namespace PameECS::Services {
	class FileService {
	public:
		FileService();
		~FileService();

		File::Assets* GetAssets();
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

extern "C" {
	PameECS::File::Assets* ServicesFileGetAssets(PameECS::Services::FileService* service);
}

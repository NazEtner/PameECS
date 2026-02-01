#include "file_service.hpp"
#include <memory>

using PameECS::Services::FileService;

struct FileService::Impl {
	std::unique_ptr<File::Assets> assets;
};

FileService::FileService() {
	m_impl = std::make_unique<Impl>();
	m_impl->assets = std::make_unique<File::Assets>();
}

FileService::~FileService() {

}

PameECS::File::Assets* FileService::GetAssets() {
	return m_impl->assets.get();
}

extern "C" {
	PameECS::File::Assets* ServicesFileGetAssets(PameECS::Services::FileService* service) {
		return service->GetAssets();
	}
}

#pragma once
#include <cstdint>
#include <memory>
#include "../macros/dll.hpp"

namespace PameECS::File {
	struct AssetHandle {
		friend class Assets;
	private:
		uint32_t id = 0; // 0は無効なハンドル
		uint32_t generation = 0; // 0世代は存在しない
	};

	struct AsyncTicket {
		friend class Assets;
	private:
		uint32_t id = 0; // 0は無効なチケット
		AssetHandle handle = {};
	};

	class Assets {
	public:
		Assets();
		~Assets();
		bool IsValidHandle(const AssetHandle& handle) const;
		bool IsValidTicket(const AsyncTicket& ticket) const;
		AssetHandle GetAssetHandle(const char* path, size_t pathSize);
		AsyncTicket Load(const AssetHandle& handle);
		// GetDataを呼んだが、AsyncTicketに対応するロードが完了していなかった場合はnullptrを返し、チケットは無効化されない
		// それ以外の場合ではデータかnullptrを返し、AsyncTicketは無効化される
		// 返されたデータをdeleteしてはいけない
		// 返されたデータはアセットハンドルごとに一意ではない
		uint8_t* GetData(AsyncTicket& ticket, size_t& outSize);
		// GetDataSyncを呼んだらAsyncTicketは無効化される
		// 返されたデータをdeleteしてはいけない
		// 返されたデータはアセットハンドルごとに一意ではない
		uint8_t* GetDataSync(AsyncTicket& ticket, size_t& outSize);
		// アセットハンドルに関連するデータを解放する
		void ReleaseData(const AssetHandle& handle, uint8_t* data);
		// アセットハンドルと、それに関連する全てのデータを解放していいことにする
		void ReleaseHandle(const AssetHandle& handle);
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

extern "C" {
	PECS_DLL_SHARED bool FileAssetsIsValidHandle(
		PameECS::File::Assets* assets,
		const PameECS::File::AssetHandle* handle);
	PECS_DLL_SHARED bool FileAssetsIsValidTicket(
		PameECS::File::Assets* assets,
		const PameECS::File::AsyncTicket* ticket);
	PECS_DLL_SHARED void FileAssetsGetAssetHandle(
		PameECS::File::Assets* assets,
		PameECS::File::AssetHandle* outHandle,
		const char* path,
		size_t pathSize);
	PECS_DLL_SHARED void FileAssetsLoad(
		PameECS::File::Assets* assets,
		PameECS::File::AsyncTicket* outTicket,
		const PameECS::File::AssetHandle* handle);
	PECS_DLL_SHARED uint8_t* FileAssetsGetData(
		PameECS::File::Assets* assets,
		PameECS::File::AsyncTicket* ticket,
		size_t* outSize);
	PECS_DLL_SHARED uint8_t* FileAssetsGetDataSync(
		PameECS::File::Assets* assets,
		PameECS::File::AsyncTicket* ticket,
		size_t* outSize);
	PECS_DLL_SHARED void FileAssetsReleaseData(
		PameECS::File::Assets* assets,
		const PameECS::File::AssetHandle* handle,
		uint8_t* data);
	PECS_DLL_SHARED void FileAssetsReleaseHandle(
		PameECS::File::Assets* assets,
		const PameECS::File::AssetHandle* handle);
}

#pragma once
#include <cstdint>
#include <memory>

namespace PameECS::File {
	struct AssetHandle {
		friend class Assets;
	private:
		uint32_t id = 0;
		bool valid = false;
	};
	struct AsyncTicket {
		friend class Assets;
	private:
		uint32_t id = 0;
		bool valid = false;
	};

	class Assets {
	public:
		Assets();
		~Assets();
		AssetHandle GetAssetHandle(const char* path, size_t pathSize);
		AsyncTicket Load(const AssetHandle& handle);
		// GetDataを呼んだが、AsyncTicketに対応するロードが完了していなかった場合はnullptrを返し、チケットは無効化されない
		// それ以外の場合ではデータかnullptrを返し、AsyncTicketは無効化される
		// 返されたデータをdeleteしてはいけない
		// 返されたデータはアセットハンドルごとに一意ではない
		char* GetData(const AsyncTicket& ticket, size_t& outSize);
		// GetDataSyncを呼んだらAsyncTicketは無効化される
		// 返されたデータをdeleteしてはいけない
		// 返されたデータはアセットハンドルごとに一意ではない
		char* GetDataSync(const AsyncTicket& ticket, size_t& outSize);
		// アセットハンドルに関連するデータを解放する
		void ReleaseData(const AssetHandle& handle, char* data);
		// アセットハンドルと、それに関連する全てのデータを解放する
		void ReleaseHandle(const AssetHandle& handle);
	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}

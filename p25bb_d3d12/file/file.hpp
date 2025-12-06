#pragma once
#include "file_implementation.hpp"
#include "file_global_state.hpp"

namespace PameECS::File {
	// ロード先はImplementId毎に固有、グローバールステート(スレッドプール等)はGlobalStateId毎に固有
	// テンプレートパラメータに関わらず、DLL(or EXE)ごとに(多分)固有
	// DLL/EXE間で共有したい場合は、FileGlobalStateとFileの両方の特殊化にPECS_DLL_SHAREDをつければいい
	// アーカイブファイルへの書き込みをFileクラスで行うのは意味不明なので、ファイルの出力はofstreamを使うべき
	template<size_t ImplementId, size_t GlobalStateId = 0>
	class File final {
	public:
		void SetArchiveLoader(std::shared_ptr<Archive::ArchiveLoader> archiveLoader) {
			m_getImplementation()->SetArchiveLoader(archiveLoader);
		}

		std::future<std::stringstream> LoadAsync(const std::string& path) {
			return m_getImplementation()->Load(path);
		}

		std::stringstream LoadSync(const std::string& path) {
			return LoadAsync(path).get();
		}
	private:
		FileGlobalState<GlobalStateId>* m_getGlobalState() {
			static FileGlobalState<GlobalStateId> globalState = {};
			return &globalState;
		}

		FileImplementation* m_getImplementation() {
			// GetThreadPool()がnullptrになるときは、デバッグビルドのときにassertion failedするので注意
			return m_getGlobalState()->GetFileImplementation<ImplementId>();
		}
	};
}

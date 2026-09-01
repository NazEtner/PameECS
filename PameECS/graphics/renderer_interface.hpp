#pragma once
#include <cstdint>

namespace Pame::Graphics {
	class IRenderer {
	public:
		virtual ~IRenderer() = default;
		virtual bool Render() = 0;
		virtual bool Present() = 0;
		virtual void Recovery() = 0;
		// 1u << 31は予約済みフラグ「リセットしない」にすること
		// 即時リセットではなく、Render(1u << 31)がtrueを返したらRender()とPresent()をバイパスしてRecoveryを呼ぶということに注意
		virtual bool Reset(uint32_t flags = 0) = 0;
		// これからCPUが書き込むフレームリソース(リングバッファのスロット)を、GPUが使い終わるまで待つ
		// 描画データを生成するより前、フレームの先頭で呼ぶこと
		virtual void WaitForFrameResources() = 0;
	};
}

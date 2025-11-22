#pragma once
#include <cstdint>
#include <array>
#include <string>
#include <format>
#include <vector>

#include "../../macros/debug.hpp"
#include "../../macros/assertion.hpp"
#include "../../helpers/binary.hpp"
#include "../../template_types/string_literal.hpp"

namespace PameECS::File::Archive::Types {
	template<typename Derived, std::endian ExpectedEndian = std::endian::little>
	struct AutoProcessing {
		void ToNativeEndian() {
			static_cast<Derived*>(this)->ForEachMember([this]<TemplateTypes::StringLiteral Name>(auto& member) {
				this->m_convertToNativeEndian(member);
			});
		}

		std::string GenerateDebugString() {
#ifdef _DEBUG
			std::string result = typeid(Derived).name() + std::string(" ");
			result.resize(50, '-');
			result += "\n";
			static_cast<Derived*>(this)->ForEachMember([&result]<TemplateTypes::StringLiteral Name>(auto& member) {
				result += std::format("{:<30}", std::string(Name.data) + ":");

				using T = std::remove_reference_t<decltype(member)>;

				if constexpr (std::is_integral_v<T> && !std::is_same_v<T, char> && !std::is_same_v<T, bool>) {
					result += std::format("{}", member);
				}
				else if constexpr (std::is_enum_v<T>) {
					result += std::format("{}", static_cast<std::underlying_type_t<T>>(member));
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					if (member.length() > 20) {
						result += std::format("\"{}...\" (len: {})", member.substr(0, 20), member.length());
					}
					else {
						result += std::format("\"{}\"", member);
					}
				}
				else if constexpr (std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>) {
					std::string_view sv(member, std::extent_v<T>);
					result += std::format("['{}']", sv);
				}
				else if constexpr (std::is_same_v<T, bool>) {
					result += (member ? "true" : "false");
				}
				else if constexpr (std::is_pointer_v<T>) {
					result += std::format("{:p}", (void*)member);
				}
				else {
					result += std::format("<Type: {}>", typeid(T).name());
				}

				result += "\n";
			});

			return result;
#endif
			return "__DELETED__";
		}
	private:
		template<typename T>
		void m_convertToNativeEndian(T& value) {
			if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
				value = Helpers::Binary::ToNativeEndian<T, ExpectedEndian>(value);
			}
		}
	};

	struct alignas(1) Header : AutoProcessing<Header> {
		char magic[4];
		uint8_t versionMajor;
		uint8_t versionMinor;
		uint8_t versionPatch;
		uint8_t reserved;

		[[nodiscard]] bool IsValid() const {
			bool isMagicValid = std::memcmp(magic, "PEAC", 4) == 0;

			return isMagicValid;
		}

		template<typename Func>
		void ForEachMember(Func&& func) {
			func.operator()<"magic">(magic);
			func.operator()<"versionMajor">(versionMajor);
			func.operator()<"versionMinor">(versionMinor);
			func.operator()<"versionPatch">(versionPatch);
			func.operator()<"reserved">(reserved);
		}
	};

	struct alignas(8) SizeInformation : AutoProcessing<SizeInformation> {
		uint32_t entryCompressedSize;
		uint32_t entryUncompressedSize;
		uint64_t dataChunkIndexCompressedSize;
		uint64_t dataChunkIndexUncompressedSize;
		uint64_t totalDataChunkCompressedSize;

		template<typename Func>
		void ForEachMember(Func&& func) {
			func.operator()<"entryCompressedSize">(entryCompressedSize);
			func.operator()<"entryUncompressedSize">(entryUncompressedSize);
			func.operator()<"dataChunkIndexCompressedSize">(dataChunkIndexCompressedSize);
			func.operator()<"dataChunkIndexUncompressedSize">(dataChunkIndexUncompressedSize);
			func.operator()<"totalDataChunkCompressedSize">(totalDataChunkCompressedSize);
		}
	};

	struct Entry : AutoProcessing<Entry> {
		uint64_t dataSize;
		uint64_t dataOffset;
		uint16_t nameLength;
		std::string name;
		std::vector<Entry> children;

		template<typename Func>
		void ForEachMember(Func&& func) {
			func.operator()<"dataSize">(dataSize);
			func.operator()<"dataOffset">(dataOffset);
			func.operator()<"nameLength">(nameLength);
			func.operator()<"name">(name);
		}
	};

	inline constexpr bool TypeAssertion() {
		static_assert(sizeof(Header) == 8, "Size of Header must be 8 bytes.");
		static_assert(sizeof(SizeInformation) == 32, "Size of SizeInformation must be 32 bytes.");

		static_assert(std::is_trivially_copyable_v<Header>, "Header must be trivially copyable.");
		static_assert(std::is_trivially_copyable_v<SizeInformation>, "SizeInformation must be trivially copyable.");

		return true; // 戻り値に意味はない
	}

	GLOBAL_CHECK_NON_ARG(TypeAssertion);
}

#pragma once
#include <cstdint>
#include <vector>

namespace PameECS::Helpers::CRC {
	class CRC64ECMACalculator {
	public:
		static constexpr uint64_t polynomial = 0x42F0E1EBA9EA3693ULL;
		static constexpr uint64_t initialValue = 0x000000000000000ULL;
		static constexpr uint64_t xorOut = 0x0000000000000000ULL;

		CRC64ECMACalculator() {
			m_createTable();
		}

		uint64_t Calculate(const std::vector<uint8_t>& data) const {
			uint64_t crc = initialValue;
			for (size_t i = 0; i < data.size(); ++i) {
				uint8_t index = static_cast<uint8_t>((crc >> 56) ^ data[i]);
				crc = (crc << 8) ^ m_table[index];
			}

			return crc ^ xorOut;
		}
	private:
		std::vector<uint64_t> m_table;
		void m_createTable() {
			m_table.resize(256);
			for (uint64_t i = 0; i < 256; ++i) {
				uint64_t rem = i << 56;
				for (int j = 0; j < 8; ++j) {
					if (rem & (1ULL << 63)) {
						rem = (rem << 1) ^ polynomial;
					}
					else {
						rem <<= 1;
					}
				}
				m_table[i] = rem;
			}
		}
	};
}

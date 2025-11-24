#include "archive_creator.hpp"

using PameECS::File::Archive::ArchiveCreator;

template<>
std::vector<uint8_t> ArchiveCreator::m_serialize(const PameECS::File::Archive::Types::Entry& value) {
	std::vector<uint8_t> result;

	auto addData = [&]<typename T>(const T * data, const size_t size = sizeof(T)) {
		result.insert(result.end(), reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
	};

	addData(&value.dataSize);
	addData(&value.dataOffset);

	if (value.nameLength != value.name.size()) throw Exceptions::FileError("Invalid file name length.");

	addData(&value.nameLength);
	addData(value.name.data(), value.nameLength);
	uint16_t childCount = static_cast<uint16_t>(value.children.size());
	if (value.children.size() != childCount) throw Exceptions::FileError("Child count is too large.");
	addData(&childCount);
	for (uint16_t i = 0; i < childCount; ++i) {
		auto childrenBinary = m_serialize(value.children[i]);
		result.insert(result.end(), childrenBinary.begin(), childrenBinary.end());
	}

	return result;
}

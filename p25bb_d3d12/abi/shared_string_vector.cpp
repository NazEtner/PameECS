#include "shared_string_vector.hpp"

using PameECS::ABI::SharedStringVector;

struct SharedStringVector::Impl {
	std::vector<SharedString*> data;
};

SharedStringVector::SharedStringVector() {
	m_impl = new Impl();
}

SharedStringVector::~SharedStringVector() {
	for (auto str : m_impl->data) {
		DestroySharedString(str);
	}
	delete m_impl;
}

size_t SharedStringVector::GetSize() const {
	return m_impl->data.size();
}

PameECS::ABI::SharedString* SharedStringVector::GetAt(size_t index) const {
	return m_impl->data.at(index);
}

void SharedStringVector::PushBack(const PameECS::ABI::SharedString* str) {
	m_impl->data.push_back(str->Copy());
}

void SharedStringVector::Clear() {
	for (auto str : m_impl->data) {
		DestroySharedString(str);
	}
	m_impl->data.clear();
}

extern "C" PECS_DLL_SHARED PameECS::ABI::SharedStringVector* CreateSharedStringVector() {
	return new PameECS::ABI::SharedStringVector();
}

extern "C" PECS_DLL_SHARED void DestroySharedStringVector(PameECS::ABI::SharedStringVector* strVec) {
	delete strVec;
}

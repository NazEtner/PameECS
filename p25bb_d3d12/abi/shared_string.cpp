#include "shared_string.hpp"

using PameECS::ABI::SharedString;

struct SharedString::Impl {
	std::string data;
};

SharedString::SharedString() {
	m_impl = new Impl();
}

SharedString::~SharedString() {
	delete m_impl;
}

SharedString* SharedString::Copy() const {
	auto newStr = CreateSharedString();
	newStr->SetFromCString(GetCStringData(), GetSize());
	return newStr;
}

const char* SharedString::GetCStringData() const {
	return m_impl->data.data();
}

size_t SharedString::GetSize() const {
	return m_impl->data.size();
}

void SharedString::SetFromCString(const char* cstr, size_t size) {
	m_impl->data.assign(cstr, size);
}

extern "C" PECS_DLL_SHARED PameECS::ABI::SharedString* CreateSharedString() {
	return new PameECS::ABI::SharedString();
}

extern "C" PECS_DLL_SHARED void DestroySharedString(PameECS::ABI::SharedString* str) {
	delete str;
}

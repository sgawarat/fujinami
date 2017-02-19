#pragma once

#include <Windows.h>

#ifndef fujinami_hook_EXPORTS
#define FUJINAMI_HOOK_API __declspec(dllimport)
#else
#define FUJINAMI_HOOK_API __declspec(dllexport)
#endif

namespace fujinami_hook {
FUJINAMI_HOOK_API HHOOK Enable(int id, HOOKPROC proc, DWORD thread_id) noexcept;
FUJINAMI_HOOK_API void Disable(HHOOK hhk) noexcept;
}  // namespace fujinami

#include <fujinami_hook/fujinami_hook.hpp>

namespace fujinami_hook {
namespace {
HINSTANCE hmodule = NULL;
}  // namespace

FUJINAMI_HOOK_API
HHOOK Enable(int id, HOOKPROC proc, DWORD thread_id) noexcept {
  return SetWindowsHookEx(id, proc, hmodule, thread_id);
}

FUJINAMI_HOOK_API
void Disable(HHOOK hhk) noexcept { UnhookWindowsHookEx(hhk); }
}  // namespace fujinami_hook

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD ul_reason_for_call, LPVOID) {
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
      fujinami_hook::hmodule = hmodule;
    default:
      break;
  }
  return TRUE;
}

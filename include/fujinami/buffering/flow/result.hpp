#pragma once

namespace fujinami {
namespace buffering {
enum class FlowResult : uint8_t {
  CONTINUE,  // 処理を継続する
  DONE,      // 処理を終了する
};
}  // namespace buffering
}  // namespace fujinami

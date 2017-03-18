#include <fujinami/keyboard.hpp>
#include <fujinami/buffering/engine.hpp>
#include <fujinami/mapping/engine.hpp>

namespace fujinami {
Keyboard::Keyboard()
    : b_engine_(), b_context_(b_engine_), m_engine_(), m_context_(m_engine_) {}

Keyboard::~Keyboard() noexcept { close(); }

bool Keyboard::open() {
  if (!is_closed_) return true;

  b_thread_ = std::thread([this]() noexcept {
    using namespace buffering;
    logging::Logger::init_tls("B");
    while (true) {
      if (b_context_.is_closed()) break;
      if (b_engine_.is_idle()) {
        AnyEvent event;
        if (b_context_.receive_event(b_engine_.timeout_tp(), event)) {
          b_engine_.update(event, m_context_);
        } else {
          if (b_context_.is_closed()) break;
          b_engine_.update(m_context_);
        }
      } else {
        b_engine_.update(m_context_);
      }
      std::this_thread::yield();
    }
  });

  m_thread_ = std::thread([this]() noexcept {
    using namespace mapping;
    logging::Logger::init_tls("M");
    while (true) {
      if (m_context_.is_closed()) break;
      AnyEvent event;
      if (m_context_.receive_event(event)) {
        m_engine_.update(event);
      } else {
        if (m_context_.is_closed()) break;
      }
      std::this_thread::yield();
    }
  });

  is_closed_ = false;
  return true;
}

void Keyboard::close() noexcept {
  if (!is_closed_) {
    if (b_thread_.joinable()) {
      b_context_.close();
      b_thread_.join();

      b_context_.reset();
      b_engine_.reset();
    }
    if (m_thread_.joinable()) {
      m_context_.close();
      m_thread_.join();

      m_context_.reset();
      m_engine_.reset();
    }
    is_closed_ = true;
  }
}

bool Keyboard::send_event(const buffering::AnyEvent& event) noexcept {
  return b_context_.send_event(event);
}
}  // namespace fujinami

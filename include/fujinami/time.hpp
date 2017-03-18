#pragma once

#include "platform.hpp"
#if defined(FUJINAMI_PLATFORM_WIN32)
#include <fujinami_win32/time.hpp>
#elif defined(FUJINAMI_PLATFORM_LINUX)
#include <fujinami_linux/time.hpp>
#endif

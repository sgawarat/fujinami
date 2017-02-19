#pragma once

#include "platform/platform.hpp"
#if defined(FUJINAMI_PLATFORM_WIN32)
#include "platform/win32/time.hpp"
#elif defined(FUJINAMI_PLATFORM_LINUX)
#include "platform/linux/time.hpp"
#endif

#include "utility.hpp"

#include <config.h>

#if PLATFORM_WINDOWS
#    include <io.h>
#    define ISATTY _isatty
#    define STDOUT_FILENO 1
#else
#    include <unistd.h>
#    define ISATTY isatty
#endif

namespace porpoise {

auto is_tty() noexcept -> bool { return ISATTY(STDOUT_FILENO); }

} // namespace porpoise

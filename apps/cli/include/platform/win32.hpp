#pragma once

namespace porpoise::cli::win32 {

// Enables UTF8 on creation, disables on destruction
//
// Can be safely created and destroyed multiple times on multiple threads
class RichConsole {
  public:
#ifdef _WIN32
    RichConsole() noexcept;
    ~RichConsole() noexcept;
#endif
};

} // namespace porpoise::cli::win32

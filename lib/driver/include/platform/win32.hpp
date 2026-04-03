#pragma once

namespace porpoise::win32 {

// Enables UTF8 on creation, disables on destruction
//
// Can be safely created and destroyed multiple times on multiple threads
class RichConsole {
#ifdef _WIN32
  public:
    RichConsole() noexcept;
    ~RichConsole();
#endif
};

} // namespace porpoise::win32

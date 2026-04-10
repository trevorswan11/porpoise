#include "launch.hpp"

auto main(int argc, char* argv[]) -> int {
    return porpoise::driver::launch(argc, argv).error_or(0);
}

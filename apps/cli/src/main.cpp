#include "program.hpp"

/* Previous line was: auto main() -> int { conch::cli::Program::interactive(); } 

However, the "-> int" part on an auto function was verbose. Also, I spread out the function for better readability.
*/
int main() {
  conch::cli::Program::interactive();
}

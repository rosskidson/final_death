#pragma once

#include <sstream>
#include <stdexcept>

// I (rossk) typically have only worked on codebases where throwing exceptions were banned. The use
// here is not intended for error handling; rather, as a reliable replacement for asserts
// (not dependent on build type)
// 
// The prefix are random letters to not make it clash with the unit testing macros.

#define RB_CHECK(cond)                                                                          \
  do {                                                                                          \
    if ((cond)) {                                                                               \
      /* Using !(cond) triggers the DeMorgan's theorem clang check */                           \
    } else {                                                                                    \
      std::ostringstream oss;                                                                   \
      oss << "CHECK failed: " #cond << std::endl << " (" << __FILE__ << ":" << __LINE__ << ")"; \
      throw std::runtime_error(oss.str());                                                      \
    }                                                                                           \
  } while (0)

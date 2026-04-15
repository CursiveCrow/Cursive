#include "00_core/terminal.h"

#include <cstdlib>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace cursive::core {

bool IsColorEnabled(FILE* stream) {
  // NO_COLOR convention: https://no-color.org/
  const char* no_color = std::getenv("NO_COLOR");
  if (no_color != nullptr) {
    return false;
  }

#ifdef _WIN32
  int fd = _fileno(stream);
  if (fd < 0) {
    return false;
  }
  if (!_isatty(fd)) {
    return false;
  }
  // Enable virtual terminal processing on Windows 10+
  HANDLE h = INVALID_HANDLE_VALUE;
  if (stream == stderr) {
    h = GetStdHandle(STD_ERROR_HANDLE);
  } else if (stream == stdout) {
    h = GetStdHandle(STD_OUTPUT_HANDLE);
  }
  if (h != INVALID_HANDLE_VALUE && h != nullptr) {
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode)) {
      SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
  }
  return true;
#else
  int fd = fileno(stream);
  if (fd < 0) {
    return false;
  }
  return isatty(fd) != 0;
#endif
}

bool IsColorEnabledWithOverride(FILE* stream, ColorOverride override_mode) {
  switch (override_mode) {
    case ColorOverride::ForceOn:
      return true;
    case ColorOverride::ForceOff:
      return false;
    case ColorOverride::Auto:
      return IsColorEnabled(stream);
  }
  return IsColorEnabled(stream);
}

int TerminalWidth() {
#ifdef _WIN32
  HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
  if (h != INVALID_HANDLE_VALUE && h != nullptr) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(h, &info)) {
      return static_cast<int>(info.dwSize.X);
    }
  }
#else
  struct winsize w;
  if (ioctl(STDERR_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
    return static_cast<int>(w.ws_col);
  }
#endif
  // Fallback: check COLUMNS environment variable
  const char* columns = std::getenv("COLUMNS");
  if (columns != nullptr) {
    int val = std::atoi(columns);
    if (val > 0) {
      return val;
    }
  }
  return 0;
}

std::string_view ColorCode(Color color, bool enabled) {
  if (!enabled) {
    return "";
  }
  switch (color) {
    case Color::Reset:
      return "\033[0m";
    case Color::Red:
      return "\033[31m";
    case Color::Green:
      return "\033[32m";
    case Color::Yellow:
      return "\033[33m";
    case Color::Blue:
      return "\033[34m";
    case Color::Magenta:
      return "\033[35m";
    case Color::Cyan:
      return "\033[36m";
    case Color::White:
      return "\033[37m";
    case Color::BoldRed:
      return "\033[1;31m";
    case Color::BoldGreen:
      return "\033[1;32m";
    case Color::BoldYellow:
      return "\033[1;33m";
    case Color::BoldBlue:
      return "\033[1;34m";
    case Color::BoldMagenta:
      return "\033[1;35m";
    case Color::BoldCyan:
      return "\033[1;36m";
    case Color::BoldWhite:
      return "\033[1;37m";
  }
  return "";
}

std::string Colorize(std::string_view text, Color color, bool enabled) {
  if (!enabled) {
    return std::string(text);
  }
  std::string out;
  const auto code = ColorCode(color, true);
  const auto reset = ColorCode(Color::Reset, true);
  out.reserve(code.size() + text.size() + reset.size());
  out.append(code);
  out.append(text);
  out.append(reset);
  return out;
}

}  // namespace cursive::core

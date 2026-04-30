#include "06_driver/lsp/server.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "06_driver/version.h"

namespace {

void PrintUsage() {
  std::cout << "Cursive language server\n"
            << "\n"
            << "USAGE\n"
            << "  Cursive_LSP [--stdio] [--log-file <path>]\n"
            << "  Cursive_LSP --version\n"
            << "\n"
            << "OPTIONS\n"
            << "  --stdio           Run LSP over standard input/output (default)\n"
            << "  --log-file <path> Write server lifecycle logs to <path>\n"
            << "  --version         Print version and exit\n"
            << "  -h, --help        Show this help message\n";
}

}  // namespace

int main(int argc, char** argv) {
  cursive::driver::lsp::LspServerOptions options;

  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg == "--stdio") {
      continue;
    }
    if (arg == "--version") {
      std::cout << cursive::driver::GetVersionString()
                << " language server\n";
      return 0;
    }
    if (arg == "-h" || arg == "--help") {
      PrintUsage();
      return 0;
    }
    if (arg == "--log-file") {
      if (i + 1 >= argc) {
        std::cerr << "Cursive_LSP: --log-file requires a path\n";
        return 2;
      }
      options.log_file = std::filesystem::path(argv[++i]);
      continue;
    }
    std::cerr << "Cursive_LSP: unknown option: " << arg << "\n";
    return 2;
  }

  cursive::driver::lsp::LspServer server(std::move(options));
  return server.Run();
}

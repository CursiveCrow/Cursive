#include "cursive0/00_core/spec_trace.h"

#include <fstream>
#include <mutex>
#include <sstream>

#include "cursive0/00_core/path.h"

namespace cursive0::core {

namespace {

struct TraceState {
  std::mutex mutex;
  std::ofstream out;
  std::string domain;
  std::string phase;
  std::string root;
  bool enabled = false;
};

TraceState& State() {
  static TraceState state;
  return state;
}

std::string EncodePayload(std::string_view payload) {
  std::string out;
  out.reserve(payload.size() + 8);
  for (char c : payload) {
    switch (c) {
      case '\t':
        out += "%09";
        break;
      case '\n':
        out += "%0A";
        break;
      case '%':
        out += "%25";
        break;
      case ';':
        out += "%3B";
        break;
      case '=':
        out += "%3D";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

std::string RelPath(std::string_view path, std::string_view root) {
  const std::string norm = Normalize(path);
  if (root.empty()) {
    return norm.empty() ? "-" : norm;
  }
  const auto rel = Relative(norm, root);
  if (!rel.has_value() || rel->empty()) {
    return norm.empty() ? "-" : norm;
  }
  return *rel;
}

}  // namespace

void SpecTrace::Init(const std::string& path, std::string_view domain) {
  auto& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  state.out.close();
  state.out.clear();
  state.out.open(path, std::ios::binary | std::ios::trunc);
  if (!state.out) {
    state.enabled = false;
    return;
  }
  state.domain = std::string(domain);
  state.phase = "";
  state.enabled = true;
  state.out << "spec_trace_v1\n";
  state.out.flush();
}

void SpecTrace::SetRoot(const std::string& root) {
  auto& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  state.root = Normalize(root);
}

void SpecTrace::SetPhase(std::string_view phase) {
  auto& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  state.phase = std::string(phase);
}

void SpecTrace::Record(std::string_view rule_id,
                       const std::optional<Span>& span,
                       std::string_view payload) {
  auto& state = State();
  std::lock_guard<std::mutex> lock(state.mutex);
  if (!state.enabled) {
    return;
  }
  const std::string file =
      span.has_value() ? RelPath(span->file, state.root) : "-";
  const std::size_t start_line = span.has_value() ? span->start_line : 0;
  const std::size_t start_col = span.has_value() ? span->start_col : 0;
  const std::size_t end_line = span.has_value() ? span->end_line : 0;
  const std::size_t end_col = span.has_value() ? span->end_col : 0;
  const std::string encoded = EncodePayload(payload);

  state.out << state.domain << '\t'
            << (state.phase.empty() ? "-" : state.phase) << '\t'
            << rule_id << '\t'
            << file << '\t'
            << start_line << '\t'
            << start_col << '\t'
            << end_line << '\t'
            << end_col << '\t'
            << encoded << '\n';
  state.out.flush();
}

void SpecTrace::Record(std::string_view rule_id) {
  Record(rule_id, std::nullopt, "");
}

bool SpecTrace::Enabled() {
  return State().enabled;
}

}  // namespace cursive0::core

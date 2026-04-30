#include "06_driver/lsp/server.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "00_core/source_load.h"
#include "06_driver/lsp/diagnostic_adapter.h"
#include "06_driver/lsp/protocol.h"
#include "06_driver/lsp/semantic_tokens.h"
#include "06_driver/tooling/line_index.h"
#include "06_driver/tooling/uri.h"

namespace cursive::driver::lsp {

namespace {

llvm::json::Object JsonRpcBase() {
  llvm::json::Object out;
  out["jsonrpc"] = "2.0";
  return out;
}

std::optional<std::string> TextDocumentUri(const llvm::json::Object* params) {
  if (params == nullptr) {
    return std::nullopt;
  }
  const auto* text_document = params->getObject("textDocument");
  if (text_document == nullptr) {
    return std::nullopt;
  }
  return GetString(*text_document, "uri");
}

std::optional<tooling::LinePosition> PositionParam(
    const llvm::json::Object* params) {
  if (params == nullptr) {
    return std::nullopt;
  }
  const auto* position = params->getObject("position");
  if (position == nullptr) {
    return std::nullopt;
  }
  const auto line = GetInteger(*position, "line");
  const auto character = GetInteger(*position, "character");
  if (!line.has_value() || !character.has_value() || *line < 0 ||
      *character < 0) {
    return std::nullopt;
  }
  return tooling::LinePosition{static_cast<std::size_t>(*line),
                               static_cast<std::size_t>(*character)};
}

bool IsIdentByte(unsigned char ch) {
  return std::isalnum(ch) != 0 || ch == '_' || ch >= 0x80;
}

struct WordRange {
  std::size_t start = 0;
  std::size_t end = 0;
  std::string text;
};

std::optional<WordRange> WordAt(std::string_view text, std::size_t offset) {
  if (text.empty()) {
    return std::nullopt;
  }
  std::size_t pos = std::min(offset, text.size() - 1);
  if (!IsIdentByte(static_cast<unsigned char>(text[pos])) && pos > 0 &&
      IsIdentByte(static_cast<unsigned char>(text[pos - 1]))) {
    --pos;
  }
  if (!IsIdentByte(static_cast<unsigned char>(text[pos]))) {
    return std::nullopt;
  }
  std::size_t start = pos;
  while (start > 0 &&
         IsIdentByte(static_cast<unsigned char>(text[start - 1]))) {
    --start;
  }
  std::size_t end = pos + 1;
  while (end < text.size() &&
         IsIdentByte(static_cast<unsigned char>(text[end]))) {
    ++end;
  }
  return WordRange{start, end, std::string(text.substr(start, end - start))};
}

llvm::json::Object LocationForSymbol(const analysis::LanguageSymbolInfo& symbol,
                                     const std::string& text) {
  tooling::LineIndex index(text);
  llvm::json::Object location;
  location["uri"] = tooling::PathToFileUri(symbol.selection_range.file);
  location["range"] = RangeJson(index.RangeFor(symbol.selection_range));
  return location;
}

bool ContainsText(std::string_view haystack, std::string_view needle) {
  return haystack.find(needle) != std::string_view::npos;
}

std::string LowerAscii(std::string_view text) {
  std::string out(text);
  for (char& ch : out) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return out;
}

bool NameMatchesQuery(std::string_view name, std::string_view query) {
  if (query.empty()) {
    return true;
  }
  return ContainsText(LowerAscii(name), LowerAscii(query));
}

llvm::json::Object DiagnosticAtStart(std::string message) {
  llvm::json::Object diagnostic;
  diagnostic["range"] = RangeJson(tooling::LineRange{});
  diagnostic["severity"] = static_cast<std::int64_t>(1);
  diagnostic["source"] = "cursive";
  diagnostic["message"] = std::move(message);
  return diagnostic;
}

int CompletionKindForSymbol(analysis::LanguageSymbolKind kind) {
  switch (kind) {
    case analysis::LanguageSymbolKind::Function:
      return 3;
    case analysis::LanguageSymbolKind::Method:
      return 2;
    case analysis::LanguageSymbolKind::Record:
    case analysis::LanguageSymbolKind::Class:
      return 7;
    case analysis::LanguageSymbolKind::Enum:
      return 13;
    case analysis::LanguageSymbolKind::Field:
      return 5;
    case analysis::LanguageSymbolKind::EnumMember:
      return 20;
    case analysis::LanguageSymbolKind::Variable:
    case analysis::LanguageSymbolKind::Parameter:
      return 6;
    case analysis::LanguageSymbolKind::Constant:
      return 21;
    case analysis::LanguageSymbolKind::Modal:
    case analysis::LanguageSymbolKind::TypeAlias:
    case analysis::LanguageSymbolKind::State:
      return 25;
    case analysis::LanguageSymbolKind::Module:
      return 9;
  }
  return 1;
}

llvm::json::Object TextEdit(const tooling::LineRange& range,
                            std::string new_text) {
  llvm::json::Object edit;
  edit["range"] = RangeJson(range);
  edit["newText"] = std::move(new_text);
  return edit;
}

std::optional<tooling::LineRange> RangeFromJson(
    const llvm::json::Object* range) {
  if (range == nullptr) {
    return std::nullopt;
  }
  const auto* start = range->getObject("start");
  const auto* end = range->getObject("end");
  if (start == nullptr || end == nullptr) {
    return std::nullopt;
  }
  const auto start_line = GetInteger(*start, "line");
  const auto start_character = GetInteger(*start, "character");
  const auto end_line = GetInteger(*end, "line");
  const auto end_character = GetInteger(*end, "character");
  if (!start_line.has_value() || !start_character.has_value() ||
      !end_line.has_value() || !end_character.has_value() ||
      *start_line < 0 || *start_character < 0 || *end_line < 0 ||
      *end_character < 0) {
    return std::nullopt;
  }
  return tooling::LineRange{
      tooling::LinePosition{static_cast<std::size_t>(*start_line),
                            static_cast<std::size_t>(*start_character)},
      tooling::LinePosition{static_cast<std::size_t>(*end_line),
                            static_cast<std::size_t>(*end_character)},
  };
}

}  // namespace

LspServer::LspServer(LspServerOptions options)
    : server_options_(std::move(options)) {
  if (server_options_.log_file.has_value()) {
    log_.open(*server_options_.log_file, std::ios::app);
  }
}

int LspServer::Run() {
  Log("server start");
  while (running_) {
    std::optional<llvm::json::Value> message = rpc_.ReadMessage();
    if (!message.has_value()) {
      break;
    }
    const auto* object = message->getAsObject();
    if (object == nullptr) {
      continue;
    }
    HandleMessage(*object);
  }
  Log("server stop");
  return shutdown_requested_ ? 0 : 1;
}

void LspServer::HandleMessage(const llvm::json::Object& message) {
  const auto method = message.getString("method");
  if (!method.has_value()) {
    return;
  }

  if (const auto* id = message.get("id")) {
    HandleRequest(message, *id, *method);
  } else {
    HandleNotification(message, *method);
  }
}

void LspServer::HandleRequest(const llvm::json::Object& message,
                              const llvm::json::Value& id,
                              llvm::StringRef method) {
  const auto* params = message.getObject("params");
  if (method == "initialize") {
    SendResponse(id, HandleInitialize(params));
    return;
  }
  if (method == "shutdown") {
    shutdown_requested_ = true;
    SendResponse(id, nullptr);
    return;
  }
  if (method == "textDocument/documentSymbol") {
    SendResponse(id, HandleDocumentSymbol(params));
    return;
  }
  if (method == "textDocument/hover") {
    SendResponse(id, HandleHover(params));
    return;
  }
  if (method == "textDocument/definition") {
    SendResponse(id, HandleDefinition(params));
    return;
  }
  if (method == "textDocument/semanticTokens/full") {
    SendResponse(id, HandleSemanticTokens(params));
    return;
  }
  if (method == "workspace/symbol") {
    SendResponse(id, HandleWorkspaceSymbol(params));
    return;
  }
  if (method == "textDocument/documentHighlight") {
    SendResponse(id, HandleDocumentHighlight(params));
    return;
  }
  if (method == "textDocument/references") {
    SendResponse(id, HandleReferences(params));
    return;
  }
  if (method == "textDocument/completion") {
    SendResponse(id, HandleCompletion(params));
    return;
  }
  if (method == "textDocument/codeAction") {
    SendResponse(id, HandleCodeAction(params));
    return;
  }

  SendError(id, -32601, "Method not found");
}

void LspServer::HandleNotification(const llvm::json::Object& message,
                                   llvm::StringRef method) {
  const auto* params = message.getObject("params");
  if (method == "initialized") {
    AnalyzeAndPublish();
    return;
  }
  if (method == "exit") {
    running_ = false;
    return;
  }
  if (method == "textDocument/didOpen") {
    DidOpen(params);
    return;
  }
  if (method == "textDocument/didChange") {
    DidChange(params);
    return;
  }
  if (method == "textDocument/didSave") {
    DidSave(params);
    return;
  }
  if (method == "textDocument/didClose") {
    DidClose(params);
    return;
  }
  if (method == "workspace/didChangeWatchedFiles") {
    DidChangeWatchedFiles();
    return;
  }
}

llvm::json::Value LspServer::HandleInitialize(
    const llvm::json::Object* params) {
  workspace_roots_.clear();
  if (params != nullptr) {
    if (const auto folders = params->getArray("workspaceFolders")) {
      for (const auto& folder : *folders) {
        if (const auto* first = folder.getAsObject()) {
          if (const auto uri = GetString(*first, "uri")) {
            if (const auto path = tooling::FileUriToPath(*uri)) {
              workspace_roots_.push_back(tooling::NormalizePath(*path));
            }
          }
        }
      }
    } else if (const auto root_uri = GetString(*params, "rootUri")) {
      if (const auto path = tooling::FileUriToPath(*root_uri)) {
        workspace_roots_.push_back(tooling::NormalizePath(*path));
      }
    } else if (const auto root_path = GetString(*params, "rootPath")) {
      workspace_roots_.push_back(tooling::NormalizePath(*root_path));
    }
  }
  if (workspace_roots_.empty()) {
    workspace_roots_.push_back(tooling::NormalizePath(std::filesystem::current_path()));
  }

  llvm::json::Array token_types;
  for (const auto& token_type : SemanticTokenTypes()) {
    token_types.emplace_back(token_type);
  }
  llvm::json::Object legend;
  legend["tokenTypes"] = std::move(token_types);
  legend["tokenModifiers"] = llvm::json::Array();

  llvm::json::Object semantic_tokens;
  semantic_tokens["legend"] = std::move(legend);
  semantic_tokens["full"] = true;

  llvm::json::Object capabilities;
  capabilities["textDocumentSync"] = 1;
  capabilities["hoverProvider"] = true;
  capabilities["definitionProvider"] = true;
  capabilities["documentSymbolProvider"] = true;
  capabilities["workspaceSymbolProvider"] = true;
  capabilities["documentHighlightProvider"] = true;
  capabilities["referencesProvider"] = true;
  capabilities["completionProvider"] = llvm::json::Object{
      {"resolveProvider", false},
      {"triggerCharacters", llvm::json::Array{".", ":", "~", " "}},
  };
  capabilities["codeActionProvider"] = llvm::json::Object{
      {"codeActionKinds", llvm::json::Array{"quickfix"}},
  };
  capabilities["semanticTokensProvider"] = std::move(semantic_tokens);

  llvm::json::Object server_info;
  server_info["name"] = "cursive-lsp";

  llvm::json::Object result;
  result["capabilities"] = std::move(capabilities);
  result["serverInfo"] = std::move(server_info);
  return result;
}

llvm::json::Value LspServer::HandleDocumentSymbol(
    const llvm::json::Object* params) {
  const auto path = PathFromTextDocument(params);
  if (!path.has_value()) {
    return llvm::json::Array();
  }
  const auto* project = SnapshotForPath(*path);
  if (project == nullptr) {
    return llvm::json::Array();
  }
  const std::string text = TextForPath(*path);
  tooling::LineIndex index(text);
  llvm::json::Array symbols;
  for (const auto* symbol :
       project->snapshot.language_service.SymbolsInFile(*path)) {
    if (!symbol->include_in_outline) {
      continue;
    }
    llvm::json::Object item;
    item["name"] = symbol->name;
    if (!symbol->detail.empty()) {
      item["detail"] = symbol->detail;
    }
    item["kind"] = static_cast<std::int64_t>(SymbolKindToLsp(symbol->kind));
    item["range"] = RangeJson(index.RangeFor(symbol->range));
    item["selectionRange"] = RangeJson(index.RangeFor(symbol->selection_range));
    symbols.emplace_back(std::move(item));
  }
  return symbols;
}

llvm::json::Value LspServer::HandleHover(const llvm::json::Object* params) {
  const auto path = PathFromTextDocument(params);
  const auto position = PositionParam(params);
  if (!path.has_value() || !position.has_value()) {
    return nullptr;
  }
  const std::string text = TextForPath(*path);
  tooling::LineIndex index(text);
  const std::size_t offset = index.ByteOffsetAt(*position);
  const auto* project = SnapshotForPath(*path);
  if (project == nullptr) {
    return nullptr;
  }
  const auto* symbol =
      project->snapshot.language_service.ResolvedSymbolAt(*path, offset);
  const analysis::TypeRef hover_type =
      symbol != nullptr && symbol->type
          ? symbol->type
          : analysis::LanguageServiceTypeAt(project->snapshot.expr_types, *path,
                                            offset);
  if (symbol == nullptr && !hover_type) {
    return nullptr;
  }
  const auto* reference =
      project->snapshot.language_service.ReferenceAt(*path, offset);
  const auto word = WordAt(text, offset);

  std::string markdown = "```cursive\n";
  if (symbol != nullptr) {
    markdown += analysis::LanguageSymbolKindName(symbol->kind);
    markdown += " ";
    markdown += symbol->qualified_name;
  } else {
    markdown += "type";
  }
  if (hover_type) {
    markdown += ": ";
    markdown += analysis::TypeToString(hover_type);
  }
  markdown += "\n```";
  if (symbol != nullptr && !symbol->documentation.empty()) {
    markdown += "\n";
    markdown += symbol->documentation;
  }

  llvm::json::Object contents;
  contents["kind"] = "markdown";
  contents["value"] = markdown;

  llvm::json::Object result;
  result["contents"] = std::move(contents);
  if (reference != nullptr) {
    result["range"] = RangeJson(index.RangeFor(reference->range));
  } else if (word.has_value()) {
    result["range"] = RangeJson(tooling::LineRange{
        index.PositionAt(word->start),
        index.PositionAt(word->end),
    });
  }
  return result;
}

llvm::json::Value LspServer::HandleDefinition(
    const llvm::json::Object* params) {
  const auto path = PathFromTextDocument(params);
  const auto position = PositionParam(params);
  if (!path.has_value() || !position.has_value()) {
    return nullptr;
  }
  const std::string text = TextForPath(*path);
  tooling::LineIndex index(text);
  const std::size_t offset = index.ByteOffsetAt(*position);
  const auto* project = SnapshotForPath(*path);
  if (project == nullptr) {
    return nullptr;
  }
  const auto* symbol =
      project->snapshot.language_service.ResolvedSymbolAt(*path, offset);
  if (symbol == nullptr) {
    return nullptr;
  }
  return LocationForSymbol(*symbol, TextForPath(symbol->selection_range.file));
}

llvm::json::Value LspServer::HandleSemanticTokens(
    const llvm::json::Object* params) {
  const auto path = PathFromTextDocument(params);
  if (!path.has_value()) {
    llvm::json::Object empty;
    empty["data"] = llvm::json::Array();
    return empty;
  }
  const auto* project = SnapshotForPath(*path);
  if (project == nullptr) {
    llvm::json::Object empty;
    empty["data"] = llvm::json::Array();
    return empty;
  }
  return SemanticTokensFull(*path, TextForPath(*path), project->snapshot);
}

llvm::json::Value LspServer::HandleWorkspaceSymbol(
    const llvm::json::Object* params) {
  const std::string query =
      params != nullptr ? GetString(*params, "query").value_or("") : "";
  llvm::json::Array results;
  for (const auto& [_, project] : projects_by_root_) {
    for (const auto& symbol : project.snapshot.language_service.Symbols()) {
      if (!symbol.include_in_workspace) {
        continue;
      }
      if (!NameMatchesQuery(symbol.name, query) &&
          !NameMatchesQuery(symbol.qualified_name, query)) {
        continue;
      }
      llvm::json::Object item;
      item["name"] = symbol.name;
      item["kind"] = static_cast<std::int64_t>(SymbolKindToLsp(symbol.kind));
      if (!symbol.module_path.empty()) {
        item["containerName"] = symbol.module_path;
      }
      item["location"] =
          LocationForSymbol(symbol, TextForPath(symbol.selection_range.file));
      results.emplace_back(std::move(item));
    }
  }
  return results;
}

llvm::json::Value LspServer::HandleDocumentHighlight(
    const llvm::json::Object* params) {
  const auto path = PathFromTextDocument(params);
  const auto position = PositionParam(params);
  if (!path.has_value() || !position.has_value()) {
    return llvm::json::Array();
  }
  const std::string text = TextForPath(*path);
  tooling::LineIndex index(text);
  const std::size_t offset = index.ByteOffsetAt(*position);
  const auto* project = SnapshotForPath(*path);
  if (project == nullptr) {
    return llvm::json::Array();
  }
  const auto* reference =
      project->snapshot.language_service.ReferenceAt(*path, offset);
  const auto* symbol =
      project->snapshot.language_service.ResolvedSymbolAt(*path, offset);
  if (symbol == nullptr) {
    return llvm::json::Array();
  }
  llvm::json::Array highlights;
  for (const auto* ref :
       project->snapshot.language_service.ReferencesForSymbol(symbol->id, true)) {
    if (tooling::PathKey(ref->range.file) != tooling::PathKey(*path)) {
      continue;
    }
    llvm::json::Object highlight;
    highlight["range"] = RangeJson(index.RangeFor(ref->range));
    highlight["kind"] = static_cast<std::int64_t>(1);
    highlights.emplace_back(std::move(highlight));
  }
  if (highlights.empty() && reference != nullptr) {
    llvm::json::Object highlight;
    highlight["range"] = RangeJson(index.RangeFor(reference->range));
    highlight["kind"] = static_cast<std::int64_t>(1);
    highlights.emplace_back(std::move(highlight));
  }
  return highlights;
}

llvm::json::Value LspServer::HandleReferences(
    const llvm::json::Object* params) {
  const auto path = PathFromTextDocument(params);
  const auto position = PositionParam(params);
  if (!path.has_value() || !position.has_value()) {
    return llvm::json::Array();
  }
  const auto* project = SnapshotForPath(*path);
  if (project == nullptr) {
    return llvm::json::Array();
  }
  const std::string text = TextForPath(*path);
  tooling::LineIndex local_index(text);
  const std::size_t offset = local_index.ByteOffsetAt(*position);
  const auto* symbol =
      project->snapshot.language_service.ResolvedSymbolAt(*path, offset);
  if (symbol == nullptr) {
    return llvm::json::Array();
  }

  bool include_declaration = true;
  if (params != nullptr) {
    const auto* context = params->getObject("context");
    if (context != nullptr) {
      if (const auto value = context->getBoolean("includeDeclaration")) {
        include_declaration = *value;
      }
    }
  }

  llvm::json::Array locations;
  for (const auto* ref :
       project->snapshot.language_service.ReferencesForSymbol(
           symbol->id, include_declaration)) {
    const std::filesystem::path file(ref->range.file);
    const std::string file_text = TextForPath(file);
    tooling::LineIndex index(file_text);
    llvm::json::Object location;
    location["uri"] = tooling::PathToFileUri(file);
    location["range"] = RangeJson(index.RangeFor(ref->range));
    locations.emplace_back(std::move(location));
  }
  return locations;
}

llvm::json::Value LspServer::HandleCompletion(
    const llvm::json::Object* params) {
  const auto path = PathFromTextDocument(params);
  const auto* project = path.has_value() ? SnapshotForPath(*path) : nullptr;
  llvm::json::Array items;

  static constexpr std::string_view kKeywords[] = {
      "public", "internal", "private", "procedure", "record", "class",
      "modal",  "enum",     "let",     "var",       "return", "move",
      "const",  "shared",   "unique",  "if",        "is",     "else",
      "loop",   "break",    "continue", "unsafe",   "extern", "using",
      "import",
  };
  for (const auto keyword : kKeywords) {
    llvm::json::Object item;
    item["label"] = std::string(keyword);
    item["kind"] = static_cast<std::int64_t>(14);
    items.emplace_back(std::move(item));
  }

  if (project != nullptr) {
    std::vector<const analysis::LanguageSymbolInfo*> completion_symbols;
    if (path.has_value()) {
      if (const auto position = PositionParam(params)) {
        const std::string text = TextForPath(*path);
        tooling::LineIndex index(text);
        completion_symbols = project->snapshot.language_service.CompletionSymbols(
            *path, index.ByteOffsetAt(*position));
      }
    }
    if (completion_symbols.empty()) {
      for (const auto& symbol : project->snapshot.language_service.Symbols()) {
        completion_symbols.push_back(&symbol);
      }
    }
    std::unordered_set<std::string> seen;
    for (const auto* symbol : completion_symbols) {
      if (symbol == nullptr || !seen.insert(symbol->name).second) {
        continue;
      }
      llvm::json::Object item;
      item["label"] = symbol->name;
      item["kind"] =
          static_cast<std::int64_t>(CompletionKindForSymbol(symbol->kind));
      item["detail"] = symbol->detail.empty()
                           ? analysis::LanguageSymbolKindName(symbol->kind)
                           : symbol->detail;
      items.emplace_back(std::move(item));
    }
  }

  llvm::json::Object result;
  result["isIncomplete"] = false;
  result["items"] = std::move(items);
  return result;
}

llvm::json::Value LspServer::HandleCodeAction(
    const llvm::json::Object* params) {
  if (params == nullptr) {
    return llvm::json::Array();
  }
  const auto uri = TextDocumentUri(params);
  if (!uri.has_value()) {
    return llvm::json::Array();
  }
  const auto* context = params->getObject("context");
  const auto* diagnostics =
      context != nullptr ? context->getArray("diagnostics") : nullptr;
  if (diagnostics == nullptr) {
    return llvm::json::Array();
  }

  llvm::json::Array actions;
  for (const auto& diag_value : *diagnostics) {
    const auto* diag = diag_value.getAsObject();
    if (diag == nullptr) {
      continue;
    }
    const auto* data = diag->getObject("data");
    const auto* fixits = data != nullptr ? data->getArray("fixits") : nullptr;
    if (fixits == nullptr) {
      continue;
    }
    for (const auto& fixit_value : *fixits) {
      const auto* fixit = fixit_value.getAsObject();
      if (fixit == nullptr) {
        continue;
      }
      const auto title = GetString(*fixit, "title").value_or("Apply fix");
      const auto fix_uri = GetString(*fixit, "uri").value_or(*uri);
      const auto new_text = GetString(*fixit, "newText");
      const auto range = RangeFromJson(fixit->getObject("range"));
      if (!new_text.has_value() || !range.has_value()) {
        continue;
      }

      llvm::json::Object edit;
      edit["changes"] = llvm::json::Object{
          {fix_uri, llvm::json::Array{TextEdit(*range, *new_text)}},
      };

      llvm::json::Object action;
      action["title"] = title;
      action["kind"] = "quickfix";
      action["edit"] = std::move(edit);
      actions.emplace_back(std::move(action));
    }
  }
  return actions;
}

void LspServer::DidOpen(const llvm::json::Object* params) {
  if (params == nullptr) {
    return;
  }
  const auto* text_document = params->getObject("textDocument");
  if (text_document == nullptr) {
    return;
  }
  const auto uri = GetString(*text_document, "uri");
  const auto text = GetString(*text_document, "text");
  if (!uri.has_value() || !text.has_value()) {
    return;
  }
  const auto path = tooling::FileUriToPath(*uri);
  if (!path.has_value()) {
    return;
  }
  const auto version = GetInteger(*text_document, "version").value_or(0);
  documents_.Open(*uri, *path, version, *text);
  AnalyzeProjectForPath(*path);
  PublishDiagnosticsForUri(*uri);
}

void LspServer::DidChange(const llvm::json::Object* params) {
  if (params == nullptr) {
    return;
  }
  const auto uri = TextDocumentUri(params);
  if (!uri.has_value()) {
    return;
  }
  const auto* text_document = params->getObject("textDocument");
  const auto version = text_document != nullptr
                           ? GetInteger(*text_document, "version").value_or(0)
                           : 0;
  const auto* changes = params->getArray("contentChanges");
  if (changes == nullptr || changes->empty()) {
    return;
  }
  const auto* last = changes->back().getAsObject();
  if (last == nullptr) {
    return;
  }
  const auto text = GetString(*last, "text");
  if (!text.has_value()) {
    return;
  }
  documents_.ChangeFull(*uri, version, *text);
  if (const auto path = tooling::FileUriToPath(*uri)) {
    AnalyzeProjectForPath(*path);
  }
  PublishDiagnosticsForUri(*uri);
}

void LspServer::DidSave(const llvm::json::Object* params) {
  if (params != nullptr) {
    const auto uri = TextDocumentUri(params);
    const auto text = GetString(*params, "text");
    if (uri.has_value() && text.has_value()) {
      if (const auto existing = documents_.FindByUri(*uri)) {
        documents_.ChangeFull(*uri, existing->version, *text);
      }
    }
  }
  if (const auto uri = TextDocumentUri(params)) {
    if (const auto path = tooling::FileUriToPath(*uri)) {
      AnalyzeProjectForPath(*path);
    }
  } else {
    AnalyzeAndPublish();
  }
  PublishDiagnosticsForOpenDocuments();
}

void LspServer::DidClose(const llvm::json::Object* params) {
  const auto uri = TextDocumentUri(params);
  if (!uri.has_value()) {
    return;
  }
  documents_.Close(*uri);
  llvm::json::Object publish;
  publish["uri"] = *uri;
  publish["diagnostics"] = llvm::json::Array();
  SendNotification("textDocument/publishDiagnostics", std::move(publish));
}

void LspServer::DidChangeWatchedFiles() {
  AnalyzeAndPublish();
}

void LspServer::AnalyzeAndPublish() {
  std::set<std::string> roots;
  for (const auto& overlay : documents_.Overlays()) {
    if (const auto root = ManifestRootForPath(overlay.path)) {
      roots.insert(tooling::PathKey(*root));
      AnalyzeProject(*root);
    }
  }
  PublishDiagnosticsForOpenDocuments();
}

void LspServer::AnalyzeProject(const std::filesystem::path& project_root) {
  tooling::ToolingAnalysisOptions options;
  options.project_root = tooling::NormalizePath(project_root);
  const auto overlays = OverlaysForRoot(options.project_root);
  ProjectSnapshot project;
  project.options = options;
  project.snapshot = tooling::AnalyzeWorkspace(options, overlays);
  projects_by_root_[tooling::PathKey(options.project_root)] = std::move(project);
  Log("analyzed " + options.project_root.generic_string());
}

void LspServer::AnalyzeProjectForPath(const std::filesystem::path& path) {
  const auto root = ManifestRootForPath(path);
  if (!root.has_value()) {
    Log("no Cursive.toml for " + tooling::NormalizePath(path).generic_string());
    return;
  }
  AnalyzeProject(*root);
}

void LspServer::PublishDiagnosticsForOpenDocuments() {
  for (const auto& overlay : documents_.Overlays()) {
    PublishDiagnosticsForUri(overlay.uri);
  }
}

void LspServer::PublishDiagnosticsForUri(const std::string& uri) {
  const auto path = tooling::FileUriToPath(uri);
  if (!path.has_value()) {
    return;
  }
  const auto* project = SnapshotForPath(*path);
  if (project == nullptr) {
    PublishNoManifestDiagnostic(uri, *path);
    return;
  }
  llvm::json::Object params;
  params["uri"] = uri;
  params["diagnostics"] =
      DiagnosticsForPath(project->snapshot, documents_, *path);
  SendNotification("textDocument/publishDiagnostics", std::move(params));
}

void LspServer::PublishNoManifestDiagnostic(
    const std::string& uri,
    const std::filesystem::path& path) {
  llvm::json::Object params;
  params["uri"] = uri;
  llvm::json::Array diagnostics;
  diagnostics.emplace_back(DiagnosticAtStart(
      "No Cursive.toml manifest found for " +
      tooling::NormalizePath(path).generic_string()));
  params["diagnostics"] = std::move(diagnostics);
  SendNotification("textDocument/publishDiagnostics", std::move(params));
}

void LspServer::SendResponse(const llvm::json::Value& id,
                             llvm::json::Value result) {
  llvm::json::Object response = JsonRpcBase();
  response["id"] = id;
  response["result"] = std::move(result);
  rpc_.WriteMessage(llvm::json::Value(std::move(response)));
}

void LspServer::SendError(const llvm::json::Value& id,
                          int code,
                          std::string message) {
  llvm::json::Object error;
  error["code"] = static_cast<std::int64_t>(code);
  error["message"] = std::move(message);

  llvm::json::Object response = JsonRpcBase();
  response["id"] = id;
  response["error"] = std::move(error);
  rpc_.WriteMessage(llvm::json::Value(std::move(response)));
}

void LspServer::SendNotification(std::string method, llvm::json::Value params) {
  llvm::json::Object notification = JsonRpcBase();
  notification["method"] = std::move(method);
  notification["params"] = std::move(params);
  rpc_.WriteMessage(llvm::json::Value(std::move(notification)));
}

void LspServer::Log(std::string_view message) {
  if (log_.is_open()) {
    log_ << message << '\n';
    log_.flush();
  }
}

std::optional<std::filesystem::path> LspServer::PathFromTextDocument(
    const llvm::json::Object* params) const {
  const auto uri = TextDocumentUri(params);
  if (!uri.has_value()) {
    return std::nullopt;
  }
  return tooling::FileUriToPath(*uri);
}

std::string LspServer::TextForPath(const std::filesystem::path& path) const {
  if (const auto overlay = documents_.FindByPath(path)) {
    std::vector<std::uint8_t> bytes(overlay->text_utf8.begin(),
                                    overlay->text_utf8.end());
    core::SourceLoadResult loaded =
        core::LoadSource(path.generic_string(), bytes);
    if (loaded.source.has_value()) {
      return loaded.source->text;
    }
    return overlay->text_utf8;
  }

  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return {};
  }
  std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
  core::SourceLoadResult loaded = core::LoadSource(path.generic_string(), bytes);
  if (!loaded.source.has_value()) {
    return {};
  }
  return loaded.source->text;
}

std::optional<std::filesystem::path> LspServer::ManifestRootForPath(
    const std::filesystem::path& path) const {
  std::filesystem::path current = tooling::NormalizePath(path);
  std::error_code ec;
  if (!std::filesystem::is_directory(current, ec)) {
    current = current.parent_path();
  }

  while (!current.empty()) {
    if (std::filesystem::exists(current / "Cursive.toml", ec) && !ec) {
      return tooling::NormalizePath(current);
    }
    if (current == current.root_path()) {
      break;
    }
    current = current.parent_path();
  }

  for (const auto& workspace_root : workspace_roots_) {
    if (std::filesystem::exists(workspace_root / "Cursive.toml", ec) && !ec) {
      return tooling::NormalizePath(workspace_root);
    }
  }
  return std::nullopt;
}

std::vector<tooling::DocumentOverlay> LspServer::OverlaysForRoot(
    const std::filesystem::path& root) const {
  std::vector<tooling::DocumentOverlay> overlays;
  const std::string root_key = tooling::PathKey(root);
  for (const auto& overlay : documents_.Overlays()) {
    const auto overlay_root = ManifestRootForPath(overlay.path);
    if (overlay_root.has_value() && tooling::PathKey(*overlay_root) == root_key) {
      overlays.push_back(overlay);
    }
  }
  return overlays;
}

LspServer::ProjectSnapshot* LspServer::SnapshotForPath(
    const std::filesystem::path& path) {
  const auto root = ManifestRootForPath(path);
  if (!root.has_value()) {
    return nullptr;
  }
  const std::string key = tooling::PathKey(*root);
  auto it = projects_by_root_.find(key);
  if (it == projects_by_root_.end()) {
    AnalyzeProject(*root);
    it = projects_by_root_.find(key);
  }
  return it == projects_by_root_.end() ? nullptr : &it->second;
}

const LspServer::ProjectSnapshot* LspServer::SnapshotForPath(
    const std::filesystem::path& path) const {
  const auto root = ManifestRootForPath(path);
  if (!root.has_value()) {
    return nullptr;
  }
  const auto it = projects_by_root_.find(tooling::PathKey(*root));
  return it == projects_by_root_.end() ? nullptr : &it->second;
}

}  // namespace cursive::driver::lsp

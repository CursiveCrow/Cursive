#include "cursive0/analysis/keys/key_context.h"

#include <algorithm>
#include <sstream>

#include "cursive0/core/assert_spec.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsKeyContext() {
  SPEC_DEF("KeyContext", "C0X.5.X");
  SPEC_DEF("HeldKey", "C0X.5.X");
  SPEC_DEF("KeyPath", "C0X.5.X");
  SPEC_DEF("Covers", "C0X.5.X");
  SPEC_DEF("Acquire", "C0X.5.X");
}

}  // namespace

std::string KeyPath::ToString() const {
  std::ostringstream oss;
  oss << root;
  for (const auto& seg : segs) {
    if (seg.boundary) {
      oss << ".#";
    } else {
      oss << ".";
    }
    if (seg.is_index) {
      oss << "[" << seg.name << "]";
    } else {
      oss << seg.name;
    }
  }
  return oss.str();
}

void KeyContext::PushScope() {
  SpecDefsKeyContext();
  SPEC_RULE("K-Push-Scope");
  scope_stack_.push_back(current_scope_);
  current_scope_ = scope_stack_.size();
}

void KeyContext::PopScope() {
  SpecDefsKeyContext();
  SPEC_RULE("K-Pop-Scope");
  if (!scope_stack_.empty()) {
    // Release all keys at current scope
    ReleaseScope();
    current_scope_ = scope_stack_.back();
    scope_stack_.pop_back();
  }
}

bool KeyContext::Acquire(const KeyPath& path, KeyAccessMode mode) {
  SpecDefsKeyContext();
  
  // Check if already covered
  if (Covers(path, mode)) {
    SPEC_RULE("K-Acquire-Covered");
    return false;  // Already covered, no acquisition needed
  }
  
  SPEC_RULE("K-Acquire-New");
  HeldKey key;
  key.path = path;
  key.mode = mode;
  key.scope = current_scope_;
  held_keys_.push_back(std::move(key));
  return true;
}

void KeyContext::ReleaseScope() {
  SpecDefsKeyContext();
  SPEC_RULE("K-Release-Scope");
  
  // Remove all keys at current scope
  held_keys_.erase(
      std::remove_if(held_keys_.begin(), held_keys_.end(),
                     [this](const HeldKey& k) {
                       return k.scope == current_scope_;
                     }),
      held_keys_.end());
}

bool KeyContext::Covers(const KeyPath& path, KeyAccessMode mode) const {
  SpecDefsKeyContext();
  SPEC_RULE("K-Covers");
  
  for (const auto& held : held_keys_) {
    // A held key covers the path if:
    // 1. The held path is a prefix of the requested path
    // 2. The held mode is >= the requested mode (Write covers both, Read covers Read)
    if (IsPrefix(held.path, path)) {
      if (held.mode == KeyAccessMode::Write) {
        return true;  // Write covers everything
      }
      if (mode == KeyAccessMode::Read) {
        return true;  // Read covers Read
      }
    }
  }
  return false;
}

KeyPath LowerKeyPath(const syntax::KeyPathExpr& ast_path) {
  SpecDefsKeyContext();
  SPEC_RULE("K-Lower-Path");
  
  KeyPath path;
  path.root = ast_path.root;
  
  for (const auto& seg : ast_path.segs) {
    KeyPathSeg lowered;
    if (const auto* field = std::get_if<syntax::KeySegField>(&seg)) {
      lowered.boundary = field->marked;
      lowered.name = field->name;
      lowered.is_index = false;
    } else if (const auto* index = std::get_if<syntax::KeySegIndex>(&seg)) {
      lowered.boundary = index->marked;
      // For now, represent index expression as string
      // More sophisticated handling would involve constant evaluation
      lowered.name = "[index]";
      lowered.is_index = true;
    }
    path.segs.push_back(std::move(lowered));
  }
  
  return path;
}

bool IsPrefix(const KeyPath& prefix, const KeyPath& path) {
  SpecDefsKeyContext();
  SPEC_RULE("K-IsPrefix");
  
  if (prefix.root != path.root) {
    return false;
  }
  
  if (prefix.segs.size() > path.segs.size()) {
    return false;
  }
  
  for (std::size_t i = 0; i < prefix.segs.size(); ++i) {
    const auto& p_seg = prefix.segs[i];
    const auto& path_seg = path.segs[i];
    
    // Check for boundary - traversal stops at boundary
    if (p_seg.boundary) {
      return true;  // Boundary marks end of key path derivation
    }
    
    if (p_seg.name != path_seg.name || p_seg.is_index != path_seg.is_index) {
      return false;
    }
  }
  
  return true;
}

bool PathsOverlap(const KeyPath& p1, const KeyPath& p2) {
  SpecDefsKeyContext();
  SPEC_RULE("K-PathsOverlap");
  
  // Two paths overlap if one is a prefix of the other
  return IsPrefix(p1, p2) || IsPrefix(p2, p1);
}

bool KeyPathLess(const KeyPath& lhs, const KeyPath& rhs) {
  SpecDefsKeyContext();
  SPEC_RULE("K-PathLess");
  
  // Lexicographic comparison
  if (lhs.root != rhs.root) {
    return lhs.root < rhs.root;
  }
  
  const std::size_t min_len = std::min(lhs.segs.size(), rhs.segs.size());
  for (std::size_t i = 0; i < min_len; ++i) {
    if (lhs.segs[i].name != rhs.segs[i].name) {
      return lhs.segs[i].name < rhs.segs[i].name;
    }
  }
  
  return lhs.segs.size() < rhs.segs.size();
}

}  // namespace cursive0::analysis

#include "cursive0/03_analysis/composite/classes.h"

#include <map>
#include <optional>
#include <set>
#include <vector>

#include "cursive0/00_core/assert_spec.h"
#include "cursive0/03_analysis/resolve/scopes.h"

namespace cursive0::analysis {

namespace {

static inline void SpecDefsClassLinearization() {
  SPEC_DEF("Supers", "5.3.1");
  SPEC_DEF("Linearize", "5.3.1");
  SPEC_DEF("Merge", "5.3.1");
  SPEC_DEF("Head", "5.3.1");
  SPEC_DEF("Tail", "5.3.1");
  SPEC_DEF("HeadOk", "5.3.1");
  SPEC_DEF("SelectHead", "5.3.1");
  SPEC_DEF("PopHead", "5.3.1");
  SPEC_DEF("PopAll", "5.3.1");
}

static const syntax::ClassDecl* LookupClassDecl(const ScopeContext& ctx,
                                                const syntax::ClassPath& path) {
  const auto it = ctx.sigma.classes.find(PathKeyOf(path));
  if (it == ctx.sigma.classes.end()) {
    return nullptr;
  }
  return &it->second;
}

using ClassList = std::vector<syntax::ClassPath>;
using ClassLists = std::vector<ClassList>;

static bool HeadOk(const syntax::ClassPath& head, const ClassLists& lists) {
  for (const auto& list : lists) {
    if (list.empty()) {
      continue;
    }
    for (std::size_t i = 1; i < list.size(); ++i) {
      if (PathEq(list[i], head)) {
        return false;
      }
    }
  }
  return true;
}

static std::optional<syntax::ClassPath> SelectHead(const ClassLists& lists) {
  for (const auto& list : lists) {
    if (list.empty()) {
      continue;
    }
    const auto& head = list.front();
    if (HeadOk(head, lists)) {
      return head;
    }
  }
  return std::nullopt;
}

static ClassLists PopAll(const syntax::ClassPath& head,
                         const ClassLists& lists) {
  ClassLists out;
  out.reserve(lists.size());
  for (const auto& list : lists) {
    if (!list.empty() && PathEq(list.front(), head)) {
      ClassList tail(list.begin() + 1, list.end());
      out.push_back(std::move(tail));
    } else {
      out.push_back(list);
    }
  }
  return out;
}

static bool AllEmpty(const ClassLists& lists) {
  for (const auto& list : lists) {
    if (!list.empty()) {
      return false;
    }
  }
  return true;
}

static LinearizationResult Merge(const ClassLists& lists) {
  SpecDefsClassLinearization();
  LinearizationResult result;

  ClassLists current = lists;
  while (true) {
    if (AllEmpty(current)) {
      SPEC_RULE("Merge-Empty");
      result.ok = true;
      return result;
    }
    const auto head_opt = SelectHead(current);
    if (!head_opt.has_value()) {
      SPEC_RULE("Merge-Fail");
      return result;
    }
    SPEC_RULE("Merge-Step");
    result.order.push_back(*head_opt);
    current = PopAll(*head_opt, current);
  }
}

struct LinearizeState {
  std::map<PathKey, LinearizationResult> memo;
  std::set<PathKey> active;
};

static LinearizationResult LinearizeImpl(const ScopeContext& ctx,
                                         const syntax::ClassPath& path,
                                         LinearizeState& state) {
  SpecDefsClassLinearization();
  const auto key = PathKeyOf(path);
  if (const auto it = state.memo.find(key); it != state.memo.end()) {
    return it->second;
  }
  if (state.active.find(key) != state.active.end()) {
    return {};
  }
  state.active.insert(key);

  LinearizationResult result;
  const auto* decl = LookupClassDecl(ctx, path);
  if (!decl) {
    result.diag_id = "Superclass-Undefined";
    state.active.erase(key);
    state.memo.emplace(key, result);
    return result;
  }

  if (decl->supers.empty()) {
    SPEC_RULE("Lin-Base");
    result.ok = true;
    result.order = {path};
    state.active.erase(key);
    state.memo.emplace(key, result);
    return result;
  }

  ClassLists lists;
  lists.reserve(decl->supers.size() + 1);
  for (const auto& super : decl->supers) {
    const auto linearized = LinearizeImpl(ctx, super, state);
    if (!linearized.ok) {
      state.active.erase(key);
      state.memo.emplace(key, linearized);
      return linearized;
    }
    lists.push_back(linearized.order);
  }
  lists.push_back(decl->supers);

  const auto merged = Merge(lists);
  if (!merged.ok) {
    SPEC_RULE("Lin-Fail");
    state.active.erase(key);
    state.memo.emplace(key, merged);
    return merged;
  }

  SPEC_RULE("Lin-Ok");
  result.ok = true;
  result.order.reserve(merged.order.size() + 1);
  result.order.push_back(path);
  result.order.insert(result.order.end(), merged.order.begin(), merged.order.end());

  state.active.erase(key);
  state.memo.emplace(key, result);
  return result;
}

}  // namespace

LinearizationResult LinearizeClass(const ScopeContext& ctx,
                                   const syntax::ClassPath& path) {
  SpecDefsClassLinearization();
  LinearizeState state;
  return LinearizeImpl(ctx, path, state);
}

}  // namespace cursive0::analysis

#include "cursive0/codegen/ir_dump.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace cursive0::codegen {

namespace {

struct Dumper {
  std::ostringstream oss;
  int indent_level = 0;

  void Indent() {
    for (int i = 0; i < indent_level; ++i) oss << "  ";
  }

  void DumpImmediateBytes(const IRValue& v) {
    std::ostringstream hex;
    hex << std::uppercase << std::hex;
    for (std::uint8_t b : v.bytes) {
      hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    std::string hex_str = hex.str();
    // Strip leading zeros for readability.
    auto it = std::find_if(hex_str.begin(), hex_str.end(), [](char c) { return c != '0'; });
    if (it == hex_str.end()) {
      oss << "0x0";
      return;
    }
    hex_str.erase(hex_str.begin(), it);
    oss << "0x" << hex_str;
  }

  void Dump(const IRValue& v) {
    switch (v.kind) {
      case IRValue::Kind::Opaque:
        if (v.name.empty()) {
          oss << "opaque";
        } else {
          oss << v.name;
        }
        break;
      case IRValue::Kind::Local:
        oss << "%" << v.name;
        break;
      case IRValue::Kind::Symbol:
        oss << "@" << v.name;
        break;
      case IRValue::Kind::Immediate:
        if (!v.bytes.empty()) {
          DumpImmediateBytes(v);
        } else {
          oss << v.name;
        }
        break;
    }
  }

  void DumpRange(const IRRange& range) {
    auto dump_opt = [this](const std::optional<IRValue>& v) {
      if (!v.has_value()) {
        oss << "?";
        return;
      }
      Dump(*v);
    };

    switch (range.kind) {
      case syntax::RangeKind::Full:
        oss << "..";
        return;
      case syntax::RangeKind::From:
        dump_opt(range.lo);
        oss << "..";
        return;
      case syntax::RangeKind::To:
        oss << "..";
        dump_opt(range.hi);
        return;
      case syntax::RangeKind::ToInclusive:
        oss << "..=";
        dump_opt(range.hi);
        return;
      case syntax::RangeKind::Exclusive:
        dump_opt(range.lo);
        oss << "..";
        dump_opt(range.hi);
        return;
      case syntax::RangeKind::Inclusive:
        dump_opt(range.lo);
        oss << "..=";
        dump_opt(range.hi);
        return;
    }
  }

  void DumpPattern(const syntax::Pattern& pattern) {
    std::visit(
        [this](const auto& pat) {
          using T = std::decay_t<decltype(pat)>;
          if constexpr (std::is_same_v<T, syntax::LiteralPattern>) {
            oss << "<literal " << pat.literal.lexeme << ">";
          } else if constexpr (std::is_same_v<T, syntax::WildcardPattern>) {
            oss << "_";
          } else if constexpr (std::is_same_v<T, syntax::IdentifierPattern>) {
            oss << pat.name;
          } else if constexpr (std::is_same_v<T, syntax::TypedPattern>) {
            oss << pat.name;
          } else {
            oss << "<pattern>";
          }
        },
        pattern.node);
  }

  void DumpCall(const IRCall& call) {
    oss << "call ";
    Dump(call.callee);
    if (!call.args.empty()) {
      oss << " (";
      for (std::size_t i = 0; i < call.args.size(); ++i) {
        if (i) oss << ", ";
        Dump(call.args[i]);
      }
      oss << ")";
    }
  }

  void Dump(const IRPtr& ir) {
    if (!ir) { oss << "null"; return; }
    std::visit([this](const auto& node) { DumpNode(node); }, ir->node);
  }

  void DumpNode(const IROpaque&) { oss << "nop"; }

  void DumpNode(const IRSeq& s) {
    oss << "seq {\n";
    indent_level++;
    for (std::size_t i = 0; i < s.items.size(); ++i) {
      const auto& item = s.items[i];
      if (!item) {
        Indent(); oss << "null\n";
        continue;
      }

      if (auto call = std::get_if<IRCall>(&item->node)) {
        if (i + 1 < s.items.size()) {
          const auto& next = s.items[i + 1];
          if (next) {
            if (auto bind = std::get_if<IRBindVar>(&next->node)) {
              const auto& val = bind->value;
              if (val.kind == IRValue::Kind::Opaque &&
                  (val.name == "call_result" || val.name == "method_call_result" ||
                   val.name == "dyncall_result")) {
                Indent();
                oss << "bind %" << bind->name << " = ";
                DumpCall(*call);
                oss << "\n";
                i++;
                continue;
              }
            }
          }
        }
      }

      if (auto addr = std::get_if<IRAddrOf>(&item->node)) {
        if (i + 1 < s.items.size()) {
          const auto& next = s.items[i + 1];
          if (next) {
            if (auto bind = std::get_if<IRBindVar>(&next->node)) {
              const auto& val = bind->value;
              if (val.kind == IRValue::Kind::Opaque && val.name == "addr_of") {
                Indent();
                oss << "bind %" << bind->name << " = addr_of";
                if (!addr->place.repr.empty()) {
                  oss << " " << addr->place.repr;
                }
                oss << "\n";
                i++;
                continue;
              }
            }
          }
        }
      }

      if (auto alloc = std::get_if<IRAlloc>(&item->node)) {
        if (i + 1 < s.items.size()) {
          const auto& next = s.items[i + 1];
          if (next) {
            if (auto bind = std::get_if<IRBindVar>(&next->node)) {
              const auto& val = bind->value;
              if (val.kind == IRValue::Kind::Opaque && val.name == "alloc_ptr") {
                Indent();
                oss << "bind %" << bind->name << " = alloc";
                if (alloc->region.has_value()) {
                  oss << " in ";
                  Dump(*alloc->region);
                }
                oss << "\n";
                i++;
                continue;
              }
            }
          }
        }
      }

      Indent();
      Dump(item);
      oss << "\n";
    }
    indent_level--;
    Indent();
    oss << "}";
  }

  void DumpNode(const IRReturn& r) {
    oss << "ret ";
    Dump(r.value);
  }

  void DumpNode(const IRBindVar& b) {
    oss << "bind %" << b.name << " = ";
    Dump(b.value);
  }

  void DumpNode(const IRCall& c) { DumpCall(c); }

  void DumpNode(const IRCallVTable& c) {
    oss << "call_vtable ";
    Dump(c.base);
    oss << " [" << c.slot << "]";
    if (!c.args.empty()) {
      oss << " (";
      for (std::size_t i = 0; i < c.args.size(); ++i) {
        if (i) oss << ", ";
        Dump(c.args[i]);
      }
      oss << ")";
    }
  }

  void DumpNode(const IRStoreGlobal& s) {
    oss << "store @" << s.symbol << " = ";
    Dump(s.value);
  }

  void DumpNode(const IRReadVar& r) { oss << "read %" << r.name; }

  void DumpNode(const IRReadPath& r) {
    oss << "read @";
    for (const auto& part : r.path) {
      oss << part << "::";
    }
    oss << r.name;
  }

  void DumpNode(const IRStoreVar& s) {
    oss << "store %" << s.name << " = ";
    Dump(s.value);
  }

  void DumpNode(const IRStoreVarNoDrop& s) {
    oss << "store_nodrop %" << s.name << " = ";
    Dump(s.value);
  }

  void DumpNode(const IRReadPlace& r) { oss << "read_place " << r.place.repr; }

  void DumpNode(const IRWritePlace& w) {
    oss << "rec_update";
    if (!w.place.repr.empty()) {
      oss << " " << w.place.repr;
    }
  }

  void DumpNode(const IRAddrOf& a) {
    oss << "addr_of";
    if (!a.place.repr.empty()) {
      oss << " " << a.place.repr;
    }
  }

  void DumpNode(const IRReadPtr& r) {
    oss << "load_ptr ";
    Dump(r.ptr);
  }

  void DumpNode(const IRWritePtr& w) {
    oss << "store_ptr ";
    Dump(w.ptr);
    oss << " = ";
    Dump(w.value);
  }

  void DumpNode(const IRUnaryOp& op) {
    oss << "unop " << op.op << " ";
    Dump(op.operand);
  }

  void DumpNode(const IRBinaryOp& op) {
    oss << "binop " << op.op << " ";
    Dump(op.lhs);
    oss << ", ";
    Dump(op.rhs);
  }

  void DumpNode(const IRCast& c) {
    oss << "cast ";
    Dump(c.value);
    oss << " to " << (c.target ? sema::TypeToString(c.target) : "<unknown>");
  }

  void DumpNode(const IRTransmute& t) {
    oss << "transmute ";
    Dump(t.value);
    oss << " to " << (t.to ? sema::TypeToString(t.to) : "<unknown>");
  }

  void DumpNode(const IRCheckIndex& c) {
    oss << "check_index ";
    Dump(c.base);
    oss << ", ";
    Dump(c.index);
  }

  void DumpNode(const IRCheckRange& c) {
    oss << "check_range ";
    Dump(c.base);
    oss << ", ";
    DumpRange(c.range);
  }

  void DumpNode(const IRCheckSliceLen& c) {
    oss << "check_slice_len ";
    Dump(c.base);
    oss << ", ";
    DumpRange(c.range);
    oss << ", ";
    Dump(c.value);
  }

  void DumpNode(const IRCheckOp& c) {
    oss << "check_op " << c.reason << " " << c.op << " ";
    Dump(c.lhs);
    if (c.rhs.has_value()) {
      oss << ", ";
      Dump(*c.rhs);
    }
  }

  void DumpNode(const IRCheckCast& c) {
    oss << "check_cast ";
    Dump(c.value);
    oss << " to " << (c.target ? sema::TypeToString(c.target) : "<unknown>");
  }

  void DumpNode(const IRAlloc& a) {
    oss << "alloc";
    if (a.region.has_value()) {
      oss << " in ";
      Dump(*a.region);
    }
    oss << " ";
    Dump(a.value);
  }

  void DumpNode(const IRResult& r) {
    oss << "result ";
    Dump(r.value);
  }

  void DumpNode(const IRBreak& b) {
    oss << "break";
    if (b.value.has_value()) {
      oss << " ";
      Dump(*b.value);
    }
  }

  void DumpNode(const IRContinue&) { oss << "continue"; }

  void DumpNode(const IRDefer&) { oss << "defer"; }

  void DumpNode(const IRMoveState& m) {
    oss << "move_state " << m.place.repr;
  }

  void DumpNode(const IRIf& i) {
    oss << "if ";
    Dump(i.cond);
    oss << " then ";
    Dump(i.then_ir);
    oss << " else ";
    Dump(i.else_ir);
  }

  void DumpNode(const IRBlock& b) {
    oss << "block ";
    Dump(b.setup);
    oss << " ";
    Dump(b.body);
  }

  void DumpNode(const IRLoop& l) {
    oss << "loop";
    switch (l.kind) {
      case IRLoopKind::Infinite:
        oss << " infinite";
        break;
      case IRLoopKind::Conditional:
        oss << " while";
        break;
      case IRLoopKind::Iter:
        oss << " iter";
        break;
    }
  }

  void DumpNode(const IRMatch& m) {
    oss << "match ";
    Dump(m.scrutinee);
    oss << " {\n";
    indent_level++;
    for (const auto& arm : m.arms) {
      Indent();
      oss << "arm {\n";
      indent_level++;
      if (arm.pattern) {
        Indent();
        oss << "pat ";
        DumpPattern(*arm.pattern);
        oss << "\n";
      }
      Indent();
      oss << "body { ";
      Dump(arm.value);
      oss << " }\n";
      indent_level--;
      Indent();
      oss << "}\n";
    }
    indent_level--;
    Indent();
    oss << "}";
  }

  void DumpNode(const IRRegion& r) {
    oss << "region ";
    Dump(r.owner);
  }

  void DumpNode(const IRFrame& f) {
    oss << "frame";
    if (f.region.has_value()) {
      oss << " in ";
      Dump(*f.region);
    }
  }

  void DumpNode(const IRBranch& b) {
    if (b.cond.has_value()) {
      oss << "br_if ";
      Dump(*b.cond);
      oss << " " << b.true_label << " " << b.false_label;
    } else {
      oss << "br " << b.true_label;
    }
  }

  void DumpNode(const IRPhi& p) {
    oss << "phi " << (p.type ? sema::TypeToString(p.type) : "<unknown>");
  }

  void DumpNode(const IRClearPanic&) { oss << "clear_panic"; }

  void DumpNode(const IRPanicCheck&) { oss << "panic_check"; }

  void DumpNode(const IRCheckPoison& c) { oss << "check_poison " << c.module; }

  void DumpNode(const IRLowerPanic& p) { oss << "panic " << p.reason; }
};

}  // namespace

std::string DumpIR(const IRDecls& decls) {
  Dumper d;
  for (const auto& decl : decls) {
    std::visit([&d](const auto& item) {
      using T = std::decay_t<decltype(item)>;
      if constexpr (std::is_same_v<T, ProcIR>) {
        d.oss << "proc @" << item.symbol << " {\n";
        d.indent_level++;
        d.Indent(); d.Dump(item.body); d.oss << "\n";
        d.indent_level--;
        d.oss << "}\n\n";
      } else if constexpr (std::is_same_v<T, GlobalConst>) {
        d.oss << "global_const @" << item.symbol << " bytes=" << item.bytes.size() << "\n\n";
      } else if constexpr (std::is_same_v<T, GlobalZero>) {
        d.oss << "global_zero @" << item.symbol << " size=" << item.size << "\n\n";
      } else if constexpr (std::is_same_v<T, GlobalVTable>) {
        d.oss << "vtable @" << item.symbol
               << " size=" << item.header.size
               << " align=" << item.header.align
               << " drop=@" << item.header.drop_sym;
        if (!item.slots.empty()) {
          d.oss << " slots=[";
          for (std::size_t i = 0; i < item.slots.size(); ++i) {
            if (i) d.oss << ", ";
            d.oss << "@" << item.slots[i];
          }
          d.oss << "]";
        }
        d.oss << "\n\n";
      } else {
        d.oss << "decl ...\n\n";
      }
    }, decl);
  }
  return d.oss.str();
}

std::string DumpIR(const IRPtr& ir) {
  Dumper d;
  d.Dump(ir);
  return d.oss.str();
}

}  // namespace cursive0::codegen

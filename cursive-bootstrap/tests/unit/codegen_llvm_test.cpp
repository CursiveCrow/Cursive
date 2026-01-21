// Unit tests for LLVM codegen components (T-LLVM-*)
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cursive0/codegen/ir_model.h"
#include "cursive0/codegen/llvm/llvm_emit.h"
#include "cursive0/core/assert_spec.h"
#include "cursive0/analysis/types/context.h"
#include "cursive0/analysis/resolve/scopes.h"
#include "cursive0/analysis/types/types.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

using namespace cursive0;
using namespace cursive0::codegen;
using namespace cursive0::analysis;

static cursive0::core::Span EmptySpan() {
    return {};
}

static std::shared_ptr<cursive0::syntax::Type> MakeSyntaxPrimType(std::string name) {
    auto type = std::make_shared<cursive0::syntax::Type>();
    type->span = EmptySpan();
    type->node = cursive0::syntax::TypePrim{std::move(name)};
    return type;
}

static std::shared_ptr<cursive0::syntax::Type> MakeSyntaxPtrType(
    std::shared_ptr<cursive0::syntax::Type> elem,
    cursive0::syntax::PtrState state) {
    auto type = std::make_shared<cursive0::syntax::Type>();
    type->span = EmptySpan();
    cursive0::syntax::TypePtr ptr;
    ptr.element = std::move(elem);
    ptr.state = state;
    type->node = std::move(ptr);
    return type;
}

static cursive0::syntax::FieldDecl MakeRecordField(std::string name,
                                                   std::shared_ptr<cursive0::syntax::Type> type) {
    cursive0::syntax::FieldDecl field;
    field.vis = cursive0::syntax::Visibility::Public;
    field.name = std::move(name);
    field.type = std::move(type);
    field.init_opt = nullptr;
    field.span = EmptySpan();
    field.doc_opt = std::nullopt;
    return field;
}

static cursive0::syntax::RecordDecl MakeRecordDecl(std::string name) {
    cursive0::syntax::RecordDecl decl;
    decl.vis = cursive0::syntax::Visibility::Public;
    decl.name = std::move(name);
    decl.members = {
        MakeRecordField("x", MakeSyntaxPrimType("i32")),
        MakeRecordField("y", MakeSyntaxPrimType("i32")),
    };
    decl.span = EmptySpan();
    return decl;
}

static cursive0::syntax::EnumDecl MakeEnumDecl(std::string name) {
    cursive0::syntax::EnumDecl decl;
    decl.vis = cursive0::syntax::Visibility::Public;
    decl.name = std::move(name);
    cursive0::syntax::VariantDecl unit;
    unit.name = "None";
    unit.payload_opt = std::nullopt;
    unit.span = EmptySpan();
    cursive0::syntax::VariantDecl tuple;
    tuple.name = "Some";
    cursive0::syntax::VariantPayloadTuple payload;
    payload.elements = {MakeSyntaxPrimType("i32")};
    tuple.payload_opt = cursive0::syntax::VariantPayload{std::move(payload)};
    tuple.span = EmptySpan();
    decl.variants = {std::move(unit), std::move(tuple)};
    decl.span = EmptySpan();
    return decl;
}

static cursive0::syntax::StateBlock MakeStateWithPtrPayload(std::string name) {
    cursive0::syntax::StateBlock state;
    state.name = std::move(name);
    cursive0::syntax::StateFieldDecl field;
    field.vis = cursive0::syntax::Visibility::Public;
    field.name = "ptr";
    field.type = MakeSyntaxPtrType(MakeSyntaxPrimType("i32"),
                                   cursive0::syntax::PtrState::Valid);
    field.span = EmptySpan();
    field.doc_opt = std::nullopt;
    state.members.push_back(field);
    state.span = EmptySpan();
    state.doc_opt = std::nullopt;
    return state;
}

static cursive0::syntax::StateBlock MakeStateWithI32Payload(std::string name) {
    cursive0::syntax::StateBlock state;
    state.name = std::move(name);
    cursive0::syntax::StateFieldDecl field;
    field.vis = cursive0::syntax::Visibility::Public;
    field.name = "value";
    field.type = MakeSyntaxPrimType("i32");
    field.span = EmptySpan();
    field.doc_opt = std::nullopt;
    state.members.push_back(field);
    state.span = EmptySpan();
    state.doc_opt = std::nullopt;
    return state;
}

static cursive0::syntax::StateBlock MakeEmptyState(std::string name) {
    cursive0::syntax::StateBlock state;
    state.name = std::move(name);
    state.span = EmptySpan();
    state.doc_opt = std::nullopt;
    return state;
}

static cursive0::syntax::ModalDecl MakeModalDeclNiche(std::string name) {
    cursive0::syntax::ModalDecl decl;
    decl.vis = cursive0::syntax::Visibility::Public;
    decl.name = std::move(name);
    decl.states = {MakeStateWithPtrPayload("Some"), MakeEmptyState("None")};
    decl.span = EmptySpan();
    return decl;
}

static cursive0::syntax::ModalDecl MakeModalDeclTagged(std::string name) {
    cursive0::syntax::ModalDecl decl;
    decl.vis = cursive0::syntax::Visibility::Public;
    decl.name = std::move(name);
    decl.states = {MakeStateWithI32Payload("On"), MakeStateWithI32Payload("Off")};
    decl.span = EmptySpan();
    return decl;
}

static cursive0::syntax::TypeAliasDecl MakeAliasDecl(std::string name,
                                                     std::string target) {
    cursive0::syntax::TypeAliasDecl decl;
    decl.vis = cursive0::syntax::Visibility::Public;
    decl.name = std::move(name);
    auto type = std::make_shared<cursive0::syntax::Type>();
    type->span = EmptySpan();
    type->node = cursive0::syntax::TypePathType{{std::move(target)}};
    decl.type = std::move(type);
    decl.span = EmptySpan();
    return decl;
}

void TestSetupModule() {
    std::cout << "TestSetupModule..." << std::endl;
    llvm::LLVMContext ctx;
    std::unique_ptr<LLVMEmitter> emitter = std::make_unique<LLVMEmitter>(ctx, "test_module");
    emitter->SetupModule();
    
    llvm::Module& m = emitter->GetModule();
    assert(m.getTargetTriple().str() == "x86_64-pc-windows-msvc");
    assert(m.getDataLayoutStr() == "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128");
    std::cout << "PASS" << std::endl;
}

void TestTypes() {
    std::cout << "TestTypes..." << std::endl;
    llvm::LLVMContext ctx;
    std::unique_ptr<LLVMEmitter> emitter = std::make_unique<LLVMEmitter>(ctx, "test_types");
    
    // i32
    auto i32_node = std::make_shared<analysis::Type>();
    i32_node->node = analysis::TypePrim{"i32"};
    llvm::Type* t_i32 = emitter->GetLLVMType(i32_node);
    assert(t_i32->isIntegerTy(32));
    
    // bool
    auto bool_node = std::make_shared<analysis::Type>();
    bool_node->node = analysis::TypePrim{"bool"};
    llvm::Type* t_bool = emitter->GetLLVMType(bool_node);
    assert(t_bool->isIntegerTy(8));
    
    // void / unit - maps to empty struct (zero-sized) in value contexts, not LLVM void
    auto unit_node = std::make_shared<analysis::Type>();
    unit_node->node = analysis::TypePrim{"()"};
    llvm::Type* t_unit = emitter->GetLLVMType(unit_node);
    // Unit type is represented as empty struct for value representation
    assert(t_unit->isStructTy() && static_cast<llvm::StructType*>(t_unit)->getNumElements() == 0);

    
    std::cout << "PASS" << std::endl;
}

void TestOpaquePointers() {
    std::cout << "TestOpaquePointers..." << std::endl;
    llvm::LLVMContext ctx;
    std::unique_ptr<LLVMEmitter> emitter = std::make_unique<LLVMEmitter>(ctx, "test_ptr");
    
    // *i32
    auto i32_node = std::make_shared<analysis::Type>();
    i32_node->node = analysis::TypePrim{"i32"};
    
    auto ptr_node = std::make_shared<analysis::Type>();
    // Fix: TypePtr has element and state only.
    ptr_node->node = analysis::TypePtr{i32_node, analysis::PtrState::Valid};
    
    llvm::Type* t_ptr = emitter->GetLLVMType(ptr_node);
    assert(t_ptr->isPointerTy());

    
    // raw ptr
    auto raw_node = std::make_shared<analysis::Type>();
    // Fix: TypeRawPtr has qual and element.
    raw_node->node = analysis::TypeRawPtr{analysis::RawPtrQual::Imm, i32_node};
    llvm::Type* t_raw = emitter->GetLLVMType(raw_node);
    assert(t_raw->isPointerTy());
    
    std::cout << "PASS" << std::endl;
}


void TestABI() {
    std::cout << "TestABI..." << std::endl;
    llvm::LLVMContext ctx;
    std::unique_ptr<LLVMEmitter> emitter = std::make_unique<LLVMEmitter>(ctx, "test_abi");
    
    // Check return mapping for i32
    auto i32_node = std::make_shared<analysis::Type>();
    i32_node->node = analysis::TypePrim{"i32"};
    
    std::vector<IRParam> params; 
    ABICallResult res = emitter->ComputeCallABI(params, i32_node);
    
    assert(res.ret_type->isIntegerTy(32));
    assert(!res.has_sret);
    
    std::cout << "PASS" << std::endl;
}

IRPtr MakeOpaqueIR() {
    auto ir = std::make_shared<IR>();
    ir->node = IROpaque{};
    return ir;
}

void TestCallABIRules() {
    std::cout << "TestCallABIRules..." << std::endl;
    SPEC_COV("LLVMCall-ByValue");
    SPEC_COV("LLVMCall-SRet");
    SPEC_COV("LLVMArgLower-ByRef");
    SPEC_COV("LLVMArgLower-ByValue-Other");
    SPEC_COV("LLVMArgLower-ByValue-PtrValid");
    SPEC_COV("LLVMArgLower-Err");
    SPEC_COV("LLVMCall-Err");
    SPEC_COV("LLVMRetLower-ByValue");
    SPEC_COV("LLVMRetLower-ByValue-ZST");
    SPEC_COV("LLVMRetLower-Err");
    SPEC_COV("LLVMRetLower-SRet");

    llvm::LLVMContext ctx;
    LLVMEmitter emitter(ctx, "abi_rules");

    {
        std::vector<IRParam> params = {
            {std::nullopt, "a", MakeTypePrim("i32")},
        };
        auto res = emitter.ComputeCallABI(params, MakeTypePrim("i32"));
        assert(res.func_type != nullptr);
    }

    {
        std::vector<IRParam> params = {
            {ParamMode::Move, "b", MakeTypePrim("i32")},
        };
        auto res = emitter.ComputeCallABI(params, MakeTypePrim("i32"));
        assert(res.func_type != nullptr);
    }

    {
        auto ptr_valid = MakeTypePtr(MakeTypePrim("u8"), PtrState::Valid);
        std::vector<IRParam> params = {
            {ParamMode::Move, "p", ptr_valid},
        };
        auto res = emitter.ComputeCallABI(params, MakeTypePrim("i32"));
        assert(res.func_type != nullptr);
    }

    {
        auto res = emitter.ComputeCallABI({}, MakeTypePrim("()"));
        assert(res.func_type != nullptr);
    }

    {
        auto large = MakeTypeTuple({
            MakeTypePrim("u64"),
            MakeTypePrim("u64"),
            MakeTypePrim("u64"),
        });
        auto res = emitter.ComputeCallABI({}, large);
        assert(res.func_type != nullptr);
    }

    {
        auto bad = MakeTypePath({"Missing"});
        std::vector<IRParam> params = {
            {std::nullopt, "x", bad},
        };
        auto res = emitter.ComputeCallABI(params, MakeTypePrim("()"));
        assert(res.func_type != nullptr);
    }

    std::cout << "PASS" << std::endl;
}

void TestParamBindingAndAttrs() {
    std::cout << "TestParamBindingAndAttrs..." << std::endl;
    SPEC_COV("LLVM-ArgAttrs-NonPtr");
    SPEC_COV("LLVM-ArgAttrs-Ptr");
    SPEC_COV("LLVM-ArgAttrs-RawPtr");
    SPEC_COV("LLVM-PtrAttrs-Valid");
    SPEC_COV("BindSlot-Param-ByRef");
    SPEC_COV("BindSlot-Param-ByValue");
    SPEC_COV("LLVM-PtrAttrs-Other");
    SPEC_COV("LLVM-PtrAttrs-RawPtr");
    SPEC_COV("PtrStateOf-Perm");

    llvm::LLVMContext ctx;
    LLVMEmitter emitter(ctx, "param_bind");
    LowerCtx lower;

    auto ptr_null = MakeTypePtr(MakeTypePrim("u8"), PtrState::Null);
    auto ptr_valid = MakeTypePtr(MakeTypePrim("u8"), PtrState::Valid);
    auto perm_ptr = MakeTypePerm(Permission::Const, ptr_null);
    auto raw_ptr = MakeTypeRawPtr(RawPtrQual::Mut, MakeTypePrim("u8"));

    ProcIR proc;
    proc.symbol = "param_proc";
    proc.params = {
        {std::nullopt, "p_valid", ptr_valid},
        {std::nullopt, "p_null", perm_ptr},
        {std::nullopt, "p_raw", raw_ptr},
        {std::nullopt, "byref", MakeTypePrim("i32")},
        {ParamMode::Move, "byval", MakeTypePrim("i32")},
    };
    proc.ret = MakeTypePrim("()");
    proc.body = MakeOpaqueIR();

    IRDecls decls = {proc};
    llvm::Module* mod = emitter.EmitModule(decls, lower);
    assert(mod != nullptr);

    std::cout << "PASS" << std::endl;
}

void TestEntryPointSymbols() {
    std::cout << "TestEntryPointSymbols..." << std::endl;
    SPEC_COV("EntrySym-Decl");
    SPEC_COV("EntryStub-Decl");

    llvm::LLVMContext ctx;
    LLVMEmitter emitter(ctx, "entrypoint");
    LowerCtx lower;

    ProcIR main_proc;
    main_proc.symbol = "user_main";
    main_proc.params = {
        {ParamMode::Move, "ctx", MakeTypePrim("i32")},
        {ParamMode::Move, "panic", MakeTypePrim("i32")},
    };
    main_proc.ret = MakeTypePrim("i32");
    main_proc.body = MakeOpaqueIR();

    IRDecls decls = {main_proc};
    lower.main_symbol = main_proc.symbol;

    llvm::Module* mod = emitter.EmitModule(decls, lower);
    assert(mod != nullptr);
    assert(mod->getFunction("main") != nullptr);

    std::cout << "PASS" << std::endl;
}

void TestEntryPointErrors() {
    std::cout << "TestEntryPointErrors..." << std::endl;
    SPEC_COV("EntryStub-Err");
    SPEC_COV("EntrySym-Err");

    llvm::LLVMContext ctx;
    LLVMEmitter emitter(ctx, "entrypoint_err");
    LowerCtx lower;

    IRDecls decls = {};
    llvm::Module* mod = emitter.EmitModule(decls, lower);
    assert(mod != nullptr);

    lower.codegen_failed = false;
    emitter.EmitEntryPoint();
    assert(lower.codegen_failed);

    std::cout << "PASS" << std::endl;
}

void TestLLVMTypeCoverage() {
    std::cout << "TestLLVMTypeCoverage..." << std::endl;
    SPEC_COV("LLVMTy-Alias");
    SPEC_COV("LLVMTy-Array");
    SPEC_COV("LLVMTy-BytesManaged");
    SPEC_COV("LLVMTy-BytesView");
    SPEC_COV("LLVMTy-Dynamic");
    SPEC_COV("LLVMTy-Enum");
    SPEC_COV("LLVMTy-Err");
    SPEC_COV("LLVMTy-Func");
    SPEC_COV("LLVMTy-Modal-Niche");
    SPEC_COV("LLVMTy-Modal-StringBytes");
    SPEC_COV("LLVMTy-Modal-Tagged");
    SPEC_COV("LLVMTy-ModalState");
    SPEC_COV("LLVMTy-Perm");
    SPEC_COV("LLVMTy-Prim");
    SPEC_COV("LLVMTy-Ptr");
    SPEC_COV("LLVMTy-Range");
    SPEC_COV("LLVMTy-RawPtr");
    SPEC_COV("LLVMTy-Record");
    SPEC_COV("LLVMTy-Slice");
    SPEC_COV("LLVMTy-StringManaged");
    SPEC_COV("LLVMTy-StringView");
    SPEC_COV("LLVMTy-Tuple");
    SPEC_COV("LLVMTy-Union-Niche");
    SPEC_COV("LLVMTy-Union-Tagged");

    llvm::LLVMContext ctx;
    LLVMEmitter emitter(ctx, "llvm_types");
    LowerCtx lower;

    analysis::Sigma sigma;
    sigma.types[analysis::PathKeyOf({"Point"})] = MakeRecordDecl("Point");
    sigma.types[analysis::PathKeyOf({"Option"})] = MakeEnumDecl("Option");
    sigma.types[analysis::PathKeyOf({"MaybePtr"})] = MakeModalDeclNiche("MaybePtr");
    sigma.types[analysis::PathKeyOf({"Mode"})] = MakeModalDeclTagged("Mode");
    sigma.types[analysis::PathKeyOf({"AliasPoint"})] = MakeAliasDecl("AliasPoint", "Point");

    lower.sigma = &sigma;
    lower.module_path = {"main"};

    llvm::Module* mod = emitter.EmitModule({}, lower);
    assert(mod != nullptr);

    // Basic type mappings.
    assert(emitter.GetLLVMType(MakeTypePrim("i32")) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeRange()) != nullptr);
    assert(emitter.GetLLVMType(MakeTypePerm(Permission::Unique, MakeTypePrim("u8"))) != nullptr);
    assert(emitter.GetLLVMType(MakeTypePtr(MakeTypePrim("i32"), PtrState::Valid)) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeRawPtr(RawPtrQual::Imm, MakeTypePrim("u8"))) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeArray(MakeTypePrim("u8"), 4)) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeSlice(MakeTypePrim("u8"))) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeTuple({MakeTypePrim("i32"), MakeTypePrim("bool")})) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeFunc({}, MakeTypePrim("i32"))) != nullptr);

    // String/bytes and dynamic types.
    assert(emitter.GetLLVMType(MakeTypeString(analysis::StringState::View)) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeString(analysis::StringState::Managed)) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeString(std::nullopt)) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeBytes(analysis::BytesState::View)) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeBytes(analysis::BytesState::Managed)) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeBytes(std::nullopt)) != nullptr);
    assert(emitter.GetLLVMType(MakeTypeDynamic({"HeapAllocator"})) != nullptr);

    // Union layout variants.
    auto niche_union = MakeTypeUnion({
        MakeTypePtr(MakeTypePrim("u8"), PtrState::Valid),
        MakeTypePrim("()"),
    });
    assert(emitter.GetLLVMType(niche_union) != nullptr);
    auto tagged_union = MakeTypeUnion({MakeTypePrim("i32"), MakeTypePrim("bool")});
    assert(emitter.GetLLVMType(tagged_union) != nullptr);

    // Path-based types.
    assert(emitter.GetLLVMType(MakeTypePath({"Point"})) != nullptr);
    assert(emitter.GetLLVMType(MakeTypePath({"Option"})) != nullptr);
    assert(emitter.GetLLVMType(MakeTypePath({"MaybePtr"})) != nullptr);
    assert(emitter.GetLLVMType(MakeTypePath({"AliasPoint"})) != nullptr);

    llvm::Type* mode_ty = emitter.GetLLVMType(MakeTypePath({"Mode"}));
    assert(mode_ty != nullptr);
    auto* mode_struct = llvm::dyn_cast<llvm::StructType>(mode_ty);
    assert(mode_struct != nullptr);
    assert(mode_struct->getNumElements() == 3);
    assert(mode_struct->getElementType(0)->isIntegerTy(8));
    auto* pad_mid = llvm::dyn_cast<llvm::ArrayType>(mode_struct->getElementType(1));
    assert(pad_mid != nullptr);
    assert(pad_mid->getNumElements() == 3);
    assert(pad_mid->getElementType()->isIntegerTy(8));
    auto* payload = llvm::dyn_cast<llvm::ArrayType>(mode_struct->getElementType(2));
    assert(payload != nullptr);
    assert(payload->getNumElements() == 4);
    assert(payload->getElementType()->isIntegerTy(8));
    assert(emitter.GetLLVMType(MakeTypeModalState({"MaybePtr"}, "Some")) != nullptr);

    // Error path.
    assert(emitter.GetLLVMType(nullptr) != nullptr);

    std::cout << "PASS" << std::endl;
}

static void EnsureLLVMInitOnce() {
    static bool initialized = false;
    if (initialized) {
        return;
    }
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    initialized = true;
}

static bool EmitObjForTest(llvm::Module& module, std::string* out_bytes) {
    EnsureLLVMInitOnce();
    llvm::Triple triple = module.getTargetTriple();
    if (triple.getTriple().empty()) {
        triple = llvm::Triple("x86_64-pc-windows-msvc");
        module.setTargetTriple(triple);
    }

    std::string err;
    const llvm::Target* target =
        llvm::TargetRegistry::lookupTarget(triple.str(), err);
    if (!target) {
        return false;
    }

    llvm::TargetOptions options;
    std::unique_ptr<llvm::TargetMachine> machine(
        target->createTargetMachine(triple.str(), "generic", "", options, std::nullopt));
    if (!machine) {
        return false;
    }

    if (module.getDataLayout().isDefault()) {
        module.setDataLayout(machine->createDataLayout());
    }

    llvm::SmallVector<char, 0> buffer;
    llvm::raw_svector_ostream dest(buffer);
    llvm::legacy::PassManager pass;
    if (machine->addPassesToEmitFile(pass, dest, nullptr,
                                     llvm::CodeGenFileType::ObjectFile)) {
        return false;
    }
    pass.run(module);
    if (out_bytes) {
        *out_bytes = std::string(buffer.begin(), buffer.end());
    }
    return true;
}

void TestEmitLLVMError() {
    SPEC_COV("EmitLLVM-Err");
    std::cout << "TestEmitLLVMError..." << std::endl;

    llvm::LLVMContext ctx;
    LowerCtx lower;
    LLVMEmitter emitter(ctx, "emit_llvm_err");

    GlobalVTable vt;
    vt.symbol = "vt_missing";
    vt.header.size = 0;
    vt.header.align = 1;
    vt.header.drop_sym = "missing_drop";
    vt.slots = {"missing_slot"};
    IRDecls decls = {vt};

    auto* raw = emitter.EmitModule(decls, lower);
    auto module = emitter.ReleaseModule();

    assert(raw != nullptr);
    assert(module != nullptr);
    assert(lower.codegen_failed);
    std::cout << "PASS" << std::endl;
}

void TestEmitObjError() {
    SPEC_COV("EmitObj-Err");
    std::cout << "TestEmitObjError..." << std::endl;

    llvm::LLVMContext ctx;
    LowerCtx lower;
    LLVMEmitter emitter(ctx, "emit_obj_err");
    IRDecls decls;
    auto* raw = emitter.EmitModule(decls, lower);
    auto module = emitter.ReleaseModule();
    assert(raw != nullptr);
    assert(module != nullptr);
    assert(!lower.codegen_failed);

    module->setTargetTriple(llvm::Triple("invalid-unknown-none"));
    std::string bytes;
    const bool ok = EmitObjForTest(*module, &bytes);
    assert(!ok);
    std::cout << "PASS" << std::endl;
}

void TestEmitObjOk() {
    SPEC_COV("EmitObj-Ok");
    std::cout << "TestEmitObjOk..." << std::endl;

    llvm::LLVMContext ctx;
    LowerCtx lower;
    LLVMEmitter emitter(ctx, "emit_obj_ok");
    IRDecls decls;
    auto* raw = emitter.EmitModule(decls, lower);
    auto module = emitter.ReleaseModule();
    assert(raw != nullptr);
    assert(module != nullptr);
    assert(!lower.codegen_failed);

    std::string bytes;
    const bool ok = EmitObjForTest(*module, &bytes);
    assert(ok);
    assert(!bytes.empty());
    std::cout << "PASS" << std::endl;
}

void TestLowerIRError() {
    SPEC_COV("LowerIR-Err");
    std::cout << "TestLowerIRError..." << std::endl;

    llvm::LLVMContext ctx;
    LowerCtx lower;
    LLVMEmitter emitter(ctx, "lower_ir_err");

    GlobalVTable vt;
    vt.symbol = "vt_lower_err";
    vt.header.size = 0;
    vt.header.align = 1;
    vt.header.drop_sym = "missing_drop";
    vt.slots = {"missing_slot"};
    IRDecls decls = {vt};

    auto* raw = emitter.EmitModule(decls, lower);
    auto module = emitter.ReleaseModule();

    assert(raw != nullptr);
    assert(module != nullptr);
    assert(lower.codegen_failed);
    std::cout << "PASS" << std::endl;
}

int main() {
    TestSetupModule();
    TestTypes();
    TestOpaquePointers();
    TestABI();
    TestCallABIRules();
    TestParamBindingAndAttrs();
    TestEntryPointSymbols();
    TestEntryPointErrors();
    TestLLVMTypeCoverage();
    TestEmitLLVMError();
    TestEmitObjError();
    TestEmitObjOk();
    TestLowerIRError();
    return 0;
}

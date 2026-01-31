// =============================================================================
// MIGRATION MAPPING: calling_conventions.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.2.1 Default Calling Convention (lines 15156-15159)
//   - CallConvDefault = Cursive0ABI (line 15159)
//   - FFI calling conventions in extern blocks (referenced in §12 FFI)
//   - ABI strings: "C", "C-unwind", "system", "stdcall", "fastcall", "vectorcall"
//
// SOURCE FILE: (New file - no direct source)
//   - Calling convention handling scattered across codegen
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/abi/abi.h (CallingConvention enum)
//   - LLVM calling convention IDs
//
// REFACTORING NOTES:
//   1. Cursive0ABI is the default for all Cursive procedures
//   2. FFI extern blocks can specify alternative conventions:
//      - "C" / "C-unwind": C calling convention
//      - "system": platform-specific system ABI
//      - "stdcall": Windows stdcall (32-bit)
//      - "fastcall": register-based fastcall
//      - "vectorcall": SIMD-friendly calling
//   3. Calling convention affects:
//      - Parameter passing order
//      - Register allocation
//      - Stack cleanup responsibility
//      - Name mangling (for some conventions)
//   4. LLVM calling convention enum mapping
//
// CALLING CONVENTION ENUM:
//   enum CallingConvention {
//     Cursive0,    // Default Cursive ABI
//     C,           // Standard C ABI
//     CUnwind,     // C with unwinding support
//     System,      // Platform default
//     Stdcall,     // Windows stdcall
//     Fastcall,    // Fastcall convention
//     Vectorcall,  // Vectorcall convention
//   };
//
// LLVM MAPPINGS:
//   - Cursive0 -> LLVM default (C)
//   - C -> llvm::CallingConv::C
//   - Stdcall -> llvm::CallingConv::X86_StdCall
//   - Fastcall -> llvm::CallingConv::X86_FastCall
// =============================================================================

#include "cursive/05_codegen/abi/abi.h"

// TODO: Implement calling convention handling

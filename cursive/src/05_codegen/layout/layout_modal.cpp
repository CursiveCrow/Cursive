// =============================================================================
// MIGRATION MAPPING: layout_modal.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 6.1.7 Modal Layout (Codegen) (lines 15086-15133)
//   - ModalDiscType definition (line 15088)
//   - StateSize, StateAlign (lines 15089-15090)
//   - ModalAlign, ModalSize (lines 15091-15092)
//   - Layout-Modal-Niche rule (lines 15095-15098)
//   - Layout-Modal-Tagged rule (lines 15100-15103)
//   - Size-Modal, Align-Modal, Layout-Modal rules (lines 15105-15118)
//   - Size-ModalState, Align-ModalState, Layout-ModalState rules (lines 15120-15133)
//   - Modal Niche Encoding (lines 14816-14835)
//   - ModalNicheBits, ModalTaggedBits, ModalBits definitions
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/layout/layout_modal.cpp
//   - ModalLayoutOf function (if exists)
//   - ModalPayload field access
//   - State enumeration helpers
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/layout/layout.h (ModalLayout struct)
//   - cursive/include/04_analysis/modal/modal_widen.h (modal type helpers)
//   - cursive/include/04_analysis/types/types.h (TypeModalState)
//   - RecordLayoutOf for state payload layout
//
// REFACTORING NOTES:
//   1. Modal types are tagged unions over states
//   2. Niche optimization applies when:
//      - One state has single-field payload with niches
//      - All other states are empty (no fields)
//      - Niche count >= number of states - 1
//   3. ModalDiscType = DiscType(|States(M)| - 1)
//   4. StateSize(S) = RecordLayout(ModalPayload(S)).size
//   5. ModalSize = AlignUp(disc_size + max_state_size, modal_align)
//   6. Modal state types (M@S) have size/align of just that state's payload
//   7. Full modal types (M) include discriminant overhead
//
// MODAL LAYOUT (TAGGED):
//   [discriminant][padding][payload: max_state_size][tail_padding]
//
// MODAL LAYOUT (NICHE):
//   Same as single payload state layout (no discriminant needed)
// =============================================================================

#include "cursive/05_codegen/layout/layout.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/layout/layout_modal.cpp

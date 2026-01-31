// =============================================================================
// MIGRATION MAPPING: stmt/frame_stmt.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   Section 5.5: Memory and Pointers - Frame statements
//   - T-FrameStmt-Implicit (line 9557): Implicit frame
//   - T-FrameStmt-Explicit (line 9562): Explicit named frame
//   - frame_stmt grammar (line 3161)
//
// SOURCE FILE: cursive-bootstrap/src/03_analysis/memory/regions.cpp
//   Frame statement typing portions
//
// KEY CONTENT TO MIGRATE:
//   FRAME STATEMENT (frame { body } | region.frame { body }):
//   1. Verify inside enclosing region
//   2. Push frame scope (sub-arena)
//   3. Type body statements
//   4. Pop frame scope
//   5. All frame allocations freed at exit
//
//   TYPING RULE (T-FrameStmt-Implicit):
//   Inside region context
//   Gamma; R'; L |- body : T
//   R' = PushFrame(R)
//   --------------------------------------------------
//   Gamma |- frame { body } : ()
//
//   TYPING RULE (T-FrameStmt-Explicit):
//   Gamma |- region : Region@Active
//   Gamma; R'; L |- body : T
//   R' = PushFrame(R, region)
//   --------------------------------------------------
//   Gamma |- region.frame { body } : ()
//
//   FRAME VS REGION:
//   - Frame subdivides an existing region
//   - Frame allocations freed when frame exits
//   - Region stays active after frame exits
//   - Enables partial deallocation within region
//
//   USE CASE:
//   region {
//       let permanent: Ptr<i32>@Valid = ^100
//       frame {
//           let temporary: Ptr<i32>@Valid = ^42
//           // use temporary
//       }
//       // temporary freed, permanent still valid
//   }
//
//   POINTER VALIDITY:
//   - Pointers allocated in frame become @Expired at frame exit
//   - Pointers from enclosing region remain @Valid
//   - No escape of frame pointers allowed
//
// DEPENDENCIES:
//   - RegionContext verification
//   - BlockTypeFn for body typing
//   - FrameScope push/pop
//   - Pointer state tracking
//
// SPEC RULES:
//   - T-FrameStmt-Implicit (line 9557)
//   - T-FrameStmt-Explicit (line 9562)
//
// RESULT TYPE:
//   StmtResult { Gamma (unchanged), body effects }
//
// NOTES:
//   - Frames enable fine-grained lifetime control
//   - Used for temporary allocations within regions
//   - No overhead for frame creation/destruction
//
// =============================================================================


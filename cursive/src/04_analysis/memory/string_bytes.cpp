/*
 * =============================================================================
 * MIGRATION MAPPING: string_bytes.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 5.6 "String and Bytes Types" (lines 13160-13300)
 *   - C0updated.md, Section 5.6.1 "String States" (lines 13170-13220)
 *   - C0updated.md, Section 5.6.2 "Bytes States" (lines 13230-13280)
 *   - C0updated.md, Section 5.6.3 "String/Bytes Methods" (lines 13290-13400)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/string_bytes.cpp (lines 1-285)
 *
 * FUNCTIONS TO MIGRATE:
 *   - BuildStringType() -> TypeRef                                 [lines 15-50]
 *       Construct the string modal type
 *   - BuildBytesType() -> TypeRef                                  [lines 55-90]
 *       Construct the bytes modal type
 *   - StringMethodSignatures() -> Vec<ProcSig>                     [lines 95-180]
 *       Get method signatures for string type
 *   - BytesMethodSignatures() -> Vec<ProcSig>                      [lines 185-250]
 *       Get method signatures for bytes type
 *   - IsStringType(TypeRef ty) -> bool                             [lines 255-265]
 *       Check if type is string@View or string@Managed
 *   - IsBytesType(TypeRef ty) -> bool                              [lines 270-280]
 *       Check if type is bytes@View or bytes@Managed
 *   - StringStateOf(TypeRef ty) -> StringState                     [lines 282-285]
 *       Get @View or @Managed state of string type
 *
 * DEPENDENCIES:
 *   - Modal type infrastructure
 *   - HeapAllocator for @Managed allocation
 *   - Method signature building
 *
 * REFACTORING NOTES:
 *   1. string is modal: @View (borrowed slice) vs @Managed (heap allocated)
 *   2. bytes is modal: @View (borrowed slice) vs @Managed (heap allocated)
 *   3. String literals are string@View
 *   4. @Managed requires HeapAllocator capability
 *   5. Conversion @View -> @Managed requires allocation
 *   6. @View cannot outlive its source
 *   7. Consider UTF-8 validation for string type
 *
 * TYPE SIGNATURES:
 *   string@View:
 *     - length(~) -> usize
 *     - is_empty(~) -> bool
 *     - as_bytes(~) -> bytes@View
 *   string@Managed:
 *     - (all @View methods)
 *     - push(~!, ch: char) -> ()
 *     - from(view: string@View, heap: $HeapAllocator) -> string@Managed | AllocationError
 *   bytes@View:
 *     - length(~) -> usize
 *     - get(~, idx: usize) -> u8 | ()
 *   bytes@Managed:
 *     - (all @View methods)
 *     - push(~!, byte: u8) -> ()
 *     - with_capacity(cap: usize, heap: $HeapAllocator) -> bytes@Managed | AllocationError
 *
 * DIAGNOSTIC CODES:
 *   - E-STR-0001: @View outlives source
 *   - E-STR-0002: @Managed without allocator
 *   - E-STR-0003: Invalid UTF-8 in string
 *
 * =============================================================================
 */

; ModuleID = 'init_order_ok'
source_filename = "init_order_ok"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError = comdat any

@cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3abeta = external global i8
@beta_x3a_x3abx = external global i32, align 4
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f08B05D07B5566BEF = internal unnamed_addr constant [2 x i8] c"ok", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f00391E19133920B8 = internal unnamed_addr constant [3 x i8] c"bad", align 1
@cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3ainit_x5forder_x5fok = global i8 0

; Function Attrs: noreturn nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3apanic(i32) #0

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3acontext_x5finit(ptr noalias noundef nonnull sret align 8 dereferenceable(32)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3adrop_x5fmanaged(ptr noundef nonnull readonly align 8 dereferenceable(24)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3adrop_x5fmanaged(ptr noundef nonnull readonly align 8 dereferenceable(24)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3anew_x5fscoped(ptr noundef nonnull readonly align 8 dereferenceable(40)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3aalloc(ptr noundef nonnull readonly align 1, ptr noundef nonnull readonly align 1) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3areset_x5funchecked(ptr noundef nonnull readonly align 1) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afreeze(ptr noundef nonnull readonly align 1) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3athaw(ptr noundef nonnull readonly align 1) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afree_x5funchecked(ptr noundef nonnull readonly align 1 dereferenceable(1)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3afrom(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { ptr, i64 } @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3aas_x5fview(ptr noundef nonnull readonly align 8 dereferenceable(24)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3ato_x5fmanaged(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3aclone_x5fwith(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(24), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3aappend(ptr noalias noundef nonnull sret align 8 dereferenceable(24), ptr noundef nonnull readonly align 8 dereferenceable(24), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i64 @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3alength(ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i8 @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3ais_x5fempty(ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3awith_x5fcapacity(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(8), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3afrom_x5fslice(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { ptr, i64 } @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aas_x5fview(ptr noundef nonnull readonly align 8 dereferenceable(24)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3ato_x5fmanaged(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { ptr, i64 } @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aview(ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { ptr, i64 } @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aview_x5fstring(ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aappend(ptr noalias noundef nonnull sret align 8 dereferenceable(24), ptr noundef nonnull readonly align 8 dereferenceable(24), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i64 @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3alength(ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i8 @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3ais_x5fempty(ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [7 x i8], [8 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [7 x i8], [8 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [7 x i8], [8 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [7 x i8], [8 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [1 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [1 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [1 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i8 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [1 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [7 x i8], [8 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [1 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [1 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { i8, [1 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { ptr, ptr } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { ptr, ptr } @cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3awith_x5fquota(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare ptr @cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3aalloc_x5fraw(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3adealloc_x5fraw(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(8), ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

define i32 @init_x5forder_x5fok_x3a_x3amain(ptr noundef nonnull readonly align 8 dereferenceable(32) %ctx, ptr %__panic) {
entry:
  %ptr_arg27 = alloca { i8, [0 x i8] }, align 8
  %tmp_addr = alloca { i8, [1 x i8] }, align 8
  %match_val21 = alloca { i8, [1 x i8] }, align 8
  %ptr_arg = alloca { i8, [0 x i8] }, align 8
  %err = alloca { i8, [0 x i8] }, align 8
  %ok = alloca {}, align 8
  %match_val = alloca { i8, [1 x i8] }, align 8
  %byref_arg = alloca { ptr, i64 }, align 8
  %msg = alloca { ptr, i64 }, align 8
  %init_ok = alloca i8, align 1
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %__panic1, align 8
  %3 = load i8, ptr %2, align 1
  %4 = icmp ne i8 %3, 0
  br i1 %4, label %panic_fail, label %panic_ok

panic_ok:                                         ; preds = %entry
  br label %panic_cont

panic_fail:                                       ; preds = %entry
  ret i32 0

panic_cont:                                       ; preds = %panic_ok
  %5 = load ptr, ptr %__panic1, align 8
  %6 = load i8, ptr %5, align 1
  %7 = icmp ne i8 %6, 0
  br i1 %7, label %panic_fail3, label %panic_ok2

panic_ok2:                                        ; preds = %panic_cont
  br label %panic_cont4

panic_fail3:                                      ; preds = %panic_cont
  ret i32 0

panic_cont4:                                      ; preds = %panic_ok2
  %8 = load ptr, ptr %__panic1, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ainit_x5forder_x5fok(ptr %8)
  %9 = load ptr, ptr %__panic1, align 8
  %10 = load i8, ptr %9, align 1
  %11 = icmp ne i8 %10, 0
  br i1 %11, label %panic_fail6, label %panic_ok5

panic_ok5:                                        ; preds = %panic_cont4
  br label %panic_cont7

panic_fail6:                                      ; preds = %panic_cont4
  ret i32 0

panic_cont7:                                      ; preds = %panic_ok5
  %12 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3abeta, align 1
  %13 = icmp eq i8 %12, 0
  br i1 %13, label %check_ok, label %check_fail

check_ok:                                         ; preds = %panic_cont7
  %14 = load i32, ptr @beta_x3a_x3abx, align 4
  %15 = icmp eq i32 %14, 42
  %16 = zext i1 %15 to i8
  store i8 0, ptr %init_ok, align 1
  store i8 %16, ptr %init_ok, align 1
  %17 = load ptr, ptr %__panic1, align 8
  %18 = load i8, ptr %17, align 1
  %19 = icmp ne i8 %18, 0
  br i1 %19, label %panic_fail9, label %panic_ok8

check_fail:                                       ; preds = %panic_cont7
  %20 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %20, align 1
  %21 = getelementptr i8, ptr %20, i64 4
  store i32 10, ptr %21, align 4
  ret i32 0

panic_ok8:                                        ; preds = %check_ok
  br label %panic_cont10

panic_fail9:                                      ; preds = %check_ok
  %22 = load ptr, ptr %__panic1, align 8
  %23 = load i8, ptr %22, align 1
  %24 = load ptr, ptr %__panic1, align 8
  %25 = getelementptr i8, ptr %24, i64 4
  %26 = load i32, ptr %25, align 4
  ret i32 0

panic_cont10:                                     ; preds = %panic_ok8
  %27 = load i8, ptr %init_ok, align 1
  %28 = icmp ne i8 %27, 0
  br i1 %28, label %then, label %else

then:                                             ; preds = %panic_cont10
  br label %ifcont

else:                                             ; preds = %panic_cont10
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %29 = phi { ptr, i64 } [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f08B05D07B5566BEF, i64 2 }, %then ], [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f00391E19133920B8, i64 3 }, %else ]
  store { ptr, i64 } zeroinitializer, ptr %msg, align 8
  store { ptr, i64 } %29, ptr %msg, align 8
  %30 = load ptr, ptr %__panic1, align 8
  %31 = load i8, ptr %30, align 1
  %32 = icmp ne i8 %31, 0
  br i1 %32, label %panic_fail12, label %panic_ok11

panic_ok11:                                       ; preds = %ifcont
  br label %panic_cont13

panic_fail12:                                     ; preds = %ifcont
  %33 = load ptr, ptr %__panic1, align 8
  %34 = load i8, ptr %33, align 1
  %35 = load ptr, ptr %__panic1, align 8
  %36 = getelementptr i8, ptr %35, i64 4
  %37 = load i32, ptr %36, align 4
  ret i32 0

panic_cont13:                                     ; preds = %panic_ok11
  %38 = load { ptr, i64 }, ptr %msg, align 8
  store { ptr, i64 } %38, ptr %byref_arg, align 8
  %39 = call { i8, [1 x i8] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(ptr %ctx, ptr %byref_arg)
  store { i8, [1 x i8] } %39, ptr %match_val, align 1
  %40 = load i8, ptr %match_val, align 1
  %41 = icmp eq i8 %40, 0
  br i1 %41, label %match_arm, label %match_next

match_merge:                                      ; No predecessors!
  %42 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %42, align 1
  %43 = getelementptr i8, ptr %42, i64 4
  store i32 0, ptr %43, align 4
  store { i8, [1 x i8] } %39, ptr %match_val21, align 1
  %44 = load i8, ptr %match_val21, align 1
  %45 = icmp eq i8 %44, 0
  br i1 %45, label %match_arm24, label %match_next25

match_fail:                                       ; preds = %match_next
  unreachable

match_arm:                                        ; preds = %panic_cont13
  store {} zeroinitializer, ptr %ok, align 1
  %46 = load ptr, ptr %__panic1, align 8
  %47 = load i8, ptr %46, align 1
  %48 = icmp ne i8 %47, 0
  br i1 %48, label %panic_fail15, label %panic_ok14

match_next:                                       ; preds = %panic_cont13
  %49 = load i8, ptr %match_val, align 1
  %50 = icmp eq i8 %49, 1
  br i1 %50, label %match_arm17, label %match_fail

panic_ok14:                                       ; preds = %match_arm
  br label %panic_cont16

panic_fail15:                                     ; preds = %match_arm
  ret i32 0

panic_cont16:                                     ; preds = %panic_ok14
  ret i32 0

match_arm17:                                      ; preds = %match_next
  store { i8, [0 x i8] } zeroinitializer, ptr %err, align 1
  %51 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %51, align 1
  %52 = getelementptr i8, ptr %51, i64 4
  store i32 0, ptr %52, align 4
  %53 = load { i8, [0 x i8] }, ptr %err, align 1
  %54 = load ptr, ptr %__panic1, align 8
  store { i8, [0 x i8] } %53, ptr %ptr_arg, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg, ptr %54)
  %55 = load ptr, ptr %__panic1, align 8
  %56 = load i8, ptr %55, align 1
  %57 = icmp ne i8 %56, 0
  br i1 %57, label %panic_fail19, label %panic_ok18

panic_ok18:                                       ; preds = %match_arm17
  br label %panic_cont20

panic_fail19:                                     ; preds = %match_arm17
  ret i32 0

panic_cont20:                                     ; preds = %panic_ok18
  ret i32 0

match_merge22:                                    ; preds = %match_arm26, %match_arm24
  %58 = phi {} [ zeroinitializer, %match_arm24 ], [ zeroinitializer, %match_arm26 ]
  %59 = load ptr, ptr %__panic1, align 8
  %60 = load i8, ptr %59, align 1
  %61 = load ptr, ptr %__panic1, align 8
  %62 = getelementptr i8, ptr %61, i64 4
  %63 = load i32, ptr %62, align 4
  %64 = icmp ne i8 %60, 0
  %65 = and i1 false, %64
  %66 = zext i1 %65 to i8
  %67 = icmp ne i8 %66, 0
  br i1 %67, label %then28, label %else29

match_fail23:                                     ; preds = %match_next25
  unreachable

match_arm24:                                      ; preds = %match_merge
  br label %match_merge22

match_next25:                                     ; preds = %match_merge
  %68 = load i8, ptr %match_val21, align 1
  %69 = icmp eq i8 %68, 1
  br i1 %69, label %match_arm26, label %match_fail23

match_arm26:                                      ; preds = %match_next25
  store { i8, [1 x i8] } %39, ptr %tmp_addr, align 1
  %70 = getelementptr i8, ptr %tmp_addr, i64 1
  %71 = load { i8, [0 x i8] }, ptr %70, align 1
  %72 = load ptr, ptr %__panic1, align 8
  store { i8, [0 x i8] } %71, ptr %ptr_arg27, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg27, ptr %72)
  br label %match_merge22

then28:                                           ; preds = %match_merge22
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %63)
  br label %ifcont30

else29:                                           ; preds = %match_merge22
  br label %ifcont30

ifcont30:                                         ; preds = %else29, %then28
  %73 = phi {} [ zeroinitializer, %then28 ], [ zeroinitializer, %else29 ]
  %74 = icmp ne i8 %60, 0
  %75 = xor i1 %74, true
  %76 = zext i1 %75 to i8
  %77 = icmp ne i8 %76, 0
  %78 = and i1 false, %77
  %79 = zext i1 %78 to i8
  %80 = icmp ne i8 %79, 0
  br i1 %80, label %then31, label %else32

then31:                                           ; preds = %ifcont30
  %81 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %81, align 1
  %82 = load ptr, ptr %__panic1, align 8
  %83 = getelementptr i8, ptr %82, i64 4
  store i32 0, ptr %83, align 4
  br label %ifcont33

else32:                                           ; preds = %ifcont30
  br label %ifcont33

ifcont33:                                         ; preds = %else32, %then31
  %84 = phi {} [ zeroinitializer, %then31 ], [ zeroinitializer, %else32 ]
  %85 = icmp ne i8 %60, 0
  %86 = or i1 false, %85
  %87 = zext i1 %86 to i8
  %88 = icmp ne i8 %60, 0
  br i1 %88, label %then34, label %else35

then34:                                           ; preds = %ifcont33
  br label %ifcont36

else35:                                           ; preds = %ifcont33
  br label %ifcont36

ifcont36:                                         ; preds = %else35, %then34
  %89 = phi i32 [ %63, %then34 ], [ 0, %else35 ]
  %90 = load ptr, ptr %__panic1, align 8
  %91 = load i8, ptr %90, align 1
  %92 = icmp ne i8 %91, 0
  br i1 %92, label %panic_fail38, label %panic_ok37

panic_ok37:                                       ; preds = %ifcont36
  br label %panic_cont39

panic_fail38:                                     ; preds = %ifcont36
  %93 = load ptr, ptr %__panic1, align 8
  %94 = load i8, ptr %93, align 1
  %95 = load ptr, ptr %__panic1, align 8
  %96 = getelementptr i8, ptr %95, i64 4
  %97 = load i32, ptr %96, align 4
  ret i32 0

panic_cont39:                                     ; preds = %panic_ok37
  %98 = load ptr, ptr %__panic1, align 8
  %99 = load i8, ptr %98, align 1
  %100 = icmp ne i8 %99, 0
  br i1 %100, label %panic_fail41, label %panic_ok40

panic_ok40:                                       ; preds = %panic_cont39
  br label %panic_cont42

panic_fail41:                                     ; preds = %panic_cont39
  ret i32 0

panic_cont42:                                     ; preds = %panic_ok40
  ret i32 0
}

define void @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3ainit_x5forder_x5fok(ptr %__panic) {
entry:
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  ret void
}

define void @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3ainit_x5forder_x5fok(ptr %__panic) {
entry:
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  ret void
}

define linkonce_odr void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %data, ptr %__panic) unnamed_addr comdat {
entry:
  %match_val = alloca { i8, [0 x i8] }, align 8
  %__panic2 = alloca ptr, align 8
  %data1 = alloca ptr, align 8
  store ptr %data, ptr %data1, align 8
  store ptr %__panic, ptr %__panic2, align 8
  %0 = load ptr, ptr %__panic2, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %data1, align 8
  %3 = load { i8, [0 x i8] }, ptr %2, align 1
  store { i8, [0 x i8] } %3, ptr %match_val, align 1
  %4 = load i8, ptr %match_val, align 1
  %5 = icmp eq i8 %4, 0
  br i1 %5, label %match_arm, label %match_next

match_merge:                                      ; preds = %match_arm11, %match_arm9, %match_arm7, %match_arm5, %match_arm3, %match_arm
  %6 = phi {} [ zeroinitializer, %match_arm ], [ zeroinitializer, %match_arm3 ], [ zeroinitializer, %match_arm5 ], [ zeroinitializer, %match_arm7 ], [ zeroinitializer, %match_arm9 ], [ zeroinitializer, %match_arm11 ]
  ret void

match_fail:                                       ; preds = %match_next10
  unreachable

match_arm:                                        ; preds = %entry
  br label %match_merge

match_next:                                       ; preds = %entry
  %7 = load i8, ptr %match_val, align 1
  %8 = icmp eq i8 %7, 1
  br i1 %8, label %match_arm3, label %match_next4

match_arm3:                                       ; preds = %match_next
  br label %match_merge

match_next4:                                      ; preds = %match_next
  %9 = load i8, ptr %match_val, align 1
  %10 = icmp eq i8 %9, 2
  br i1 %10, label %match_arm5, label %match_next6

match_arm5:                                       ; preds = %match_next4
  br label %match_merge

match_next6:                                      ; preds = %match_next4
  %11 = load i8, ptr %match_val, align 1
  %12 = icmp eq i8 %11, 3
  br i1 %12, label %match_arm7, label %match_next8

match_arm7:                                       ; preds = %match_next6
  br label %match_merge

match_next8:                                      ; preds = %match_next6
  %13 = load i8, ptr %match_val, align 1
  %14 = icmp eq i8 %13, 4
  br i1 %14, label %match_arm9, label %match_next10

match_arm9:                                       ; preds = %match_next8
  br label %match_merge

match_next10:                                     ; preds = %match_next8
  %15 = load i8, ptr %match_val, align 1
  %16 = icmp eq i8 %15, 5
  br i1 %16, label %match_arm11, label %match_fail

match_arm11:                                      ; preds = %match_next10
  br label %match_merge
}

define i32 @main() {
entry:
  %byref_arg = alloca { { ptr, ptr }, { ptr, ptr }, {} }, align 8
  %sret = alloca { { ptr, ptr }, { ptr, ptr }, {} }, align 8
  %panic_record = alloca { i8, [3 x i8], i32 }, align 8
  %__panic = alloca ptr, align 8
  store ptr %panic_record, ptr %__panic, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3acontext_x5finit(ptr %sret)
  %0 = load { { ptr, ptr }, { ptr, ptr }, {} }, ptr %sret, align 8
  %1 = load ptr, ptr %__panic, align 8
  store i8 0, ptr %1, align 1
  %2 = getelementptr i8, ptr %1, i64 4
  store i32 0, ptr %2, align 4
  %3 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3ainit_x5forder_x5fok, align 1
  %4 = icmp eq i8 %3, 0
  br i1 %4, label %check_ok, label %check_fail

check_ok:                                         ; preds = %entry
  %5 = load ptr, ptr %__panic, align 8
  store { { ptr, ptr }, { ptr, ptr }, {} } %0, ptr %byref_arg, align 8
  %6 = call i32 @init_x5forder_x5fok_x3a_x3amain(ptr %byref_arg, ptr %5)
  %7 = load i8, ptr %panic_record, align 1
  %8 = getelementptr i8, ptr %panic_record, i64 4
  %9 = load i32, ptr %8, align 4
  %10 = icmp ne i8 %7, 0
  br i1 %10, label %panic_fail, label %panic_ok

check_fail:                                       ; preds = %entry
  %11 = load ptr, ptr %__panic, align 8
  store i8 1, ptr %11, align 1
  %12 = getelementptr i8, ptr %11, i64 4
  store i32 10, ptr %12, align 4
  ret i32 0

panic_fail:                                       ; preds = %check_ok
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %9)
  unreachable

panic_ok:                                         ; preds = %check_ok
  %13 = load ptr, ptr %__panic, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3ainit_x5forder_x5fok(ptr %13)
  %14 = load ptr, ptr %__panic, align 8
  %15 = load i8, ptr %14, align 1
  %16 = icmp ne i8 %15, 0
  br i1 %16, label %panic_fail2, label %panic_ok1

panic_ok1:                                        ; preds = %panic_ok
  br label %panic_cont

panic_fail2:                                      ; preds = %panic_ok
  ret i32 0

panic_cont:                                       ; preds = %panic_ok1
  %17 = load ptr, ptr %__panic, align 8
  %18 = load i8, ptr %17, align 1
  %19 = icmp ne i8 %18, 0
  br i1 %19, label %panic_fail4, label %panic_ok3

panic_ok3:                                        ; preds = %panic_cont
  br label %panic_cont5

panic_fail4:                                      ; preds = %panic_cont
  ret i32 0

panic_cont5:                                      ; preds = %panic_ok3
  %20 = load ptr, ptr %__panic, align 8
  %21 = load i8, ptr %20, align 1
  %22 = icmp ne i8 %21, 0
  br i1 %22, label %panic_fail7, label %panic_ok6

panic_ok6:                                        ; preds = %panic_cont5
  br label %panic_cont8

panic_fail7:                                      ; preds = %panic_cont5
  ret i32 0

panic_cont8:                                      ; preds = %panic_ok6
  ret i32 %6
}

attributes #0 = { noreturn nounwind }
attributes #1 = { nounwind }

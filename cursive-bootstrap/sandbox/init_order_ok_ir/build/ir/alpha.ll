; ModuleID = 'alpha'
source_filename = "alpha"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@alpha_x3a_x3aax = constant i32 41, align 4
@cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3aalpha = global i8 0

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

define void @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3aalpha(ptr %__panic) {
entry:
  %ax = alloca i8, align 1
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  store i8 0, ptr %ax, align 1
  store i8 41, ptr %ax, align 1
  %2 = load i8, ptr %ax, align 1
  %3 = sext i8 %2 to i32
  store i32 %3, ptr @alpha_x3a_x3aax, align 4
  %4 = load ptr, ptr %__panic1, align 8
  %5 = load i8, ptr %4, align 1
  %6 = icmp ne i8 %5, 0
  br i1 %6, label %init_panic, label %init_ok

init_panic:                                       ; preds = %entry
  store i8 1, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3aalpha, align 1
  store i8 1, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3aalpha, align 1
  %7 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %7, align 1
  %8 = getelementptr i8, ptr %7, i64 4
  store i32 10, ptr %8, align 4
  ret void

init_ok:                                          ; preds = %entry
  br label %init_cont

init_cont:                                        ; preds = %init_ok
  ret void
}

define void @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3aalpha(ptr %__panic) {
entry:
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3aalpha, align 1
  %3 = icmp eq i8 %2, 0
  br i1 %3, label %check_ok, label %check_fail

check_ok:                                         ; preds = %entry
  ret void

check_fail:                                       ; preds = %entry
  %4 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %4, align 1
  %5 = getelementptr i8, ptr %4, i64 4
  store i32 10, ptr %5, align 4
  ret void
}

attributes #0 = { noreturn nounwind }
attributes #1 = { nounwind }

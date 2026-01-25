; ModuleID = 'test'
source_filename = "test"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f66BB19BB43C361AF = internal unnamed_addr constant [13 x i8] c"Cleanup-Empty", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325 = internal unnamed_addr constant [0 x i8] zeroinitializer, align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f64F225C50587781C = internal unnamed_addr constant [10 x i8] c"PanicCheck", align 1
@cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest = global i8 0
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fB23BD3142F5DCB82 = internal unnamed_addr constant [13 x i8] c"Cleanup-Start", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f74A2725F2350AC62 = internal unnamed_addr constant [12 x i8] c"Cleanup-Done", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fC0DE5040743375DA = internal unnamed_addr constant [14 x i8] c"EntryStub-Decl", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f3CDD957A3B293B34 = internal unnamed_addr constant [19 x i8] c"ContextInitSym-Decl", align 1

; Function Attrs: noreturn nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3apanic(i32) #0

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3acontext_x5finit(ptr noalias noundef nonnull sret align 8 dereferenceable(32)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3adrop_x5fmanaged(ptr noundef nonnull readonly align 8 dereferenceable(24)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3adrop_x5fmanaged(ptr noundef nonnull readonly align 8 dereferenceable(24)) #1

; Function Attrs: nounwind
declare { i64 } @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3anew_x5fscoped(ptr noundef nonnull readonly align 8 dereferenceable(40)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3aalloc(ptr noundef nonnull readonly align 8 dereferenceable(8), ptr noundef nonnull readonly align 1) #1

; Function Attrs: nounwind
declare { i64 } @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3areset_x5funchecked(ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare { i64 } @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afreeze(ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare { i64 } @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3athaw(ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare { i64 } @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afree_x5funchecked(ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3afrom(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3aas_x5fview(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(24)) #1

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
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aas_x5fview(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(24)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3ato_x5fmanaged(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aview(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aview_x5fstring(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aappend(ptr noalias noundef nonnull sret align 8 dereferenceable(24), ptr noundef nonnull readonly align 8 dereferenceable(24), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i64 @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3alength(ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i8 @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3ais_x5fempty(ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i8 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3awith_x5fquota(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare ptr @cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3aalloc_x5fraw(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3adealloc_x5fraw(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(8), ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare void @File_x3a_x3aRead_x3a_x3aread_x5fall(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare void @File_x3a_x3aRead_x3a_x3aread_x5fall_x5fbytes(ptr noalias noundef nonnull sret align 8 dereferenceable(32), ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare void @File_x3a_x3aRead_x3a_x3aclose({ i64 }) #1

; Function Attrs: nounwind
declare i16 @File_x3a_x3aWrite_x3a_x3awrite(ptr noundef nonnull readonly align 8 dereferenceable(8), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @File_x3a_x3aWrite_x3a_x3aflush(ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare void @File_x3a_x3aWrite_x3a_x3aclose({ i64 }) #1

; Function Attrs: nounwind
declare i16 @File_x3a_x3aAppend_x3a_x3awrite(ptr noundef nonnull readonly align 8 dereferenceable(8), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @File_x3a_x3aAppend_x3a_x3aflush(ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare void @File_x3a_x3aAppend_x3a_x3aclose({ i64 }) #1

; Function Attrs: nounwind
declare void @DirIter_x3a_x3aOpen_x3a_x3anext(ptr noalias noundef nonnull sret align 8 dereferenceable(64), ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare void @DirIter_x3a_x3aOpen_x3a_x3aclose({ i64 }) #1

; Function Attrs: nounwind
declare void @CancelToken_x3a_x3aActive_x3a_x3acancel(ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare i8 @CancelToken_x3a_x3aActive_x3a_x3ais_x5fcancelled(ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare ptr @CancelToken_x3a_x3aActive_x3a_x3achild(ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare i8 @CancelToken_x3a_x3aCancelled_x3a_x3ais_x5fcancelled(ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

; Function Attrs: nounwind
declare ptr @cursive0_parallel_begin(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull align 1 dereferenceable(1), ptr noundef nonnull align 1 dereferenceable(1)) #1

; Function Attrs: nounwind
declare i32 @cursive0_parallel_join(ptr noundef nonnull align 1 dereferenceable(1)) #1

; Function Attrs: nounwind
declare ptr @cursive0_spawn_create(ptr noundef nonnull align 1 dereferenceable(1), i64, ptr, i64) #1

; Function Attrs: nounwind
declare ptr @cursive0_spawn_wait(ptr noundef nonnull align 1 dereferenceable(1)) #1

; Function Attrs: nounwind
declare void @cursive0_dispatch_run(ptr noundef nonnull readonly align 8 dereferenceable(24), i64, i64, ptr, ptr noundef nonnull align 1 dereferenceable(1), ptr noundef nonnull align 1 dereferenceable(1), ptr noundef nonnull align 1 dereferenceable(1), ptr, i32, i64) #1

; Function Attrs: nounwind
declare ptr @cursive0_cancel_token_new() #1

; Function Attrs: nounwind
declare void @cursive0_cancel_token_cancel(ptr noundef nonnull align 1 dereferenceable(1)) #1

; Function Attrs: nounwind
declare i32 @cursive0_cancel_token_is_cancelled(ptr noundef nonnull align 1 dereferenceable(1)) #1

; Function Attrs: nounwind
declare void @cursive0_parallel_work_panic(ptr noundef nonnull readonly align 1 dereferenceable(1), ptr noundef nonnull readonly align 4 dereferenceable(4)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3acontext_x3a_x3acpu(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(32)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3acontext_x3a_x3agpu(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(32)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3acontext_x3a_x3ainline(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(32)) #1

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3aexecution_x5fdomain_x3a_x3aname(ptr noalias noundef nonnull sret align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i64 @cursive_x3a_x3aruntime_x3a_x3aexecution_x5fdomain_x3a_x3amax_x5fconcurrency(ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare ptr @CancelToken_x3a_x3anew() #1

define i32 @test_x3a_x3aidentity(ptr noundef nonnull readonly align 4 dereferenceable(4) %x, ptr %__panic) {
entry:
  %byref_arg2 = alloca { ptr, i64 }, align 8
  %byref_arg = alloca { ptr, i64 }, align 8
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f66BB19BB43C361AF, i64 13 }, ptr %byref_arg, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg2, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg, ptr %byref_arg2)
  %2 = load i32, ptr %x, align 4
  ret i32 %2
}

define i32 @test_x3a_x3aadd(ptr noundef nonnull readonly align 4 dereferenceable(4) %x, ptr noundef nonnull readonly align 4 dereferenceable(4) %y, ptr %__panic) {
entry:
  %byref_arg6 = alloca { ptr, i64 }, align 8
  %byref_arg5 = alloca { ptr, i64 }, align 8
  %byref_arg4 = alloca { ptr, i64 }, align 8
  %byref_arg3 = alloca { ptr, i64 }, align 8
  %byref_arg2 = alloca { ptr, i64 }, align 8
  %byref_arg = alloca { ptr, i64 }, align 8
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load i32, ptr %x, align 4
  %3 = load i32, ptr %y, align 4
  %4 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %2, i32 %3)
  %5 = extractvalue { i32, i1 } %4, 1
  %6 = xor i1 %5, true
  br i1 %6, label %check_ok, label %check_fail

check_ok:                                         ; preds = %check_fail, %entry
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f64F225C50587781C, i64 10 }, ptr %byref_arg, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg2, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg, ptr %byref_arg2)
  %7 = load ptr, ptr %__panic1, align 8
  %8 = load i8, ptr %7, align 1
  %9 = icmp ne i8 %8, 0
  br i1 %9, label %panic_fail, label %panic_ok

check_fail:                                       ; preds = %entry
  %10 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %10, align 1
  %11 = getelementptr i8, ptr %10, i64 4
  store i32 4, ptr %11, align 4
  br label %check_ok

panic_ok:                                         ; preds = %check_ok
  br label %panic_cont

panic_fail:                                       ; preds = %check_ok
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f66BB19BB43C361AF, i64 13 }, ptr %byref_arg3, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg4, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg3, ptr %byref_arg4)
  ret i32 0

panic_cont:                                       ; preds = %panic_ok
  %12 = load i32, ptr %x, align 4
  %13 = load i32, ptr %y, align 4
  %14 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %12, i32 %13)
  %15 = extractvalue { i32, i1 } %14, 0
  %16 = extractvalue { i32, i1 } %14, 1
  %17 = freeze i32 %15
  br i1 %16, label %op_fail, label %op_ok

op_ok:                                            ; preds = %panic_cont
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f66BB19BB43C361AF, i64 13 }, ptr %byref_arg5, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg6, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg5, ptr %byref_arg6)
  ret i32 %17

op_fail:                                          ; preds = %panic_cont
  %18 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %18, align 1
  %19 = getelementptr i8, ptr %18, i64 4
  store i32 4, ptr %19, align 4
  ret i32 0
}

define i32 @test_x3a_x3acompute(ptr noundef nonnull readonly align 4 dereferenceable(4) %a, ptr noundef nonnull readonly align 4 dereferenceable(4) %b, ptr noundef nonnull readonly align 4 dereferenceable(4) %c, ptr %__panic) {
entry:
  %byref_arg23 = alloca { ptr, i64 }, align 8
  %byref_arg22 = alloca { ptr, i64 }, align 8
  %byref_arg21 = alloca { ptr, i64 }, align 8
  %byref_arg20 = alloca { ptr, i64 }, align 8
  %byref_arg18 = alloca { ptr, i64 }, align 8
  %byref_arg17 = alloca { ptr, i64 }, align 8
  %byref_arg16 = alloca { ptr, i64 }, align 8
  %byref_arg15 = alloca { ptr, i64 }, align 8
  %byref_arg12 = alloca { ptr, i64 }, align 8
  %byref_arg11 = alloca { ptr, i64 }, align 8
  %sum = alloca i32, align 4
  %byref_arg6 = alloca { ptr, i64 }, align 8
  %byref_arg5 = alloca { ptr, i64 }, align 8
  %byref_arg4 = alloca { ptr, i64 }, align 8
  %byref_arg = alloca { ptr, i64 }, align 8
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %3 = icmp eq i8 %2, 0
  br i1 %3, label %check_ok, label %check_fail

check_ok:                                         ; preds = %check_fail, %entry
  %4 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %5 = icmp eq i8 %4, 0
  br i1 %5, label %check_ok2, label %check_fail3

check_fail:                                       ; preds = %entry
  %6 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %6, align 1
  %7 = getelementptr i8, ptr %6, i64 4
  store i32 10, ptr %7, align 4
  br label %check_ok

check_ok2:                                        ; preds = %check_fail3, %check_ok
  %8 = load ptr, ptr %__panic1, align 8
  %9 = call i32 @test_x3a_x3aadd(ptr %a, ptr %b, ptr %8)
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f64F225C50587781C, i64 10 }, ptr %byref_arg, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg4, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg, ptr %byref_arg4)
  %10 = load ptr, ptr %__panic1, align 8
  %11 = load i8, ptr %10, align 1
  %12 = icmp ne i8 %11, 0
  br i1 %12, label %panic_fail, label %panic_ok

check_fail3:                                      ; preds = %check_ok
  %13 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %13, align 1
  %14 = getelementptr i8, ptr %13, i64 4
  store i32 10, ptr %14, align 4
  br label %check_ok2

panic_ok:                                         ; preds = %check_ok2
  br label %panic_cont

panic_fail:                                       ; preds = %check_ok2
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f66BB19BB43C361AF, i64 13 }, ptr %byref_arg5, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg6, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg5, ptr %byref_arg6)
  ret i32 0

panic_cont:                                       ; preds = %panic_ok
  store i32 0, ptr %sum, align 4
  store i32 %9, ptr %sum, align 4
  %15 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %16 = icmp eq i8 %15, 0
  br i1 %16, label %check_ok7, label %check_fail8

check_ok7:                                        ; preds = %check_fail8, %panic_cont
  %17 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %18 = icmp eq i8 %17, 0
  br i1 %18, label %check_ok9, label %check_fail10

check_fail8:                                      ; preds = %panic_cont
  %19 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %19, align 1
  %20 = getelementptr i8, ptr %19, i64 4
  store i32 10, ptr %20, align 4
  br label %check_ok7

check_ok9:                                        ; preds = %check_fail10, %check_ok7
  %21 = load ptr, ptr %__panic1, align 8
  %22 = call i32 @test_x3a_x3aadd(ptr %sum, ptr %c, ptr %21)
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f64F225C50587781C, i64 10 }, ptr %byref_arg11, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg12, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg11, ptr %byref_arg12)
  %23 = load ptr, ptr %__panic1, align 8
  %24 = load i8, ptr %23, align 1
  %25 = icmp ne i8 %24, 0
  br i1 %25, label %panic_fail14, label %panic_ok13

check_fail10:                                     ; preds = %check_ok7
  %26 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %26, align 1
  %27 = getelementptr i8, ptr %26, i64 4
  store i32 10, ptr %27, align 4
  br label %check_ok9

panic_ok13:                                       ; preds = %check_ok9
  br label %panic_cont19

panic_fail14:                                     ; preds = %check_ok9
  %28 = load ptr, ptr %__panic1, align 8
  %29 = load i8, ptr %28, align 1
  %30 = load ptr, ptr %__panic1, align 8
  %31 = getelementptr i8, ptr %30, i64 4
  %32 = load i32, ptr %31, align 4
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fB23BD3142F5DCB82, i64 13 }, ptr %byref_arg15, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg16, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg15, ptr %byref_arg16)
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f74A2725F2350AC62, i64 12 }, ptr %byref_arg17, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg18, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg17, ptr %byref_arg18)
  ret i32 0

panic_cont19:                                     ; preds = %panic_ok13
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fB23BD3142F5DCB82, i64 13 }, ptr %byref_arg20, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg21, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg20, ptr %byref_arg21)
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f74A2725F2350AC62, i64 12 }, ptr %byref_arg22, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg23, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg22, ptr %byref_arg23)
  %33 = load ptr, ptr %__panic1, align 8
  %34 = load i8, ptr %33, align 1
  %35 = icmp ne i8 %34, 0
  br i1 %35, label %panic_fail25, label %panic_ok24

panic_ok24:                                       ; preds = %panic_cont19
  br label %panic_cont26

panic_fail25:                                     ; preds = %panic_cont19
  ret i32 0

panic_cont26:                                     ; preds = %panic_ok24
  ret i32 %22
}

define i32 @test_x3a_x3amain(ptr noundef nonnull readonly align 8 dereferenceable(32) %ctx, ptr %__panic) {
entry:
  %byref_arg19 = alloca { ptr, i64 }, align 8
  %byref_arg18 = alloca { ptr, i64 }, align 8
  %byref_arg17 = alloca { ptr, i64 }, align 8
  %byref_arg16 = alloca { ptr, i64 }, align 8
  %byref_arg14 = alloca { ptr, i64 }, align 8
  %byref_arg13 = alloca { ptr, i64 }, align 8
  %byref_arg12 = alloca { ptr, i64 }, align 8
  %byref_arg11 = alloca { ptr, i64 }, align 8
  %byref_arg8 = alloca { ptr, i64 }, align 8
  %byref_arg7 = alloca { ptr, i64 }, align 8
  %z = alloca i32, align 4
  %y = alloca i32, align 4
  %x = alloca i32, align 4
  %byref_arg4 = alloca { ptr, i64 }, align 8
  %byref_arg3 = alloca { ptr, i64 }, align 8
  %byref_arg2 = alloca { ptr, i64 }, align 8
  %byref_arg = alloca { ptr, i64 }, align 8
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %__panic1, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3atest(ptr %2)
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f64F225C50587781C, i64 10 }, ptr %byref_arg, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg2, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg, ptr %byref_arg2)
  %3 = load ptr, ptr %__panic1, align 8
  %4 = load i8, ptr %3, align 1
  %5 = icmp ne i8 %4, 0
  br i1 %5, label %panic_fail, label %panic_ok

panic_ok:                                         ; preds = %entry
  br label %panic_cont

panic_fail:                                       ; preds = %entry
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f66BB19BB43C361AF, i64 13 }, ptr %byref_arg3, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg4, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg3, ptr %byref_arg4)
  ret i32 0

panic_cont:                                       ; preds = %panic_ok
  store i32 0, ptr %x, align 4
  store i32 10, ptr %x, align 4
  store i32 0, ptr %y, align 4
  store i32 20, ptr %y, align 4
  store i32 0, ptr %z, align 4
  store i32 12, ptr %z, align 4
  %6 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %7 = icmp eq i8 %6, 0
  br i1 %7, label %check_ok, label %check_fail

check_ok:                                         ; preds = %check_fail, %panic_cont
  %8 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %9 = icmp eq i8 %8, 0
  br i1 %9, label %check_ok5, label %check_fail6

check_fail:                                       ; preds = %panic_cont
  %10 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %10, align 1
  %11 = getelementptr i8, ptr %10, i64 4
  store i32 10, ptr %11, align 4
  br label %check_ok

check_ok5:                                        ; preds = %check_fail6, %check_ok
  %12 = load ptr, ptr %__panic1, align 8
  %13 = call i32 @test_x3a_x3acompute(ptr %x, ptr %y, ptr %z, ptr %12)
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f64F225C50587781C, i64 10 }, ptr %byref_arg7, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg8, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg7, ptr %byref_arg8)
  %14 = load ptr, ptr %__panic1, align 8
  %15 = load i8, ptr %14, align 1
  %16 = icmp ne i8 %15, 0
  br i1 %16, label %panic_fail10, label %panic_ok9

check_fail6:                                      ; preds = %check_ok
  %17 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %17, align 1
  %18 = getelementptr i8, ptr %17, i64 4
  store i32 10, ptr %18, align 4
  br label %check_ok5

panic_ok9:                                        ; preds = %check_ok5
  br label %panic_cont15

panic_fail10:                                     ; preds = %check_ok5
  %19 = load ptr, ptr %__panic1, align 8
  %20 = load i8, ptr %19, align 1
  %21 = load ptr, ptr %__panic1, align 8
  %22 = getelementptr i8, ptr %21, i64 4
  %23 = load i32, ptr %22, align 4
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fB23BD3142F5DCB82, i64 13 }, ptr %byref_arg11, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg12, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg11, ptr %byref_arg12)
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f74A2725F2350AC62, i64 12 }, ptr %byref_arg13, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg14, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg13, ptr %byref_arg14)
  ret i32 0

panic_cont15:                                     ; preds = %panic_ok9
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fB23BD3142F5DCB82, i64 13 }, ptr %byref_arg16, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg17, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg16, ptr %byref_arg17)
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f74A2725F2350AC62, i64 12 }, ptr %byref_arg18, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg19, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg18, ptr %byref_arg19)
  %24 = load ptr, ptr %__panic1, align 8
  %25 = load i8, ptr %24, align 1
  %26 = icmp ne i8 %25, 0
  br i1 %26, label %panic_fail21, label %panic_ok20

panic_ok20:                                       ; preds = %panic_cont15
  br label %panic_cont22

panic_fail21:                                     ; preds = %panic_cont15
  ret i32 0

panic_cont22:                                     ; preds = %panic_ok20
  ret i32 %13
}

define void @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3atest(ptr %__panic) {
entry:
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  ret void
}

define void @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3atest(ptr %__panic) {
entry:
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32) #2

define i32 @main() {
entry:
  %byref_arg10 = alloca { ptr, i64 }, align 8
  %byref_arg9 = alloca { ptr, i64 }, align 8
  %byref_arg6 = alloca { ptr, i64 }, align 8
  %byref_arg5 = alloca { ptr, i64 }, align 8
  %byref_arg4 = alloca { { ptr, ptr }, { ptr, ptr }, {} }, align 8
  %sret = alloca { { ptr, ptr }, { ptr, ptr }, {} }, align 8
  %byref_arg3 = alloca { ptr, i64 }, align 8
  %byref_arg2 = alloca { ptr, i64 }, align 8
  %byref_arg1 = alloca { ptr, i64 }, align 8
  %byref_arg = alloca { ptr, i64 }, align 8
  %panic_record = alloca { i8, [3 x i8], i32 }, align 8
  %__panic = alloca ptr, align 8
  store ptr %panic_record, ptr %__panic, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fC0DE5040743375DA, i64 14 }, ptr %byref_arg, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg1, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg, ptr %byref_arg1)
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f3CDD957A3B293B34, i64 19 }, ptr %byref_arg2, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg3, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg2, ptr %byref_arg3)
  call void @cursive_x3a_x3aruntime_x3a_x3acontext_x5finit(ptr %sret)
  %0 = load { { ptr, ptr }, { ptr, ptr }, {} }, ptr %sret, align 8
  %1 = load ptr, ptr %__panic, align 8
  store i8 0, ptr %1, align 1
  %2 = getelementptr i8, ptr %1, i64 4
  store i32 0, ptr %2, align 4
  %3 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %4 = icmp eq i8 %3, 0
  br i1 %4, label %check_ok, label %check_fail

check_ok:                                         ; preds = %check_fail, %entry
  %5 = load ptr, ptr %__panic, align 8
  store { { ptr, ptr }, { ptr, ptr }, {} } %0, ptr %byref_arg4, align 8
  %6 = call i32 @test_x3a_x3amain(ptr %byref_arg4, ptr %5)
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
  br label %check_ok

panic_fail:                                       ; preds = %check_ok
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %9)
  unreachable

panic_ok:                                         ; preds = %check_ok
  %13 = load ptr, ptr %__panic, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3atest(ptr %13)
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f64F225C50587781C, i64 10 }, ptr %byref_arg5, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg6, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg5, ptr %byref_arg6)
  %14 = load ptr, ptr %__panic, align 8
  %15 = load i8, ptr %14, align 1
  %16 = icmp ne i8 %15, 0
  br i1 %16, label %panic_fail8, label %panic_ok7

panic_ok7:                                        ; preds = %panic_ok
  br label %panic_cont

panic_fail8:                                      ; preds = %panic_ok
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f66BB19BB43C361AF, i64 13 }, ptr %byref_arg9, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fCBF29CE484222325, i64 0 }, ptr %byref_arg10, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3aspec_x5ftrace_x3a_x3aemit(ptr %byref_arg9, ptr %byref_arg10)
  ret i32 0

panic_cont:                                       ; preds = %panic_ok7
  ret i32 %6
}

attributes #0 = { noreturn nounwind }
attributes #1 = { nounwind }
attributes #2 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

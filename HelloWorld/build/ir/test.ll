; ModuleID = 'test'
source_filename = "test"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aLogger = comdat any

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError = comdat any

@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fAF63FC4C860222EC = internal unnamed_addr constant [1 x i8] c"A", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fAF63FF4C86022805 = internal unnamed_addr constant [1 x i8] c"B", align 1
@cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest = global i8 0

; Function Attrs: noreturn nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3apanic(i32) #0

; Function Attrs: nounwind
declare void @cursive_x3a_x3aruntime_x3a_x3acontext_x5finit(ptr noalias noundef nonnull sret align 8 dereferenceable(32)) #1

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
declare { [16 x i8], [0 x i64] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { [16 x i8], [0 x i64] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { [16 x i8], [0 x i64] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { [16 x i8], [0 x i64] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

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
declare { [16 x i8], [0 x i64] } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { ptr, ptr } @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(16)) #1

; Function Attrs: nounwind
declare { ptr, ptr } @cursive_x3a_x3aruntime_x3a_x3aheap_x3a_x3awith_x5fquota(ptr noundef nonnull readonly align 8 dereferenceable(16), ptr noundef nonnull readonly align 8 dereferenceable(8)) #1

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

define void @test_x3a_x3aLogger_x3a_x3adrop(ptr noundef nonnull readonly align 8 dereferenceable(32) %self, ptr %__panic) {
entry:
  %ptr_arg21 = alloca i8, align 1
  %tmp_addr20 = alloca i16, align 2
  %match_val14 = alloca i16, align 2
  %ptr_arg = alloca i8, align 1
  %tmp_addr4 = alloca i16, align 2
  %err = alloca i8, align 1
  %tmp_addr2 = alloca i16, align 2
  %ok = alloca {}, align 8
  %match_val = alloca i16, align 2
  %byref_arg = alloca { ptr, i64 }, align 8
  %tmp_addr = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load { { ptr, ptr }, { ptr, i64 } }, ptr %self, align 8
  store { { ptr, ptr }, { ptr, i64 } } %2, ptr %tmp_addr, align 8
  %3 = getelementptr i8, ptr %tmp_addr, i64 16
  %4 = load { ptr, i64 }, ptr %3, align 8
  store { ptr, i64 } %4, ptr %byref_arg, align 8
  %5 = call i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(ptr %self, ptr %byref_arg)
  store i16 %5, ptr %match_val, align 2
  %6 = load i8, ptr %match_val, align 1
  %7 = icmp eq i8 %6, 0
  br i1 %7, label %match_arm, label %match_next

match_merge:                                      ; preds = %panic_cont13, %panic_cont
  %8 = phi {} [ zeroinitializer, %panic_cont ], [ zeroinitializer, %panic_cont13 ]
  %9 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %9, align 1
  %10 = getelementptr i8, ptr %9, i64 4
  store i32 0, ptr %10, align 4
  store i16 %5, ptr %match_val14, align 2
  %11 = load i8, ptr %match_val14, align 1
  %12 = icmp eq i8 %11, 0
  br i1 %12, label %match_arm17, label %match_next18

match_fail:                                       ; preds = %match_next
  unreachable

match_arm:                                        ; preds = %entry
  store i16 %5, ptr %tmp_addr2, align 2
  store {} zeroinitializer, ptr %ok, align 1
  store {} zeroinitializer, ptr %ok, align 1
  %13 = load ptr, ptr %__panic1, align 8
  %14 = load i8, ptr %13, align 1
  %15 = icmp ne i8 %14, 0
  br i1 %15, label %panic_fail, label %panic_ok

match_next:                                       ; preds = %entry
  %16 = load i8, ptr %match_val, align 1
  %17 = icmp eq i8 %16, 1
  br i1 %17, label %match_arm3, label %match_fail

panic_ok:                                         ; preds = %match_arm
  br label %panic_cont

panic_fail:                                       ; preds = %match_arm
  ret void

panic_cont:                                       ; preds = %panic_ok
  br label %match_merge

match_arm3:                                       ; preds = %match_next
  store i16 %5, ptr %tmp_addr4, align 2
  %18 = getelementptr i8, ptr %tmp_addr4, i64 1
  %19 = load i8, ptr %18, align 1
  store i8 0, ptr %err, align 1
  store i8 %19, ptr %err, align 1
  %20 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %20, align 1
  %21 = getelementptr i8, ptr %20, i64 4
  store i32 0, ptr %21, align 4
  %22 = load i8, ptr %err, align 1
  %23 = load ptr, ptr %__panic1, align 8
  store i8 %22, ptr %ptr_arg, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg, ptr %23)
  %24 = load ptr, ptr %__panic1, align 8
  %25 = load i8, ptr %24, align 1
  %26 = load ptr, ptr %__panic1, align 8
  %27 = getelementptr i8, ptr %26, i64 4
  %28 = load i32, ptr %27, align 4
  %29 = icmp ne i8 %25, 0
  %30 = and i1 false, %29
  %31 = zext i1 %30 to i8
  %32 = icmp ne i8 %31, 0
  br i1 %32, label %then, label %else

then:                                             ; preds = %match_arm3
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %28)
  br label %ifcont

else:                                             ; preds = %match_arm3
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %33 = phi {} [ zeroinitializer, %then ], [ zeroinitializer, %else ]
  %34 = icmp ne i8 %25, 0
  %35 = xor i1 %34, true
  %36 = zext i1 %35 to i8
  %37 = icmp ne i8 %36, 0
  %38 = and i1 false, %37
  %39 = zext i1 %38 to i8
  %40 = icmp ne i8 %39, 0
  br i1 %40, label %then5, label %else6

then5:                                            ; preds = %ifcont
  %41 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %41, align 1
  %42 = load ptr, ptr %__panic1, align 8
  %43 = getelementptr i8, ptr %42, i64 4
  store i32 0, ptr %43, align 4
  br label %ifcont7

else6:                                            ; preds = %ifcont
  br label %ifcont7

ifcont7:                                          ; preds = %else6, %then5
  %44 = phi {} [ zeroinitializer, %then5 ], [ zeroinitializer, %else6 ]
  %45 = icmp ne i8 %25, 0
  %46 = or i1 false, %45
  %47 = zext i1 %46 to i8
  %48 = icmp ne i8 %25, 0
  br i1 %48, label %then8, label %else9

then8:                                            ; preds = %ifcont7
  br label %ifcont10

else9:                                            ; preds = %ifcont7
  br label %ifcont10

ifcont10:                                         ; preds = %else9, %then8
  %49 = phi i32 [ %28, %then8 ], [ 0, %else9 ]
  %50 = load ptr, ptr %__panic1, align 8
  %51 = load i8, ptr %50, align 1
  %52 = icmp ne i8 %51, 0
  br i1 %52, label %panic_fail12, label %panic_ok11

panic_ok11:                                       ; preds = %ifcont10
  br label %panic_cont13

panic_fail12:                                     ; preds = %ifcont10
  ret void

panic_cont13:                                     ; preds = %panic_ok11
  br label %match_merge

match_merge15:                                    ; preds = %match_arm19, %match_arm17
  %53 = phi {} [ zeroinitializer, %match_arm17 ], [ zeroinitializer, %match_arm19 ]
  %54 = load ptr, ptr %__panic1, align 8
  %55 = load i8, ptr %54, align 1
  %56 = load ptr, ptr %__panic1, align 8
  %57 = getelementptr i8, ptr %56, i64 4
  %58 = load i32, ptr %57, align 4
  %59 = icmp ne i8 %55, 0
  %60 = and i1 false, %59
  %61 = zext i1 %60 to i8
  %62 = icmp ne i8 %61, 0
  br i1 %62, label %then22, label %else23

match_fail16:                                     ; preds = %match_next18
  unreachable

match_arm17:                                      ; preds = %match_merge
  br label %match_merge15

match_next18:                                     ; preds = %match_merge
  %63 = load i8, ptr %match_val14, align 1
  %64 = icmp eq i8 %63, 1
  br i1 %64, label %match_arm19, label %match_fail16

match_arm19:                                      ; preds = %match_next18
  store i16 %5, ptr %tmp_addr20, align 2
  %65 = getelementptr i8, ptr %tmp_addr20, i64 1
  %66 = load i8, ptr %65, align 1
  %67 = load ptr, ptr %__panic1, align 8
  store i8 %66, ptr %ptr_arg21, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg21, ptr %67)
  br label %match_merge15

then22:                                           ; preds = %match_merge15
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %58)
  br label %ifcont24

else23:                                           ; preds = %match_merge15
  br label %ifcont24

ifcont24:                                         ; preds = %else23, %then22
  %68 = phi {} [ zeroinitializer, %then22 ], [ zeroinitializer, %else23 ]
  %69 = icmp ne i8 %55, 0
  %70 = xor i1 %69, true
  %71 = zext i1 %70 to i8
  %72 = icmp ne i8 %71, 0
  %73 = and i1 false, %72
  %74 = zext i1 %73 to i8
  %75 = icmp ne i8 %74, 0
  br i1 %75, label %then25, label %else26

then25:                                           ; preds = %ifcont24
  %76 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %76, align 1
  %77 = load ptr, ptr %__panic1, align 8
  %78 = getelementptr i8, ptr %77, i64 4
  store i32 0, ptr %78, align 4
  br label %ifcont27

else26:                                           ; preds = %ifcont24
  br label %ifcont27

ifcont27:                                         ; preds = %else26, %then25
  %79 = phi {} [ zeroinitializer, %then25 ], [ zeroinitializer, %else26 ]
  %80 = icmp ne i8 %55, 0
  %81 = or i1 false, %80
  %82 = zext i1 %81 to i8
  %83 = icmp ne i8 %55, 0
  br i1 %83, label %then28, label %else29

then28:                                           ; preds = %ifcont27
  br label %ifcont30

else29:                                           ; preds = %ifcont27
  br label %ifcont30

ifcont30:                                         ; preds = %else29, %then28
  %84 = phi i32 [ %58, %then28 ], [ 0, %else29 ]
  %85 = load ptr, ptr %__panic1, align 8
  %86 = load i8, ptr %85, align 1
  %87 = icmp ne i8 %86, 0
  br i1 %87, label %panic_fail32, label %panic_ok31

panic_ok31:                                       ; preds = %ifcont30
  br label %panic_cont33

panic_fail32:                                     ; preds = %ifcont30
  ret void

panic_cont33:                                     ; preds = %panic_ok31
  ret void
}

define i32 @test_x3a_x3amain(ptr noundef nonnull readonly align 8 dereferenceable(32) %ctx, ptr %__panic) {
entry:
  %ptr_arg46 = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %ptr_arg36 = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %ptr_arg25 = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %ptr_arg15 = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %tmp_addr12 = alloca { { ptr, ptr }, { ptr, ptr }, {} }, align 8
  %record11 = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %b = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %ptr_arg = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %tmp_addr = alloca { { ptr, ptr }, { ptr, ptr }, {} }, align 8
  %record = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %a = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %__panic1, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3atest(ptr %2)
  %3 = load ptr, ptr %__panic1, align 8
  %4 = load i8, ptr %3, align 1
  %5 = icmp ne i8 %4, 0
  br i1 %5, label %panic_fail, label %panic_ok

panic_ok:                                         ; preds = %entry
  br label %panic_cont

panic_fail:                                       ; preds = %entry
  ret i32 0

panic_cont:                                       ; preds = %panic_ok
  store { { ptr, ptr }, { ptr, i64 } } zeroinitializer, ptr %record, align 8
  %6 = load { { ptr, ptr }, { ptr, ptr }, {} }, ptr %ctx, align 8
  store { { ptr, ptr }, { ptr, ptr }, {} } %6, ptr %tmp_addr, align 8
  %7 = load { ptr, ptr }, ptr %tmp_addr, align 8
  store { ptr, ptr } %7, ptr %record, align 8
  %8 = getelementptr i8, ptr %record, i64 16
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fAF63FC4C860222EC, i64 1 }, ptr %8, align 8
  %9 = load { { ptr, ptr }, { ptr, i64 } }, ptr %record, align 8
  store { { ptr, ptr }, { ptr, i64 } } zeroinitializer, ptr %a, align 8
  store { { ptr, ptr }, { ptr, i64 } } %9, ptr %a, align 8
  %10 = load ptr, ptr %__panic1, align 8
  %11 = load i8, ptr %10, align 1
  %12 = icmp ne i8 %11, 0
  br i1 %12, label %panic_fail3, label %panic_ok2

panic_ok2:                                        ; preds = %panic_cont
  br label %panic_cont10

panic_fail3:                                      ; preds = %panic_cont
  %13 = load ptr, ptr %__panic1, align 8
  %14 = load i8, ptr %13, align 1
  %15 = load ptr, ptr %__panic1, align 8
  %16 = getelementptr i8, ptr %15, i64 4
  %17 = load i32, ptr %16, align 4
  %18 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %18, align 1
  %19 = getelementptr i8, ptr %18, i64 4
  store i32 0, ptr %19, align 4
  %20 = load { { ptr, ptr }, { ptr, i64 } }, ptr %a, align 8
  %21 = load ptr, ptr %__panic1, align 8
  store { { ptr, ptr }, { ptr, i64 } } %20, ptr %ptr_arg, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aLogger(ptr %ptr_arg, ptr %21)
  %22 = load ptr, ptr %__panic1, align 8
  %23 = load i8, ptr %22, align 1
  %24 = load ptr, ptr %__panic1, align 8
  %25 = getelementptr i8, ptr %24, i64 4
  %26 = load i32, ptr %25, align 4
  %27 = icmp ne i8 %23, 0
  %28 = and i1 true, %27
  %29 = zext i1 %28 to i8
  %30 = icmp ne i8 %29, 0
  br i1 %30, label %then, label %else

then:                                             ; preds = %panic_fail3
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %26)
  br label %ifcont

else:                                             ; preds = %panic_fail3
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %31 = phi {} [ zeroinitializer, %then ], [ zeroinitializer, %else ]
  %32 = icmp ne i8 %23, 0
  %33 = xor i1 %32, true
  %34 = zext i1 %33 to i8
  %35 = icmp ne i8 %34, 0
  %36 = and i1 true, %35
  %37 = zext i1 %36 to i8
  %38 = icmp ne i8 %37, 0
  br i1 %38, label %then4, label %else5

then4:                                            ; preds = %ifcont
  %39 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %39, align 1
  %40 = load ptr, ptr %__panic1, align 8
  %41 = getelementptr i8, ptr %40, i64 4
  store i32 %17, ptr %41, align 4
  br label %ifcont6

else5:                                            ; preds = %ifcont
  br label %ifcont6

ifcont6:                                          ; preds = %else5, %then4
  %42 = phi {} [ zeroinitializer, %then4 ], [ zeroinitializer, %else5 ]
  %43 = icmp ne i8 %23, 0
  %44 = or i1 true, %43
  %45 = zext i1 %44 to i8
  %46 = icmp ne i8 %23, 0
  br i1 %46, label %then7, label %else8

then7:                                            ; preds = %ifcont6
  br label %ifcont9

else8:                                            ; preds = %ifcont6
  br label %ifcont9

ifcont9:                                          ; preds = %else8, %then7
  %47 = phi i32 [ %26, %then7 ], [ %17, %else8 ]
  ret i32 0

panic_cont10:                                     ; preds = %panic_ok2
  store { { ptr, ptr }, { ptr, i64 } } zeroinitializer, ptr %record11, align 8
  %48 = load { { ptr, ptr }, { ptr, ptr }, {} }, ptr %ctx, align 8
  store { { ptr, ptr }, { ptr, ptr }, {} } %48, ptr %tmp_addr12, align 8
  %49 = load { ptr, ptr }, ptr %tmp_addr12, align 8
  store { ptr, ptr } %49, ptr %record11, align 8
  %50 = getelementptr i8, ptr %record11, i64 16
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fAF63FF4C86022805, i64 1 }, ptr %50, align 8
  %51 = load { { ptr, ptr }, { ptr, i64 } }, ptr %record11, align 8
  store { { ptr, ptr }, { ptr, i64 } } zeroinitializer, ptr %b, align 8
  store { { ptr, ptr }, { ptr, i64 } } %51, ptr %b, align 8
  %52 = load ptr, ptr %__panic1, align 8
  %53 = load i8, ptr %52, align 1
  %54 = icmp ne i8 %53, 0
  br i1 %54, label %panic_fail14, label %panic_ok13

panic_ok13:                                       ; preds = %panic_cont10
  br label %panic_cont35

panic_fail14:                                     ; preds = %panic_cont10
  %55 = load ptr, ptr %__panic1, align 8
  %56 = load i8, ptr %55, align 1
  %57 = load ptr, ptr %__panic1, align 8
  %58 = getelementptr i8, ptr %57, i64 4
  %59 = load i32, ptr %58, align 4
  %60 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %60, align 1
  %61 = getelementptr i8, ptr %60, i64 4
  store i32 0, ptr %61, align 4
  %62 = load { { ptr, ptr }, { ptr, i64 } }, ptr %b, align 8
  %63 = load ptr, ptr %__panic1, align 8
  store { { ptr, ptr }, { ptr, i64 } } %62, ptr %ptr_arg15, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aLogger(ptr %ptr_arg15, ptr %63)
  %64 = load ptr, ptr %__panic1, align 8
  %65 = load i8, ptr %64, align 1
  %66 = load ptr, ptr %__panic1, align 8
  %67 = getelementptr i8, ptr %66, i64 4
  %68 = load i32, ptr %67, align 4
  %69 = icmp ne i8 %65, 0
  %70 = and i1 true, %69
  %71 = zext i1 %70 to i8
  %72 = icmp ne i8 %71, 0
  br i1 %72, label %then16, label %else17

then16:                                           ; preds = %panic_fail14
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %68)
  br label %ifcont18

else17:                                           ; preds = %panic_fail14
  br label %ifcont18

ifcont18:                                         ; preds = %else17, %then16
  %73 = phi {} [ zeroinitializer, %then16 ], [ zeroinitializer, %else17 ]
  %74 = icmp ne i8 %65, 0
  %75 = xor i1 %74, true
  %76 = zext i1 %75 to i8
  %77 = icmp ne i8 %76, 0
  %78 = and i1 true, %77
  %79 = zext i1 %78 to i8
  %80 = icmp ne i8 %79, 0
  br i1 %80, label %then19, label %else20

then19:                                           ; preds = %ifcont18
  %81 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %81, align 1
  %82 = load ptr, ptr %__panic1, align 8
  %83 = getelementptr i8, ptr %82, i64 4
  store i32 %59, ptr %83, align 4
  br label %ifcont21

else20:                                           ; preds = %ifcont18
  br label %ifcont21

ifcont21:                                         ; preds = %else20, %then19
  %84 = phi {} [ zeroinitializer, %then19 ], [ zeroinitializer, %else20 ]
  %85 = icmp ne i8 %65, 0
  %86 = or i1 true, %85
  %87 = zext i1 %86 to i8
  %88 = icmp ne i8 %65, 0
  br i1 %88, label %then22, label %else23

then22:                                           ; preds = %ifcont21
  br label %ifcont24

else23:                                           ; preds = %ifcont21
  br label %ifcont24

ifcont24:                                         ; preds = %else23, %then22
  %89 = phi i32 [ %68, %then22 ], [ %59, %else23 ]
  %90 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %90, align 1
  %91 = getelementptr i8, ptr %90, i64 4
  store i32 0, ptr %91, align 4
  %92 = load { { ptr, ptr }, { ptr, i64 } }, ptr %a, align 8
  %93 = load ptr, ptr %__panic1, align 8
  store { { ptr, ptr }, { ptr, i64 } } %92, ptr %ptr_arg25, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aLogger(ptr %ptr_arg25, ptr %93)
  %94 = load ptr, ptr %__panic1, align 8
  %95 = load i8, ptr %94, align 1
  %96 = load ptr, ptr %__panic1, align 8
  %97 = getelementptr i8, ptr %96, i64 4
  %98 = load i32, ptr %97, align 4
  %99 = icmp ne i8 %87, 0
  %100 = icmp ne i8 %95, 0
  %101 = and i1 %99, %100
  %102 = zext i1 %101 to i8
  %103 = icmp ne i8 %102, 0
  br i1 %103, label %then26, label %else27

then26:                                           ; preds = %ifcont24
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %98)
  br label %ifcont28

else27:                                           ; preds = %ifcont24
  br label %ifcont28

ifcont28:                                         ; preds = %else27, %then26
  %104 = phi {} [ zeroinitializer, %then26 ], [ zeroinitializer, %else27 ]
  %105 = icmp ne i8 %95, 0
  %106 = xor i1 %105, true
  %107 = zext i1 %106 to i8
  %108 = icmp ne i8 %87, 0
  %109 = icmp ne i8 %107, 0
  %110 = and i1 %108, %109
  %111 = zext i1 %110 to i8
  %112 = icmp ne i8 %111, 0
  br i1 %112, label %then29, label %else30

then29:                                           ; preds = %ifcont28
  %113 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %113, align 1
  %114 = load ptr, ptr %__panic1, align 8
  %115 = getelementptr i8, ptr %114, i64 4
  store i32 %89, ptr %115, align 4
  br label %ifcont31

else30:                                           ; preds = %ifcont28
  br label %ifcont31

ifcont31:                                         ; preds = %else30, %then29
  %116 = phi {} [ zeroinitializer, %then29 ], [ zeroinitializer, %else30 ]
  %117 = icmp ne i8 %87, 0
  %118 = icmp ne i8 %95, 0
  %119 = or i1 %117, %118
  %120 = zext i1 %119 to i8
  %121 = icmp ne i8 %95, 0
  br i1 %121, label %then32, label %else33

then32:                                           ; preds = %ifcont31
  br label %ifcont34

else33:                                           ; preds = %ifcont31
  br label %ifcont34

ifcont34:                                         ; preds = %else33, %then32
  %122 = phi i32 [ %98, %then32 ], [ %89, %else33 ]
  ret i32 0

panic_cont35:                                     ; preds = %panic_ok13
  %123 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %123, align 1
  %124 = getelementptr i8, ptr %123, i64 4
  store i32 0, ptr %124, align 4
  %125 = load { { ptr, ptr }, { ptr, i64 } }, ptr %b, align 8
  %126 = load ptr, ptr %__panic1, align 8
  store { { ptr, ptr }, { ptr, i64 } } %125, ptr %ptr_arg36, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aLogger(ptr %ptr_arg36, ptr %126)
  %127 = load ptr, ptr %__panic1, align 8
  %128 = load i8, ptr %127, align 1
  %129 = load ptr, ptr %__panic1, align 8
  %130 = getelementptr i8, ptr %129, i64 4
  %131 = load i32, ptr %130, align 4
  %132 = icmp ne i8 %128, 0
  %133 = and i1 false, %132
  %134 = zext i1 %133 to i8
  %135 = icmp ne i8 %134, 0
  br i1 %135, label %then37, label %else38

then37:                                           ; preds = %panic_cont35
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %131)
  br label %ifcont39

else38:                                           ; preds = %panic_cont35
  br label %ifcont39

ifcont39:                                         ; preds = %else38, %then37
  %136 = phi {} [ zeroinitializer, %then37 ], [ zeroinitializer, %else38 ]
  %137 = icmp ne i8 %128, 0
  %138 = xor i1 %137, true
  %139 = zext i1 %138 to i8
  %140 = icmp ne i8 %139, 0
  %141 = and i1 false, %140
  %142 = zext i1 %141 to i8
  %143 = icmp ne i8 %142, 0
  br i1 %143, label %then40, label %else41

then40:                                           ; preds = %ifcont39
  %144 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %144, align 1
  %145 = load ptr, ptr %__panic1, align 8
  %146 = getelementptr i8, ptr %145, i64 4
  store i32 0, ptr %146, align 4
  br label %ifcont42

else41:                                           ; preds = %ifcont39
  br label %ifcont42

ifcont42:                                         ; preds = %else41, %then40
  %147 = phi {} [ zeroinitializer, %then40 ], [ zeroinitializer, %else41 ]
  %148 = icmp ne i8 %128, 0
  %149 = or i1 false, %148
  %150 = zext i1 %149 to i8
  %151 = icmp ne i8 %128, 0
  br i1 %151, label %then43, label %else44

then43:                                           ; preds = %ifcont42
  br label %ifcont45

else44:                                           ; preds = %ifcont42
  br label %ifcont45

ifcont45:                                         ; preds = %else44, %then43
  %152 = phi i32 [ %131, %then43 ], [ 0, %else44 ]
  %153 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %153, align 1
  %154 = getelementptr i8, ptr %153, i64 4
  store i32 0, ptr %154, align 4
  %155 = load { { ptr, ptr }, { ptr, i64 } }, ptr %a, align 8
  %156 = load ptr, ptr %__panic1, align 8
  store { { ptr, ptr }, { ptr, i64 } } %155, ptr %ptr_arg46, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aLogger(ptr %ptr_arg46, ptr %156)
  %157 = load ptr, ptr %__panic1, align 8
  %158 = load i8, ptr %157, align 1
  %159 = load ptr, ptr %__panic1, align 8
  %160 = getelementptr i8, ptr %159, i64 4
  %161 = load i32, ptr %160, align 4
  %162 = icmp ne i8 %150, 0
  %163 = icmp ne i8 %158, 0
  %164 = and i1 %162, %163
  %165 = zext i1 %164 to i8
  %166 = icmp ne i8 %165, 0
  br i1 %166, label %then47, label %else48

then47:                                           ; preds = %ifcont45
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %161)
  br label %ifcont49

else48:                                           ; preds = %ifcont45
  br label %ifcont49

ifcont49:                                         ; preds = %else48, %then47
  %167 = phi {} [ zeroinitializer, %then47 ], [ zeroinitializer, %else48 ]
  %168 = icmp ne i8 %158, 0
  %169 = xor i1 %168, true
  %170 = zext i1 %169 to i8
  %171 = icmp ne i8 %150, 0
  %172 = icmp ne i8 %170, 0
  %173 = and i1 %171, %172
  %174 = zext i1 %173 to i8
  %175 = icmp ne i8 %174, 0
  br i1 %175, label %then50, label %else51

then50:                                           ; preds = %ifcont49
  %176 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %176, align 1
  %177 = load ptr, ptr %__panic1, align 8
  %178 = getelementptr i8, ptr %177, i64 4
  store i32 %152, ptr %178, align 4
  br label %ifcont52

else51:                                           ; preds = %ifcont49
  br label %ifcont52

ifcont52:                                         ; preds = %else51, %then50
  %179 = phi {} [ zeroinitializer, %then50 ], [ zeroinitializer, %else51 ]
  %180 = icmp ne i8 %150, 0
  %181 = icmp ne i8 %158, 0
  %182 = or i1 %180, %181
  %183 = zext i1 %182 to i8
  %184 = icmp ne i8 %158, 0
  br i1 %184, label %then53, label %else54

then53:                                           ; preds = %ifcont52
  br label %ifcont55

else54:                                           ; preds = %ifcont52
  br label %ifcont55

ifcont55:                                         ; preds = %else54, %then53
  %185 = phi i32 [ %161, %then53 ], [ %152, %else54 ]
  %186 = load ptr, ptr %__panic1, align 8
  %187 = load i8, ptr %186, align 1
  %188 = icmp ne i8 %187, 0
  br i1 %188, label %panic_fail57, label %panic_ok56

panic_ok56:                                       ; preds = %ifcont55
  br label %panic_cont58

panic_fail57:                                     ; preds = %ifcont55
  ret i32 0

panic_cont58:                                     ; preds = %panic_ok56
  %189 = load ptr, ptr %__panic1, align 8
  %190 = load i8, ptr %189, align 1
  %191 = icmp ne i8 %190, 0
  br i1 %191, label %panic_fail60, label %panic_ok59

panic_ok59:                                       ; preds = %panic_cont58
  br label %panic_cont61

panic_fail60:                                     ; preds = %panic_cont58
  ret i32 0

panic_cont61:                                     ; preds = %panic_ok59
  ret i32 0
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

define linkonce_odr void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aLogger(ptr %data, ptr %__panic) unnamed_addr comdat {
entry:
  %byref_arg = alloca { { ptr, ptr }, { ptr, i64 } }, align 8
  %__panic2 = alloca ptr, align 8
  %data1 = alloca ptr, align 8
  store ptr %data, ptr %data1, align 8
  store ptr %__panic, ptr %__panic2, align 8
  %0 = load ptr, ptr %__panic2, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %data1, align 8
  %3 = load { { ptr, ptr }, { ptr, i64 } }, ptr %2, align 8
  %4 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %5 = icmp eq i8 %4, 0
  br i1 %5, label %check_ok, label %check_fail

check_ok:                                         ; preds = %check_fail, %entry
  %6 = load ptr, ptr %__panic2, align 8
  store { { ptr, ptr }, { ptr, i64 } } %3, ptr %byref_arg, align 8
  call void @test_x3a_x3aLogger_x3a_x3adrop(ptr %byref_arg, ptr %6)
  ret void

check_fail:                                       ; preds = %entry
  %7 = load ptr, ptr %__panic2, align 8
  store i8 1, ptr %7, align 1
  %8 = getelementptr i8, ptr %7, i64 4
  store i32 10, ptr %8, align 4
  br label %check_ok
}

define linkonce_odr void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %data, ptr %__panic) unnamed_addr comdat {
entry:
  %match_val = alloca i8, align 1
  %__panic2 = alloca ptr, align 8
  %data1 = alloca ptr, align 8
  store ptr %data, ptr %data1, align 8
  store ptr %__panic, ptr %__panic2, align 8
  %0 = load ptr, ptr %__panic2, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %data1, align 8
  %3 = load i8, ptr %2, align 1
  store i8 %3, ptr %match_val, align 1
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
  %3 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %4 = icmp eq i8 %3, 0
  br i1 %4, label %check_ok, label %check_fail

check_ok:                                         ; preds = %check_fail, %entry
  %5 = load ptr, ptr %__panic, align 8
  store { { ptr, ptr }, { ptr, ptr }, {} } %0, ptr %byref_arg, align 8
  %6 = call i32 @test_x3a_x3amain(ptr %byref_arg, ptr %5)
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
  %14 = load ptr, ptr %__panic, align 8
  %15 = load i8, ptr %14, align 1
  %16 = icmp ne i8 %15, 0
  br i1 %16, label %panic_fail2, label %panic_ok1

panic_ok1:                                        ; preds = %panic_ok
  br label %panic_cont

panic_fail2:                                      ; preds = %panic_ok
  ret i32 0

panic_cont:                                       ; preds = %panic_ok1
  ret i32 %6
}

attributes #0 = { noreturn nounwind }
attributes #1 = { nounwind }

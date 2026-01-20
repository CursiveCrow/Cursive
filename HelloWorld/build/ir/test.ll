; ModuleID = 'test'
source_filename = "test"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter = comdat any

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aStatus = comdat any

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError = comdat any

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aStatus_x3a_x3aIdle = comdat any

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aStatus_x3a_x3aBusy = comdat any

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aOptPtr = comdat any

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aOptPtr_x3a_x3aValid = comdat any

$cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aOptPtr_x3a_x3aNull = comdat any

@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f63F0BFACF2C00F6B = internal unnamed_addr constant [5 x i8] c"Hello", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fBA4979E39B077E33 = internal unnamed_addr constant [11 x i8] c"greet=len5\0A", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fBA3519E39AF63209 = internal unnamed_addr constant [11 x i8] c"greet=len?\0A", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f765D3078EE3004D5 = internal unnamed_addr constant [11 x i8] c"bytes=len5\0A", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f763B3078EE131E6F = internal unnamed_addr constant [11 x i8] c"bytes=len?\0A", align 1
@cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest = global i8 0
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f7AC500837025B2F4 = internal unnamed_addr constant [13 x i8] c"status=idle2\0A", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fA01491A17C83EC9C = internal unnamed_addr constant [18 x i8] c"status=busy_retry\0A", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f5F0E756B894533BC = internal unnamed_addr constant [10 x i8] c"ptr=valid\0A", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f798F97E93DE70729 = internal unnamed_addr constant [9 x i8] c"ptr=null\0A", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f7A92DDCF07135D78 = internal unnamed_addr constant [10 x i8] c"region=ok\0A", align 1
@cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fF378FD567236B127 = internal unnamed_addr constant [12 x i8] c"region=skip\0A", align 1

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

define { i32 } @test_x3a_x3aCounter_x3a_x3aclone(ptr noundef nonnull readonly align 4 dereferenceable(4) %self, ptr %__panic) {
entry:
  %tmp_addr = alloca { i32 }, align 8
  %record = alloca { i32 }, align 8
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  store { i32 } zeroinitializer, ptr %record, align 4
  %2 = load { i32 }, ptr %self, align 4
  store { i32 } %2, ptr %tmp_addr, align 4
  %3 = load i32, ptr %tmp_addr, align 4
  store i32 %3, ptr %record, align 4
  %4 = load { i32 }, ptr %record, align 4
  ret { i32 } %4
}

define i32 @test_x3a_x3aCounter_x3a_x3aread(ptr noundef nonnull readonly align 4 dereferenceable(4) %self, ptr %__panic) {
entry:
  %tmp_addr = alloca { i32 }, align 8
  %__panic1 = alloca ptr, align 8
  store ptr %__panic, ptr %__panic1, align 8
  %0 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load { i32 }, ptr %self, align 4
  store { i32 } %2, ptr %tmp_addr, align 4
  %3 = load i32, ptr %tmp_addr, align 4
  ret i32 %3
}

define i32 @test_x3a_x3amain(ptr noundef nonnull readonly align 8 dereferenceable(32) %ctx, ptr %__panic) {
entry:
  %ptr_arg1169 = alloca { i32 }, align 8
  %ptr_arg1159 = alloca { i32 }, align 8
  %ptr_arg1148 = alloca { i32 }, align 8
  %ptr_arg1138 = alloca { i32 }, align 8
  %ptr_arg1126 = alloca i8, align 1
  %tmp_addr1125 = alloca i16, align 2
  %match_val1119 = alloca i16, align 2
  %ptr_arg1108 = alloca { i32 }, align 8
  %ptr_arg1098 = alloca { i32 }, align 8
  %ptr_arg1086 = alloca i8, align 1
  %tmp_addr1084 = alloca i16, align 2
  %err1085 = alloca i8, align 1
  %ptr_arg1072 = alloca { i32 }, align 8
  %ptr_arg1062 = alloca { i32 }, align 8
  %tmp_addr1058 = alloca i16, align 2
  %ok1059 = alloca {}, align 8
  %match_val1053 = alloca i16, align 2
  %byref_arg1052 = alloca { ptr, i64 }, align 8
  %ptr_arg1041 = alloca { i32 }, align 8
  %ptr_arg1031 = alloca { i32 }, align 8
  %ptr_arg1019 = alloca i8, align 1
  %tmp_addr1018 = alloca i16, align 2
  %match_val1012 = alloca i16, align 2
  %ptr_arg1001 = alloca { i32 }, align 8
  %ptr_arg991 = alloca { i32 }, align 8
  %ptr_arg979 = alloca i8, align 1
  %tmp_addr977 = alloca i16, align 2
  %err978 = alloca i8, align 1
  %ptr_arg965 = alloca { i32 }, align 8
  %ptr_arg955 = alloca { i32 }, align 8
  %tmp_addr951 = alloca i16, align 2
  %ok952 = alloca {}, align 8
  %match_val946 = alloca i16, align 2
  %byref_arg945 = alloca { ptr, i64 }, align 8
  %ptr_arg934 = alloca { i32 }, align 8
  %ptr_arg924 = alloca { i32 }, align 8
  %ptr_arg912 = alloca i8, align 1
  %tmp_addr911 = alloca i16, align 2
  %match_val905 = alloca i16, align 2
  %ptr_arg894 = alloca { i32 }, align 8
  %ptr_arg884 = alloca { i32 }, align 8
  %ptr_arg872 = alloca i8, align 1
  %tmp_addr870 = alloca i16, align 2
  %err871 = alloca i8, align 1
  %ptr_arg858 = alloca { i32 }, align 8
  %ptr_arg848 = alloca { i32 }, align 8
  %tmp_addr844 = alloca i16, align 2
  %ok845 = alloca {}, align 8
  %match_val839 = alloca i16, align 2
  %byref_arg838 = alloca { ptr, i64 }, align 8
  %ptr_arg827 = alloca { i32 }, align 8
  %ptr_arg817 = alloca { i32 }, align 8
  %ptr_arg805 = alloca i8, align 1
  %tmp_addr804 = alloca i16, align 2
  %match_val798 = alloca i16, align 2
  %ptr_arg787 = alloca { i32 }, align 8
  %ptr_arg777 = alloca { i32 }, align 8
  %ptr_arg765 = alloca i8, align 1
  %tmp_addr763 = alloca i16, align 2
  %err764 = alloca i8, align 1
  %ptr_arg751 = alloca { i32 }, align 8
  %ptr_arg741 = alloca { i32 }, align 8
  %tmp_addr737 = alloca i16, align 2
  %ok738 = alloca {}, align 8
  %match_val732 = alloca i16, align 2
  %byref_arg731 = alloca { ptr, i64 }, align 8
  %ptr_arg720 = alloca { i32 }, align 8
  %ptr_arg710 = alloca { i32 }, align 8
  %ptr_arg698 = alloca i8, align 1
  %tmp_addr697 = alloca i16, align 2
  %match_val691 = alloca i16, align 2
  %ptr_arg680 = alloca { i32 }, align 8
  %ptr_arg670 = alloca { i32 }, align 8
  %ptr_arg658 = alloca i8, align 1
  %tmp_addr657 = alloca i16, align 2
  %err = alloca i8, align 1
  %ptr_arg645 = alloca { i32 }, align 8
  %ptr_arg635 = alloca { i32 }, align 8
  %tmp_addr632 = alloca i16, align 2
  %ok = alloca {}, align 8
  %match_val627 = alloca i16, align 2
  %byref_arg626 = alloca { ptr, i64 }, align 8
  %region_msg = alloca { ptr, i64 }, align 8
  %ptr_arg615 = alloca { i32 }, align 8
  %ptr_arg605 = alloca { i32 }, align 8
  %ptr_arg592 = alloca { i32 }, align 8
  %ptr_arg582 = alloca { i32 }, align 8
  %ptr_arg566 = alloca { i32 }, align 8
  %ptr_arg556 = alloca { i32 }, align 8
  %byref_arg = alloca { [16 x i8], [0 x i64] }, align 8
  %ptr_arg543 = alloca { i32 }, align 8
  %ptr_arg533 = alloca { i32 }, align 8
  %region_flag = alloca i8, align 1
  %ptr_arg520 = alloca { i32 }, align 8
  %ptr_arg510 = alloca { i32 }, align 8
  %ptr_msg = alloca { ptr, i64 }, align 8
  %ptr_arg497 = alloca { i32 }, align 8
  %ptr_arg487 = alloca { i32 }, align 8
  %tmp_addr484 = alloca { [16 x i8], [0 x i64] }, align 8
  %tag_val = alloca i8, align 1
  %ptr_arg471 = alloca { i32 }, align 8
  %ptr_arg461 = alloca { i32 }, align 8
  %tmp_addr458 = alloca { [16 x i8], [0 x i64] }, align 8
  %ptr_val = alloca ptr, align 8
  %match_val452 = alloca { [16 x i8], [0 x i64] }, align 8
  %opt = alloca { [16 x i8], [0 x i64] }, align 8
  %ptr_arg441 = alloca { i32 }, align 8
  %ptr_arg431 = alloca { i32 }, align 8
  %ptr_arg419 = alloca { i8 }, align 8
  %record418 = alloca { i8 }, align 8
  %ptr_arg408 = alloca { [16 x i8], [0 x i64] }, align 8
  %tmp_addr407 = alloca { i8 }, align 8
  %record406 = alloca { i8 }, align 8
  %widen_modal405 = alloca { [16 x i8], [0 x i64] }, align 8
  %record404 = alloca { i8 }, align 8
  %ptr_arg393 = alloca { i32 }, align 8
  %ptr_arg383 = alloca { i32 }, align 8
  %ptr_arg371 = alloca { ptr }, align 8
  %record370 = alloca { ptr }, align 8
  %ptr_arg360 = alloca { [16 x i8], [0 x i64] }, align 8
  %tmp_addr359 = alloca { ptr }, align 8
  %record358 = alloca { ptr }, align 8
  %widen_modal357 = alloca { [16 x i8], [0 x i64] }, align 8
  %record356 = alloca { ptr }, align 8
  %ptr_arg342 = alloca { i32 }, align 8
  %ptr_arg332 = alloca { i32 }, align 8
  %p = alloca ptr, align 8
  %ptr_arg319 = alloca { i32 }, align 8
  %ptr_arg309 = alloca { i32 }, align 8
  %x = alloca i32, align 4
  %ptr_arg296 = alloca { i32 }, align 8
  %ptr_arg286 = alloca { i32 }, align 8
  %ptr_arg271 = alloca { i32 }, align 8
  %ptr_arg261 = alloca { i32 }, align 8
  %status_msg = alloca { ptr, i64 }, align 8
  %ptr_arg248 = alloca { i32 }, align 8
  %ptr_arg238 = alloca { i32 }, align 8
  %tmp_addr235 = alloca { [12 x i8], [0 x i32] }, align 8
  %retry_val = alloca i32, align 4
  %tmp_addr234 = alloca { [12 x i8], [0 x i32] }, align 8
  %code_val = alloca i32, align 4
  %ptr_arg221 = alloca { i32 }, align 8
  %ptr_arg211 = alloca { i32 }, align 8
  %tmp_addr208 = alloca { [12 x i8], [0 x i32] }, align 8
  %ticks_val = alloca i32, align 4
  %match_val = alloca { [12 x i8], [0 x i32] }, align 8
  %status = alloca { [12 x i8], [0 x i32] }, align 8
  %ptr_arg197 = alloca { i32 }, align 8
  %ptr_arg187 = alloca { i32 }, align 8
  %ptr_arg175 = alloca { i32, i32 }, align 8
  %record174 = alloca { i32, i32 }, align 8
  %ptr_arg164 = alloca { [12 x i8], [0 x i32] }, align 8
  %tmp_addr163 = alloca { i32, i32 }, align 8
  %record162 = alloca { i32, i32 }, align 8
  %widen_modal161 = alloca { [12 x i8], [0 x i32] }, align 8
  %record160 = alloca { i32, i32 }, align 8
  %ptr_arg149 = alloca { i32 }, align 8
  %ptr_arg139 = alloca { i32 }, align 8
  %ptr_arg127 = alloca { i32 }, align 8
  %record126 = alloca { i32 }, align 8
  %ptr_arg116 = alloca { [12 x i8], [0 x i32] }, align 8
  %tmp_addr = alloca { i32 }, align 8
  %record115 = alloca { i32 }, align 8
  %widen_modal = alloca { [12 x i8], [0 x i32] }, align 8
  %record114 = alloca { i32 }, align 8
  %ptr_arg100 = alloca { i32 }, align 8
  %ptr_arg90 = alloca { i32 }, align 8
  %count = alloca i32, align 4
  %ptr_arg77 = alloca { i32 }, align 8
  %ptr_arg67 = alloca { i32 }, align 8
  %next = alloca { i32 }, align 8
  %ptr_arg52 = alloca { i32 }, align 8
  %ptr_arg = alloca { i32 }, align 8
  %record = alloca { i32 }, align 8
  %counter = alloca { i32 }, align 8
  %bytes_msg = alloca { ptr, i64 }, align 8
  %b_view = alloca { ptr, i64 }, align 8
  %greet_msg = alloca { ptr, i64 }, align 8
  %greet_len = alloca i64, align 8
  %greeting = alloca { ptr, i64 }, align 8
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
  store { ptr, i64 } zeroinitializer, ptr %greeting, align 8
  store { ptr, i64 } { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f63F0BFACF2C00F6B, i64 5 }, ptr %greeting, align 8
  %6 = load ptr, ptr %__panic1, align 8
  %7 = call i64 @cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3alength(ptr %greeting)
  %8 = load ptr, ptr %__panic1, align 8
  %9 = load i8, ptr %8, align 1
  %10 = icmp ne i8 %9, 0
  br i1 %10, label %panic_fail3, label %panic_ok2

panic_ok2:                                        ; preds = %panic_cont
  br label %panic_cont4

panic_fail3:                                      ; preds = %panic_cont
  %11 = load ptr, ptr %__panic1, align 8
  %12 = load i8, ptr %11, align 1
  %13 = load ptr, ptr %__panic1, align 8
  %14 = getelementptr i8, ptr %13, i64 4
  %15 = load i32, ptr %14, align 4
  ret i32 0

panic_cont4:                                      ; preds = %panic_ok2
  store i64 0, ptr %greet_len, align 8
  store i64 %7, ptr %greet_len, align 8
  %16 = load ptr, ptr %__panic1, align 8
  %17 = load i8, ptr %16, align 1
  %18 = icmp ne i8 %17, 0
  br i1 %18, label %panic_fail6, label %panic_ok5

panic_ok5:                                        ; preds = %panic_cont4
  br label %panic_cont7

panic_fail6:                                      ; preds = %panic_cont4
  %19 = load ptr, ptr %__panic1, align 8
  %20 = load i8, ptr %19, align 1
  %21 = load ptr, ptr %__panic1, align 8
  %22 = getelementptr i8, ptr %21, i64 4
  %23 = load i32, ptr %22, align 4
  ret i32 0

panic_cont7:                                      ; preds = %panic_ok5
  %24 = load i64, ptr %greet_len, align 8
  %25 = icmp eq i64 %24, 5
  %26 = zext i1 %25 to i8
  %27 = load ptr, ptr %__panic1, align 8
  %28 = load i8, ptr %27, align 1
  %29 = icmp ne i8 %28, 0
  br i1 %29, label %panic_fail9, label %panic_ok8

panic_ok8:                                        ; preds = %panic_cont7
  br label %panic_cont10

panic_fail9:                                      ; preds = %panic_cont7
  %30 = load ptr, ptr %__panic1, align 8
  %31 = load i8, ptr %30, align 1
  %32 = load ptr, ptr %__panic1, align 8
  %33 = getelementptr i8, ptr %32, i64 4
  %34 = load i32, ptr %33, align 4
  ret i32 0

panic_cont10:                                     ; preds = %panic_ok8
  %35 = icmp ne i8 %26, 0
  br i1 %35, label %then, label %else

then:                                             ; preds = %panic_cont10
  %36 = load ptr, ptr %__panic1, align 8
  %37 = load i8, ptr %36, align 1
  %38 = icmp ne i8 %37, 0
  br i1 %38, label %panic_fail12, label %panic_ok11

else:                                             ; preds = %panic_cont10
  %39 = load ptr, ptr %__panic1, align 8
  %40 = load i8, ptr %39, align 1
  %41 = icmp ne i8 %40, 0
  br i1 %41, label %panic_fail15, label %panic_ok14

ifcont:                                           ; preds = %panic_cont16, %panic_cont13
  %42 = phi { ptr, i64 } [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fBA4979E39B077E33, i64 11 }, %panic_cont13 ], [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fBA3519E39AF63209, i64 11 }, %panic_cont16 ]
  store { ptr, i64 } zeroinitializer, ptr %greet_msg, align 8
  store { ptr, i64 } %42, ptr %greet_msg, align 8
  %43 = load ptr, ptr %__panic1, align 8
  %44 = call { ptr, i64 } @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aview_x5fstring(ptr %greeting)
  %45 = load ptr, ptr %__panic1, align 8
  %46 = load i8, ptr %45, align 1
  %47 = icmp ne i8 %46, 0
  br i1 %47, label %panic_fail18, label %panic_ok17

panic_ok11:                                       ; preds = %then
  br label %panic_cont13

panic_fail12:                                     ; preds = %then
  %48 = load ptr, ptr %__panic1, align 8
  %49 = load i8, ptr %48, align 1
  %50 = load ptr, ptr %__panic1, align 8
  %51 = getelementptr i8, ptr %50, i64 4
  %52 = load i32, ptr %51, align 4
  ret i32 0

panic_cont13:                                     ; preds = %panic_ok11
  br label %ifcont

panic_ok14:                                       ; preds = %else
  br label %panic_cont16

panic_fail15:                                     ; preds = %else
  %53 = load ptr, ptr %__panic1, align 8
  %54 = load i8, ptr %53, align 1
  %55 = load ptr, ptr %__panic1, align 8
  %56 = getelementptr i8, ptr %55, i64 4
  %57 = load i32, ptr %56, align 4
  ret i32 0

panic_cont16:                                     ; preds = %panic_ok14
  br label %ifcont

panic_ok17:                                       ; preds = %ifcont
  br label %panic_cont19

panic_fail18:                                     ; preds = %ifcont
  %58 = load ptr, ptr %__panic1, align 8
  %59 = load i8, ptr %58, align 1
  %60 = load ptr, ptr %__panic1, align 8
  %61 = getelementptr i8, ptr %60, i64 4
  %62 = load i32, ptr %61, align 4
  ret i32 0

panic_cont19:                                     ; preds = %panic_ok17
  store { ptr, i64 } zeroinitializer, ptr %b_view, align 8
  store { ptr, i64 } %44, ptr %b_view, align 8
  %63 = load ptr, ptr %__panic1, align 8
  %64 = load i8, ptr %63, align 1
  %65 = icmp ne i8 %64, 0
  br i1 %65, label %panic_fail21, label %panic_ok20

panic_ok20:                                       ; preds = %panic_cont19
  br label %panic_cont22

panic_fail21:                                     ; preds = %panic_cont19
  %66 = load ptr, ptr %__panic1, align 8
  %67 = load i8, ptr %66, align 1
  %68 = load ptr, ptr %__panic1, align 8
  %69 = getelementptr i8, ptr %68, i64 4
  %70 = load i32, ptr %69, align 4
  ret i32 0

panic_cont22:                                     ; preds = %panic_ok20
  %71 = load ptr, ptr %__panic1, align 8
  %72 = call i64 @cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3alength(ptr %b_view)
  %73 = load ptr, ptr %__panic1, align 8
  %74 = load i8, ptr %73, align 1
  %75 = icmp ne i8 %74, 0
  br i1 %75, label %panic_fail24, label %panic_ok23

panic_ok23:                                       ; preds = %panic_cont22
  br label %panic_cont25

panic_fail24:                                     ; preds = %panic_cont22
  %76 = load ptr, ptr %__panic1, align 8
  %77 = load i8, ptr %76, align 1
  %78 = load ptr, ptr %__panic1, align 8
  %79 = getelementptr i8, ptr %78, i64 4
  %80 = load i32, ptr %79, align 4
  ret i32 0

panic_cont25:                                     ; preds = %panic_ok23
  %81 = icmp eq i64 %72, 5
  %82 = zext i1 %81 to i8
  %83 = load ptr, ptr %__panic1, align 8
  %84 = load i8, ptr %83, align 1
  %85 = icmp ne i8 %84, 0
  br i1 %85, label %panic_fail27, label %panic_ok26

panic_ok26:                                       ; preds = %panic_cont25
  br label %panic_cont28

panic_fail27:                                     ; preds = %panic_cont25
  %86 = load ptr, ptr %__panic1, align 8
  %87 = load i8, ptr %86, align 1
  %88 = load ptr, ptr %__panic1, align 8
  %89 = getelementptr i8, ptr %88, i64 4
  %90 = load i32, ptr %89, align 4
  ret i32 0

panic_cont28:                                     ; preds = %panic_ok26
  %91 = icmp ne i8 %82, 0
  br i1 %91, label %then29, label %else30

then29:                                           ; preds = %panic_cont28
  %92 = load ptr, ptr %__panic1, align 8
  %93 = load i8, ptr %92, align 1
  %94 = icmp ne i8 %93, 0
  br i1 %94, label %panic_fail33, label %panic_ok32

else30:                                           ; preds = %panic_cont28
  %95 = load ptr, ptr %__panic1, align 8
  %96 = load i8, ptr %95, align 1
  %97 = icmp ne i8 %96, 0
  br i1 %97, label %panic_fail36, label %panic_ok35

ifcont31:                                         ; preds = %panic_cont37, %panic_cont34
  %98 = phi { ptr, i64 } [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f765D3078EE3004D5, i64 11 }, %panic_cont34 ], [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f763B3078EE131E6F, i64 11 }, %panic_cont37 ]
  store { ptr, i64 } zeroinitializer, ptr %bytes_msg, align 8
  store { ptr, i64 } %98, ptr %bytes_msg, align 8
  store { i32 } zeroinitializer, ptr %record, align 4
  store i8 1, ptr %record, align 1
  %99 = load { i32 }, ptr %record, align 4
  store { i32 } zeroinitializer, ptr %counter, align 4
  store { i32 } %99, ptr %counter, align 4
  %100 = load ptr, ptr %__panic1, align 8
  %101 = load i8, ptr %100, align 1
  %102 = icmp ne i8 %101, 0
  br i1 %102, label %panic_fail39, label %panic_ok38

panic_ok32:                                       ; preds = %then29
  br label %panic_cont34

panic_fail33:                                     ; preds = %then29
  %103 = load ptr, ptr %__panic1, align 8
  %104 = load i8, ptr %103, align 1
  %105 = load ptr, ptr %__panic1, align 8
  %106 = getelementptr i8, ptr %105, i64 4
  %107 = load i32, ptr %106, align 4
  ret i32 0

panic_cont34:                                     ; preds = %panic_ok32
  br label %ifcont31

panic_ok35:                                       ; preds = %else30
  br label %panic_cont37

panic_fail36:                                     ; preds = %else30
  %108 = load ptr, ptr %__panic1, align 8
  %109 = load i8, ptr %108, align 1
  %110 = load ptr, ptr %__panic1, align 8
  %111 = getelementptr i8, ptr %110, i64 4
  %112 = load i32, ptr %111, align 4
  ret i32 0

panic_cont37:                                     ; preds = %panic_ok35
  br label %ifcont31

panic_ok38:                                       ; preds = %ifcont31
  br label %panic_cont49

panic_fail39:                                     ; preds = %ifcont31
  %113 = load ptr, ptr %__panic1, align 8
  %114 = load i8, ptr %113, align 1
  %115 = load ptr, ptr %__panic1, align 8
  %116 = getelementptr i8, ptr %115, i64 4
  %117 = load i32, ptr %116, align 4
  %118 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %118, align 1
  %119 = getelementptr i8, ptr %118, i64 4
  store i32 0, ptr %119, align 4
  %120 = load { i32 }, ptr %counter, align 4
  %121 = load ptr, ptr %__panic1, align 8
  store { i32 } %120, ptr %ptr_arg, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg, ptr %121)
  %122 = load ptr, ptr %__panic1, align 8
  %123 = load i8, ptr %122, align 1
  %124 = load ptr, ptr %__panic1, align 8
  %125 = getelementptr i8, ptr %124, i64 4
  %126 = load i32, ptr %125, align 4
  %127 = icmp ne i8 %123, 0
  %128 = and i1 true, %127
  %129 = zext i1 %128 to i8
  %130 = icmp ne i8 %129, 0
  br i1 %130, label %then40, label %else41

then40:                                           ; preds = %panic_fail39
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %126)
  br label %ifcont42

else41:                                           ; preds = %panic_fail39
  br label %ifcont42

ifcont42:                                         ; preds = %else41, %then40
  %131 = phi {} [ zeroinitializer, %then40 ], [ zeroinitializer, %else41 ]
  %132 = icmp ne i8 %123, 0
  %133 = xor i1 %132, true
  %134 = zext i1 %133 to i8
  %135 = icmp ne i8 %134, 0
  %136 = and i1 true, %135
  %137 = zext i1 %136 to i8
  %138 = icmp ne i8 %137, 0
  br i1 %138, label %then43, label %else44

then43:                                           ; preds = %ifcont42
  %139 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %139, align 1
  %140 = load ptr, ptr %__panic1, align 8
  %141 = getelementptr i8, ptr %140, i64 4
  store i32 %117, ptr %141, align 4
  br label %ifcont45

else44:                                           ; preds = %ifcont42
  br label %ifcont45

ifcont45:                                         ; preds = %else44, %then43
  %142 = phi {} [ zeroinitializer, %then43 ], [ zeroinitializer, %else44 ]
  %143 = icmp ne i8 %123, 0
  %144 = or i1 true, %143
  %145 = zext i1 %144 to i8
  %146 = icmp ne i8 %123, 0
  br i1 %146, label %then46, label %else47

then46:                                           ; preds = %ifcont45
  br label %ifcont48

else47:                                           ; preds = %ifcont45
  br label %ifcont48

ifcont48:                                         ; preds = %else47, %then46
  %147 = phi i32 [ %126, %then46 ], [ %117, %else47 ]
  ret i32 0

panic_cont49:                                     ; preds = %panic_ok38
  %148 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %149 = icmp eq i8 %148, 0
  br i1 %149, label %check_ok, label %check_fail

check_ok:                                         ; preds = %check_fail, %panic_cont49
  %150 = load ptr, ptr %__panic1, align 8
  %151 = call { i32 } @test_x3a_x3aCounter_x3a_x3aclone(ptr %counter, ptr %150)
  %152 = load ptr, ptr %__panic1, align 8
  %153 = load i8, ptr %152, align 1
  %154 = icmp ne i8 %153, 0
  br i1 %154, label %panic_fail51, label %panic_ok50

check_fail:                                       ; preds = %panic_cont49
  %155 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %155, align 1
  %156 = getelementptr i8, ptr %155, i64 4
  store i32 10, ptr %156, align 4
  br label %check_ok

panic_ok50:                                       ; preds = %check_ok
  br label %panic_cont62

panic_fail51:                                     ; preds = %check_ok
  %157 = load ptr, ptr %__panic1, align 8
  %158 = load i8, ptr %157, align 1
  %159 = load ptr, ptr %__panic1, align 8
  %160 = getelementptr i8, ptr %159, i64 4
  %161 = load i32, ptr %160, align 4
  %162 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %162, align 1
  %163 = getelementptr i8, ptr %162, i64 4
  store i32 0, ptr %163, align 4
  %164 = load { i32 }, ptr %counter, align 4
  %165 = load ptr, ptr %__panic1, align 8
  store { i32 } %164, ptr %ptr_arg52, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg52, ptr %165)
  %166 = load ptr, ptr %__panic1, align 8
  %167 = load i8, ptr %166, align 1
  %168 = load ptr, ptr %__panic1, align 8
  %169 = getelementptr i8, ptr %168, i64 4
  %170 = load i32, ptr %169, align 4
  %171 = icmp ne i8 %167, 0
  %172 = and i1 true, %171
  %173 = zext i1 %172 to i8
  %174 = icmp ne i8 %173, 0
  br i1 %174, label %then53, label %else54

then53:                                           ; preds = %panic_fail51
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %170)
  br label %ifcont55

else54:                                           ; preds = %panic_fail51
  br label %ifcont55

ifcont55:                                         ; preds = %else54, %then53
  %175 = phi {} [ zeroinitializer, %then53 ], [ zeroinitializer, %else54 ]
  %176 = icmp ne i8 %167, 0
  %177 = xor i1 %176, true
  %178 = zext i1 %177 to i8
  %179 = icmp ne i8 %178, 0
  %180 = and i1 true, %179
  %181 = zext i1 %180 to i8
  %182 = icmp ne i8 %181, 0
  br i1 %182, label %then56, label %else57

then56:                                           ; preds = %ifcont55
  %183 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %183, align 1
  %184 = load ptr, ptr %__panic1, align 8
  %185 = getelementptr i8, ptr %184, i64 4
  store i32 %161, ptr %185, align 4
  br label %ifcont58

else57:                                           ; preds = %ifcont55
  br label %ifcont58

ifcont58:                                         ; preds = %else57, %then56
  %186 = phi {} [ zeroinitializer, %then56 ], [ zeroinitializer, %else57 ]
  %187 = icmp ne i8 %167, 0
  %188 = or i1 true, %187
  %189 = zext i1 %188 to i8
  %190 = icmp ne i8 %167, 0
  br i1 %190, label %then59, label %else60

then59:                                           ; preds = %ifcont58
  br label %ifcont61

else60:                                           ; preds = %ifcont58
  br label %ifcont61

ifcont61:                                         ; preds = %else60, %then59
  %191 = phi i32 [ %170, %then59 ], [ %161, %else60 ]
  ret i32 0

panic_cont62:                                     ; preds = %panic_ok50
  store { i32 } zeroinitializer, ptr %next, align 4
  store { i32 } %151, ptr %next, align 4
  %192 = load i8, ptr @cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3atest, align 1
  %193 = icmp eq i8 %192, 0
  br i1 %193, label %check_ok63, label %check_fail64

check_ok63:                                       ; preds = %check_fail64, %panic_cont62
  %194 = load ptr, ptr %__panic1, align 8
  %195 = call i32 @test_x3a_x3aCounter_x3a_x3aread(ptr %next, ptr %194)
  %196 = load ptr, ptr %__panic1, align 8
  %197 = load i8, ptr %196, align 1
  %198 = icmp ne i8 %197, 0
  br i1 %198, label %panic_fail66, label %panic_ok65

check_fail64:                                     ; preds = %panic_cont62
  %199 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %199, align 1
  %200 = getelementptr i8, ptr %199, i64 4
  store i32 10, ptr %200, align 4
  br label %check_ok63

panic_ok65:                                       ; preds = %check_ok63
  br label %panic_cont87

panic_fail66:                                     ; preds = %check_ok63
  %201 = load ptr, ptr %__panic1, align 8
  %202 = load i8, ptr %201, align 1
  %203 = load ptr, ptr %__panic1, align 8
  %204 = getelementptr i8, ptr %203, i64 4
  %205 = load i32, ptr %204, align 4
  %206 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %206, align 1
  %207 = getelementptr i8, ptr %206, i64 4
  store i32 0, ptr %207, align 4
  %208 = load { i32 }, ptr %next, align 4
  %209 = load ptr, ptr %__panic1, align 8
  store { i32 } %208, ptr %ptr_arg67, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg67, ptr %209)
  %210 = load ptr, ptr %__panic1, align 8
  %211 = load i8, ptr %210, align 1
  %212 = load ptr, ptr %__panic1, align 8
  %213 = getelementptr i8, ptr %212, i64 4
  %214 = load i32, ptr %213, align 4
  %215 = icmp ne i8 %211, 0
  %216 = and i1 true, %215
  %217 = zext i1 %216 to i8
  %218 = icmp ne i8 %217, 0
  br i1 %218, label %then68, label %else69

then68:                                           ; preds = %panic_fail66
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %214)
  br label %ifcont70

else69:                                           ; preds = %panic_fail66
  br label %ifcont70

ifcont70:                                         ; preds = %else69, %then68
  %219 = phi {} [ zeroinitializer, %then68 ], [ zeroinitializer, %else69 ]
  %220 = icmp ne i8 %211, 0
  %221 = xor i1 %220, true
  %222 = zext i1 %221 to i8
  %223 = icmp ne i8 %222, 0
  %224 = and i1 true, %223
  %225 = zext i1 %224 to i8
  %226 = icmp ne i8 %225, 0
  br i1 %226, label %then71, label %else72

then71:                                           ; preds = %ifcont70
  %227 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %227, align 1
  %228 = load ptr, ptr %__panic1, align 8
  %229 = getelementptr i8, ptr %228, i64 4
  store i32 %205, ptr %229, align 4
  br label %ifcont73

else72:                                           ; preds = %ifcont70
  br label %ifcont73

ifcont73:                                         ; preds = %else72, %then71
  %230 = phi {} [ zeroinitializer, %then71 ], [ zeroinitializer, %else72 ]
  %231 = icmp ne i8 %211, 0
  %232 = or i1 true, %231
  %233 = zext i1 %232 to i8
  %234 = icmp ne i8 %211, 0
  br i1 %234, label %then74, label %else75

then74:                                           ; preds = %ifcont73
  br label %ifcont76

else75:                                           ; preds = %ifcont73
  br label %ifcont76

ifcont76:                                         ; preds = %else75, %then74
  %235 = phi i32 [ %214, %then74 ], [ %205, %else75 ]
  %236 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %236, align 1
  %237 = getelementptr i8, ptr %236, i64 4
  store i32 0, ptr %237, align 4
  %238 = load { i32 }, ptr %counter, align 4
  %239 = load ptr, ptr %__panic1, align 8
  store { i32 } %238, ptr %ptr_arg77, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg77, ptr %239)
  %240 = load ptr, ptr %__panic1, align 8
  %241 = load i8, ptr %240, align 1
  %242 = load ptr, ptr %__panic1, align 8
  %243 = getelementptr i8, ptr %242, i64 4
  %244 = load i32, ptr %243, align 4
  %245 = icmp ne i8 %233, 0
  %246 = icmp ne i8 %241, 0
  %247 = and i1 %245, %246
  %248 = zext i1 %247 to i8
  %249 = icmp ne i8 %248, 0
  br i1 %249, label %then78, label %else79

then78:                                           ; preds = %ifcont76
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %244)
  br label %ifcont80

else79:                                           ; preds = %ifcont76
  br label %ifcont80

ifcont80:                                         ; preds = %else79, %then78
  %250 = phi {} [ zeroinitializer, %then78 ], [ zeroinitializer, %else79 ]
  %251 = icmp ne i8 %241, 0
  %252 = xor i1 %251, true
  %253 = zext i1 %252 to i8
  %254 = icmp ne i8 %233, 0
  %255 = icmp ne i8 %253, 0
  %256 = and i1 %254, %255
  %257 = zext i1 %256 to i8
  %258 = icmp ne i8 %257, 0
  br i1 %258, label %then81, label %else82

then81:                                           ; preds = %ifcont80
  %259 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %259, align 1
  %260 = load ptr, ptr %__panic1, align 8
  %261 = getelementptr i8, ptr %260, i64 4
  store i32 %235, ptr %261, align 4
  br label %ifcont83

else82:                                           ; preds = %ifcont80
  br label %ifcont83

ifcont83:                                         ; preds = %else82, %then81
  %262 = phi {} [ zeroinitializer, %then81 ], [ zeroinitializer, %else82 ]
  %263 = icmp ne i8 %233, 0
  %264 = icmp ne i8 %241, 0
  %265 = or i1 %263, %264
  %266 = zext i1 %265 to i8
  %267 = icmp ne i8 %241, 0
  br i1 %267, label %then84, label %else85

then84:                                           ; preds = %ifcont83
  br label %ifcont86

else85:                                           ; preds = %ifcont83
  br label %ifcont86

ifcont86:                                         ; preds = %else85, %then84
  %268 = phi i32 [ %244, %then84 ], [ %235, %else85 ]
  ret i32 0

panic_cont87:                                     ; preds = %panic_ok65
  store i32 0, ptr %count, align 4
  store i32 %195, ptr %count, align 4
  %269 = load i32, ptr %count, align 4
  %270 = icmp eq i32 %269, 2
  %271 = zext i1 %270 to i8
  %272 = load ptr, ptr %__panic1, align 8
  %273 = load i8, ptr %272, align 1
  %274 = icmp ne i8 %273, 0
  br i1 %274, label %panic_fail89, label %panic_ok88

panic_ok88:                                       ; preds = %panic_cont87
  br label %panic_cont110

panic_fail89:                                     ; preds = %panic_cont87
  %275 = load ptr, ptr %__panic1, align 8
  %276 = load i8, ptr %275, align 1
  %277 = load ptr, ptr %__panic1, align 8
  %278 = getelementptr i8, ptr %277, i64 4
  %279 = load i32, ptr %278, align 4
  %280 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %280, align 1
  %281 = getelementptr i8, ptr %280, i64 4
  store i32 0, ptr %281, align 4
  %282 = load { i32 }, ptr %next, align 4
  %283 = load ptr, ptr %__panic1, align 8
  store { i32 } %282, ptr %ptr_arg90, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg90, ptr %283)
  %284 = load ptr, ptr %__panic1, align 8
  %285 = load i8, ptr %284, align 1
  %286 = load ptr, ptr %__panic1, align 8
  %287 = getelementptr i8, ptr %286, i64 4
  %288 = load i32, ptr %287, align 4
  %289 = icmp ne i8 %285, 0
  %290 = and i1 true, %289
  %291 = zext i1 %290 to i8
  %292 = icmp ne i8 %291, 0
  br i1 %292, label %then91, label %else92

then91:                                           ; preds = %panic_fail89
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %288)
  br label %ifcont93

else92:                                           ; preds = %panic_fail89
  br label %ifcont93

ifcont93:                                         ; preds = %else92, %then91
  %293 = phi {} [ zeroinitializer, %then91 ], [ zeroinitializer, %else92 ]
  %294 = icmp ne i8 %285, 0
  %295 = xor i1 %294, true
  %296 = zext i1 %295 to i8
  %297 = icmp ne i8 %296, 0
  %298 = and i1 true, %297
  %299 = zext i1 %298 to i8
  %300 = icmp ne i8 %299, 0
  br i1 %300, label %then94, label %else95

then94:                                           ; preds = %ifcont93
  %301 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %301, align 1
  %302 = load ptr, ptr %__panic1, align 8
  %303 = getelementptr i8, ptr %302, i64 4
  store i32 %279, ptr %303, align 4
  br label %ifcont96

else95:                                           ; preds = %ifcont93
  br label %ifcont96

ifcont96:                                         ; preds = %else95, %then94
  %304 = phi {} [ zeroinitializer, %then94 ], [ zeroinitializer, %else95 ]
  %305 = icmp ne i8 %285, 0
  %306 = or i1 true, %305
  %307 = zext i1 %306 to i8
  %308 = icmp ne i8 %285, 0
  br i1 %308, label %then97, label %else98

then97:                                           ; preds = %ifcont96
  br label %ifcont99

else98:                                           ; preds = %ifcont96
  br label %ifcont99

ifcont99:                                         ; preds = %else98, %then97
  %309 = phi i32 [ %288, %then97 ], [ %279, %else98 ]
  %310 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %310, align 1
  %311 = getelementptr i8, ptr %310, i64 4
  store i32 0, ptr %311, align 4
  %312 = load { i32 }, ptr %counter, align 4
  %313 = load ptr, ptr %__panic1, align 8
  store { i32 } %312, ptr %ptr_arg100, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg100, ptr %313)
  %314 = load ptr, ptr %__panic1, align 8
  %315 = load i8, ptr %314, align 1
  %316 = load ptr, ptr %__panic1, align 8
  %317 = getelementptr i8, ptr %316, i64 4
  %318 = load i32, ptr %317, align 4
  %319 = icmp ne i8 %307, 0
  %320 = icmp ne i8 %315, 0
  %321 = and i1 %319, %320
  %322 = zext i1 %321 to i8
  %323 = icmp ne i8 %322, 0
  br i1 %323, label %then101, label %else102

then101:                                          ; preds = %ifcont99
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %318)
  br label %ifcont103

else102:                                          ; preds = %ifcont99
  br label %ifcont103

ifcont103:                                        ; preds = %else102, %then101
  %324 = phi {} [ zeroinitializer, %then101 ], [ zeroinitializer, %else102 ]
  %325 = icmp ne i8 %315, 0
  %326 = xor i1 %325, true
  %327 = zext i1 %326 to i8
  %328 = icmp ne i8 %307, 0
  %329 = icmp ne i8 %327, 0
  %330 = and i1 %328, %329
  %331 = zext i1 %330 to i8
  %332 = icmp ne i8 %331, 0
  br i1 %332, label %then104, label %else105

then104:                                          ; preds = %ifcont103
  %333 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %333, align 1
  %334 = load ptr, ptr %__panic1, align 8
  %335 = getelementptr i8, ptr %334, i64 4
  store i32 %309, ptr %335, align 4
  br label %ifcont106

else105:                                          ; preds = %ifcont103
  br label %ifcont106

ifcont106:                                        ; preds = %else105, %then104
  %336 = phi {} [ zeroinitializer, %then104 ], [ zeroinitializer, %else105 ]
  %337 = icmp ne i8 %307, 0
  %338 = icmp ne i8 %315, 0
  %339 = or i1 %337, %338
  %340 = zext i1 %339 to i8
  %341 = icmp ne i8 %315, 0
  br i1 %341, label %then107, label %else108

then107:                                          ; preds = %ifcont106
  br label %ifcont109

else108:                                          ; preds = %ifcont106
  br label %ifcont109

ifcont109:                                        ; preds = %else108, %then107
  %342 = phi i32 [ %318, %then107 ], [ %309, %else108 ]
  ret i32 0

panic_cont110:                                    ; preds = %panic_ok88
  %343 = icmp ne i8 %271, 0
  br i1 %343, label %then111, label %else112

then111:                                          ; preds = %panic_cont110
  store { i32 } zeroinitializer, ptr %record114, align 4
  %344 = load i32, ptr %count, align 4
  store i32 %344, ptr %record114, align 4
  %345 = load { i32 }, ptr %record114, align 4
  store { [12 x i8], [0 x i32] } zeroinitializer, ptr %widen_modal, align 4
  store i8 0, ptr %widen_modal, align 1
  store { i32 } zeroinitializer, ptr %record115, align 4
  %346 = load i32, ptr %count, align 4
  store i32 %346, ptr %record115, align 4
  %347 = load { i32 }, ptr %record115, align 4
  store { i32 } %347, ptr %tmp_addr, align 4
  %348 = load i32, ptr %tmp_addr, align 4
  %349 = getelementptr i8, ptr %widen_modal, i64 4
  store i32 %348, ptr %349, align 4
  %350 = load { [12 x i8], [0 x i32] }, ptr %widen_modal, align 4
  %351 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %351, align 1
  %352 = getelementptr i8, ptr %351, i64 4
  store i32 0, ptr %352, align 4
  %353 = load ptr, ptr %__panic1, align 8
  store { [12 x i8], [0 x i32] } %350, ptr %ptr_arg116, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aStatus(ptr %ptr_arg116, ptr %353)
  %354 = load ptr, ptr %__panic1, align 8
  %355 = load i8, ptr %354, align 1
  %356 = load ptr, ptr %__panic1, align 8
  %357 = getelementptr i8, ptr %356, i64 4
  %358 = load i32, ptr %357, align 4
  %359 = icmp ne i8 %355, 0
  %360 = and i1 false, %359
  %361 = zext i1 %360 to i8
  %362 = icmp ne i8 %361, 0
  br i1 %362, label %then117, label %else118

else112:                                          ; preds = %panic_cont110
  store { i32, i32 } zeroinitializer, ptr %record160, align 4
  %363 = load i32, ptr %count, align 4
  store i32 %363, ptr %record160, align 4
  %364 = getelementptr i8, ptr %record160, i64 4
  store i8 1, ptr %364, align 1
  %365 = load { i32, i32 }, ptr %record160, align 4
  store { [12 x i8], [0 x i32] } zeroinitializer, ptr %widen_modal161, align 4
  store i8 1, ptr %widen_modal161, align 1
  store { i32, i32 } zeroinitializer, ptr %record162, align 4
  %366 = load i32, ptr %count, align 4
  store i32 %366, ptr %record162, align 4
  %367 = getelementptr i8, ptr %record162, i64 4
  store i8 1, ptr %367, align 1
  %368 = load { i32, i32 }, ptr %record162, align 4
  store { i32, i32 } %368, ptr %tmp_addr163, align 4
  %369 = load i32, ptr %tmp_addr163, align 4
  %370 = getelementptr i8, ptr %widen_modal161, i64 4
  store i32 %369, ptr %370, align 4
  %371 = getelementptr i8, ptr %tmp_addr163, i64 4
  %372 = load i32, ptr %371, align 4
  %373 = getelementptr i8, ptr %widen_modal161, i64 8
  store i32 %372, ptr %373, align 4
  %374 = load { [12 x i8], [0 x i32] }, ptr %widen_modal161, align 4
  %375 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %375, align 1
  %376 = getelementptr i8, ptr %375, i64 4
  store i32 0, ptr %376, align 4
  %377 = load ptr, ptr %__panic1, align 8
  store { [12 x i8], [0 x i32] } %374, ptr %ptr_arg164, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aStatus(ptr %ptr_arg164, ptr %377)
  %378 = load ptr, ptr %__panic1, align 8
  %379 = load i8, ptr %378, align 1
  %380 = load ptr, ptr %__panic1, align 8
  %381 = getelementptr i8, ptr %380, i64 4
  %382 = load i32, ptr %381, align 4
  %383 = icmp ne i8 %379, 0
  %384 = and i1 false, %383
  %385 = zext i1 %384 to i8
  %386 = icmp ne i8 %385, 0
  br i1 %386, label %then165, label %else166

ifcont113:                                        ; preds = %panic_cont207, %panic_cont159
  %387 = phi { [12 x i8], [0 x i32] } [ %350, %panic_cont159 ], [ %374, %panic_cont207 ]
  store { [12 x i8], [0 x i32] } zeroinitializer, ptr %status, align 4
  store { [12 x i8], [0 x i32] } %387, ptr %status, align 4
  %388 = load { [12 x i8], [0 x i32] }, ptr %status, align 4
  store { [12 x i8], [0 x i32] } %388, ptr %match_val, align 4
  %389 = load i8, ptr %match_val, align 1
  %390 = icmp eq i8 %389, 0
  br i1 %390, label %pat_modal_payload, label %match_next

then117:                                          ; preds = %then111
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %358)
  br label %ifcont119

else118:                                          ; preds = %then111
  br label %ifcont119

ifcont119:                                        ; preds = %else118, %then117
  %391 = phi {} [ zeroinitializer, %then117 ], [ zeroinitializer, %else118 ]
  %392 = icmp ne i8 %355, 0
  %393 = xor i1 %392, true
  %394 = zext i1 %393 to i8
  %395 = icmp ne i8 %394, 0
  %396 = and i1 false, %395
  %397 = zext i1 %396 to i8
  %398 = icmp ne i8 %397, 0
  br i1 %398, label %then120, label %else121

then120:                                          ; preds = %ifcont119
  %399 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %399, align 1
  %400 = load ptr, ptr %__panic1, align 8
  %401 = getelementptr i8, ptr %400, i64 4
  store i32 0, ptr %401, align 4
  br label %ifcont122

else121:                                          ; preds = %ifcont119
  br label %ifcont122

ifcont122:                                        ; preds = %else121, %then120
  %402 = phi {} [ zeroinitializer, %then120 ], [ zeroinitializer, %else121 ]
  %403 = icmp ne i8 %355, 0
  %404 = or i1 false, %403
  %405 = zext i1 %404 to i8
  %406 = icmp ne i8 %355, 0
  br i1 %406, label %then123, label %else124

then123:                                          ; preds = %ifcont122
  br label %ifcont125

else124:                                          ; preds = %ifcont122
  br label %ifcont125

ifcont125:                                        ; preds = %else124, %then123
  %407 = phi i32 [ %358, %then123 ], [ 0, %else124 ]
  %408 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %408, align 1
  %409 = getelementptr i8, ptr %408, i64 4
  store i32 0, ptr %409, align 4
  store { i32 } zeroinitializer, ptr %record126, align 4
  %410 = load i32, ptr %count, align 4
  store i32 %410, ptr %record126, align 4
  %411 = load { i32 }, ptr %record126, align 4
  %412 = load ptr, ptr %__panic1, align 8
  store { i32 } %411, ptr %ptr_arg127, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aStatus_x3a_x3aIdle(ptr %ptr_arg127, ptr %412)
  %413 = load ptr, ptr %__panic1, align 8
  %414 = load i8, ptr %413, align 1
  %415 = load ptr, ptr %__panic1, align 8
  %416 = getelementptr i8, ptr %415, i64 4
  %417 = load i32, ptr %416, align 4
  %418 = icmp ne i8 %405, 0
  %419 = icmp ne i8 %414, 0
  %420 = and i1 %418, %419
  %421 = zext i1 %420 to i8
  %422 = icmp ne i8 %421, 0
  br i1 %422, label %then128, label %else129

then128:                                          ; preds = %ifcont125
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %417)
  br label %ifcont130

else129:                                          ; preds = %ifcont125
  br label %ifcont130

ifcont130:                                        ; preds = %else129, %then128
  %423 = phi {} [ zeroinitializer, %then128 ], [ zeroinitializer, %else129 ]
  %424 = icmp ne i8 %414, 0
  %425 = xor i1 %424, true
  %426 = zext i1 %425 to i8
  %427 = icmp ne i8 %405, 0
  %428 = icmp ne i8 %426, 0
  %429 = and i1 %427, %428
  %430 = zext i1 %429 to i8
  %431 = icmp ne i8 %430, 0
  br i1 %431, label %then131, label %else132

then131:                                          ; preds = %ifcont130
  %432 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %432, align 1
  %433 = load ptr, ptr %__panic1, align 8
  %434 = getelementptr i8, ptr %433, i64 4
  store i32 %407, ptr %434, align 4
  br label %ifcont133

else132:                                          ; preds = %ifcont130
  br label %ifcont133

ifcont133:                                        ; preds = %else132, %then131
  %435 = phi {} [ zeroinitializer, %then131 ], [ zeroinitializer, %else132 ]
  %436 = icmp ne i8 %405, 0
  %437 = icmp ne i8 %414, 0
  %438 = or i1 %436, %437
  %439 = zext i1 %438 to i8
  %440 = icmp ne i8 %414, 0
  br i1 %440, label %then134, label %else135

then134:                                          ; preds = %ifcont133
  br label %ifcont136

else135:                                          ; preds = %ifcont133
  br label %ifcont136

ifcont136:                                        ; preds = %else135, %then134
  %441 = phi i32 [ %417, %then134 ], [ %407, %else135 ]
  %442 = load ptr, ptr %__panic1, align 8
  %443 = load i8, ptr %442, align 1
  %444 = icmp ne i8 %443, 0
  br i1 %444, label %panic_fail138, label %panic_ok137

panic_ok137:                                      ; preds = %ifcont136
  br label %panic_cont159

panic_fail138:                                    ; preds = %ifcont136
  %445 = load ptr, ptr %__panic1, align 8
  %446 = load i8, ptr %445, align 1
  %447 = load ptr, ptr %__panic1, align 8
  %448 = getelementptr i8, ptr %447, i64 4
  %449 = load i32, ptr %448, align 4
  %450 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %450, align 1
  %451 = getelementptr i8, ptr %450, i64 4
  store i32 0, ptr %451, align 4
  %452 = load { i32 }, ptr %next, align 4
  %453 = load ptr, ptr %__panic1, align 8
  store { i32 } %452, ptr %ptr_arg139, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg139, ptr %453)
  %454 = load ptr, ptr %__panic1, align 8
  %455 = load i8, ptr %454, align 1
  %456 = load ptr, ptr %__panic1, align 8
  %457 = getelementptr i8, ptr %456, i64 4
  %458 = load i32, ptr %457, align 4
  %459 = icmp ne i8 %455, 0
  %460 = and i1 true, %459
  %461 = zext i1 %460 to i8
  %462 = icmp ne i8 %461, 0
  br i1 %462, label %then140, label %else141

then140:                                          ; preds = %panic_fail138
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %458)
  br label %ifcont142

else141:                                          ; preds = %panic_fail138
  br label %ifcont142

ifcont142:                                        ; preds = %else141, %then140
  %463 = phi {} [ zeroinitializer, %then140 ], [ zeroinitializer, %else141 ]
  %464 = icmp ne i8 %455, 0
  %465 = xor i1 %464, true
  %466 = zext i1 %465 to i8
  %467 = icmp ne i8 %466, 0
  %468 = and i1 true, %467
  %469 = zext i1 %468 to i8
  %470 = icmp ne i8 %469, 0
  br i1 %470, label %then143, label %else144

then143:                                          ; preds = %ifcont142
  %471 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %471, align 1
  %472 = load ptr, ptr %__panic1, align 8
  %473 = getelementptr i8, ptr %472, i64 4
  store i32 %449, ptr %473, align 4
  br label %ifcont145

else144:                                          ; preds = %ifcont142
  br label %ifcont145

ifcont145:                                        ; preds = %else144, %then143
  %474 = phi {} [ zeroinitializer, %then143 ], [ zeroinitializer, %else144 ]
  %475 = icmp ne i8 %455, 0
  %476 = or i1 true, %475
  %477 = zext i1 %476 to i8
  %478 = icmp ne i8 %455, 0
  br i1 %478, label %then146, label %else147

then146:                                          ; preds = %ifcont145
  br label %ifcont148

else147:                                          ; preds = %ifcont145
  br label %ifcont148

ifcont148:                                        ; preds = %else147, %then146
  %479 = phi i32 [ %458, %then146 ], [ %449, %else147 ]
  %480 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %480, align 1
  %481 = getelementptr i8, ptr %480, i64 4
  store i32 0, ptr %481, align 4
  %482 = load { i32 }, ptr %counter, align 4
  %483 = load ptr, ptr %__panic1, align 8
  store { i32 } %482, ptr %ptr_arg149, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg149, ptr %483)
  %484 = load ptr, ptr %__panic1, align 8
  %485 = load i8, ptr %484, align 1
  %486 = load ptr, ptr %__panic1, align 8
  %487 = getelementptr i8, ptr %486, i64 4
  %488 = load i32, ptr %487, align 4
  %489 = icmp ne i8 %477, 0
  %490 = icmp ne i8 %485, 0
  %491 = and i1 %489, %490
  %492 = zext i1 %491 to i8
  %493 = icmp ne i8 %492, 0
  br i1 %493, label %then150, label %else151

then150:                                          ; preds = %ifcont148
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %488)
  br label %ifcont152

else151:                                          ; preds = %ifcont148
  br label %ifcont152

ifcont152:                                        ; preds = %else151, %then150
  %494 = phi {} [ zeroinitializer, %then150 ], [ zeroinitializer, %else151 ]
  %495 = icmp ne i8 %485, 0
  %496 = xor i1 %495, true
  %497 = zext i1 %496 to i8
  %498 = icmp ne i8 %477, 0
  %499 = icmp ne i8 %497, 0
  %500 = and i1 %498, %499
  %501 = zext i1 %500 to i8
  %502 = icmp ne i8 %501, 0
  br i1 %502, label %then153, label %else154

then153:                                          ; preds = %ifcont152
  %503 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %503, align 1
  %504 = load ptr, ptr %__panic1, align 8
  %505 = getelementptr i8, ptr %504, i64 4
  store i32 %479, ptr %505, align 4
  br label %ifcont155

else154:                                          ; preds = %ifcont152
  br label %ifcont155

ifcont155:                                        ; preds = %else154, %then153
  %506 = phi {} [ zeroinitializer, %then153 ], [ zeroinitializer, %else154 ]
  %507 = icmp ne i8 %477, 0
  %508 = icmp ne i8 %485, 0
  %509 = or i1 %507, %508
  %510 = zext i1 %509 to i8
  %511 = icmp ne i8 %485, 0
  br i1 %511, label %then156, label %else157

then156:                                          ; preds = %ifcont155
  br label %ifcont158

else157:                                          ; preds = %ifcont155
  br label %ifcont158

ifcont158:                                        ; preds = %else157, %then156
  %512 = phi i32 [ %488, %then156 ], [ %479, %else157 ]
  ret i32 0

panic_cont159:                                    ; preds = %panic_ok137
  br label %ifcont113

then165:                                          ; preds = %else112
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %382)
  br label %ifcont167

else166:                                          ; preds = %else112
  br label %ifcont167

ifcont167:                                        ; preds = %else166, %then165
  %513 = phi {} [ zeroinitializer, %then165 ], [ zeroinitializer, %else166 ]
  %514 = icmp ne i8 %379, 0
  %515 = xor i1 %514, true
  %516 = zext i1 %515 to i8
  %517 = icmp ne i8 %516, 0
  %518 = and i1 false, %517
  %519 = zext i1 %518 to i8
  %520 = icmp ne i8 %519, 0
  br i1 %520, label %then168, label %else169

then168:                                          ; preds = %ifcont167
  %521 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %521, align 1
  %522 = load ptr, ptr %__panic1, align 8
  %523 = getelementptr i8, ptr %522, i64 4
  store i32 0, ptr %523, align 4
  br label %ifcont170

else169:                                          ; preds = %ifcont167
  br label %ifcont170

ifcont170:                                        ; preds = %else169, %then168
  %524 = phi {} [ zeroinitializer, %then168 ], [ zeroinitializer, %else169 ]
  %525 = icmp ne i8 %379, 0
  %526 = or i1 false, %525
  %527 = zext i1 %526 to i8
  %528 = icmp ne i8 %379, 0
  br i1 %528, label %then171, label %else172

then171:                                          ; preds = %ifcont170
  br label %ifcont173

else172:                                          ; preds = %ifcont170
  br label %ifcont173

ifcont173:                                        ; preds = %else172, %then171
  %529 = phi i32 [ %382, %then171 ], [ 0, %else172 ]
  %530 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %530, align 1
  %531 = getelementptr i8, ptr %530, i64 4
  store i32 0, ptr %531, align 4
  store { i32, i32 } zeroinitializer, ptr %record174, align 4
  %532 = load i32, ptr %count, align 4
  store i32 %532, ptr %record174, align 4
  %533 = getelementptr i8, ptr %record174, i64 4
  store i8 1, ptr %533, align 1
  %534 = load { i32, i32 }, ptr %record174, align 4
  %535 = load ptr, ptr %__panic1, align 8
  store { i32, i32 } %534, ptr %ptr_arg175, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aStatus_x3a_x3aBusy(ptr %ptr_arg175, ptr %535)
  %536 = load ptr, ptr %__panic1, align 8
  %537 = load i8, ptr %536, align 1
  %538 = load ptr, ptr %__panic1, align 8
  %539 = getelementptr i8, ptr %538, i64 4
  %540 = load i32, ptr %539, align 4
  %541 = icmp ne i8 %527, 0
  %542 = icmp ne i8 %537, 0
  %543 = and i1 %541, %542
  %544 = zext i1 %543 to i8
  %545 = icmp ne i8 %544, 0
  br i1 %545, label %then176, label %else177

then176:                                          ; preds = %ifcont173
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %540)
  br label %ifcont178

else177:                                          ; preds = %ifcont173
  br label %ifcont178

ifcont178:                                        ; preds = %else177, %then176
  %546 = phi {} [ zeroinitializer, %then176 ], [ zeroinitializer, %else177 ]
  %547 = icmp ne i8 %537, 0
  %548 = xor i1 %547, true
  %549 = zext i1 %548 to i8
  %550 = icmp ne i8 %527, 0
  %551 = icmp ne i8 %549, 0
  %552 = and i1 %550, %551
  %553 = zext i1 %552 to i8
  %554 = icmp ne i8 %553, 0
  br i1 %554, label %then179, label %else180

then179:                                          ; preds = %ifcont178
  %555 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %555, align 1
  %556 = load ptr, ptr %__panic1, align 8
  %557 = getelementptr i8, ptr %556, i64 4
  store i32 %529, ptr %557, align 4
  br label %ifcont181

else180:                                          ; preds = %ifcont178
  br label %ifcont181

ifcont181:                                        ; preds = %else180, %then179
  %558 = phi {} [ zeroinitializer, %then179 ], [ zeroinitializer, %else180 ]
  %559 = icmp ne i8 %527, 0
  %560 = icmp ne i8 %537, 0
  %561 = or i1 %559, %560
  %562 = zext i1 %561 to i8
  %563 = icmp ne i8 %537, 0
  br i1 %563, label %then182, label %else183

then182:                                          ; preds = %ifcont181
  br label %ifcont184

else183:                                          ; preds = %ifcont181
  br label %ifcont184

ifcont184:                                        ; preds = %else183, %then182
  %564 = phi i32 [ %540, %then182 ], [ %529, %else183 ]
  %565 = load ptr, ptr %__panic1, align 8
  %566 = load i8, ptr %565, align 1
  %567 = icmp ne i8 %566, 0
  br i1 %567, label %panic_fail186, label %panic_ok185

panic_ok185:                                      ; preds = %ifcont184
  br label %panic_cont207

panic_fail186:                                    ; preds = %ifcont184
  %568 = load ptr, ptr %__panic1, align 8
  %569 = load i8, ptr %568, align 1
  %570 = load ptr, ptr %__panic1, align 8
  %571 = getelementptr i8, ptr %570, i64 4
  %572 = load i32, ptr %571, align 4
  %573 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %573, align 1
  %574 = getelementptr i8, ptr %573, i64 4
  store i32 0, ptr %574, align 4
  %575 = load { i32 }, ptr %next, align 4
  %576 = load ptr, ptr %__panic1, align 8
  store { i32 } %575, ptr %ptr_arg187, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg187, ptr %576)
  %577 = load ptr, ptr %__panic1, align 8
  %578 = load i8, ptr %577, align 1
  %579 = load ptr, ptr %__panic1, align 8
  %580 = getelementptr i8, ptr %579, i64 4
  %581 = load i32, ptr %580, align 4
  %582 = icmp ne i8 %578, 0
  %583 = and i1 true, %582
  %584 = zext i1 %583 to i8
  %585 = icmp ne i8 %584, 0
  br i1 %585, label %then188, label %else189

then188:                                          ; preds = %panic_fail186
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %581)
  br label %ifcont190

else189:                                          ; preds = %panic_fail186
  br label %ifcont190

ifcont190:                                        ; preds = %else189, %then188
  %586 = phi {} [ zeroinitializer, %then188 ], [ zeroinitializer, %else189 ]
  %587 = icmp ne i8 %578, 0
  %588 = xor i1 %587, true
  %589 = zext i1 %588 to i8
  %590 = icmp ne i8 %589, 0
  %591 = and i1 true, %590
  %592 = zext i1 %591 to i8
  %593 = icmp ne i8 %592, 0
  br i1 %593, label %then191, label %else192

then191:                                          ; preds = %ifcont190
  %594 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %594, align 1
  %595 = load ptr, ptr %__panic1, align 8
  %596 = getelementptr i8, ptr %595, i64 4
  store i32 %572, ptr %596, align 4
  br label %ifcont193

else192:                                          ; preds = %ifcont190
  br label %ifcont193

ifcont193:                                        ; preds = %else192, %then191
  %597 = phi {} [ zeroinitializer, %then191 ], [ zeroinitializer, %else192 ]
  %598 = icmp ne i8 %578, 0
  %599 = or i1 true, %598
  %600 = zext i1 %599 to i8
  %601 = icmp ne i8 %578, 0
  br i1 %601, label %then194, label %else195

then194:                                          ; preds = %ifcont193
  br label %ifcont196

else195:                                          ; preds = %ifcont193
  br label %ifcont196

ifcont196:                                        ; preds = %else195, %then194
  %602 = phi i32 [ %581, %then194 ], [ %572, %else195 ]
  %603 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %603, align 1
  %604 = getelementptr i8, ptr %603, i64 4
  store i32 0, ptr %604, align 4
  %605 = load { i32 }, ptr %counter, align 4
  %606 = load ptr, ptr %__panic1, align 8
  store { i32 } %605, ptr %ptr_arg197, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg197, ptr %606)
  %607 = load ptr, ptr %__panic1, align 8
  %608 = load i8, ptr %607, align 1
  %609 = load ptr, ptr %__panic1, align 8
  %610 = getelementptr i8, ptr %609, i64 4
  %611 = load i32, ptr %610, align 4
  %612 = icmp ne i8 %600, 0
  %613 = icmp ne i8 %608, 0
  %614 = and i1 %612, %613
  %615 = zext i1 %614 to i8
  %616 = icmp ne i8 %615, 0
  br i1 %616, label %then198, label %else199

then198:                                          ; preds = %ifcont196
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %611)
  br label %ifcont200

else199:                                          ; preds = %ifcont196
  br label %ifcont200

ifcont200:                                        ; preds = %else199, %then198
  %617 = phi {} [ zeroinitializer, %then198 ], [ zeroinitializer, %else199 ]
  %618 = icmp ne i8 %608, 0
  %619 = xor i1 %618, true
  %620 = zext i1 %619 to i8
  %621 = icmp ne i8 %600, 0
  %622 = icmp ne i8 %620, 0
  %623 = and i1 %621, %622
  %624 = zext i1 %623 to i8
  %625 = icmp ne i8 %624, 0
  br i1 %625, label %then201, label %else202

then201:                                          ; preds = %ifcont200
  %626 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %626, align 1
  %627 = load ptr, ptr %__panic1, align 8
  %628 = getelementptr i8, ptr %627, i64 4
  store i32 %602, ptr %628, align 4
  br label %ifcont203

else202:                                          ; preds = %ifcont200
  br label %ifcont203

ifcont203:                                        ; preds = %else202, %then201
  %629 = phi {} [ zeroinitializer, %then201 ], [ zeroinitializer, %else202 ]
  %630 = icmp ne i8 %600, 0
  %631 = icmp ne i8 %608, 0
  %632 = or i1 %630, %631
  %633 = zext i1 %632 to i8
  %634 = icmp ne i8 %608, 0
  br i1 %634, label %then204, label %else205

then204:                                          ; preds = %ifcont203
  br label %ifcont206

else205:                                          ; preds = %ifcont203
  br label %ifcont206

ifcont206:                                        ; preds = %else205, %then204
  %635 = phi i32 [ %611, %then204 ], [ %602, %else205 ]
  ret i32 0

panic_cont207:                                    ; preds = %panic_ok185
  br label %ifcont113

match_merge:                                      ; preds = %panic_cont258, %panic_cont231
  %636 = phi { ptr, i64 } [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f7AC500837025B2F4, i64 13 }, %panic_cont231 ], [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fA01491A17C83EC9C, i64 18 }, %panic_cont258 ]
  store { ptr, i64 } zeroinitializer, ptr %status_msg, align 8
  store { ptr, i64 } %636, ptr %status_msg, align 8
  %637 = load ptr, ptr %__panic1, align 8
  %638 = load i8, ptr %637, align 1
  %639 = icmp ne i8 %638, 0
  br i1 %639, label %panic_fail260, label %panic_ok259

match_fail:                                       ; preds = %match_next
  unreachable

match_arm:                                        ; preds = %pat_modal_payload
  %640 = load { [12 x i8], [0 x i32] }, ptr %status, align 4
  store { [12 x i8], [0 x i32] } %640, ptr %tmp_addr208, align 4
  %641 = getelementptr i8, ptr %tmp_addr208, i64 4
  %642 = load i32, ptr %641, align 4
  store i32 0, ptr %ticks_val, align 4
  store i32 %642, ptr %ticks_val, align 4
  %643 = load ptr, ptr %__panic1, align 8
  %644 = load i8, ptr %643, align 1
  %645 = icmp ne i8 %644, 0
  br i1 %645, label %panic_fail210, label %panic_ok209

match_next:                                       ; preds = %ifcont113
  %646 = load i8, ptr %match_val, align 1
  %647 = icmp eq i8 %646, 1
  br i1 %647, label %pat_modal_payload233, label %match_fail

pat_modal_payload:                                ; preds = %ifcont113
  %648 = getelementptr i8, ptr %match_val, i64 4
  br label %match_arm

panic_ok209:                                      ; preds = %match_arm
  br label %panic_cont231

panic_fail210:                                    ; preds = %match_arm
  %649 = load ptr, ptr %__panic1, align 8
  %650 = load i8, ptr %649, align 1
  %651 = load ptr, ptr %__panic1, align 8
  %652 = getelementptr i8, ptr %651, i64 4
  %653 = load i32, ptr %652, align 4
  %654 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %654, align 1
  %655 = getelementptr i8, ptr %654, i64 4
  store i32 0, ptr %655, align 4
  %656 = load { i32 }, ptr %next, align 4
  %657 = load ptr, ptr %__panic1, align 8
  store { i32 } %656, ptr %ptr_arg211, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg211, ptr %657)
  %658 = load ptr, ptr %__panic1, align 8
  %659 = load i8, ptr %658, align 1
  %660 = load ptr, ptr %__panic1, align 8
  %661 = getelementptr i8, ptr %660, i64 4
  %662 = load i32, ptr %661, align 4
  %663 = icmp ne i8 %659, 0
  %664 = and i1 true, %663
  %665 = zext i1 %664 to i8
  %666 = icmp ne i8 %665, 0
  br i1 %666, label %then212, label %else213

then212:                                          ; preds = %panic_fail210
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %662)
  br label %ifcont214

else213:                                          ; preds = %panic_fail210
  br label %ifcont214

ifcont214:                                        ; preds = %else213, %then212
  %667 = phi {} [ zeroinitializer, %then212 ], [ zeroinitializer, %else213 ]
  %668 = icmp ne i8 %659, 0
  %669 = xor i1 %668, true
  %670 = zext i1 %669 to i8
  %671 = icmp ne i8 %670, 0
  %672 = and i1 true, %671
  %673 = zext i1 %672 to i8
  %674 = icmp ne i8 %673, 0
  br i1 %674, label %then215, label %else216

then215:                                          ; preds = %ifcont214
  %675 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %675, align 1
  %676 = load ptr, ptr %__panic1, align 8
  %677 = getelementptr i8, ptr %676, i64 4
  store i32 %653, ptr %677, align 4
  br label %ifcont217

else216:                                          ; preds = %ifcont214
  br label %ifcont217

ifcont217:                                        ; preds = %else216, %then215
  %678 = phi {} [ zeroinitializer, %then215 ], [ zeroinitializer, %else216 ]
  %679 = icmp ne i8 %659, 0
  %680 = or i1 true, %679
  %681 = zext i1 %680 to i8
  %682 = icmp ne i8 %659, 0
  br i1 %682, label %then218, label %else219

then218:                                          ; preds = %ifcont217
  br label %ifcont220

else219:                                          ; preds = %ifcont217
  br label %ifcont220

ifcont220:                                        ; preds = %else219, %then218
  %683 = phi i32 [ %662, %then218 ], [ %653, %else219 ]
  %684 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %684, align 1
  %685 = getelementptr i8, ptr %684, i64 4
  store i32 0, ptr %685, align 4
  %686 = load { i32 }, ptr %counter, align 4
  %687 = load ptr, ptr %__panic1, align 8
  store { i32 } %686, ptr %ptr_arg221, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg221, ptr %687)
  %688 = load ptr, ptr %__panic1, align 8
  %689 = load i8, ptr %688, align 1
  %690 = load ptr, ptr %__panic1, align 8
  %691 = getelementptr i8, ptr %690, i64 4
  %692 = load i32, ptr %691, align 4
  %693 = icmp ne i8 %681, 0
  %694 = icmp ne i8 %689, 0
  %695 = and i1 %693, %694
  %696 = zext i1 %695 to i8
  %697 = icmp ne i8 %696, 0
  br i1 %697, label %then222, label %else223

then222:                                          ; preds = %ifcont220
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %692)
  br label %ifcont224

else223:                                          ; preds = %ifcont220
  br label %ifcont224

ifcont224:                                        ; preds = %else223, %then222
  %698 = phi {} [ zeroinitializer, %then222 ], [ zeroinitializer, %else223 ]
  %699 = icmp ne i8 %689, 0
  %700 = xor i1 %699, true
  %701 = zext i1 %700 to i8
  %702 = icmp ne i8 %681, 0
  %703 = icmp ne i8 %701, 0
  %704 = and i1 %702, %703
  %705 = zext i1 %704 to i8
  %706 = icmp ne i8 %705, 0
  br i1 %706, label %then225, label %else226

then225:                                          ; preds = %ifcont224
  %707 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %707, align 1
  %708 = load ptr, ptr %__panic1, align 8
  %709 = getelementptr i8, ptr %708, i64 4
  store i32 %683, ptr %709, align 4
  br label %ifcont227

else226:                                          ; preds = %ifcont224
  br label %ifcont227

ifcont227:                                        ; preds = %else226, %then225
  %710 = phi {} [ zeroinitializer, %then225 ], [ zeroinitializer, %else226 ]
  %711 = icmp ne i8 %681, 0
  %712 = icmp ne i8 %689, 0
  %713 = or i1 %711, %712
  %714 = zext i1 %713 to i8
  %715 = icmp ne i8 %689, 0
  br i1 %715, label %then228, label %else229

then228:                                          ; preds = %ifcont227
  br label %ifcont230

else229:                                          ; preds = %ifcont227
  br label %ifcont230

ifcont230:                                        ; preds = %else229, %then228
  %716 = phi i32 [ %692, %then228 ], [ %683, %else229 ]
  ret i32 0

panic_cont231:                                    ; preds = %panic_ok209
  br label %match_merge

match_arm232:                                     ; preds = %pat_modal_next
  %717 = load { [12 x i8], [0 x i32] }, ptr %status, align 4
  store { [12 x i8], [0 x i32] } %717, ptr %tmp_addr234, align 4
  %718 = getelementptr i8, ptr %tmp_addr234, i64 4
  %719 = load i32, ptr %718, align 4
  store i32 0, ptr %code_val, align 4
  store i32 %719, ptr %code_val, align 4
  %720 = load { [12 x i8], [0 x i32] }, ptr %status, align 4
  store { [12 x i8], [0 x i32] } %720, ptr %tmp_addr235, align 4
  %721 = getelementptr i8, ptr %tmp_addr235, i64 8
  %722 = load i32, ptr %721, align 4
  store i32 0, ptr %retry_val, align 4
  store i32 %722, ptr %retry_val, align 4
  %723 = load ptr, ptr %__panic1, align 8
  %724 = load i8, ptr %723, align 1
  %725 = icmp ne i8 %724, 0
  br i1 %725, label %panic_fail237, label %panic_ok236

pat_modal_payload233:                             ; preds = %match_next
  %726 = getelementptr i8, ptr %match_val, i64 4
  br label %pat_modal_next

pat_modal_next:                                   ; preds = %pat_modal_payload233
  %727 = getelementptr i8, ptr %match_val, i64 8
  br label %match_arm232

panic_ok236:                                      ; preds = %match_arm232
  br label %panic_cont258

panic_fail237:                                    ; preds = %match_arm232
  %728 = load ptr, ptr %__panic1, align 8
  %729 = load i8, ptr %728, align 1
  %730 = load ptr, ptr %__panic1, align 8
  %731 = getelementptr i8, ptr %730, i64 4
  %732 = load i32, ptr %731, align 4
  %733 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %733, align 1
  %734 = getelementptr i8, ptr %733, i64 4
  store i32 0, ptr %734, align 4
  %735 = load { i32 }, ptr %next, align 4
  %736 = load ptr, ptr %__panic1, align 8
  store { i32 } %735, ptr %ptr_arg238, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg238, ptr %736)
  %737 = load ptr, ptr %__panic1, align 8
  %738 = load i8, ptr %737, align 1
  %739 = load ptr, ptr %__panic1, align 8
  %740 = getelementptr i8, ptr %739, i64 4
  %741 = load i32, ptr %740, align 4
  %742 = icmp ne i8 %738, 0
  %743 = and i1 true, %742
  %744 = zext i1 %743 to i8
  %745 = icmp ne i8 %744, 0
  br i1 %745, label %then239, label %else240

then239:                                          ; preds = %panic_fail237
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %741)
  br label %ifcont241

else240:                                          ; preds = %panic_fail237
  br label %ifcont241

ifcont241:                                        ; preds = %else240, %then239
  %746 = phi {} [ zeroinitializer, %then239 ], [ zeroinitializer, %else240 ]
  %747 = icmp ne i8 %738, 0
  %748 = xor i1 %747, true
  %749 = zext i1 %748 to i8
  %750 = icmp ne i8 %749, 0
  %751 = and i1 true, %750
  %752 = zext i1 %751 to i8
  %753 = icmp ne i8 %752, 0
  br i1 %753, label %then242, label %else243

then242:                                          ; preds = %ifcont241
  %754 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %754, align 1
  %755 = load ptr, ptr %__panic1, align 8
  %756 = getelementptr i8, ptr %755, i64 4
  store i32 %732, ptr %756, align 4
  br label %ifcont244

else243:                                          ; preds = %ifcont241
  br label %ifcont244

ifcont244:                                        ; preds = %else243, %then242
  %757 = phi {} [ zeroinitializer, %then242 ], [ zeroinitializer, %else243 ]
  %758 = icmp ne i8 %738, 0
  %759 = or i1 true, %758
  %760 = zext i1 %759 to i8
  %761 = icmp ne i8 %738, 0
  br i1 %761, label %then245, label %else246

then245:                                          ; preds = %ifcont244
  br label %ifcont247

else246:                                          ; preds = %ifcont244
  br label %ifcont247

ifcont247:                                        ; preds = %else246, %then245
  %762 = phi i32 [ %741, %then245 ], [ %732, %else246 ]
  %763 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %763, align 1
  %764 = getelementptr i8, ptr %763, i64 4
  store i32 0, ptr %764, align 4
  %765 = load { i32 }, ptr %counter, align 4
  %766 = load ptr, ptr %__panic1, align 8
  store { i32 } %765, ptr %ptr_arg248, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg248, ptr %766)
  %767 = load ptr, ptr %__panic1, align 8
  %768 = load i8, ptr %767, align 1
  %769 = load ptr, ptr %__panic1, align 8
  %770 = getelementptr i8, ptr %769, i64 4
  %771 = load i32, ptr %770, align 4
  %772 = icmp ne i8 %760, 0
  %773 = icmp ne i8 %768, 0
  %774 = and i1 %772, %773
  %775 = zext i1 %774 to i8
  %776 = icmp ne i8 %775, 0
  br i1 %776, label %then249, label %else250

then249:                                          ; preds = %ifcont247
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %771)
  br label %ifcont251

else250:                                          ; preds = %ifcont247
  br label %ifcont251

ifcont251:                                        ; preds = %else250, %then249
  %777 = phi {} [ zeroinitializer, %then249 ], [ zeroinitializer, %else250 ]
  %778 = icmp ne i8 %768, 0
  %779 = xor i1 %778, true
  %780 = zext i1 %779 to i8
  %781 = icmp ne i8 %760, 0
  %782 = icmp ne i8 %780, 0
  %783 = and i1 %781, %782
  %784 = zext i1 %783 to i8
  %785 = icmp ne i8 %784, 0
  br i1 %785, label %then252, label %else253

then252:                                          ; preds = %ifcont251
  %786 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %786, align 1
  %787 = load ptr, ptr %__panic1, align 8
  %788 = getelementptr i8, ptr %787, i64 4
  store i32 %762, ptr %788, align 4
  br label %ifcont254

else253:                                          ; preds = %ifcont251
  br label %ifcont254

ifcont254:                                        ; preds = %else253, %then252
  %789 = phi {} [ zeroinitializer, %then252 ], [ zeroinitializer, %else253 ]
  %790 = icmp ne i8 %760, 0
  %791 = icmp ne i8 %768, 0
  %792 = or i1 %790, %791
  %793 = zext i1 %792 to i8
  %794 = icmp ne i8 %768, 0
  br i1 %794, label %then255, label %else256

then255:                                          ; preds = %ifcont254
  br label %ifcont257

else256:                                          ; preds = %ifcont254
  br label %ifcont257

ifcont257:                                        ; preds = %else256, %then255
  %795 = phi i32 [ %771, %then255 ], [ %762, %else256 ]
  ret i32 0

panic_cont258:                                    ; preds = %panic_ok236
  br label %match_merge

panic_ok259:                                      ; preds = %match_merge
  br label %panic_cont281

panic_fail260:                                    ; preds = %match_merge
  %796 = load ptr, ptr %__panic1, align 8
  %797 = load i8, ptr %796, align 1
  %798 = load ptr, ptr %__panic1, align 8
  %799 = getelementptr i8, ptr %798, i64 4
  %800 = load i32, ptr %799, align 4
  %801 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %801, align 1
  %802 = getelementptr i8, ptr %801, i64 4
  store i32 0, ptr %802, align 4
  %803 = load { i32 }, ptr %next, align 4
  %804 = load ptr, ptr %__panic1, align 8
  store { i32 } %803, ptr %ptr_arg261, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg261, ptr %804)
  %805 = load ptr, ptr %__panic1, align 8
  %806 = load i8, ptr %805, align 1
  %807 = load ptr, ptr %__panic1, align 8
  %808 = getelementptr i8, ptr %807, i64 4
  %809 = load i32, ptr %808, align 4
  %810 = icmp ne i8 %806, 0
  %811 = and i1 true, %810
  %812 = zext i1 %811 to i8
  %813 = icmp ne i8 %812, 0
  br i1 %813, label %then262, label %else263

then262:                                          ; preds = %panic_fail260
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %809)
  br label %ifcont264

else263:                                          ; preds = %panic_fail260
  br label %ifcont264

ifcont264:                                        ; preds = %else263, %then262
  %814 = phi {} [ zeroinitializer, %then262 ], [ zeroinitializer, %else263 ]
  %815 = icmp ne i8 %806, 0
  %816 = xor i1 %815, true
  %817 = zext i1 %816 to i8
  %818 = icmp ne i8 %817, 0
  %819 = and i1 true, %818
  %820 = zext i1 %819 to i8
  %821 = icmp ne i8 %820, 0
  br i1 %821, label %then265, label %else266

then265:                                          ; preds = %ifcont264
  %822 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %822, align 1
  %823 = load ptr, ptr %__panic1, align 8
  %824 = getelementptr i8, ptr %823, i64 4
  store i32 %800, ptr %824, align 4
  br label %ifcont267

else266:                                          ; preds = %ifcont264
  br label %ifcont267

ifcont267:                                        ; preds = %else266, %then265
  %825 = phi {} [ zeroinitializer, %then265 ], [ zeroinitializer, %else266 ]
  %826 = icmp ne i8 %806, 0
  %827 = or i1 true, %826
  %828 = zext i1 %827 to i8
  %829 = icmp ne i8 %806, 0
  br i1 %829, label %then268, label %else269

then268:                                          ; preds = %ifcont267
  br label %ifcont270

else269:                                          ; preds = %ifcont267
  br label %ifcont270

ifcont270:                                        ; preds = %else269, %then268
  %830 = phi i32 [ %809, %then268 ], [ %800, %else269 ]
  %831 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %831, align 1
  %832 = getelementptr i8, ptr %831, i64 4
  store i32 0, ptr %832, align 4
  %833 = load { i32 }, ptr %counter, align 4
  %834 = load ptr, ptr %__panic1, align 8
  store { i32 } %833, ptr %ptr_arg271, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg271, ptr %834)
  %835 = load ptr, ptr %__panic1, align 8
  %836 = load i8, ptr %835, align 1
  %837 = load ptr, ptr %__panic1, align 8
  %838 = getelementptr i8, ptr %837, i64 4
  %839 = load i32, ptr %838, align 4
  %840 = icmp ne i8 %828, 0
  %841 = icmp ne i8 %836, 0
  %842 = and i1 %840, %841
  %843 = zext i1 %842 to i8
  %844 = icmp ne i8 %843, 0
  br i1 %844, label %then272, label %else273

then272:                                          ; preds = %ifcont270
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %839)
  br label %ifcont274

else273:                                          ; preds = %ifcont270
  br label %ifcont274

ifcont274:                                        ; preds = %else273, %then272
  %845 = phi {} [ zeroinitializer, %then272 ], [ zeroinitializer, %else273 ]
  %846 = icmp ne i8 %836, 0
  %847 = xor i1 %846, true
  %848 = zext i1 %847 to i8
  %849 = icmp ne i8 %828, 0
  %850 = icmp ne i8 %848, 0
  %851 = and i1 %849, %850
  %852 = zext i1 %851 to i8
  %853 = icmp ne i8 %852, 0
  br i1 %853, label %then275, label %else276

then275:                                          ; preds = %ifcont274
  %854 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %854, align 1
  %855 = load ptr, ptr %__panic1, align 8
  %856 = getelementptr i8, ptr %855, i64 4
  store i32 %830, ptr %856, align 4
  br label %ifcont277

else276:                                          ; preds = %ifcont274
  br label %ifcont277

ifcont277:                                        ; preds = %else276, %then275
  %857 = phi {} [ zeroinitializer, %then275 ], [ zeroinitializer, %else276 ]
  %858 = icmp ne i8 %828, 0
  %859 = icmp ne i8 %836, 0
  %860 = or i1 %858, %859
  %861 = zext i1 %860 to i8
  %862 = icmp ne i8 %836, 0
  br i1 %862, label %then278, label %else279

then278:                                          ; preds = %ifcont277
  br label %ifcont280

else279:                                          ; preds = %ifcont277
  br label %ifcont280

ifcont280:                                        ; preds = %else279, %then278
  %863 = phi i32 [ %839, %then278 ], [ %830, %else279 ]
  ret i32 0

panic_cont281:                                    ; preds = %panic_ok259
  %864 = load i32, ptr %count, align 4
  %865 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %864, i32 5)
  %866 = extractvalue { i32, i1 } %865, 1
  %867 = xor i1 %866, true
  br i1 %867, label %check_ok282, label %check_fail283

check_ok282:                                      ; preds = %check_fail283, %panic_cont281
  %868 = load ptr, ptr %__panic1, align 8
  %869 = load i8, ptr %868, align 1
  %870 = icmp ne i8 %869, 0
  br i1 %870, label %panic_fail285, label %panic_ok284

check_fail283:                                    ; preds = %panic_cont281
  %871 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %871, align 1
  %872 = getelementptr i8, ptr %871, i64 4
  store i32 4, ptr %872, align 4
  br label %check_ok282

panic_ok284:                                      ; preds = %check_ok282
  br label %panic_cont306

panic_fail285:                                    ; preds = %check_ok282
  %873 = load ptr, ptr %__panic1, align 8
  %874 = load i8, ptr %873, align 1
  %875 = load ptr, ptr %__panic1, align 8
  %876 = getelementptr i8, ptr %875, i64 4
  %877 = load i32, ptr %876, align 4
  %878 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %878, align 1
  %879 = getelementptr i8, ptr %878, i64 4
  store i32 0, ptr %879, align 4
  %880 = load { i32 }, ptr %next, align 4
  %881 = load ptr, ptr %__panic1, align 8
  store { i32 } %880, ptr %ptr_arg286, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg286, ptr %881)
  %882 = load ptr, ptr %__panic1, align 8
  %883 = load i8, ptr %882, align 1
  %884 = load ptr, ptr %__panic1, align 8
  %885 = getelementptr i8, ptr %884, i64 4
  %886 = load i32, ptr %885, align 4
  %887 = icmp ne i8 %883, 0
  %888 = and i1 true, %887
  %889 = zext i1 %888 to i8
  %890 = icmp ne i8 %889, 0
  br i1 %890, label %then287, label %else288

then287:                                          ; preds = %panic_fail285
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %886)
  br label %ifcont289

else288:                                          ; preds = %panic_fail285
  br label %ifcont289

ifcont289:                                        ; preds = %else288, %then287
  %891 = phi {} [ zeroinitializer, %then287 ], [ zeroinitializer, %else288 ]
  %892 = icmp ne i8 %883, 0
  %893 = xor i1 %892, true
  %894 = zext i1 %893 to i8
  %895 = icmp ne i8 %894, 0
  %896 = and i1 true, %895
  %897 = zext i1 %896 to i8
  %898 = icmp ne i8 %897, 0
  br i1 %898, label %then290, label %else291

then290:                                          ; preds = %ifcont289
  %899 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %899, align 1
  %900 = load ptr, ptr %__panic1, align 8
  %901 = getelementptr i8, ptr %900, i64 4
  store i32 %877, ptr %901, align 4
  br label %ifcont292

else291:                                          ; preds = %ifcont289
  br label %ifcont292

ifcont292:                                        ; preds = %else291, %then290
  %902 = phi {} [ zeroinitializer, %then290 ], [ zeroinitializer, %else291 ]
  %903 = icmp ne i8 %883, 0
  %904 = or i1 true, %903
  %905 = zext i1 %904 to i8
  %906 = icmp ne i8 %883, 0
  br i1 %906, label %then293, label %else294

then293:                                          ; preds = %ifcont292
  br label %ifcont295

else294:                                          ; preds = %ifcont292
  br label %ifcont295

ifcont295:                                        ; preds = %else294, %then293
  %907 = phi i32 [ %886, %then293 ], [ %877, %else294 ]
  %908 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %908, align 1
  %909 = getelementptr i8, ptr %908, i64 4
  store i32 0, ptr %909, align 4
  %910 = load { i32 }, ptr %counter, align 4
  %911 = load ptr, ptr %__panic1, align 8
  store { i32 } %910, ptr %ptr_arg296, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg296, ptr %911)
  %912 = load ptr, ptr %__panic1, align 8
  %913 = load i8, ptr %912, align 1
  %914 = load ptr, ptr %__panic1, align 8
  %915 = getelementptr i8, ptr %914, i64 4
  %916 = load i32, ptr %915, align 4
  %917 = icmp ne i8 %905, 0
  %918 = icmp ne i8 %913, 0
  %919 = and i1 %917, %918
  %920 = zext i1 %919 to i8
  %921 = icmp ne i8 %920, 0
  br i1 %921, label %then297, label %else298

then297:                                          ; preds = %ifcont295
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %916)
  br label %ifcont299

else298:                                          ; preds = %ifcont295
  br label %ifcont299

ifcont299:                                        ; preds = %else298, %then297
  %922 = phi {} [ zeroinitializer, %then297 ], [ zeroinitializer, %else298 ]
  %923 = icmp ne i8 %913, 0
  %924 = xor i1 %923, true
  %925 = zext i1 %924 to i8
  %926 = icmp ne i8 %905, 0
  %927 = icmp ne i8 %925, 0
  %928 = and i1 %926, %927
  %929 = zext i1 %928 to i8
  %930 = icmp ne i8 %929, 0
  br i1 %930, label %then300, label %else301

then300:                                          ; preds = %ifcont299
  %931 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %931, align 1
  %932 = load ptr, ptr %__panic1, align 8
  %933 = getelementptr i8, ptr %932, i64 4
  store i32 %907, ptr %933, align 4
  br label %ifcont302

else301:                                          ; preds = %ifcont299
  br label %ifcont302

ifcont302:                                        ; preds = %else301, %then300
  %934 = phi {} [ zeroinitializer, %then300 ], [ zeroinitializer, %else301 ]
  %935 = icmp ne i8 %905, 0
  %936 = icmp ne i8 %913, 0
  %937 = or i1 %935, %936
  %938 = zext i1 %937 to i8
  %939 = icmp ne i8 %913, 0
  br i1 %939, label %then303, label %else304

then303:                                          ; preds = %ifcont302
  br label %ifcont305

else304:                                          ; preds = %ifcont302
  br label %ifcont305

ifcont305:                                        ; preds = %else304, %then303
  %940 = phi i32 [ %916, %then303 ], [ %907, %else304 ]
  ret i32 0

panic_cont306:                                    ; preds = %panic_ok284
  %941 = load i32, ptr %count, align 4
  %942 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %941, i32 5)
  %943 = extractvalue { i32, i1 } %942, 0
  %944 = extractvalue { i32, i1 } %942, 1
  %945 = freeze i32 %943
  br i1 %944, label %op_fail, label %op_ok

op_ok:                                            ; preds = %panic_cont306
  store i32 0, ptr %x, align 4
  store i32 %945, ptr %x, align 4
  %946 = load ptr, ptr %__panic1, align 8
  %947 = load i8, ptr %946, align 1
  %948 = icmp ne i8 %947, 0
  br i1 %948, label %panic_fail308, label %panic_ok307

op_fail:                                          ; preds = %panic_cont306
  %949 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %949, align 1
  %950 = getelementptr i8, ptr %949, i64 4
  store i32 4, ptr %950, align 4
  ret i32 0

panic_ok307:                                      ; preds = %op_ok
  br label %panic_cont329

panic_fail308:                                    ; preds = %op_ok
  %951 = load ptr, ptr %__panic1, align 8
  %952 = load i8, ptr %951, align 1
  %953 = load ptr, ptr %__panic1, align 8
  %954 = getelementptr i8, ptr %953, i64 4
  %955 = load i32, ptr %954, align 4
  %956 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %956, align 1
  %957 = getelementptr i8, ptr %956, i64 4
  store i32 0, ptr %957, align 4
  %958 = load { i32 }, ptr %next, align 4
  %959 = load ptr, ptr %__panic1, align 8
  store { i32 } %958, ptr %ptr_arg309, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg309, ptr %959)
  %960 = load ptr, ptr %__panic1, align 8
  %961 = load i8, ptr %960, align 1
  %962 = load ptr, ptr %__panic1, align 8
  %963 = getelementptr i8, ptr %962, i64 4
  %964 = load i32, ptr %963, align 4
  %965 = icmp ne i8 %961, 0
  %966 = and i1 true, %965
  %967 = zext i1 %966 to i8
  %968 = icmp ne i8 %967, 0
  br i1 %968, label %then310, label %else311

then310:                                          ; preds = %panic_fail308
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %964)
  br label %ifcont312

else311:                                          ; preds = %panic_fail308
  br label %ifcont312

ifcont312:                                        ; preds = %else311, %then310
  %969 = phi {} [ zeroinitializer, %then310 ], [ zeroinitializer, %else311 ]
  %970 = icmp ne i8 %961, 0
  %971 = xor i1 %970, true
  %972 = zext i1 %971 to i8
  %973 = icmp ne i8 %972, 0
  %974 = and i1 true, %973
  %975 = zext i1 %974 to i8
  %976 = icmp ne i8 %975, 0
  br i1 %976, label %then313, label %else314

then313:                                          ; preds = %ifcont312
  %977 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %977, align 1
  %978 = load ptr, ptr %__panic1, align 8
  %979 = getelementptr i8, ptr %978, i64 4
  store i32 %955, ptr %979, align 4
  br label %ifcont315

else314:                                          ; preds = %ifcont312
  br label %ifcont315

ifcont315:                                        ; preds = %else314, %then313
  %980 = phi {} [ zeroinitializer, %then313 ], [ zeroinitializer, %else314 ]
  %981 = icmp ne i8 %961, 0
  %982 = or i1 true, %981
  %983 = zext i1 %982 to i8
  %984 = icmp ne i8 %961, 0
  br i1 %984, label %then316, label %else317

then316:                                          ; preds = %ifcont315
  br label %ifcont318

else317:                                          ; preds = %ifcont315
  br label %ifcont318

ifcont318:                                        ; preds = %else317, %then316
  %985 = phi i32 [ %964, %then316 ], [ %955, %else317 ]
  %986 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %986, align 1
  %987 = getelementptr i8, ptr %986, i64 4
  store i32 0, ptr %987, align 4
  %988 = load { i32 }, ptr %counter, align 4
  %989 = load ptr, ptr %__panic1, align 8
  store { i32 } %988, ptr %ptr_arg319, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg319, ptr %989)
  %990 = load ptr, ptr %__panic1, align 8
  %991 = load i8, ptr %990, align 1
  %992 = load ptr, ptr %__panic1, align 8
  %993 = getelementptr i8, ptr %992, i64 4
  %994 = load i32, ptr %993, align 4
  %995 = icmp ne i8 %983, 0
  %996 = icmp ne i8 %991, 0
  %997 = and i1 %995, %996
  %998 = zext i1 %997 to i8
  %999 = icmp ne i8 %998, 0
  br i1 %999, label %then320, label %else321

then320:                                          ; preds = %ifcont318
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %994)
  br label %ifcont322

else321:                                          ; preds = %ifcont318
  br label %ifcont322

ifcont322:                                        ; preds = %else321, %then320
  %1000 = phi {} [ zeroinitializer, %then320 ], [ zeroinitializer, %else321 ]
  %1001 = icmp ne i8 %991, 0
  %1002 = xor i1 %1001, true
  %1003 = zext i1 %1002 to i8
  %1004 = icmp ne i8 %983, 0
  %1005 = icmp ne i8 %1003, 0
  %1006 = and i1 %1004, %1005
  %1007 = zext i1 %1006 to i8
  %1008 = icmp ne i8 %1007, 0
  br i1 %1008, label %then323, label %else324

then323:                                          ; preds = %ifcont322
  %1009 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1009, align 1
  %1010 = load ptr, ptr %__panic1, align 8
  %1011 = getelementptr i8, ptr %1010, i64 4
  store i32 %985, ptr %1011, align 4
  br label %ifcont325

else324:                                          ; preds = %ifcont322
  br label %ifcont325

ifcont325:                                        ; preds = %else324, %then323
  %1012 = phi {} [ zeroinitializer, %then323 ], [ zeroinitializer, %else324 ]
  %1013 = icmp ne i8 %983, 0
  %1014 = icmp ne i8 %991, 0
  %1015 = or i1 %1013, %1014
  %1016 = zext i1 %1015 to i8
  %1017 = icmp ne i8 %991, 0
  br i1 %1017, label %then326, label %else327

then326:                                          ; preds = %ifcont325
  br label %ifcont328

else327:                                          ; preds = %ifcont325
  br label %ifcont328

ifcont328:                                        ; preds = %else327, %then326
  %1018 = phi i32 [ %994, %then326 ], [ %985, %else327 ]
  ret i32 0

panic_cont329:                                    ; preds = %panic_ok307
  store ptr null, ptr %p, align 8
  store ptr %x, ptr %p, align 8
  %1019 = load i32, ptr %x, align 4
  %1020 = icmp sgt i32 %1019, 6
  %1021 = zext i1 %1020 to i8
  %1022 = load ptr, ptr %__panic1, align 8
  %1023 = load i8, ptr %1022, align 1
  %1024 = icmp ne i8 %1023, 0
  br i1 %1024, label %panic_fail331, label %panic_ok330

panic_ok330:                                      ; preds = %panic_cont329
  br label %panic_cont352

panic_fail331:                                    ; preds = %panic_cont329
  %1025 = load ptr, ptr %__panic1, align 8
  %1026 = load i8, ptr %1025, align 1
  %1027 = load ptr, ptr %__panic1, align 8
  %1028 = getelementptr i8, ptr %1027, i64 4
  %1029 = load i32, ptr %1028, align 4
  %1030 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1030, align 1
  %1031 = getelementptr i8, ptr %1030, i64 4
  store i32 0, ptr %1031, align 4
  %1032 = load { i32 }, ptr %next, align 4
  %1033 = load ptr, ptr %__panic1, align 8
  store { i32 } %1032, ptr %ptr_arg332, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg332, ptr %1033)
  %1034 = load ptr, ptr %__panic1, align 8
  %1035 = load i8, ptr %1034, align 1
  %1036 = load ptr, ptr %__panic1, align 8
  %1037 = getelementptr i8, ptr %1036, i64 4
  %1038 = load i32, ptr %1037, align 4
  %1039 = icmp ne i8 %1035, 0
  %1040 = and i1 true, %1039
  %1041 = zext i1 %1040 to i8
  %1042 = icmp ne i8 %1041, 0
  br i1 %1042, label %then333, label %else334

then333:                                          ; preds = %panic_fail331
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1038)
  br label %ifcont335

else334:                                          ; preds = %panic_fail331
  br label %ifcont335

ifcont335:                                        ; preds = %else334, %then333
  %1043 = phi {} [ zeroinitializer, %then333 ], [ zeroinitializer, %else334 ]
  %1044 = icmp ne i8 %1035, 0
  %1045 = xor i1 %1044, true
  %1046 = zext i1 %1045 to i8
  %1047 = icmp ne i8 %1046, 0
  %1048 = and i1 true, %1047
  %1049 = zext i1 %1048 to i8
  %1050 = icmp ne i8 %1049, 0
  br i1 %1050, label %then336, label %else337

then336:                                          ; preds = %ifcont335
  %1051 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1051, align 1
  %1052 = load ptr, ptr %__panic1, align 8
  %1053 = getelementptr i8, ptr %1052, i64 4
  store i32 %1029, ptr %1053, align 4
  br label %ifcont338

else337:                                          ; preds = %ifcont335
  br label %ifcont338

ifcont338:                                        ; preds = %else337, %then336
  %1054 = phi {} [ zeroinitializer, %then336 ], [ zeroinitializer, %else337 ]
  %1055 = icmp ne i8 %1035, 0
  %1056 = or i1 true, %1055
  %1057 = zext i1 %1056 to i8
  %1058 = icmp ne i8 %1035, 0
  br i1 %1058, label %then339, label %else340

then339:                                          ; preds = %ifcont338
  br label %ifcont341

else340:                                          ; preds = %ifcont338
  br label %ifcont341

ifcont341:                                        ; preds = %else340, %then339
  %1059 = phi i32 [ %1038, %then339 ], [ %1029, %else340 ]
  %1060 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1060, align 1
  %1061 = getelementptr i8, ptr %1060, i64 4
  store i32 0, ptr %1061, align 4
  %1062 = load { i32 }, ptr %counter, align 4
  %1063 = load ptr, ptr %__panic1, align 8
  store { i32 } %1062, ptr %ptr_arg342, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg342, ptr %1063)
  %1064 = load ptr, ptr %__panic1, align 8
  %1065 = load i8, ptr %1064, align 1
  %1066 = load ptr, ptr %__panic1, align 8
  %1067 = getelementptr i8, ptr %1066, i64 4
  %1068 = load i32, ptr %1067, align 4
  %1069 = icmp ne i8 %1057, 0
  %1070 = icmp ne i8 %1065, 0
  %1071 = and i1 %1069, %1070
  %1072 = zext i1 %1071 to i8
  %1073 = icmp ne i8 %1072, 0
  br i1 %1073, label %then343, label %else344

then343:                                          ; preds = %ifcont341
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1068)
  br label %ifcont345

else344:                                          ; preds = %ifcont341
  br label %ifcont345

ifcont345:                                        ; preds = %else344, %then343
  %1074 = phi {} [ zeroinitializer, %then343 ], [ zeroinitializer, %else344 ]
  %1075 = icmp ne i8 %1065, 0
  %1076 = xor i1 %1075, true
  %1077 = zext i1 %1076 to i8
  %1078 = icmp ne i8 %1057, 0
  %1079 = icmp ne i8 %1077, 0
  %1080 = and i1 %1078, %1079
  %1081 = zext i1 %1080 to i8
  %1082 = icmp ne i8 %1081, 0
  br i1 %1082, label %then346, label %else347

then346:                                          ; preds = %ifcont345
  %1083 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1083, align 1
  %1084 = load ptr, ptr %__panic1, align 8
  %1085 = getelementptr i8, ptr %1084, i64 4
  store i32 %1059, ptr %1085, align 4
  br label %ifcont348

else347:                                          ; preds = %ifcont345
  br label %ifcont348

ifcont348:                                        ; preds = %else347, %then346
  %1086 = phi {} [ zeroinitializer, %then346 ], [ zeroinitializer, %else347 ]
  %1087 = icmp ne i8 %1057, 0
  %1088 = icmp ne i8 %1065, 0
  %1089 = or i1 %1087, %1088
  %1090 = zext i1 %1089 to i8
  %1091 = icmp ne i8 %1065, 0
  br i1 %1091, label %then349, label %else350

then349:                                          ; preds = %ifcont348
  br label %ifcont351

else350:                                          ; preds = %ifcont348
  br label %ifcont351

ifcont351:                                        ; preds = %else350, %then349
  %1092 = phi i32 [ %1068, %then349 ], [ %1059, %else350 ]
  ret i32 0

panic_cont352:                                    ; preds = %panic_ok330
  %1093 = icmp ne i8 %1021, 0
  br i1 %1093, label %then353, label %else354

then353:                                          ; preds = %panic_cont352
  store { ptr } zeroinitializer, ptr %record356, align 8
  %1094 = load ptr, ptr %p, align 8
  store ptr %1094, ptr %record356, align 8
  %1095 = load { ptr }, ptr %record356, align 8
  store { [16 x i8], [0 x i64] } zeroinitializer, ptr %widen_modal357, align 8
  store i8 1, ptr %widen_modal357, align 1
  store { ptr } zeroinitializer, ptr %record358, align 8
  %1096 = load ptr, ptr %p, align 8
  store ptr %1096, ptr %record358, align 8
  %1097 = load { ptr }, ptr %record358, align 8
  store { ptr } %1097, ptr %tmp_addr359, align 8
  %1098 = load ptr, ptr %tmp_addr359, align 8
  %1099 = getelementptr i8, ptr %widen_modal357, i64 8
  store ptr %1098, ptr %1099, align 8
  %1100 = load { [16 x i8], [0 x i64] }, ptr %widen_modal357, align 8
  %1101 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1101, align 1
  %1102 = getelementptr i8, ptr %1101, i64 4
  store i32 0, ptr %1102, align 4
  %1103 = load ptr, ptr %__panic1, align 8
  store { [16 x i8], [0 x i64] } %1100, ptr %ptr_arg360, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aOptPtr(ptr %ptr_arg360, ptr %1103)
  %1104 = load ptr, ptr %__panic1, align 8
  %1105 = load i8, ptr %1104, align 1
  %1106 = load ptr, ptr %__panic1, align 8
  %1107 = getelementptr i8, ptr %1106, i64 4
  %1108 = load i32, ptr %1107, align 4
  %1109 = icmp ne i8 %1105, 0
  %1110 = and i1 false, %1109
  %1111 = zext i1 %1110 to i8
  %1112 = icmp ne i8 %1111, 0
  br i1 %1112, label %then361, label %else362

else354:                                          ; preds = %panic_cont352
  store { i8 } zeroinitializer, ptr %record404, align 1
  store i8 0, ptr %record404, align 1
  %1113 = load { i8 }, ptr %record404, align 1
  store { [16 x i8], [0 x i64] } zeroinitializer, ptr %widen_modal405, align 8
  store i8 0, ptr %widen_modal405, align 1
  store { i8 } zeroinitializer, ptr %record406, align 1
  store i8 0, ptr %record406, align 1
  %1114 = load { i8 }, ptr %record406, align 1
  store { i8 } %1114, ptr %tmp_addr407, align 1
  %1115 = load i8, ptr %tmp_addr407, align 1
  %1116 = getelementptr i8, ptr %widen_modal405, i64 8
  store i8 %1115, ptr %1116, align 1
  %1117 = load { [16 x i8], [0 x i64] }, ptr %widen_modal405, align 8
  %1118 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1118, align 1
  %1119 = getelementptr i8, ptr %1118, i64 4
  store i32 0, ptr %1119, align 4
  %1120 = load ptr, ptr %__panic1, align 8
  store { [16 x i8], [0 x i64] } %1117, ptr %ptr_arg408, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aOptPtr(ptr %ptr_arg408, ptr %1120)
  %1121 = load ptr, ptr %__panic1, align 8
  %1122 = load i8, ptr %1121, align 1
  %1123 = load ptr, ptr %__panic1, align 8
  %1124 = getelementptr i8, ptr %1123, i64 4
  %1125 = load i32, ptr %1124, align 4
  %1126 = icmp ne i8 %1122, 0
  %1127 = and i1 false, %1126
  %1128 = zext i1 %1127 to i8
  %1129 = icmp ne i8 %1128, 0
  br i1 %1129, label %then409, label %else410

ifcont355:                                        ; preds = %panic_cont451, %panic_cont403
  %1130 = phi { [16 x i8], [0 x i64] } [ %1100, %panic_cont403 ], [ %1117, %panic_cont451 ]
  store { [16 x i8], [0 x i64] } zeroinitializer, ptr %opt, align 8
  store { [16 x i8], [0 x i64] } %1130, ptr %opt, align 8
  %1131 = load { [16 x i8], [0 x i64] }, ptr %opt, align 8
  store { [16 x i8], [0 x i64] } %1131, ptr %match_val452, align 8
  %1132 = load i8, ptr %match_val452, align 1
  %1133 = icmp eq i8 %1132, 1
  br i1 %1133, label %pat_modal_payload457, label %match_next456

then361:                                          ; preds = %then353
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1108)
  br label %ifcont363

else362:                                          ; preds = %then353
  br label %ifcont363

ifcont363:                                        ; preds = %else362, %then361
  %1134 = phi {} [ zeroinitializer, %then361 ], [ zeroinitializer, %else362 ]
  %1135 = icmp ne i8 %1105, 0
  %1136 = xor i1 %1135, true
  %1137 = zext i1 %1136 to i8
  %1138 = icmp ne i8 %1137, 0
  %1139 = and i1 false, %1138
  %1140 = zext i1 %1139 to i8
  %1141 = icmp ne i8 %1140, 0
  br i1 %1141, label %then364, label %else365

then364:                                          ; preds = %ifcont363
  %1142 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1142, align 1
  %1143 = load ptr, ptr %__panic1, align 8
  %1144 = getelementptr i8, ptr %1143, i64 4
  store i32 0, ptr %1144, align 4
  br label %ifcont366

else365:                                          ; preds = %ifcont363
  br label %ifcont366

ifcont366:                                        ; preds = %else365, %then364
  %1145 = phi {} [ zeroinitializer, %then364 ], [ zeroinitializer, %else365 ]
  %1146 = icmp ne i8 %1105, 0
  %1147 = or i1 false, %1146
  %1148 = zext i1 %1147 to i8
  %1149 = icmp ne i8 %1105, 0
  br i1 %1149, label %then367, label %else368

then367:                                          ; preds = %ifcont366
  br label %ifcont369

else368:                                          ; preds = %ifcont366
  br label %ifcont369

ifcont369:                                        ; preds = %else368, %then367
  %1150 = phi i32 [ %1108, %then367 ], [ 0, %else368 ]
  %1151 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1151, align 1
  %1152 = getelementptr i8, ptr %1151, i64 4
  store i32 0, ptr %1152, align 4
  store { ptr } zeroinitializer, ptr %record370, align 8
  %1153 = load ptr, ptr %p, align 8
  store ptr %1153, ptr %record370, align 8
  %1154 = load { ptr }, ptr %record370, align 8
  %1155 = load ptr, ptr %__panic1, align 8
  store { ptr } %1154, ptr %ptr_arg371, align 8
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aOptPtr_x3a_x3aValid(ptr %ptr_arg371, ptr %1155)
  %1156 = load ptr, ptr %__panic1, align 8
  %1157 = load i8, ptr %1156, align 1
  %1158 = load ptr, ptr %__panic1, align 8
  %1159 = getelementptr i8, ptr %1158, i64 4
  %1160 = load i32, ptr %1159, align 4
  %1161 = icmp ne i8 %1148, 0
  %1162 = icmp ne i8 %1157, 0
  %1163 = and i1 %1161, %1162
  %1164 = zext i1 %1163 to i8
  %1165 = icmp ne i8 %1164, 0
  br i1 %1165, label %then372, label %else373

then372:                                          ; preds = %ifcont369
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1160)
  br label %ifcont374

else373:                                          ; preds = %ifcont369
  br label %ifcont374

ifcont374:                                        ; preds = %else373, %then372
  %1166 = phi {} [ zeroinitializer, %then372 ], [ zeroinitializer, %else373 ]
  %1167 = icmp ne i8 %1157, 0
  %1168 = xor i1 %1167, true
  %1169 = zext i1 %1168 to i8
  %1170 = icmp ne i8 %1148, 0
  %1171 = icmp ne i8 %1169, 0
  %1172 = and i1 %1170, %1171
  %1173 = zext i1 %1172 to i8
  %1174 = icmp ne i8 %1173, 0
  br i1 %1174, label %then375, label %else376

then375:                                          ; preds = %ifcont374
  %1175 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1175, align 1
  %1176 = load ptr, ptr %__panic1, align 8
  %1177 = getelementptr i8, ptr %1176, i64 4
  store i32 %1150, ptr %1177, align 4
  br label %ifcont377

else376:                                          ; preds = %ifcont374
  br label %ifcont377

ifcont377:                                        ; preds = %else376, %then375
  %1178 = phi {} [ zeroinitializer, %then375 ], [ zeroinitializer, %else376 ]
  %1179 = icmp ne i8 %1148, 0
  %1180 = icmp ne i8 %1157, 0
  %1181 = or i1 %1179, %1180
  %1182 = zext i1 %1181 to i8
  %1183 = icmp ne i8 %1157, 0
  br i1 %1183, label %then378, label %else379

then378:                                          ; preds = %ifcont377
  br label %ifcont380

else379:                                          ; preds = %ifcont377
  br label %ifcont380

ifcont380:                                        ; preds = %else379, %then378
  %1184 = phi i32 [ %1160, %then378 ], [ %1150, %else379 ]
  %1185 = load ptr, ptr %__panic1, align 8
  %1186 = load i8, ptr %1185, align 1
  %1187 = icmp ne i8 %1186, 0
  br i1 %1187, label %panic_fail382, label %panic_ok381

panic_ok381:                                      ; preds = %ifcont380
  br label %panic_cont403

panic_fail382:                                    ; preds = %ifcont380
  %1188 = load ptr, ptr %__panic1, align 8
  %1189 = load i8, ptr %1188, align 1
  %1190 = load ptr, ptr %__panic1, align 8
  %1191 = getelementptr i8, ptr %1190, i64 4
  %1192 = load i32, ptr %1191, align 4
  %1193 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1193, align 1
  %1194 = getelementptr i8, ptr %1193, i64 4
  store i32 0, ptr %1194, align 4
  %1195 = load { i32 }, ptr %next, align 4
  %1196 = load ptr, ptr %__panic1, align 8
  store { i32 } %1195, ptr %ptr_arg383, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg383, ptr %1196)
  %1197 = load ptr, ptr %__panic1, align 8
  %1198 = load i8, ptr %1197, align 1
  %1199 = load ptr, ptr %__panic1, align 8
  %1200 = getelementptr i8, ptr %1199, i64 4
  %1201 = load i32, ptr %1200, align 4
  %1202 = icmp ne i8 %1198, 0
  %1203 = and i1 true, %1202
  %1204 = zext i1 %1203 to i8
  %1205 = icmp ne i8 %1204, 0
  br i1 %1205, label %then384, label %else385

then384:                                          ; preds = %panic_fail382
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1201)
  br label %ifcont386

else385:                                          ; preds = %panic_fail382
  br label %ifcont386

ifcont386:                                        ; preds = %else385, %then384
  %1206 = phi {} [ zeroinitializer, %then384 ], [ zeroinitializer, %else385 ]
  %1207 = icmp ne i8 %1198, 0
  %1208 = xor i1 %1207, true
  %1209 = zext i1 %1208 to i8
  %1210 = icmp ne i8 %1209, 0
  %1211 = and i1 true, %1210
  %1212 = zext i1 %1211 to i8
  %1213 = icmp ne i8 %1212, 0
  br i1 %1213, label %then387, label %else388

then387:                                          ; preds = %ifcont386
  %1214 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1214, align 1
  %1215 = load ptr, ptr %__panic1, align 8
  %1216 = getelementptr i8, ptr %1215, i64 4
  store i32 %1192, ptr %1216, align 4
  br label %ifcont389

else388:                                          ; preds = %ifcont386
  br label %ifcont389

ifcont389:                                        ; preds = %else388, %then387
  %1217 = phi {} [ zeroinitializer, %then387 ], [ zeroinitializer, %else388 ]
  %1218 = icmp ne i8 %1198, 0
  %1219 = or i1 true, %1218
  %1220 = zext i1 %1219 to i8
  %1221 = icmp ne i8 %1198, 0
  br i1 %1221, label %then390, label %else391

then390:                                          ; preds = %ifcont389
  br label %ifcont392

else391:                                          ; preds = %ifcont389
  br label %ifcont392

ifcont392:                                        ; preds = %else391, %then390
  %1222 = phi i32 [ %1201, %then390 ], [ %1192, %else391 ]
  %1223 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1223, align 1
  %1224 = getelementptr i8, ptr %1223, i64 4
  store i32 0, ptr %1224, align 4
  %1225 = load { i32 }, ptr %counter, align 4
  %1226 = load ptr, ptr %__panic1, align 8
  store { i32 } %1225, ptr %ptr_arg393, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg393, ptr %1226)
  %1227 = load ptr, ptr %__panic1, align 8
  %1228 = load i8, ptr %1227, align 1
  %1229 = load ptr, ptr %__panic1, align 8
  %1230 = getelementptr i8, ptr %1229, i64 4
  %1231 = load i32, ptr %1230, align 4
  %1232 = icmp ne i8 %1220, 0
  %1233 = icmp ne i8 %1228, 0
  %1234 = and i1 %1232, %1233
  %1235 = zext i1 %1234 to i8
  %1236 = icmp ne i8 %1235, 0
  br i1 %1236, label %then394, label %else395

then394:                                          ; preds = %ifcont392
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1231)
  br label %ifcont396

else395:                                          ; preds = %ifcont392
  br label %ifcont396

ifcont396:                                        ; preds = %else395, %then394
  %1237 = phi {} [ zeroinitializer, %then394 ], [ zeroinitializer, %else395 ]
  %1238 = icmp ne i8 %1228, 0
  %1239 = xor i1 %1238, true
  %1240 = zext i1 %1239 to i8
  %1241 = icmp ne i8 %1220, 0
  %1242 = icmp ne i8 %1240, 0
  %1243 = and i1 %1241, %1242
  %1244 = zext i1 %1243 to i8
  %1245 = icmp ne i8 %1244, 0
  br i1 %1245, label %then397, label %else398

then397:                                          ; preds = %ifcont396
  %1246 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1246, align 1
  %1247 = load ptr, ptr %__panic1, align 8
  %1248 = getelementptr i8, ptr %1247, i64 4
  store i32 %1222, ptr %1248, align 4
  br label %ifcont399

else398:                                          ; preds = %ifcont396
  br label %ifcont399

ifcont399:                                        ; preds = %else398, %then397
  %1249 = phi {} [ zeroinitializer, %then397 ], [ zeroinitializer, %else398 ]
  %1250 = icmp ne i8 %1220, 0
  %1251 = icmp ne i8 %1228, 0
  %1252 = or i1 %1250, %1251
  %1253 = zext i1 %1252 to i8
  %1254 = icmp ne i8 %1228, 0
  br i1 %1254, label %then400, label %else401

then400:                                          ; preds = %ifcont399
  br label %ifcont402

else401:                                          ; preds = %ifcont399
  br label %ifcont402

ifcont402:                                        ; preds = %else401, %then400
  %1255 = phi i32 [ %1231, %then400 ], [ %1222, %else401 ]
  ret i32 0

panic_cont403:                                    ; preds = %panic_ok381
  br label %ifcont355

then409:                                          ; preds = %else354
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1125)
  br label %ifcont411

else410:                                          ; preds = %else354
  br label %ifcont411

ifcont411:                                        ; preds = %else410, %then409
  %1256 = phi {} [ zeroinitializer, %then409 ], [ zeroinitializer, %else410 ]
  %1257 = icmp ne i8 %1122, 0
  %1258 = xor i1 %1257, true
  %1259 = zext i1 %1258 to i8
  %1260 = icmp ne i8 %1259, 0
  %1261 = and i1 false, %1260
  %1262 = zext i1 %1261 to i8
  %1263 = icmp ne i8 %1262, 0
  br i1 %1263, label %then412, label %else413

then412:                                          ; preds = %ifcont411
  %1264 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1264, align 1
  %1265 = load ptr, ptr %__panic1, align 8
  %1266 = getelementptr i8, ptr %1265, i64 4
  store i32 0, ptr %1266, align 4
  br label %ifcont414

else413:                                          ; preds = %ifcont411
  br label %ifcont414

ifcont414:                                        ; preds = %else413, %then412
  %1267 = phi {} [ zeroinitializer, %then412 ], [ zeroinitializer, %else413 ]
  %1268 = icmp ne i8 %1122, 0
  %1269 = or i1 false, %1268
  %1270 = zext i1 %1269 to i8
  %1271 = icmp ne i8 %1122, 0
  br i1 %1271, label %then415, label %else416

then415:                                          ; preds = %ifcont414
  br label %ifcont417

else416:                                          ; preds = %ifcont414
  br label %ifcont417

ifcont417:                                        ; preds = %else416, %then415
  %1272 = phi i32 [ %1125, %then415 ], [ 0, %else416 ]
  %1273 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1273, align 1
  %1274 = getelementptr i8, ptr %1273, i64 4
  store i32 0, ptr %1274, align 4
  store { i8 } zeroinitializer, ptr %record418, align 1
  store i8 0, ptr %record418, align 1
  %1275 = load { i8 }, ptr %record418, align 1
  %1276 = load ptr, ptr %__panic1, align 8
  store { i8 } %1275, ptr %ptr_arg419, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aOptPtr_x3a_x3aNull(ptr %ptr_arg419, ptr %1276)
  %1277 = load ptr, ptr %__panic1, align 8
  %1278 = load i8, ptr %1277, align 1
  %1279 = load ptr, ptr %__panic1, align 8
  %1280 = getelementptr i8, ptr %1279, i64 4
  %1281 = load i32, ptr %1280, align 4
  %1282 = icmp ne i8 %1270, 0
  %1283 = icmp ne i8 %1278, 0
  %1284 = and i1 %1282, %1283
  %1285 = zext i1 %1284 to i8
  %1286 = icmp ne i8 %1285, 0
  br i1 %1286, label %then420, label %else421

then420:                                          ; preds = %ifcont417
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1281)
  br label %ifcont422

else421:                                          ; preds = %ifcont417
  br label %ifcont422

ifcont422:                                        ; preds = %else421, %then420
  %1287 = phi {} [ zeroinitializer, %then420 ], [ zeroinitializer, %else421 ]
  %1288 = icmp ne i8 %1278, 0
  %1289 = xor i1 %1288, true
  %1290 = zext i1 %1289 to i8
  %1291 = icmp ne i8 %1270, 0
  %1292 = icmp ne i8 %1290, 0
  %1293 = and i1 %1291, %1292
  %1294 = zext i1 %1293 to i8
  %1295 = icmp ne i8 %1294, 0
  br i1 %1295, label %then423, label %else424

then423:                                          ; preds = %ifcont422
  %1296 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1296, align 1
  %1297 = load ptr, ptr %__panic1, align 8
  %1298 = getelementptr i8, ptr %1297, i64 4
  store i32 %1272, ptr %1298, align 4
  br label %ifcont425

else424:                                          ; preds = %ifcont422
  br label %ifcont425

ifcont425:                                        ; preds = %else424, %then423
  %1299 = phi {} [ zeroinitializer, %then423 ], [ zeroinitializer, %else424 ]
  %1300 = icmp ne i8 %1270, 0
  %1301 = icmp ne i8 %1278, 0
  %1302 = or i1 %1300, %1301
  %1303 = zext i1 %1302 to i8
  %1304 = icmp ne i8 %1278, 0
  br i1 %1304, label %then426, label %else427

then426:                                          ; preds = %ifcont425
  br label %ifcont428

else427:                                          ; preds = %ifcont425
  br label %ifcont428

ifcont428:                                        ; preds = %else427, %then426
  %1305 = phi i32 [ %1281, %then426 ], [ %1272, %else427 ]
  %1306 = load ptr, ptr %__panic1, align 8
  %1307 = load i8, ptr %1306, align 1
  %1308 = icmp ne i8 %1307, 0
  br i1 %1308, label %panic_fail430, label %panic_ok429

panic_ok429:                                      ; preds = %ifcont428
  br label %panic_cont451

panic_fail430:                                    ; preds = %ifcont428
  %1309 = load ptr, ptr %__panic1, align 8
  %1310 = load i8, ptr %1309, align 1
  %1311 = load ptr, ptr %__panic1, align 8
  %1312 = getelementptr i8, ptr %1311, i64 4
  %1313 = load i32, ptr %1312, align 4
  %1314 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1314, align 1
  %1315 = getelementptr i8, ptr %1314, i64 4
  store i32 0, ptr %1315, align 4
  %1316 = load { i32 }, ptr %next, align 4
  %1317 = load ptr, ptr %__panic1, align 8
  store { i32 } %1316, ptr %ptr_arg431, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg431, ptr %1317)
  %1318 = load ptr, ptr %__panic1, align 8
  %1319 = load i8, ptr %1318, align 1
  %1320 = load ptr, ptr %__panic1, align 8
  %1321 = getelementptr i8, ptr %1320, i64 4
  %1322 = load i32, ptr %1321, align 4
  %1323 = icmp ne i8 %1319, 0
  %1324 = and i1 true, %1323
  %1325 = zext i1 %1324 to i8
  %1326 = icmp ne i8 %1325, 0
  br i1 %1326, label %then432, label %else433

then432:                                          ; preds = %panic_fail430
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1322)
  br label %ifcont434

else433:                                          ; preds = %panic_fail430
  br label %ifcont434

ifcont434:                                        ; preds = %else433, %then432
  %1327 = phi {} [ zeroinitializer, %then432 ], [ zeroinitializer, %else433 ]
  %1328 = icmp ne i8 %1319, 0
  %1329 = xor i1 %1328, true
  %1330 = zext i1 %1329 to i8
  %1331 = icmp ne i8 %1330, 0
  %1332 = and i1 true, %1331
  %1333 = zext i1 %1332 to i8
  %1334 = icmp ne i8 %1333, 0
  br i1 %1334, label %then435, label %else436

then435:                                          ; preds = %ifcont434
  %1335 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1335, align 1
  %1336 = load ptr, ptr %__panic1, align 8
  %1337 = getelementptr i8, ptr %1336, i64 4
  store i32 %1313, ptr %1337, align 4
  br label %ifcont437

else436:                                          ; preds = %ifcont434
  br label %ifcont437

ifcont437:                                        ; preds = %else436, %then435
  %1338 = phi {} [ zeroinitializer, %then435 ], [ zeroinitializer, %else436 ]
  %1339 = icmp ne i8 %1319, 0
  %1340 = or i1 true, %1339
  %1341 = zext i1 %1340 to i8
  %1342 = icmp ne i8 %1319, 0
  br i1 %1342, label %then438, label %else439

then438:                                          ; preds = %ifcont437
  br label %ifcont440

else439:                                          ; preds = %ifcont437
  br label %ifcont440

ifcont440:                                        ; preds = %else439, %then438
  %1343 = phi i32 [ %1322, %then438 ], [ %1313, %else439 ]
  %1344 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1344, align 1
  %1345 = getelementptr i8, ptr %1344, i64 4
  store i32 0, ptr %1345, align 4
  %1346 = load { i32 }, ptr %counter, align 4
  %1347 = load ptr, ptr %__panic1, align 8
  store { i32 } %1346, ptr %ptr_arg441, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg441, ptr %1347)
  %1348 = load ptr, ptr %__panic1, align 8
  %1349 = load i8, ptr %1348, align 1
  %1350 = load ptr, ptr %__panic1, align 8
  %1351 = getelementptr i8, ptr %1350, i64 4
  %1352 = load i32, ptr %1351, align 4
  %1353 = icmp ne i8 %1341, 0
  %1354 = icmp ne i8 %1349, 0
  %1355 = and i1 %1353, %1354
  %1356 = zext i1 %1355 to i8
  %1357 = icmp ne i8 %1356, 0
  br i1 %1357, label %then442, label %else443

then442:                                          ; preds = %ifcont440
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1352)
  br label %ifcont444

else443:                                          ; preds = %ifcont440
  br label %ifcont444

ifcont444:                                        ; preds = %else443, %then442
  %1358 = phi {} [ zeroinitializer, %then442 ], [ zeroinitializer, %else443 ]
  %1359 = icmp ne i8 %1349, 0
  %1360 = xor i1 %1359, true
  %1361 = zext i1 %1360 to i8
  %1362 = icmp ne i8 %1341, 0
  %1363 = icmp ne i8 %1361, 0
  %1364 = and i1 %1362, %1363
  %1365 = zext i1 %1364 to i8
  %1366 = icmp ne i8 %1365, 0
  br i1 %1366, label %then445, label %else446

then445:                                          ; preds = %ifcont444
  %1367 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1367, align 1
  %1368 = load ptr, ptr %__panic1, align 8
  %1369 = getelementptr i8, ptr %1368, i64 4
  store i32 %1343, ptr %1369, align 4
  br label %ifcont447

else446:                                          ; preds = %ifcont444
  br label %ifcont447

ifcont447:                                        ; preds = %else446, %then445
  %1370 = phi {} [ zeroinitializer, %then445 ], [ zeroinitializer, %else446 ]
  %1371 = icmp ne i8 %1341, 0
  %1372 = icmp ne i8 %1349, 0
  %1373 = or i1 %1371, %1372
  %1374 = zext i1 %1373 to i8
  %1375 = icmp ne i8 %1349, 0
  br i1 %1375, label %then448, label %else449

then448:                                          ; preds = %ifcont447
  br label %ifcont450

else449:                                          ; preds = %ifcont447
  br label %ifcont450

ifcont450:                                        ; preds = %else449, %then448
  %1376 = phi i32 [ %1352, %then448 ], [ %1343, %else449 ]
  ret i32 0

panic_cont451:                                    ; preds = %panic_ok429
  br label %ifcont355

match_merge453:                                   ; preds = %panic_cont507, %panic_cont481
  %1377 = phi { ptr, i64 } [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f5F0E756B894533BC, i64 10 }, %panic_cont481 ], [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f798F97E93DE70729, i64 9 }, %panic_cont507 ]
  store { ptr, i64 } zeroinitializer, ptr %ptr_msg, align 8
  store { ptr, i64 } %1377, ptr %ptr_msg, align 8
  %1378 = load ptr, ptr %__panic1, align 8
  %1379 = load i8, ptr %1378, align 1
  %1380 = icmp ne i8 %1379, 0
  br i1 %1380, label %panic_fail509, label %panic_ok508

match_fail454:                                    ; preds = %match_next456
  unreachable

match_arm455:                                     ; preds = %pat_modal_payload457
  %1381 = load { [16 x i8], [0 x i64] }, ptr %opt, align 8
  store { [16 x i8], [0 x i64] } %1381, ptr %tmp_addr458, align 8
  %1382 = getelementptr i8, ptr %tmp_addr458, i64 8
  %1383 = load ptr, ptr %1382, align 8
  store ptr null, ptr %ptr_val, align 8
  store ptr %1383, ptr %ptr_val, align 8
  %1384 = load ptr, ptr %__panic1, align 8
  %1385 = load i8, ptr %1384, align 1
  %1386 = icmp ne i8 %1385, 0
  br i1 %1386, label %panic_fail460, label %panic_ok459

match_next456:                                    ; preds = %ifcont355
  %1387 = load i8, ptr %match_val452, align 1
  %1388 = icmp eq i8 %1387, 0
  br i1 %1388, label %pat_modal_payload483, label %match_fail454

pat_modal_payload457:                             ; preds = %ifcont355
  %1389 = getelementptr i8, ptr %match_val452, i64 8
  br label %match_arm455

panic_ok459:                                      ; preds = %match_arm455
  br label %panic_cont481

panic_fail460:                                    ; preds = %match_arm455
  %1390 = load ptr, ptr %__panic1, align 8
  %1391 = load i8, ptr %1390, align 1
  %1392 = load ptr, ptr %__panic1, align 8
  %1393 = getelementptr i8, ptr %1392, i64 4
  %1394 = load i32, ptr %1393, align 4
  %1395 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1395, align 1
  %1396 = getelementptr i8, ptr %1395, i64 4
  store i32 0, ptr %1396, align 4
  %1397 = load { i32 }, ptr %next, align 4
  %1398 = load ptr, ptr %__panic1, align 8
  store { i32 } %1397, ptr %ptr_arg461, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg461, ptr %1398)
  %1399 = load ptr, ptr %__panic1, align 8
  %1400 = load i8, ptr %1399, align 1
  %1401 = load ptr, ptr %__panic1, align 8
  %1402 = getelementptr i8, ptr %1401, i64 4
  %1403 = load i32, ptr %1402, align 4
  %1404 = icmp ne i8 %1400, 0
  %1405 = and i1 true, %1404
  %1406 = zext i1 %1405 to i8
  %1407 = icmp ne i8 %1406, 0
  br i1 %1407, label %then462, label %else463

then462:                                          ; preds = %panic_fail460
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1403)
  br label %ifcont464

else463:                                          ; preds = %panic_fail460
  br label %ifcont464

ifcont464:                                        ; preds = %else463, %then462
  %1408 = phi {} [ zeroinitializer, %then462 ], [ zeroinitializer, %else463 ]
  %1409 = icmp ne i8 %1400, 0
  %1410 = xor i1 %1409, true
  %1411 = zext i1 %1410 to i8
  %1412 = icmp ne i8 %1411, 0
  %1413 = and i1 true, %1412
  %1414 = zext i1 %1413 to i8
  %1415 = icmp ne i8 %1414, 0
  br i1 %1415, label %then465, label %else466

then465:                                          ; preds = %ifcont464
  %1416 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1416, align 1
  %1417 = load ptr, ptr %__panic1, align 8
  %1418 = getelementptr i8, ptr %1417, i64 4
  store i32 %1394, ptr %1418, align 4
  br label %ifcont467

else466:                                          ; preds = %ifcont464
  br label %ifcont467

ifcont467:                                        ; preds = %else466, %then465
  %1419 = phi {} [ zeroinitializer, %then465 ], [ zeroinitializer, %else466 ]
  %1420 = icmp ne i8 %1400, 0
  %1421 = or i1 true, %1420
  %1422 = zext i1 %1421 to i8
  %1423 = icmp ne i8 %1400, 0
  br i1 %1423, label %then468, label %else469

then468:                                          ; preds = %ifcont467
  br label %ifcont470

else469:                                          ; preds = %ifcont467
  br label %ifcont470

ifcont470:                                        ; preds = %else469, %then468
  %1424 = phi i32 [ %1403, %then468 ], [ %1394, %else469 ]
  %1425 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1425, align 1
  %1426 = getelementptr i8, ptr %1425, i64 4
  store i32 0, ptr %1426, align 4
  %1427 = load { i32 }, ptr %counter, align 4
  %1428 = load ptr, ptr %__panic1, align 8
  store { i32 } %1427, ptr %ptr_arg471, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg471, ptr %1428)
  %1429 = load ptr, ptr %__panic1, align 8
  %1430 = load i8, ptr %1429, align 1
  %1431 = load ptr, ptr %__panic1, align 8
  %1432 = getelementptr i8, ptr %1431, i64 4
  %1433 = load i32, ptr %1432, align 4
  %1434 = icmp ne i8 %1422, 0
  %1435 = icmp ne i8 %1430, 0
  %1436 = and i1 %1434, %1435
  %1437 = zext i1 %1436 to i8
  %1438 = icmp ne i8 %1437, 0
  br i1 %1438, label %then472, label %else473

then472:                                          ; preds = %ifcont470
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1433)
  br label %ifcont474

else473:                                          ; preds = %ifcont470
  br label %ifcont474

ifcont474:                                        ; preds = %else473, %then472
  %1439 = phi {} [ zeroinitializer, %then472 ], [ zeroinitializer, %else473 ]
  %1440 = icmp ne i8 %1430, 0
  %1441 = xor i1 %1440, true
  %1442 = zext i1 %1441 to i8
  %1443 = icmp ne i8 %1422, 0
  %1444 = icmp ne i8 %1442, 0
  %1445 = and i1 %1443, %1444
  %1446 = zext i1 %1445 to i8
  %1447 = icmp ne i8 %1446, 0
  br i1 %1447, label %then475, label %else476

then475:                                          ; preds = %ifcont474
  %1448 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1448, align 1
  %1449 = load ptr, ptr %__panic1, align 8
  %1450 = getelementptr i8, ptr %1449, i64 4
  store i32 %1424, ptr %1450, align 4
  br label %ifcont477

else476:                                          ; preds = %ifcont474
  br label %ifcont477

ifcont477:                                        ; preds = %else476, %then475
  %1451 = phi {} [ zeroinitializer, %then475 ], [ zeroinitializer, %else476 ]
  %1452 = icmp ne i8 %1422, 0
  %1453 = icmp ne i8 %1430, 0
  %1454 = or i1 %1452, %1453
  %1455 = zext i1 %1454 to i8
  %1456 = icmp ne i8 %1430, 0
  br i1 %1456, label %then478, label %else479

then478:                                          ; preds = %ifcont477
  br label %ifcont480

else479:                                          ; preds = %ifcont477
  br label %ifcont480

ifcont480:                                        ; preds = %else479, %then478
  %1457 = phi i32 [ %1433, %then478 ], [ %1424, %else479 ]
  ret i32 0

panic_cont481:                                    ; preds = %panic_ok459
  br label %match_merge453

match_arm482:                                     ; preds = %pat_modal_payload483
  %1458 = load { [16 x i8], [0 x i64] }, ptr %opt, align 8
  store { [16 x i8], [0 x i64] } %1458, ptr %tmp_addr484, align 8
  %1459 = getelementptr i8, ptr %tmp_addr484, i64 8
  %1460 = load i8, ptr %1459, align 1
  store i8 0, ptr %tag_val, align 1
  store i8 %1460, ptr %tag_val, align 1
  %1461 = load ptr, ptr %__panic1, align 8
  %1462 = load i8, ptr %1461, align 1
  %1463 = icmp ne i8 %1462, 0
  br i1 %1463, label %panic_fail486, label %panic_ok485

pat_modal_payload483:                             ; preds = %match_next456
  %1464 = getelementptr i8, ptr %match_val452, i64 8
  br label %match_arm482

panic_ok485:                                      ; preds = %match_arm482
  br label %panic_cont507

panic_fail486:                                    ; preds = %match_arm482
  %1465 = load ptr, ptr %__panic1, align 8
  %1466 = load i8, ptr %1465, align 1
  %1467 = load ptr, ptr %__panic1, align 8
  %1468 = getelementptr i8, ptr %1467, i64 4
  %1469 = load i32, ptr %1468, align 4
  %1470 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1470, align 1
  %1471 = getelementptr i8, ptr %1470, i64 4
  store i32 0, ptr %1471, align 4
  %1472 = load { i32 }, ptr %next, align 4
  %1473 = load ptr, ptr %__panic1, align 8
  store { i32 } %1472, ptr %ptr_arg487, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg487, ptr %1473)
  %1474 = load ptr, ptr %__panic1, align 8
  %1475 = load i8, ptr %1474, align 1
  %1476 = load ptr, ptr %__panic1, align 8
  %1477 = getelementptr i8, ptr %1476, i64 4
  %1478 = load i32, ptr %1477, align 4
  %1479 = icmp ne i8 %1475, 0
  %1480 = and i1 true, %1479
  %1481 = zext i1 %1480 to i8
  %1482 = icmp ne i8 %1481, 0
  br i1 %1482, label %then488, label %else489

then488:                                          ; preds = %panic_fail486
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1478)
  br label %ifcont490

else489:                                          ; preds = %panic_fail486
  br label %ifcont490

ifcont490:                                        ; preds = %else489, %then488
  %1483 = phi {} [ zeroinitializer, %then488 ], [ zeroinitializer, %else489 ]
  %1484 = icmp ne i8 %1475, 0
  %1485 = xor i1 %1484, true
  %1486 = zext i1 %1485 to i8
  %1487 = icmp ne i8 %1486, 0
  %1488 = and i1 true, %1487
  %1489 = zext i1 %1488 to i8
  %1490 = icmp ne i8 %1489, 0
  br i1 %1490, label %then491, label %else492

then491:                                          ; preds = %ifcont490
  %1491 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1491, align 1
  %1492 = load ptr, ptr %__panic1, align 8
  %1493 = getelementptr i8, ptr %1492, i64 4
  store i32 %1469, ptr %1493, align 4
  br label %ifcont493

else492:                                          ; preds = %ifcont490
  br label %ifcont493

ifcont493:                                        ; preds = %else492, %then491
  %1494 = phi {} [ zeroinitializer, %then491 ], [ zeroinitializer, %else492 ]
  %1495 = icmp ne i8 %1475, 0
  %1496 = or i1 true, %1495
  %1497 = zext i1 %1496 to i8
  %1498 = icmp ne i8 %1475, 0
  br i1 %1498, label %then494, label %else495

then494:                                          ; preds = %ifcont493
  br label %ifcont496

else495:                                          ; preds = %ifcont493
  br label %ifcont496

ifcont496:                                        ; preds = %else495, %then494
  %1499 = phi i32 [ %1478, %then494 ], [ %1469, %else495 ]
  %1500 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1500, align 1
  %1501 = getelementptr i8, ptr %1500, i64 4
  store i32 0, ptr %1501, align 4
  %1502 = load { i32 }, ptr %counter, align 4
  %1503 = load ptr, ptr %__panic1, align 8
  store { i32 } %1502, ptr %ptr_arg497, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg497, ptr %1503)
  %1504 = load ptr, ptr %__panic1, align 8
  %1505 = load i8, ptr %1504, align 1
  %1506 = load ptr, ptr %__panic1, align 8
  %1507 = getelementptr i8, ptr %1506, i64 4
  %1508 = load i32, ptr %1507, align 4
  %1509 = icmp ne i8 %1497, 0
  %1510 = icmp ne i8 %1505, 0
  %1511 = and i1 %1509, %1510
  %1512 = zext i1 %1511 to i8
  %1513 = icmp ne i8 %1512, 0
  br i1 %1513, label %then498, label %else499

then498:                                          ; preds = %ifcont496
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1508)
  br label %ifcont500

else499:                                          ; preds = %ifcont496
  br label %ifcont500

ifcont500:                                        ; preds = %else499, %then498
  %1514 = phi {} [ zeroinitializer, %then498 ], [ zeroinitializer, %else499 ]
  %1515 = icmp ne i8 %1505, 0
  %1516 = xor i1 %1515, true
  %1517 = zext i1 %1516 to i8
  %1518 = icmp ne i8 %1497, 0
  %1519 = icmp ne i8 %1517, 0
  %1520 = and i1 %1518, %1519
  %1521 = zext i1 %1520 to i8
  %1522 = icmp ne i8 %1521, 0
  br i1 %1522, label %then501, label %else502

then501:                                          ; preds = %ifcont500
  %1523 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1523, align 1
  %1524 = load ptr, ptr %__panic1, align 8
  %1525 = getelementptr i8, ptr %1524, i64 4
  store i32 %1499, ptr %1525, align 4
  br label %ifcont503

else502:                                          ; preds = %ifcont500
  br label %ifcont503

ifcont503:                                        ; preds = %else502, %then501
  %1526 = phi {} [ zeroinitializer, %then501 ], [ zeroinitializer, %else502 ]
  %1527 = icmp ne i8 %1497, 0
  %1528 = icmp ne i8 %1505, 0
  %1529 = or i1 %1527, %1528
  %1530 = zext i1 %1529 to i8
  %1531 = icmp ne i8 %1505, 0
  br i1 %1531, label %then504, label %else505

then504:                                          ; preds = %ifcont503
  br label %ifcont506

else505:                                          ; preds = %ifcont503
  br label %ifcont506

ifcont506:                                        ; preds = %else505, %then504
  %1532 = phi i32 [ %1508, %then504 ], [ %1499, %else505 ]
  ret i32 0

panic_cont507:                                    ; preds = %panic_ok485
  br label %match_merge453

panic_ok508:                                      ; preds = %match_merge453
  br label %panic_cont530

panic_fail509:                                    ; preds = %match_merge453
  %1533 = load ptr, ptr %__panic1, align 8
  %1534 = load i8, ptr %1533, align 1
  %1535 = load ptr, ptr %__panic1, align 8
  %1536 = getelementptr i8, ptr %1535, i64 4
  %1537 = load i32, ptr %1536, align 4
  %1538 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1538, align 1
  %1539 = getelementptr i8, ptr %1538, i64 4
  store i32 0, ptr %1539, align 4
  %1540 = load { i32 }, ptr %next, align 4
  %1541 = load ptr, ptr %__panic1, align 8
  store { i32 } %1540, ptr %ptr_arg510, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg510, ptr %1541)
  %1542 = load ptr, ptr %__panic1, align 8
  %1543 = load i8, ptr %1542, align 1
  %1544 = load ptr, ptr %__panic1, align 8
  %1545 = getelementptr i8, ptr %1544, i64 4
  %1546 = load i32, ptr %1545, align 4
  %1547 = icmp ne i8 %1543, 0
  %1548 = and i1 true, %1547
  %1549 = zext i1 %1548 to i8
  %1550 = icmp ne i8 %1549, 0
  br i1 %1550, label %then511, label %else512

then511:                                          ; preds = %panic_fail509
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1546)
  br label %ifcont513

else512:                                          ; preds = %panic_fail509
  br label %ifcont513

ifcont513:                                        ; preds = %else512, %then511
  %1551 = phi {} [ zeroinitializer, %then511 ], [ zeroinitializer, %else512 ]
  %1552 = icmp ne i8 %1543, 0
  %1553 = xor i1 %1552, true
  %1554 = zext i1 %1553 to i8
  %1555 = icmp ne i8 %1554, 0
  %1556 = and i1 true, %1555
  %1557 = zext i1 %1556 to i8
  %1558 = icmp ne i8 %1557, 0
  br i1 %1558, label %then514, label %else515

then514:                                          ; preds = %ifcont513
  %1559 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1559, align 1
  %1560 = load ptr, ptr %__panic1, align 8
  %1561 = getelementptr i8, ptr %1560, i64 4
  store i32 %1537, ptr %1561, align 4
  br label %ifcont516

else515:                                          ; preds = %ifcont513
  br label %ifcont516

ifcont516:                                        ; preds = %else515, %then514
  %1562 = phi {} [ zeroinitializer, %then514 ], [ zeroinitializer, %else515 ]
  %1563 = icmp ne i8 %1543, 0
  %1564 = or i1 true, %1563
  %1565 = zext i1 %1564 to i8
  %1566 = icmp ne i8 %1543, 0
  br i1 %1566, label %then517, label %else518

then517:                                          ; preds = %ifcont516
  br label %ifcont519

else518:                                          ; preds = %ifcont516
  br label %ifcont519

ifcont519:                                        ; preds = %else518, %then517
  %1567 = phi i32 [ %1546, %then517 ], [ %1537, %else518 ]
  %1568 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1568, align 1
  %1569 = getelementptr i8, ptr %1568, i64 4
  store i32 0, ptr %1569, align 4
  %1570 = load { i32 }, ptr %counter, align 4
  %1571 = load ptr, ptr %__panic1, align 8
  store { i32 } %1570, ptr %ptr_arg520, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg520, ptr %1571)
  %1572 = load ptr, ptr %__panic1, align 8
  %1573 = load i8, ptr %1572, align 1
  %1574 = load ptr, ptr %__panic1, align 8
  %1575 = getelementptr i8, ptr %1574, i64 4
  %1576 = load i32, ptr %1575, align 4
  %1577 = icmp ne i8 %1565, 0
  %1578 = icmp ne i8 %1573, 0
  %1579 = and i1 %1577, %1578
  %1580 = zext i1 %1579 to i8
  %1581 = icmp ne i8 %1580, 0
  br i1 %1581, label %then521, label %else522

then521:                                          ; preds = %ifcont519
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1576)
  br label %ifcont523

else522:                                          ; preds = %ifcont519
  br label %ifcont523

ifcont523:                                        ; preds = %else522, %then521
  %1582 = phi {} [ zeroinitializer, %then521 ], [ zeroinitializer, %else522 ]
  %1583 = icmp ne i8 %1573, 0
  %1584 = xor i1 %1583, true
  %1585 = zext i1 %1584 to i8
  %1586 = icmp ne i8 %1565, 0
  %1587 = icmp ne i8 %1585, 0
  %1588 = and i1 %1586, %1587
  %1589 = zext i1 %1588 to i8
  %1590 = icmp ne i8 %1589, 0
  br i1 %1590, label %then524, label %else525

then524:                                          ; preds = %ifcont523
  %1591 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1591, align 1
  %1592 = load ptr, ptr %__panic1, align 8
  %1593 = getelementptr i8, ptr %1592, i64 4
  store i32 %1567, ptr %1593, align 4
  br label %ifcont526

else525:                                          ; preds = %ifcont523
  br label %ifcont526

ifcont526:                                        ; preds = %else525, %then524
  %1594 = phi {} [ zeroinitializer, %then524 ], [ zeroinitializer, %else525 ]
  %1595 = icmp ne i8 %1565, 0
  %1596 = icmp ne i8 %1573, 0
  %1597 = or i1 %1595, %1596
  %1598 = zext i1 %1597 to i8
  %1599 = icmp ne i8 %1573, 0
  br i1 %1599, label %then527, label %else528

then527:                                          ; preds = %ifcont526
  br label %ifcont529

else528:                                          ; preds = %ifcont526
  br label %ifcont529

ifcont529:                                        ; preds = %else528, %then527
  %1600 = phi i32 [ %1576, %then527 ], [ %1567, %else528 ]
  ret i32 0

panic_cont530:                                    ; preds = %panic_ok508
  store i8 0, ptr %region_flag, align 1
  store i8 0, ptr %region_flag, align 1
  %1601 = call { i64 } @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3anew_x5fscoped(ptr null)
  %1602 = load i32, ptr %x, align 4
  %1603 = icmp sgt i32 %1602, 0
  %1604 = zext i1 %1603 to i8
  store i8 %1604, ptr %region_flag, align 1
  %1605 = load ptr, ptr %__panic1, align 8
  %1606 = load i8, ptr %1605, align 1
  %1607 = icmp ne i8 %1606, 0
  br i1 %1607, label %panic_fail532, label %panic_ok531

panic_ok531:                                      ; preds = %panic_cont530
  br label %panic_cont553

panic_fail532:                                    ; preds = %panic_cont530
  %1608 = load ptr, ptr %__panic1, align 8
  %1609 = load i8, ptr %1608, align 1
  %1610 = load ptr, ptr %__panic1, align 8
  %1611 = getelementptr i8, ptr %1610, i64 4
  %1612 = load i32, ptr %1611, align 4
  %1613 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1613, align 1
  %1614 = getelementptr i8, ptr %1613, i64 4
  store i32 0, ptr %1614, align 4
  %1615 = load { i32 }, ptr %next, align 4
  %1616 = load ptr, ptr %__panic1, align 8
  store { i32 } %1615, ptr %ptr_arg533, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg533, ptr %1616)
  %1617 = load ptr, ptr %__panic1, align 8
  %1618 = load i8, ptr %1617, align 1
  %1619 = load ptr, ptr %__panic1, align 8
  %1620 = getelementptr i8, ptr %1619, i64 4
  %1621 = load i32, ptr %1620, align 4
  %1622 = icmp ne i8 %1618, 0
  %1623 = and i1 true, %1622
  %1624 = zext i1 %1623 to i8
  %1625 = icmp ne i8 %1624, 0
  br i1 %1625, label %then534, label %else535

then534:                                          ; preds = %panic_fail532
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1621)
  br label %ifcont536

else535:                                          ; preds = %panic_fail532
  br label %ifcont536

ifcont536:                                        ; preds = %else535, %then534
  %1626 = phi {} [ zeroinitializer, %then534 ], [ zeroinitializer, %else535 ]
  %1627 = icmp ne i8 %1618, 0
  %1628 = xor i1 %1627, true
  %1629 = zext i1 %1628 to i8
  %1630 = icmp ne i8 %1629, 0
  %1631 = and i1 true, %1630
  %1632 = zext i1 %1631 to i8
  %1633 = icmp ne i8 %1632, 0
  br i1 %1633, label %then537, label %else538

then537:                                          ; preds = %ifcont536
  %1634 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1634, align 1
  %1635 = load ptr, ptr %__panic1, align 8
  %1636 = getelementptr i8, ptr %1635, i64 4
  store i32 %1612, ptr %1636, align 4
  br label %ifcont539

else538:                                          ; preds = %ifcont536
  br label %ifcont539

ifcont539:                                        ; preds = %else538, %then537
  %1637 = phi {} [ zeroinitializer, %then537 ], [ zeroinitializer, %else538 ]
  %1638 = icmp ne i8 %1618, 0
  %1639 = or i1 true, %1638
  %1640 = zext i1 %1639 to i8
  %1641 = icmp ne i8 %1618, 0
  br i1 %1641, label %then540, label %else541

then540:                                          ; preds = %ifcont539
  br label %ifcont542

else541:                                          ; preds = %ifcont539
  br label %ifcont542

ifcont542:                                        ; preds = %else541, %then540
  %1642 = phi i32 [ %1621, %then540 ], [ %1612, %else541 ]
  %1643 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1643, align 1
  %1644 = getelementptr i8, ptr %1643, i64 4
  store i32 0, ptr %1644, align 4
  %1645 = load { i32 }, ptr %counter, align 4
  %1646 = load ptr, ptr %__panic1, align 8
  store { i32 } %1645, ptr %ptr_arg543, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg543, ptr %1646)
  %1647 = load ptr, ptr %__panic1, align 8
  %1648 = load i8, ptr %1647, align 1
  %1649 = load ptr, ptr %__panic1, align 8
  %1650 = getelementptr i8, ptr %1649, i64 4
  %1651 = load i32, ptr %1650, align 4
  %1652 = icmp ne i8 %1640, 0
  %1653 = icmp ne i8 %1648, 0
  %1654 = and i1 %1652, %1653
  %1655 = zext i1 %1654 to i8
  %1656 = icmp ne i8 %1655, 0
  br i1 %1656, label %then544, label %else545

then544:                                          ; preds = %ifcont542
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1651)
  br label %ifcont546

else545:                                          ; preds = %ifcont542
  br label %ifcont546

ifcont546:                                        ; preds = %else545, %then544
  %1657 = phi {} [ zeroinitializer, %then544 ], [ zeroinitializer, %else545 ]
  %1658 = icmp ne i8 %1648, 0
  %1659 = xor i1 %1658, true
  %1660 = zext i1 %1659 to i8
  %1661 = icmp ne i8 %1640, 0
  %1662 = icmp ne i8 %1660, 0
  %1663 = and i1 %1661, %1662
  %1664 = zext i1 %1663 to i8
  %1665 = icmp ne i8 %1664, 0
  br i1 %1665, label %then547, label %else548

then547:                                          ; preds = %ifcont546
  %1666 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1666, align 1
  %1667 = load ptr, ptr %__panic1, align 8
  %1668 = getelementptr i8, ptr %1667, i64 4
  store i32 %1642, ptr %1668, align 4
  br label %ifcont549

else548:                                          ; preds = %ifcont546
  br label %ifcont549

ifcont549:                                        ; preds = %else548, %then547
  %1669 = phi {} [ zeroinitializer, %then547 ], [ zeroinitializer, %else548 ]
  %1670 = icmp ne i8 %1640, 0
  %1671 = icmp ne i8 %1648, 0
  %1672 = or i1 %1670, %1671
  %1673 = zext i1 %1672 to i8
  %1674 = icmp ne i8 %1648, 0
  br i1 %1674, label %then550, label %else551

then550:                                          ; preds = %ifcont549
  br label %ifcont552

else551:                                          ; preds = %ifcont549
  br label %ifcont552

ifcont552:                                        ; preds = %else551, %then550
  %1675 = phi i32 [ %1651, %then550 ], [ %1642, %else551 ]
  ret i32 0

panic_cont553:                                    ; preds = %panic_ok531
  store { i64 } %1601, ptr %byref_arg, align 8
  %1676 = call { i64 } @cursive_x3a_x3aruntime_x3a_x3aregion_x3a_x3afree_x5funchecked(ptr %byref_arg)
  %1677 = load ptr, ptr %__panic1, align 8
  %1678 = load i8, ptr %1677, align 1
  %1679 = icmp ne i8 %1678, 0
  br i1 %1679, label %panic_fail555, label %panic_ok554

panic_ok554:                                      ; preds = %panic_cont553
  br label %panic_cont576

panic_fail555:                                    ; preds = %panic_cont553
  %1680 = load ptr, ptr %__panic1, align 8
  %1681 = load i8, ptr %1680, align 1
  %1682 = load ptr, ptr %__panic1, align 8
  %1683 = getelementptr i8, ptr %1682, i64 4
  %1684 = load i32, ptr %1683, align 4
  %1685 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1685, align 1
  %1686 = getelementptr i8, ptr %1685, i64 4
  store i32 0, ptr %1686, align 4
  %1687 = load { i32 }, ptr %next, align 4
  %1688 = load ptr, ptr %__panic1, align 8
  store { i32 } %1687, ptr %ptr_arg556, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg556, ptr %1688)
  %1689 = load ptr, ptr %__panic1, align 8
  %1690 = load i8, ptr %1689, align 1
  %1691 = load ptr, ptr %__panic1, align 8
  %1692 = getelementptr i8, ptr %1691, i64 4
  %1693 = load i32, ptr %1692, align 4
  %1694 = icmp ne i8 %1690, 0
  %1695 = and i1 true, %1694
  %1696 = zext i1 %1695 to i8
  %1697 = icmp ne i8 %1696, 0
  br i1 %1697, label %then557, label %else558

then557:                                          ; preds = %panic_fail555
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1693)
  br label %ifcont559

else558:                                          ; preds = %panic_fail555
  br label %ifcont559

ifcont559:                                        ; preds = %else558, %then557
  %1698 = phi {} [ zeroinitializer, %then557 ], [ zeroinitializer, %else558 ]
  %1699 = icmp ne i8 %1690, 0
  %1700 = xor i1 %1699, true
  %1701 = zext i1 %1700 to i8
  %1702 = icmp ne i8 %1701, 0
  %1703 = and i1 true, %1702
  %1704 = zext i1 %1703 to i8
  %1705 = icmp ne i8 %1704, 0
  br i1 %1705, label %then560, label %else561

then560:                                          ; preds = %ifcont559
  %1706 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1706, align 1
  %1707 = load ptr, ptr %__panic1, align 8
  %1708 = getelementptr i8, ptr %1707, i64 4
  store i32 %1684, ptr %1708, align 4
  br label %ifcont562

else561:                                          ; preds = %ifcont559
  br label %ifcont562

ifcont562:                                        ; preds = %else561, %then560
  %1709 = phi {} [ zeroinitializer, %then560 ], [ zeroinitializer, %else561 ]
  %1710 = icmp ne i8 %1690, 0
  %1711 = or i1 true, %1710
  %1712 = zext i1 %1711 to i8
  %1713 = icmp ne i8 %1690, 0
  br i1 %1713, label %then563, label %else564

then563:                                          ; preds = %ifcont562
  br label %ifcont565

else564:                                          ; preds = %ifcont562
  br label %ifcont565

ifcont565:                                        ; preds = %else564, %then563
  %1714 = phi i32 [ %1693, %then563 ], [ %1684, %else564 ]
  %1715 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1715, align 1
  %1716 = getelementptr i8, ptr %1715, i64 4
  store i32 0, ptr %1716, align 4
  %1717 = load { i32 }, ptr %counter, align 4
  %1718 = load ptr, ptr %__panic1, align 8
  store { i32 } %1717, ptr %ptr_arg566, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg566, ptr %1718)
  %1719 = load ptr, ptr %__panic1, align 8
  %1720 = load i8, ptr %1719, align 1
  %1721 = load ptr, ptr %__panic1, align 8
  %1722 = getelementptr i8, ptr %1721, i64 4
  %1723 = load i32, ptr %1722, align 4
  %1724 = icmp ne i8 %1712, 0
  %1725 = icmp ne i8 %1720, 0
  %1726 = and i1 %1724, %1725
  %1727 = zext i1 %1726 to i8
  %1728 = icmp ne i8 %1727, 0
  br i1 %1728, label %then567, label %else568

then567:                                          ; preds = %ifcont565
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1723)
  br label %ifcont569

else568:                                          ; preds = %ifcont565
  br label %ifcont569

ifcont569:                                        ; preds = %else568, %then567
  %1729 = phi {} [ zeroinitializer, %then567 ], [ zeroinitializer, %else568 ]
  %1730 = icmp ne i8 %1720, 0
  %1731 = xor i1 %1730, true
  %1732 = zext i1 %1731 to i8
  %1733 = icmp ne i8 %1712, 0
  %1734 = icmp ne i8 %1732, 0
  %1735 = and i1 %1733, %1734
  %1736 = zext i1 %1735 to i8
  %1737 = icmp ne i8 %1736, 0
  br i1 %1737, label %then570, label %else571

then570:                                          ; preds = %ifcont569
  %1738 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1738, align 1
  %1739 = load ptr, ptr %__panic1, align 8
  %1740 = getelementptr i8, ptr %1739, i64 4
  store i32 %1714, ptr %1740, align 4
  br label %ifcont572

else571:                                          ; preds = %ifcont569
  br label %ifcont572

ifcont572:                                        ; preds = %else571, %then570
  %1741 = phi {} [ zeroinitializer, %then570 ], [ zeroinitializer, %else571 ]
  %1742 = icmp ne i8 %1712, 0
  %1743 = icmp ne i8 %1720, 0
  %1744 = or i1 %1742, %1743
  %1745 = zext i1 %1744 to i8
  %1746 = icmp ne i8 %1720, 0
  br i1 %1746, label %then573, label %else574

then573:                                          ; preds = %ifcont572
  br label %ifcont575

else574:                                          ; preds = %ifcont572
  br label %ifcont575

ifcont575:                                        ; preds = %else574, %then573
  %1747 = phi i32 [ %1723, %then573 ], [ %1714, %else574 ]
  ret i32 0

panic_cont576:                                    ; preds = %panic_ok554
  %1748 = load i8, ptr %region_flag, align 1
  %1749 = icmp ne i8 %1748, 0
  br i1 %1749, label %then577, label %else578

then577:                                          ; preds = %panic_cont576
  %1750 = load ptr, ptr %__panic1, align 8
  %1751 = load i8, ptr %1750, align 1
  %1752 = icmp ne i8 %1751, 0
  br i1 %1752, label %panic_fail581, label %panic_ok580

else578:                                          ; preds = %panic_cont576
  %1753 = load ptr, ptr %__panic1, align 8
  %1754 = load i8, ptr %1753, align 1
  %1755 = icmp ne i8 %1754, 0
  br i1 %1755, label %panic_fail604, label %panic_ok603

ifcont579:                                        ; preds = %panic_cont625, %panic_cont602
  %1756 = phi { ptr, i64 } [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f7A92DDCF07135D78, i64 10 }, %panic_cont602 ], [ { ptr @cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5fF378FD567236B127, i64 12 }, %panic_cont625 ]
  store { ptr, i64 } zeroinitializer, ptr %region_msg, align 8
  store { ptr, i64 } %1756, ptr %region_msg, align 8
  %1757 = load { ptr, i64 }, ptr %greet_msg, align 8
  store { ptr, i64 } %1757, ptr %byref_arg626, align 8
  %1758 = call i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(ptr %ctx, ptr %byref_arg626)
  store i16 %1758, ptr %match_val627, align 2
  %1759 = load i8, ptr %match_val627, align 1
  %1760 = icmp eq i8 %1759, 0
  br i1 %1760, label %match_arm630, label %match_next631

panic_ok580:                                      ; preds = %then577
  br label %panic_cont602

panic_fail581:                                    ; preds = %then577
  %1761 = load ptr, ptr %__panic1, align 8
  %1762 = load i8, ptr %1761, align 1
  %1763 = load ptr, ptr %__panic1, align 8
  %1764 = getelementptr i8, ptr %1763, i64 4
  %1765 = load i32, ptr %1764, align 4
  %1766 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1766, align 1
  %1767 = getelementptr i8, ptr %1766, i64 4
  store i32 0, ptr %1767, align 4
  %1768 = load { i32 }, ptr %next, align 4
  %1769 = load ptr, ptr %__panic1, align 8
  store { i32 } %1768, ptr %ptr_arg582, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg582, ptr %1769)
  %1770 = load ptr, ptr %__panic1, align 8
  %1771 = load i8, ptr %1770, align 1
  %1772 = load ptr, ptr %__panic1, align 8
  %1773 = getelementptr i8, ptr %1772, i64 4
  %1774 = load i32, ptr %1773, align 4
  %1775 = icmp ne i8 %1771, 0
  %1776 = and i1 true, %1775
  %1777 = zext i1 %1776 to i8
  %1778 = icmp ne i8 %1777, 0
  br i1 %1778, label %then583, label %else584

then583:                                          ; preds = %panic_fail581
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1774)
  br label %ifcont585

else584:                                          ; preds = %panic_fail581
  br label %ifcont585

ifcont585:                                        ; preds = %else584, %then583
  %1779 = phi {} [ zeroinitializer, %then583 ], [ zeroinitializer, %else584 ]
  %1780 = icmp ne i8 %1771, 0
  %1781 = xor i1 %1780, true
  %1782 = zext i1 %1781 to i8
  %1783 = icmp ne i8 %1782, 0
  %1784 = and i1 true, %1783
  %1785 = zext i1 %1784 to i8
  %1786 = icmp ne i8 %1785, 0
  br i1 %1786, label %then586, label %else587

then586:                                          ; preds = %ifcont585
  %1787 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1787, align 1
  %1788 = load ptr, ptr %__panic1, align 8
  %1789 = getelementptr i8, ptr %1788, i64 4
  store i32 %1765, ptr %1789, align 4
  br label %ifcont588

else587:                                          ; preds = %ifcont585
  br label %ifcont588

ifcont588:                                        ; preds = %else587, %then586
  %1790 = phi {} [ zeroinitializer, %then586 ], [ zeroinitializer, %else587 ]
  %1791 = icmp ne i8 %1771, 0
  %1792 = or i1 true, %1791
  %1793 = zext i1 %1792 to i8
  %1794 = icmp ne i8 %1771, 0
  br i1 %1794, label %then589, label %else590

then589:                                          ; preds = %ifcont588
  br label %ifcont591

else590:                                          ; preds = %ifcont588
  br label %ifcont591

ifcont591:                                        ; preds = %else590, %then589
  %1795 = phi i32 [ %1774, %then589 ], [ %1765, %else590 ]
  %1796 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1796, align 1
  %1797 = getelementptr i8, ptr %1796, i64 4
  store i32 0, ptr %1797, align 4
  %1798 = load { i32 }, ptr %counter, align 4
  %1799 = load ptr, ptr %__panic1, align 8
  store { i32 } %1798, ptr %ptr_arg592, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg592, ptr %1799)
  %1800 = load ptr, ptr %__panic1, align 8
  %1801 = load i8, ptr %1800, align 1
  %1802 = load ptr, ptr %__panic1, align 8
  %1803 = getelementptr i8, ptr %1802, i64 4
  %1804 = load i32, ptr %1803, align 4
  %1805 = icmp ne i8 %1793, 0
  %1806 = icmp ne i8 %1801, 0
  %1807 = and i1 %1805, %1806
  %1808 = zext i1 %1807 to i8
  %1809 = icmp ne i8 %1808, 0
  br i1 %1809, label %then593, label %else594

then593:                                          ; preds = %ifcont591
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1804)
  br label %ifcont595

else594:                                          ; preds = %ifcont591
  br label %ifcont595

ifcont595:                                        ; preds = %else594, %then593
  %1810 = phi {} [ zeroinitializer, %then593 ], [ zeroinitializer, %else594 ]
  %1811 = icmp ne i8 %1801, 0
  %1812 = xor i1 %1811, true
  %1813 = zext i1 %1812 to i8
  %1814 = icmp ne i8 %1793, 0
  %1815 = icmp ne i8 %1813, 0
  %1816 = and i1 %1814, %1815
  %1817 = zext i1 %1816 to i8
  %1818 = icmp ne i8 %1817, 0
  br i1 %1818, label %then596, label %else597

then596:                                          ; preds = %ifcont595
  %1819 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1819, align 1
  %1820 = load ptr, ptr %__panic1, align 8
  %1821 = getelementptr i8, ptr %1820, i64 4
  store i32 %1795, ptr %1821, align 4
  br label %ifcont598

else597:                                          ; preds = %ifcont595
  br label %ifcont598

ifcont598:                                        ; preds = %else597, %then596
  %1822 = phi {} [ zeroinitializer, %then596 ], [ zeroinitializer, %else597 ]
  %1823 = icmp ne i8 %1793, 0
  %1824 = icmp ne i8 %1801, 0
  %1825 = or i1 %1823, %1824
  %1826 = zext i1 %1825 to i8
  %1827 = icmp ne i8 %1801, 0
  br i1 %1827, label %then599, label %else600

then599:                                          ; preds = %ifcont598
  br label %ifcont601

else600:                                          ; preds = %ifcont598
  br label %ifcont601

ifcont601:                                        ; preds = %else600, %then599
  %1828 = phi i32 [ %1804, %then599 ], [ %1795, %else600 ]
  ret i32 0

panic_cont602:                                    ; preds = %panic_ok580
  br label %ifcont579

panic_ok603:                                      ; preds = %else578
  br label %panic_cont625

panic_fail604:                                    ; preds = %else578
  %1829 = load ptr, ptr %__panic1, align 8
  %1830 = load i8, ptr %1829, align 1
  %1831 = load ptr, ptr %__panic1, align 8
  %1832 = getelementptr i8, ptr %1831, i64 4
  %1833 = load i32, ptr %1832, align 4
  %1834 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1834, align 1
  %1835 = getelementptr i8, ptr %1834, i64 4
  store i32 0, ptr %1835, align 4
  %1836 = load { i32 }, ptr %next, align 4
  %1837 = load ptr, ptr %__panic1, align 8
  store { i32 } %1836, ptr %ptr_arg605, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg605, ptr %1837)
  %1838 = load ptr, ptr %__panic1, align 8
  %1839 = load i8, ptr %1838, align 1
  %1840 = load ptr, ptr %__panic1, align 8
  %1841 = getelementptr i8, ptr %1840, i64 4
  %1842 = load i32, ptr %1841, align 4
  %1843 = icmp ne i8 %1839, 0
  %1844 = and i1 true, %1843
  %1845 = zext i1 %1844 to i8
  %1846 = icmp ne i8 %1845, 0
  br i1 %1846, label %then606, label %else607

then606:                                          ; preds = %panic_fail604
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1842)
  br label %ifcont608

else607:                                          ; preds = %panic_fail604
  br label %ifcont608

ifcont608:                                        ; preds = %else607, %then606
  %1847 = phi {} [ zeroinitializer, %then606 ], [ zeroinitializer, %else607 ]
  %1848 = icmp ne i8 %1839, 0
  %1849 = xor i1 %1848, true
  %1850 = zext i1 %1849 to i8
  %1851 = icmp ne i8 %1850, 0
  %1852 = and i1 true, %1851
  %1853 = zext i1 %1852 to i8
  %1854 = icmp ne i8 %1853, 0
  br i1 %1854, label %then609, label %else610

then609:                                          ; preds = %ifcont608
  %1855 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1855, align 1
  %1856 = load ptr, ptr %__panic1, align 8
  %1857 = getelementptr i8, ptr %1856, i64 4
  store i32 %1833, ptr %1857, align 4
  br label %ifcont611

else610:                                          ; preds = %ifcont608
  br label %ifcont611

ifcont611:                                        ; preds = %else610, %then609
  %1858 = phi {} [ zeroinitializer, %then609 ], [ zeroinitializer, %else610 ]
  %1859 = icmp ne i8 %1839, 0
  %1860 = or i1 true, %1859
  %1861 = zext i1 %1860 to i8
  %1862 = icmp ne i8 %1839, 0
  br i1 %1862, label %then612, label %else613

then612:                                          ; preds = %ifcont611
  br label %ifcont614

else613:                                          ; preds = %ifcont611
  br label %ifcont614

ifcont614:                                        ; preds = %else613, %then612
  %1863 = phi i32 [ %1842, %then612 ], [ %1833, %else613 ]
  %1864 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1864, align 1
  %1865 = getelementptr i8, ptr %1864, i64 4
  store i32 0, ptr %1865, align 4
  %1866 = load { i32 }, ptr %counter, align 4
  %1867 = load ptr, ptr %__panic1, align 8
  store { i32 } %1866, ptr %ptr_arg615, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg615, ptr %1867)
  %1868 = load ptr, ptr %__panic1, align 8
  %1869 = load i8, ptr %1868, align 1
  %1870 = load ptr, ptr %__panic1, align 8
  %1871 = getelementptr i8, ptr %1870, i64 4
  %1872 = load i32, ptr %1871, align 4
  %1873 = icmp ne i8 %1861, 0
  %1874 = icmp ne i8 %1869, 0
  %1875 = and i1 %1873, %1874
  %1876 = zext i1 %1875 to i8
  %1877 = icmp ne i8 %1876, 0
  br i1 %1877, label %then616, label %else617

then616:                                          ; preds = %ifcont614
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1872)
  br label %ifcont618

else617:                                          ; preds = %ifcont614
  br label %ifcont618

ifcont618:                                        ; preds = %else617, %then616
  %1878 = phi {} [ zeroinitializer, %then616 ], [ zeroinitializer, %else617 ]
  %1879 = icmp ne i8 %1869, 0
  %1880 = xor i1 %1879, true
  %1881 = zext i1 %1880 to i8
  %1882 = icmp ne i8 %1861, 0
  %1883 = icmp ne i8 %1881, 0
  %1884 = and i1 %1882, %1883
  %1885 = zext i1 %1884 to i8
  %1886 = icmp ne i8 %1885, 0
  br i1 %1886, label %then619, label %else620

then619:                                          ; preds = %ifcont618
  %1887 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1887, align 1
  %1888 = load ptr, ptr %__panic1, align 8
  %1889 = getelementptr i8, ptr %1888, i64 4
  store i32 %1863, ptr %1889, align 4
  br label %ifcont621

else620:                                          ; preds = %ifcont618
  br label %ifcont621

ifcont621:                                        ; preds = %else620, %then619
  %1890 = phi {} [ zeroinitializer, %then619 ], [ zeroinitializer, %else620 ]
  %1891 = icmp ne i8 %1861, 0
  %1892 = icmp ne i8 %1869, 0
  %1893 = or i1 %1891, %1892
  %1894 = zext i1 %1893 to i8
  %1895 = icmp ne i8 %1869, 0
  br i1 %1895, label %then622, label %else623

then622:                                          ; preds = %ifcont621
  br label %ifcont624

else623:                                          ; preds = %ifcont621
  br label %ifcont624

ifcont624:                                        ; preds = %else623, %then622
  %1896 = phi i32 [ %1872, %then622 ], [ %1863, %else623 ]
  ret i32 0

panic_cont625:                                    ; preds = %panic_ok603
  br label %ifcont579

match_merge628:                                   ; preds = %panic_cont690, %panic_cont655
  %1897 = phi {} [ zeroinitializer, %panic_cont655 ], [ zeroinitializer, %panic_cont690 ]
  %1898 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1898, align 1
  %1899 = getelementptr i8, ptr %1898, i64 4
  store i32 0, ptr %1899, align 4
  store i16 %1758, ptr %match_val691, align 2
  %1900 = load i8, ptr %match_val691, align 1
  %1901 = icmp eq i8 %1900, 0
  br i1 %1901, label %match_arm694, label %match_next695

match_fail629:                                    ; preds = %match_next631
  unreachable

match_arm630:                                     ; preds = %ifcont579
  store i16 %1758, ptr %tmp_addr632, align 2
  store {} zeroinitializer, ptr %ok, align 1
  store {} zeroinitializer, ptr %ok, align 1
  %1902 = load ptr, ptr %__panic1, align 8
  %1903 = load i8, ptr %1902, align 1
  %1904 = icmp ne i8 %1903, 0
  br i1 %1904, label %panic_fail634, label %panic_ok633

match_next631:                                    ; preds = %ifcont579
  %1905 = load i8, ptr %match_val627, align 1
  %1906 = icmp eq i8 %1905, 1
  br i1 %1906, label %match_arm656, label %match_fail629

panic_ok633:                                      ; preds = %match_arm630
  br label %panic_cont655

panic_fail634:                                    ; preds = %match_arm630
  %1907 = load ptr, ptr %__panic1, align 8
  %1908 = load i8, ptr %1907, align 1
  %1909 = load ptr, ptr %__panic1, align 8
  %1910 = getelementptr i8, ptr %1909, i64 4
  %1911 = load i32, ptr %1910, align 4
  %1912 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1912, align 1
  %1913 = getelementptr i8, ptr %1912, i64 4
  store i32 0, ptr %1913, align 4
  %1914 = load { i32 }, ptr %next, align 4
  %1915 = load ptr, ptr %__panic1, align 8
  store { i32 } %1914, ptr %ptr_arg635, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg635, ptr %1915)
  %1916 = load ptr, ptr %__panic1, align 8
  %1917 = load i8, ptr %1916, align 1
  %1918 = load ptr, ptr %__panic1, align 8
  %1919 = getelementptr i8, ptr %1918, i64 4
  %1920 = load i32, ptr %1919, align 4
  %1921 = icmp ne i8 %1917, 0
  %1922 = and i1 true, %1921
  %1923 = zext i1 %1922 to i8
  %1924 = icmp ne i8 %1923, 0
  br i1 %1924, label %then636, label %else637

then636:                                          ; preds = %panic_fail634
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1920)
  br label %ifcont638

else637:                                          ; preds = %panic_fail634
  br label %ifcont638

ifcont638:                                        ; preds = %else637, %then636
  %1925 = phi {} [ zeroinitializer, %then636 ], [ zeroinitializer, %else637 ]
  %1926 = icmp ne i8 %1917, 0
  %1927 = xor i1 %1926, true
  %1928 = zext i1 %1927 to i8
  %1929 = icmp ne i8 %1928, 0
  %1930 = and i1 true, %1929
  %1931 = zext i1 %1930 to i8
  %1932 = icmp ne i8 %1931, 0
  br i1 %1932, label %then639, label %else640

then639:                                          ; preds = %ifcont638
  %1933 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1933, align 1
  %1934 = load ptr, ptr %__panic1, align 8
  %1935 = getelementptr i8, ptr %1934, i64 4
  store i32 %1911, ptr %1935, align 4
  br label %ifcont641

else640:                                          ; preds = %ifcont638
  br label %ifcont641

ifcont641:                                        ; preds = %else640, %then639
  %1936 = phi {} [ zeroinitializer, %then639 ], [ zeroinitializer, %else640 ]
  %1937 = icmp ne i8 %1917, 0
  %1938 = or i1 true, %1937
  %1939 = zext i1 %1938 to i8
  %1940 = icmp ne i8 %1917, 0
  br i1 %1940, label %then642, label %else643

then642:                                          ; preds = %ifcont641
  br label %ifcont644

else643:                                          ; preds = %ifcont641
  br label %ifcont644

ifcont644:                                        ; preds = %else643, %then642
  %1941 = phi i32 [ %1920, %then642 ], [ %1911, %else643 ]
  %1942 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1942, align 1
  %1943 = getelementptr i8, ptr %1942, i64 4
  store i32 0, ptr %1943, align 4
  %1944 = load { i32 }, ptr %counter, align 4
  %1945 = load ptr, ptr %__panic1, align 8
  store { i32 } %1944, ptr %ptr_arg645, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg645, ptr %1945)
  %1946 = load ptr, ptr %__panic1, align 8
  %1947 = load i8, ptr %1946, align 1
  %1948 = load ptr, ptr %__panic1, align 8
  %1949 = getelementptr i8, ptr %1948, i64 4
  %1950 = load i32, ptr %1949, align 4
  %1951 = icmp ne i8 %1939, 0
  %1952 = icmp ne i8 %1947, 0
  %1953 = and i1 %1951, %1952
  %1954 = zext i1 %1953 to i8
  %1955 = icmp ne i8 %1954, 0
  br i1 %1955, label %then646, label %else647

then646:                                          ; preds = %ifcont644
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1950)
  br label %ifcont648

else647:                                          ; preds = %ifcont644
  br label %ifcont648

ifcont648:                                        ; preds = %else647, %then646
  %1956 = phi {} [ zeroinitializer, %then646 ], [ zeroinitializer, %else647 ]
  %1957 = icmp ne i8 %1947, 0
  %1958 = xor i1 %1957, true
  %1959 = zext i1 %1958 to i8
  %1960 = icmp ne i8 %1939, 0
  %1961 = icmp ne i8 %1959, 0
  %1962 = and i1 %1960, %1961
  %1963 = zext i1 %1962 to i8
  %1964 = icmp ne i8 %1963, 0
  br i1 %1964, label %then649, label %else650

then649:                                          ; preds = %ifcont648
  %1965 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1965, align 1
  %1966 = load ptr, ptr %__panic1, align 8
  %1967 = getelementptr i8, ptr %1966, i64 4
  store i32 %1941, ptr %1967, align 4
  br label %ifcont651

else650:                                          ; preds = %ifcont648
  br label %ifcont651

ifcont651:                                        ; preds = %else650, %then649
  %1968 = phi {} [ zeroinitializer, %then649 ], [ zeroinitializer, %else650 ]
  %1969 = icmp ne i8 %1939, 0
  %1970 = icmp ne i8 %1947, 0
  %1971 = or i1 %1969, %1970
  %1972 = zext i1 %1971 to i8
  %1973 = icmp ne i8 %1947, 0
  br i1 %1973, label %then652, label %else653

then652:                                          ; preds = %ifcont651
  br label %ifcont654

else653:                                          ; preds = %ifcont651
  br label %ifcont654

ifcont654:                                        ; preds = %else653, %then652
  %1974 = phi i32 [ %1950, %then652 ], [ %1941, %else653 ]
  ret i32 0

panic_cont655:                                    ; preds = %panic_ok633
  br label %match_merge628

match_arm656:                                     ; preds = %match_next631
  store i16 %1758, ptr %tmp_addr657, align 2
  %1975 = getelementptr i8, ptr %tmp_addr657, i64 1
  %1976 = load i8, ptr %1975, align 1
  store i8 0, ptr %err, align 1
  store i8 %1976, ptr %err, align 1
  %1977 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %1977, align 1
  %1978 = getelementptr i8, ptr %1977, i64 4
  store i32 0, ptr %1978, align 4
  %1979 = load i8, ptr %err, align 1
  %1980 = load ptr, ptr %__panic1, align 8
  store i8 %1979, ptr %ptr_arg658, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg658, ptr %1980)
  %1981 = load ptr, ptr %__panic1, align 8
  %1982 = load i8, ptr %1981, align 1
  %1983 = load ptr, ptr %__panic1, align 8
  %1984 = getelementptr i8, ptr %1983, i64 4
  %1985 = load i32, ptr %1984, align 4
  %1986 = icmp ne i8 %1982, 0
  %1987 = and i1 false, %1986
  %1988 = zext i1 %1987 to i8
  %1989 = icmp ne i8 %1988, 0
  br i1 %1989, label %then659, label %else660

then659:                                          ; preds = %match_arm656
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %1985)
  br label %ifcont661

else660:                                          ; preds = %match_arm656
  br label %ifcont661

ifcont661:                                        ; preds = %else660, %then659
  %1990 = phi {} [ zeroinitializer, %then659 ], [ zeroinitializer, %else660 ]
  %1991 = icmp ne i8 %1982, 0
  %1992 = xor i1 %1991, true
  %1993 = zext i1 %1992 to i8
  %1994 = icmp ne i8 %1993, 0
  %1995 = and i1 false, %1994
  %1996 = zext i1 %1995 to i8
  %1997 = icmp ne i8 %1996, 0
  br i1 %1997, label %then662, label %else663

then662:                                          ; preds = %ifcont661
  %1998 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %1998, align 1
  %1999 = load ptr, ptr %__panic1, align 8
  %2000 = getelementptr i8, ptr %1999, i64 4
  store i32 0, ptr %2000, align 4
  br label %ifcont664

else663:                                          ; preds = %ifcont661
  br label %ifcont664

ifcont664:                                        ; preds = %else663, %then662
  %2001 = phi {} [ zeroinitializer, %then662 ], [ zeroinitializer, %else663 ]
  %2002 = icmp ne i8 %1982, 0
  %2003 = or i1 false, %2002
  %2004 = zext i1 %2003 to i8
  %2005 = icmp ne i8 %1982, 0
  br i1 %2005, label %then665, label %else666

then665:                                          ; preds = %ifcont664
  br label %ifcont667

else666:                                          ; preds = %ifcont664
  br label %ifcont667

ifcont667:                                        ; preds = %else666, %then665
  %2006 = phi i32 [ %1985, %then665 ], [ 0, %else666 ]
  %2007 = load ptr, ptr %__panic1, align 8
  %2008 = load i8, ptr %2007, align 1
  %2009 = icmp ne i8 %2008, 0
  br i1 %2009, label %panic_fail669, label %panic_ok668

panic_ok668:                                      ; preds = %ifcont667
  br label %panic_cont690

panic_fail669:                                    ; preds = %ifcont667
  %2010 = load ptr, ptr %__panic1, align 8
  %2011 = load i8, ptr %2010, align 1
  %2012 = load ptr, ptr %__panic1, align 8
  %2013 = getelementptr i8, ptr %2012, i64 4
  %2014 = load i32, ptr %2013, align 4
  %2015 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2015, align 1
  %2016 = getelementptr i8, ptr %2015, i64 4
  store i32 0, ptr %2016, align 4
  %2017 = load { i32 }, ptr %next, align 4
  %2018 = load ptr, ptr %__panic1, align 8
  store { i32 } %2017, ptr %ptr_arg670, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg670, ptr %2018)
  %2019 = load ptr, ptr %__panic1, align 8
  %2020 = load i8, ptr %2019, align 1
  %2021 = load ptr, ptr %__panic1, align 8
  %2022 = getelementptr i8, ptr %2021, i64 4
  %2023 = load i32, ptr %2022, align 4
  %2024 = icmp ne i8 %2020, 0
  %2025 = and i1 true, %2024
  %2026 = zext i1 %2025 to i8
  %2027 = icmp ne i8 %2026, 0
  br i1 %2027, label %then671, label %else672

then671:                                          ; preds = %panic_fail669
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2023)
  br label %ifcont673

else672:                                          ; preds = %panic_fail669
  br label %ifcont673

ifcont673:                                        ; preds = %else672, %then671
  %2028 = phi {} [ zeroinitializer, %then671 ], [ zeroinitializer, %else672 ]
  %2029 = icmp ne i8 %2020, 0
  %2030 = xor i1 %2029, true
  %2031 = zext i1 %2030 to i8
  %2032 = icmp ne i8 %2031, 0
  %2033 = and i1 true, %2032
  %2034 = zext i1 %2033 to i8
  %2035 = icmp ne i8 %2034, 0
  br i1 %2035, label %then674, label %else675

then674:                                          ; preds = %ifcont673
  %2036 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2036, align 1
  %2037 = load ptr, ptr %__panic1, align 8
  %2038 = getelementptr i8, ptr %2037, i64 4
  store i32 %2014, ptr %2038, align 4
  br label %ifcont676

else675:                                          ; preds = %ifcont673
  br label %ifcont676

ifcont676:                                        ; preds = %else675, %then674
  %2039 = phi {} [ zeroinitializer, %then674 ], [ zeroinitializer, %else675 ]
  %2040 = icmp ne i8 %2020, 0
  %2041 = or i1 true, %2040
  %2042 = zext i1 %2041 to i8
  %2043 = icmp ne i8 %2020, 0
  br i1 %2043, label %then677, label %else678

then677:                                          ; preds = %ifcont676
  br label %ifcont679

else678:                                          ; preds = %ifcont676
  br label %ifcont679

ifcont679:                                        ; preds = %else678, %then677
  %2044 = phi i32 [ %2023, %then677 ], [ %2014, %else678 ]
  %2045 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2045, align 1
  %2046 = getelementptr i8, ptr %2045, i64 4
  store i32 0, ptr %2046, align 4
  %2047 = load { i32 }, ptr %counter, align 4
  %2048 = load ptr, ptr %__panic1, align 8
  store { i32 } %2047, ptr %ptr_arg680, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg680, ptr %2048)
  %2049 = load ptr, ptr %__panic1, align 8
  %2050 = load i8, ptr %2049, align 1
  %2051 = load ptr, ptr %__panic1, align 8
  %2052 = getelementptr i8, ptr %2051, i64 4
  %2053 = load i32, ptr %2052, align 4
  %2054 = icmp ne i8 %2042, 0
  %2055 = icmp ne i8 %2050, 0
  %2056 = and i1 %2054, %2055
  %2057 = zext i1 %2056 to i8
  %2058 = icmp ne i8 %2057, 0
  br i1 %2058, label %then681, label %else682

then681:                                          ; preds = %ifcont679
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2053)
  br label %ifcont683

else682:                                          ; preds = %ifcont679
  br label %ifcont683

ifcont683:                                        ; preds = %else682, %then681
  %2059 = phi {} [ zeroinitializer, %then681 ], [ zeroinitializer, %else682 ]
  %2060 = icmp ne i8 %2050, 0
  %2061 = xor i1 %2060, true
  %2062 = zext i1 %2061 to i8
  %2063 = icmp ne i8 %2042, 0
  %2064 = icmp ne i8 %2062, 0
  %2065 = and i1 %2063, %2064
  %2066 = zext i1 %2065 to i8
  %2067 = icmp ne i8 %2066, 0
  br i1 %2067, label %then684, label %else685

then684:                                          ; preds = %ifcont683
  %2068 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2068, align 1
  %2069 = load ptr, ptr %__panic1, align 8
  %2070 = getelementptr i8, ptr %2069, i64 4
  store i32 %2044, ptr %2070, align 4
  br label %ifcont686

else685:                                          ; preds = %ifcont683
  br label %ifcont686

ifcont686:                                        ; preds = %else685, %then684
  %2071 = phi {} [ zeroinitializer, %then684 ], [ zeroinitializer, %else685 ]
  %2072 = icmp ne i8 %2042, 0
  %2073 = icmp ne i8 %2050, 0
  %2074 = or i1 %2072, %2073
  %2075 = zext i1 %2074 to i8
  %2076 = icmp ne i8 %2050, 0
  br i1 %2076, label %then687, label %else688

then687:                                          ; preds = %ifcont686
  br label %ifcont689

else688:                                          ; preds = %ifcont686
  br label %ifcont689

ifcont689:                                        ; preds = %else688, %then687
  %2077 = phi i32 [ %2053, %then687 ], [ %2044, %else688 ]
  ret i32 0

panic_cont690:                                    ; preds = %panic_ok668
  br label %match_merge628

match_merge692:                                   ; preds = %match_arm696, %match_arm694
  %2078 = phi {} [ zeroinitializer, %match_arm694 ], [ zeroinitializer, %match_arm696 ]
  %2079 = load ptr, ptr %__panic1, align 8
  %2080 = load i8, ptr %2079, align 1
  %2081 = load ptr, ptr %__panic1, align 8
  %2082 = getelementptr i8, ptr %2081, i64 4
  %2083 = load i32, ptr %2082, align 4
  %2084 = icmp ne i8 %2080, 0
  %2085 = and i1 false, %2084
  %2086 = zext i1 %2085 to i8
  %2087 = icmp ne i8 %2086, 0
  br i1 %2087, label %then699, label %else700

match_fail693:                                    ; preds = %match_next695
  unreachable

match_arm694:                                     ; preds = %match_merge628
  br label %match_merge692

match_next695:                                    ; preds = %match_merge628
  %2088 = load i8, ptr %match_val691, align 1
  %2089 = icmp eq i8 %2088, 1
  br i1 %2089, label %match_arm696, label %match_fail693

match_arm696:                                     ; preds = %match_next695
  store i16 %1758, ptr %tmp_addr697, align 2
  %2090 = getelementptr i8, ptr %tmp_addr697, i64 1
  %2091 = load i8, ptr %2090, align 1
  %2092 = load ptr, ptr %__panic1, align 8
  store i8 %2091, ptr %ptr_arg698, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg698, ptr %2092)
  br label %match_merge692

then699:                                          ; preds = %match_merge692
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2083)
  br label %ifcont701

else700:                                          ; preds = %match_merge692
  br label %ifcont701

ifcont701:                                        ; preds = %else700, %then699
  %2093 = phi {} [ zeroinitializer, %then699 ], [ zeroinitializer, %else700 ]
  %2094 = icmp ne i8 %2080, 0
  %2095 = xor i1 %2094, true
  %2096 = zext i1 %2095 to i8
  %2097 = icmp ne i8 %2096, 0
  %2098 = and i1 false, %2097
  %2099 = zext i1 %2098 to i8
  %2100 = icmp ne i8 %2099, 0
  br i1 %2100, label %then702, label %else703

then702:                                          ; preds = %ifcont701
  %2101 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2101, align 1
  %2102 = load ptr, ptr %__panic1, align 8
  %2103 = getelementptr i8, ptr %2102, i64 4
  store i32 0, ptr %2103, align 4
  br label %ifcont704

else703:                                          ; preds = %ifcont701
  br label %ifcont704

ifcont704:                                        ; preds = %else703, %then702
  %2104 = phi {} [ zeroinitializer, %then702 ], [ zeroinitializer, %else703 ]
  %2105 = icmp ne i8 %2080, 0
  %2106 = or i1 false, %2105
  %2107 = zext i1 %2106 to i8
  %2108 = icmp ne i8 %2080, 0
  br i1 %2108, label %then705, label %else706

then705:                                          ; preds = %ifcont704
  br label %ifcont707

else706:                                          ; preds = %ifcont704
  br label %ifcont707

ifcont707:                                        ; preds = %else706, %then705
  %2109 = phi i32 [ %2083, %then705 ], [ 0, %else706 ]
  %2110 = load ptr, ptr %__panic1, align 8
  %2111 = load i8, ptr %2110, align 1
  %2112 = icmp ne i8 %2111, 0
  br i1 %2112, label %panic_fail709, label %panic_ok708

panic_ok708:                                      ; preds = %ifcont707
  br label %panic_cont730

panic_fail709:                                    ; preds = %ifcont707
  %2113 = load ptr, ptr %__panic1, align 8
  %2114 = load i8, ptr %2113, align 1
  %2115 = load ptr, ptr %__panic1, align 8
  %2116 = getelementptr i8, ptr %2115, i64 4
  %2117 = load i32, ptr %2116, align 4
  %2118 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2118, align 1
  %2119 = getelementptr i8, ptr %2118, i64 4
  store i32 0, ptr %2119, align 4
  %2120 = load { i32 }, ptr %next, align 4
  %2121 = load ptr, ptr %__panic1, align 8
  store { i32 } %2120, ptr %ptr_arg710, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg710, ptr %2121)
  %2122 = load ptr, ptr %__panic1, align 8
  %2123 = load i8, ptr %2122, align 1
  %2124 = load ptr, ptr %__panic1, align 8
  %2125 = getelementptr i8, ptr %2124, i64 4
  %2126 = load i32, ptr %2125, align 4
  %2127 = icmp ne i8 %2123, 0
  %2128 = and i1 true, %2127
  %2129 = zext i1 %2128 to i8
  %2130 = icmp ne i8 %2129, 0
  br i1 %2130, label %then711, label %else712

then711:                                          ; preds = %panic_fail709
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2126)
  br label %ifcont713

else712:                                          ; preds = %panic_fail709
  br label %ifcont713

ifcont713:                                        ; preds = %else712, %then711
  %2131 = phi {} [ zeroinitializer, %then711 ], [ zeroinitializer, %else712 ]
  %2132 = icmp ne i8 %2123, 0
  %2133 = xor i1 %2132, true
  %2134 = zext i1 %2133 to i8
  %2135 = icmp ne i8 %2134, 0
  %2136 = and i1 true, %2135
  %2137 = zext i1 %2136 to i8
  %2138 = icmp ne i8 %2137, 0
  br i1 %2138, label %then714, label %else715

then714:                                          ; preds = %ifcont713
  %2139 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2139, align 1
  %2140 = load ptr, ptr %__panic1, align 8
  %2141 = getelementptr i8, ptr %2140, i64 4
  store i32 %2117, ptr %2141, align 4
  br label %ifcont716

else715:                                          ; preds = %ifcont713
  br label %ifcont716

ifcont716:                                        ; preds = %else715, %then714
  %2142 = phi {} [ zeroinitializer, %then714 ], [ zeroinitializer, %else715 ]
  %2143 = icmp ne i8 %2123, 0
  %2144 = or i1 true, %2143
  %2145 = zext i1 %2144 to i8
  %2146 = icmp ne i8 %2123, 0
  br i1 %2146, label %then717, label %else718

then717:                                          ; preds = %ifcont716
  br label %ifcont719

else718:                                          ; preds = %ifcont716
  br label %ifcont719

ifcont719:                                        ; preds = %else718, %then717
  %2147 = phi i32 [ %2126, %then717 ], [ %2117, %else718 ]
  %2148 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2148, align 1
  %2149 = getelementptr i8, ptr %2148, i64 4
  store i32 0, ptr %2149, align 4
  %2150 = load { i32 }, ptr %counter, align 4
  %2151 = load ptr, ptr %__panic1, align 8
  store { i32 } %2150, ptr %ptr_arg720, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg720, ptr %2151)
  %2152 = load ptr, ptr %__panic1, align 8
  %2153 = load i8, ptr %2152, align 1
  %2154 = load ptr, ptr %__panic1, align 8
  %2155 = getelementptr i8, ptr %2154, i64 4
  %2156 = load i32, ptr %2155, align 4
  %2157 = icmp ne i8 %2145, 0
  %2158 = icmp ne i8 %2153, 0
  %2159 = and i1 %2157, %2158
  %2160 = zext i1 %2159 to i8
  %2161 = icmp ne i8 %2160, 0
  br i1 %2161, label %then721, label %else722

then721:                                          ; preds = %ifcont719
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2156)
  br label %ifcont723

else722:                                          ; preds = %ifcont719
  br label %ifcont723

ifcont723:                                        ; preds = %else722, %then721
  %2162 = phi {} [ zeroinitializer, %then721 ], [ zeroinitializer, %else722 ]
  %2163 = icmp ne i8 %2153, 0
  %2164 = xor i1 %2163, true
  %2165 = zext i1 %2164 to i8
  %2166 = icmp ne i8 %2145, 0
  %2167 = icmp ne i8 %2165, 0
  %2168 = and i1 %2166, %2167
  %2169 = zext i1 %2168 to i8
  %2170 = icmp ne i8 %2169, 0
  br i1 %2170, label %then724, label %else725

then724:                                          ; preds = %ifcont723
  %2171 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2171, align 1
  %2172 = load ptr, ptr %__panic1, align 8
  %2173 = getelementptr i8, ptr %2172, i64 4
  store i32 %2147, ptr %2173, align 4
  br label %ifcont726

else725:                                          ; preds = %ifcont723
  br label %ifcont726

ifcont726:                                        ; preds = %else725, %then724
  %2174 = phi {} [ zeroinitializer, %then724 ], [ zeroinitializer, %else725 ]
  %2175 = icmp ne i8 %2145, 0
  %2176 = icmp ne i8 %2153, 0
  %2177 = or i1 %2175, %2176
  %2178 = zext i1 %2177 to i8
  %2179 = icmp ne i8 %2153, 0
  br i1 %2179, label %then727, label %else728

then727:                                          ; preds = %ifcont726
  br label %ifcont729

else728:                                          ; preds = %ifcont726
  br label %ifcont729

ifcont729:                                        ; preds = %else728, %then727
  %2180 = phi i32 [ %2156, %then727 ], [ %2147, %else728 ]
  ret i32 0

panic_cont730:                                    ; preds = %panic_ok708
  %2181 = load { ptr, i64 }, ptr %bytes_msg, align 8
  store { ptr, i64 } %2181, ptr %byref_arg731, align 8
  %2182 = call i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(ptr %ctx, ptr %byref_arg731)
  store i16 %2182, ptr %match_val732, align 2
  %2183 = load i8, ptr %match_val732, align 1
  %2184 = icmp eq i8 %2183, 0
  br i1 %2184, label %match_arm735, label %match_next736

match_merge733:                                   ; preds = %panic_cont797, %panic_cont761
  %2185 = phi {} [ zeroinitializer, %panic_cont761 ], [ zeroinitializer, %panic_cont797 ]
  %2186 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2186, align 1
  %2187 = getelementptr i8, ptr %2186, i64 4
  store i32 0, ptr %2187, align 4
  store i16 %2182, ptr %match_val798, align 2
  %2188 = load i8, ptr %match_val798, align 1
  %2189 = icmp eq i8 %2188, 0
  br i1 %2189, label %match_arm801, label %match_next802

match_fail734:                                    ; preds = %match_next736
  unreachable

match_arm735:                                     ; preds = %panic_cont730
  store i16 %2182, ptr %tmp_addr737, align 2
  store {} zeroinitializer, ptr %ok738, align 1
  store {} zeroinitializer, ptr %ok738, align 1
  %2190 = load ptr, ptr %__panic1, align 8
  %2191 = load i8, ptr %2190, align 1
  %2192 = icmp ne i8 %2191, 0
  br i1 %2192, label %panic_fail740, label %panic_ok739

match_next736:                                    ; preds = %panic_cont730
  %2193 = load i8, ptr %match_val732, align 1
  %2194 = icmp eq i8 %2193, 1
  br i1 %2194, label %match_arm762, label %match_fail734

panic_ok739:                                      ; preds = %match_arm735
  br label %panic_cont761

panic_fail740:                                    ; preds = %match_arm735
  %2195 = load ptr, ptr %__panic1, align 8
  %2196 = load i8, ptr %2195, align 1
  %2197 = load ptr, ptr %__panic1, align 8
  %2198 = getelementptr i8, ptr %2197, i64 4
  %2199 = load i32, ptr %2198, align 4
  %2200 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2200, align 1
  %2201 = getelementptr i8, ptr %2200, i64 4
  store i32 0, ptr %2201, align 4
  %2202 = load { i32 }, ptr %next, align 4
  %2203 = load ptr, ptr %__panic1, align 8
  store { i32 } %2202, ptr %ptr_arg741, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg741, ptr %2203)
  %2204 = load ptr, ptr %__panic1, align 8
  %2205 = load i8, ptr %2204, align 1
  %2206 = load ptr, ptr %__panic1, align 8
  %2207 = getelementptr i8, ptr %2206, i64 4
  %2208 = load i32, ptr %2207, align 4
  %2209 = icmp ne i8 %2205, 0
  %2210 = and i1 true, %2209
  %2211 = zext i1 %2210 to i8
  %2212 = icmp ne i8 %2211, 0
  br i1 %2212, label %then742, label %else743

then742:                                          ; preds = %panic_fail740
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2208)
  br label %ifcont744

else743:                                          ; preds = %panic_fail740
  br label %ifcont744

ifcont744:                                        ; preds = %else743, %then742
  %2213 = phi {} [ zeroinitializer, %then742 ], [ zeroinitializer, %else743 ]
  %2214 = icmp ne i8 %2205, 0
  %2215 = xor i1 %2214, true
  %2216 = zext i1 %2215 to i8
  %2217 = icmp ne i8 %2216, 0
  %2218 = and i1 true, %2217
  %2219 = zext i1 %2218 to i8
  %2220 = icmp ne i8 %2219, 0
  br i1 %2220, label %then745, label %else746

then745:                                          ; preds = %ifcont744
  %2221 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2221, align 1
  %2222 = load ptr, ptr %__panic1, align 8
  %2223 = getelementptr i8, ptr %2222, i64 4
  store i32 %2199, ptr %2223, align 4
  br label %ifcont747

else746:                                          ; preds = %ifcont744
  br label %ifcont747

ifcont747:                                        ; preds = %else746, %then745
  %2224 = phi {} [ zeroinitializer, %then745 ], [ zeroinitializer, %else746 ]
  %2225 = icmp ne i8 %2205, 0
  %2226 = or i1 true, %2225
  %2227 = zext i1 %2226 to i8
  %2228 = icmp ne i8 %2205, 0
  br i1 %2228, label %then748, label %else749

then748:                                          ; preds = %ifcont747
  br label %ifcont750

else749:                                          ; preds = %ifcont747
  br label %ifcont750

ifcont750:                                        ; preds = %else749, %then748
  %2229 = phi i32 [ %2208, %then748 ], [ %2199, %else749 ]
  %2230 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2230, align 1
  %2231 = getelementptr i8, ptr %2230, i64 4
  store i32 0, ptr %2231, align 4
  %2232 = load { i32 }, ptr %counter, align 4
  %2233 = load ptr, ptr %__panic1, align 8
  store { i32 } %2232, ptr %ptr_arg751, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg751, ptr %2233)
  %2234 = load ptr, ptr %__panic1, align 8
  %2235 = load i8, ptr %2234, align 1
  %2236 = load ptr, ptr %__panic1, align 8
  %2237 = getelementptr i8, ptr %2236, i64 4
  %2238 = load i32, ptr %2237, align 4
  %2239 = icmp ne i8 %2227, 0
  %2240 = icmp ne i8 %2235, 0
  %2241 = and i1 %2239, %2240
  %2242 = zext i1 %2241 to i8
  %2243 = icmp ne i8 %2242, 0
  br i1 %2243, label %then752, label %else753

then752:                                          ; preds = %ifcont750
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2238)
  br label %ifcont754

else753:                                          ; preds = %ifcont750
  br label %ifcont754

ifcont754:                                        ; preds = %else753, %then752
  %2244 = phi {} [ zeroinitializer, %then752 ], [ zeroinitializer, %else753 ]
  %2245 = icmp ne i8 %2235, 0
  %2246 = xor i1 %2245, true
  %2247 = zext i1 %2246 to i8
  %2248 = icmp ne i8 %2227, 0
  %2249 = icmp ne i8 %2247, 0
  %2250 = and i1 %2248, %2249
  %2251 = zext i1 %2250 to i8
  %2252 = icmp ne i8 %2251, 0
  br i1 %2252, label %then755, label %else756

then755:                                          ; preds = %ifcont754
  %2253 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2253, align 1
  %2254 = load ptr, ptr %__panic1, align 8
  %2255 = getelementptr i8, ptr %2254, i64 4
  store i32 %2229, ptr %2255, align 4
  br label %ifcont757

else756:                                          ; preds = %ifcont754
  br label %ifcont757

ifcont757:                                        ; preds = %else756, %then755
  %2256 = phi {} [ zeroinitializer, %then755 ], [ zeroinitializer, %else756 ]
  %2257 = icmp ne i8 %2227, 0
  %2258 = icmp ne i8 %2235, 0
  %2259 = or i1 %2257, %2258
  %2260 = zext i1 %2259 to i8
  %2261 = icmp ne i8 %2235, 0
  br i1 %2261, label %then758, label %else759

then758:                                          ; preds = %ifcont757
  br label %ifcont760

else759:                                          ; preds = %ifcont757
  br label %ifcont760

ifcont760:                                        ; preds = %else759, %then758
  %2262 = phi i32 [ %2238, %then758 ], [ %2229, %else759 ]
  ret i32 0

panic_cont761:                                    ; preds = %panic_ok739
  br label %match_merge733

match_arm762:                                     ; preds = %match_next736
  store i16 %2182, ptr %tmp_addr763, align 2
  %2263 = getelementptr i8, ptr %tmp_addr763, i64 1
  %2264 = load i8, ptr %2263, align 1
  store i8 0, ptr %err764, align 1
  store i8 %2264, ptr %err764, align 1
  %2265 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2265, align 1
  %2266 = getelementptr i8, ptr %2265, i64 4
  store i32 0, ptr %2266, align 4
  %2267 = load i8, ptr %err764, align 1
  %2268 = load ptr, ptr %__panic1, align 8
  store i8 %2267, ptr %ptr_arg765, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg765, ptr %2268)
  %2269 = load ptr, ptr %__panic1, align 8
  %2270 = load i8, ptr %2269, align 1
  %2271 = load ptr, ptr %__panic1, align 8
  %2272 = getelementptr i8, ptr %2271, i64 4
  %2273 = load i32, ptr %2272, align 4
  %2274 = icmp ne i8 %2270, 0
  %2275 = and i1 false, %2274
  %2276 = zext i1 %2275 to i8
  %2277 = icmp ne i8 %2276, 0
  br i1 %2277, label %then766, label %else767

then766:                                          ; preds = %match_arm762
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2273)
  br label %ifcont768

else767:                                          ; preds = %match_arm762
  br label %ifcont768

ifcont768:                                        ; preds = %else767, %then766
  %2278 = phi {} [ zeroinitializer, %then766 ], [ zeroinitializer, %else767 ]
  %2279 = icmp ne i8 %2270, 0
  %2280 = xor i1 %2279, true
  %2281 = zext i1 %2280 to i8
  %2282 = icmp ne i8 %2281, 0
  %2283 = and i1 false, %2282
  %2284 = zext i1 %2283 to i8
  %2285 = icmp ne i8 %2284, 0
  br i1 %2285, label %then769, label %else770

then769:                                          ; preds = %ifcont768
  %2286 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2286, align 1
  %2287 = load ptr, ptr %__panic1, align 8
  %2288 = getelementptr i8, ptr %2287, i64 4
  store i32 0, ptr %2288, align 4
  br label %ifcont771

else770:                                          ; preds = %ifcont768
  br label %ifcont771

ifcont771:                                        ; preds = %else770, %then769
  %2289 = phi {} [ zeroinitializer, %then769 ], [ zeroinitializer, %else770 ]
  %2290 = icmp ne i8 %2270, 0
  %2291 = or i1 false, %2290
  %2292 = zext i1 %2291 to i8
  %2293 = icmp ne i8 %2270, 0
  br i1 %2293, label %then772, label %else773

then772:                                          ; preds = %ifcont771
  br label %ifcont774

else773:                                          ; preds = %ifcont771
  br label %ifcont774

ifcont774:                                        ; preds = %else773, %then772
  %2294 = phi i32 [ %2273, %then772 ], [ 0, %else773 ]
  %2295 = load ptr, ptr %__panic1, align 8
  %2296 = load i8, ptr %2295, align 1
  %2297 = icmp ne i8 %2296, 0
  br i1 %2297, label %panic_fail776, label %panic_ok775

panic_ok775:                                      ; preds = %ifcont774
  br label %panic_cont797

panic_fail776:                                    ; preds = %ifcont774
  %2298 = load ptr, ptr %__panic1, align 8
  %2299 = load i8, ptr %2298, align 1
  %2300 = load ptr, ptr %__panic1, align 8
  %2301 = getelementptr i8, ptr %2300, i64 4
  %2302 = load i32, ptr %2301, align 4
  %2303 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2303, align 1
  %2304 = getelementptr i8, ptr %2303, i64 4
  store i32 0, ptr %2304, align 4
  %2305 = load { i32 }, ptr %next, align 4
  %2306 = load ptr, ptr %__panic1, align 8
  store { i32 } %2305, ptr %ptr_arg777, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg777, ptr %2306)
  %2307 = load ptr, ptr %__panic1, align 8
  %2308 = load i8, ptr %2307, align 1
  %2309 = load ptr, ptr %__panic1, align 8
  %2310 = getelementptr i8, ptr %2309, i64 4
  %2311 = load i32, ptr %2310, align 4
  %2312 = icmp ne i8 %2308, 0
  %2313 = and i1 true, %2312
  %2314 = zext i1 %2313 to i8
  %2315 = icmp ne i8 %2314, 0
  br i1 %2315, label %then778, label %else779

then778:                                          ; preds = %panic_fail776
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2311)
  br label %ifcont780

else779:                                          ; preds = %panic_fail776
  br label %ifcont780

ifcont780:                                        ; preds = %else779, %then778
  %2316 = phi {} [ zeroinitializer, %then778 ], [ zeroinitializer, %else779 ]
  %2317 = icmp ne i8 %2308, 0
  %2318 = xor i1 %2317, true
  %2319 = zext i1 %2318 to i8
  %2320 = icmp ne i8 %2319, 0
  %2321 = and i1 true, %2320
  %2322 = zext i1 %2321 to i8
  %2323 = icmp ne i8 %2322, 0
  br i1 %2323, label %then781, label %else782

then781:                                          ; preds = %ifcont780
  %2324 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2324, align 1
  %2325 = load ptr, ptr %__panic1, align 8
  %2326 = getelementptr i8, ptr %2325, i64 4
  store i32 %2302, ptr %2326, align 4
  br label %ifcont783

else782:                                          ; preds = %ifcont780
  br label %ifcont783

ifcont783:                                        ; preds = %else782, %then781
  %2327 = phi {} [ zeroinitializer, %then781 ], [ zeroinitializer, %else782 ]
  %2328 = icmp ne i8 %2308, 0
  %2329 = or i1 true, %2328
  %2330 = zext i1 %2329 to i8
  %2331 = icmp ne i8 %2308, 0
  br i1 %2331, label %then784, label %else785

then784:                                          ; preds = %ifcont783
  br label %ifcont786

else785:                                          ; preds = %ifcont783
  br label %ifcont786

ifcont786:                                        ; preds = %else785, %then784
  %2332 = phi i32 [ %2311, %then784 ], [ %2302, %else785 ]
  %2333 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2333, align 1
  %2334 = getelementptr i8, ptr %2333, i64 4
  store i32 0, ptr %2334, align 4
  %2335 = load { i32 }, ptr %counter, align 4
  %2336 = load ptr, ptr %__panic1, align 8
  store { i32 } %2335, ptr %ptr_arg787, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg787, ptr %2336)
  %2337 = load ptr, ptr %__panic1, align 8
  %2338 = load i8, ptr %2337, align 1
  %2339 = load ptr, ptr %__panic1, align 8
  %2340 = getelementptr i8, ptr %2339, i64 4
  %2341 = load i32, ptr %2340, align 4
  %2342 = icmp ne i8 %2330, 0
  %2343 = icmp ne i8 %2338, 0
  %2344 = and i1 %2342, %2343
  %2345 = zext i1 %2344 to i8
  %2346 = icmp ne i8 %2345, 0
  br i1 %2346, label %then788, label %else789

then788:                                          ; preds = %ifcont786
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2341)
  br label %ifcont790

else789:                                          ; preds = %ifcont786
  br label %ifcont790

ifcont790:                                        ; preds = %else789, %then788
  %2347 = phi {} [ zeroinitializer, %then788 ], [ zeroinitializer, %else789 ]
  %2348 = icmp ne i8 %2338, 0
  %2349 = xor i1 %2348, true
  %2350 = zext i1 %2349 to i8
  %2351 = icmp ne i8 %2330, 0
  %2352 = icmp ne i8 %2350, 0
  %2353 = and i1 %2351, %2352
  %2354 = zext i1 %2353 to i8
  %2355 = icmp ne i8 %2354, 0
  br i1 %2355, label %then791, label %else792

then791:                                          ; preds = %ifcont790
  %2356 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2356, align 1
  %2357 = load ptr, ptr %__panic1, align 8
  %2358 = getelementptr i8, ptr %2357, i64 4
  store i32 %2332, ptr %2358, align 4
  br label %ifcont793

else792:                                          ; preds = %ifcont790
  br label %ifcont793

ifcont793:                                        ; preds = %else792, %then791
  %2359 = phi {} [ zeroinitializer, %then791 ], [ zeroinitializer, %else792 ]
  %2360 = icmp ne i8 %2330, 0
  %2361 = icmp ne i8 %2338, 0
  %2362 = or i1 %2360, %2361
  %2363 = zext i1 %2362 to i8
  %2364 = icmp ne i8 %2338, 0
  br i1 %2364, label %then794, label %else795

then794:                                          ; preds = %ifcont793
  br label %ifcont796

else795:                                          ; preds = %ifcont793
  br label %ifcont796

ifcont796:                                        ; preds = %else795, %then794
  %2365 = phi i32 [ %2341, %then794 ], [ %2332, %else795 ]
  ret i32 0

panic_cont797:                                    ; preds = %panic_ok775
  br label %match_merge733

match_merge799:                                   ; preds = %match_arm803, %match_arm801
  %2366 = phi {} [ zeroinitializer, %match_arm801 ], [ zeroinitializer, %match_arm803 ]
  %2367 = load ptr, ptr %__panic1, align 8
  %2368 = load i8, ptr %2367, align 1
  %2369 = load ptr, ptr %__panic1, align 8
  %2370 = getelementptr i8, ptr %2369, i64 4
  %2371 = load i32, ptr %2370, align 4
  %2372 = icmp ne i8 %2368, 0
  %2373 = and i1 false, %2372
  %2374 = zext i1 %2373 to i8
  %2375 = icmp ne i8 %2374, 0
  br i1 %2375, label %then806, label %else807

match_fail800:                                    ; preds = %match_next802
  unreachable

match_arm801:                                     ; preds = %match_merge733
  br label %match_merge799

match_next802:                                    ; preds = %match_merge733
  %2376 = load i8, ptr %match_val798, align 1
  %2377 = icmp eq i8 %2376, 1
  br i1 %2377, label %match_arm803, label %match_fail800

match_arm803:                                     ; preds = %match_next802
  store i16 %2182, ptr %tmp_addr804, align 2
  %2378 = getelementptr i8, ptr %tmp_addr804, i64 1
  %2379 = load i8, ptr %2378, align 1
  %2380 = load ptr, ptr %__panic1, align 8
  store i8 %2379, ptr %ptr_arg805, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg805, ptr %2380)
  br label %match_merge799

then806:                                          ; preds = %match_merge799
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2371)
  br label %ifcont808

else807:                                          ; preds = %match_merge799
  br label %ifcont808

ifcont808:                                        ; preds = %else807, %then806
  %2381 = phi {} [ zeroinitializer, %then806 ], [ zeroinitializer, %else807 ]
  %2382 = icmp ne i8 %2368, 0
  %2383 = xor i1 %2382, true
  %2384 = zext i1 %2383 to i8
  %2385 = icmp ne i8 %2384, 0
  %2386 = and i1 false, %2385
  %2387 = zext i1 %2386 to i8
  %2388 = icmp ne i8 %2387, 0
  br i1 %2388, label %then809, label %else810

then809:                                          ; preds = %ifcont808
  %2389 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2389, align 1
  %2390 = load ptr, ptr %__panic1, align 8
  %2391 = getelementptr i8, ptr %2390, i64 4
  store i32 0, ptr %2391, align 4
  br label %ifcont811

else810:                                          ; preds = %ifcont808
  br label %ifcont811

ifcont811:                                        ; preds = %else810, %then809
  %2392 = phi {} [ zeroinitializer, %then809 ], [ zeroinitializer, %else810 ]
  %2393 = icmp ne i8 %2368, 0
  %2394 = or i1 false, %2393
  %2395 = zext i1 %2394 to i8
  %2396 = icmp ne i8 %2368, 0
  br i1 %2396, label %then812, label %else813

then812:                                          ; preds = %ifcont811
  br label %ifcont814

else813:                                          ; preds = %ifcont811
  br label %ifcont814

ifcont814:                                        ; preds = %else813, %then812
  %2397 = phi i32 [ %2371, %then812 ], [ 0, %else813 ]
  %2398 = load ptr, ptr %__panic1, align 8
  %2399 = load i8, ptr %2398, align 1
  %2400 = icmp ne i8 %2399, 0
  br i1 %2400, label %panic_fail816, label %panic_ok815

panic_ok815:                                      ; preds = %ifcont814
  br label %panic_cont837

panic_fail816:                                    ; preds = %ifcont814
  %2401 = load ptr, ptr %__panic1, align 8
  %2402 = load i8, ptr %2401, align 1
  %2403 = load ptr, ptr %__panic1, align 8
  %2404 = getelementptr i8, ptr %2403, i64 4
  %2405 = load i32, ptr %2404, align 4
  %2406 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2406, align 1
  %2407 = getelementptr i8, ptr %2406, i64 4
  store i32 0, ptr %2407, align 4
  %2408 = load { i32 }, ptr %next, align 4
  %2409 = load ptr, ptr %__panic1, align 8
  store { i32 } %2408, ptr %ptr_arg817, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg817, ptr %2409)
  %2410 = load ptr, ptr %__panic1, align 8
  %2411 = load i8, ptr %2410, align 1
  %2412 = load ptr, ptr %__panic1, align 8
  %2413 = getelementptr i8, ptr %2412, i64 4
  %2414 = load i32, ptr %2413, align 4
  %2415 = icmp ne i8 %2411, 0
  %2416 = and i1 true, %2415
  %2417 = zext i1 %2416 to i8
  %2418 = icmp ne i8 %2417, 0
  br i1 %2418, label %then818, label %else819

then818:                                          ; preds = %panic_fail816
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2414)
  br label %ifcont820

else819:                                          ; preds = %panic_fail816
  br label %ifcont820

ifcont820:                                        ; preds = %else819, %then818
  %2419 = phi {} [ zeroinitializer, %then818 ], [ zeroinitializer, %else819 ]
  %2420 = icmp ne i8 %2411, 0
  %2421 = xor i1 %2420, true
  %2422 = zext i1 %2421 to i8
  %2423 = icmp ne i8 %2422, 0
  %2424 = and i1 true, %2423
  %2425 = zext i1 %2424 to i8
  %2426 = icmp ne i8 %2425, 0
  br i1 %2426, label %then821, label %else822

then821:                                          ; preds = %ifcont820
  %2427 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2427, align 1
  %2428 = load ptr, ptr %__panic1, align 8
  %2429 = getelementptr i8, ptr %2428, i64 4
  store i32 %2405, ptr %2429, align 4
  br label %ifcont823

else822:                                          ; preds = %ifcont820
  br label %ifcont823

ifcont823:                                        ; preds = %else822, %then821
  %2430 = phi {} [ zeroinitializer, %then821 ], [ zeroinitializer, %else822 ]
  %2431 = icmp ne i8 %2411, 0
  %2432 = or i1 true, %2431
  %2433 = zext i1 %2432 to i8
  %2434 = icmp ne i8 %2411, 0
  br i1 %2434, label %then824, label %else825

then824:                                          ; preds = %ifcont823
  br label %ifcont826

else825:                                          ; preds = %ifcont823
  br label %ifcont826

ifcont826:                                        ; preds = %else825, %then824
  %2435 = phi i32 [ %2414, %then824 ], [ %2405, %else825 ]
  %2436 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2436, align 1
  %2437 = getelementptr i8, ptr %2436, i64 4
  store i32 0, ptr %2437, align 4
  %2438 = load { i32 }, ptr %counter, align 4
  %2439 = load ptr, ptr %__panic1, align 8
  store { i32 } %2438, ptr %ptr_arg827, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg827, ptr %2439)
  %2440 = load ptr, ptr %__panic1, align 8
  %2441 = load i8, ptr %2440, align 1
  %2442 = load ptr, ptr %__panic1, align 8
  %2443 = getelementptr i8, ptr %2442, i64 4
  %2444 = load i32, ptr %2443, align 4
  %2445 = icmp ne i8 %2433, 0
  %2446 = icmp ne i8 %2441, 0
  %2447 = and i1 %2445, %2446
  %2448 = zext i1 %2447 to i8
  %2449 = icmp ne i8 %2448, 0
  br i1 %2449, label %then828, label %else829

then828:                                          ; preds = %ifcont826
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2444)
  br label %ifcont830

else829:                                          ; preds = %ifcont826
  br label %ifcont830

ifcont830:                                        ; preds = %else829, %then828
  %2450 = phi {} [ zeroinitializer, %then828 ], [ zeroinitializer, %else829 ]
  %2451 = icmp ne i8 %2441, 0
  %2452 = xor i1 %2451, true
  %2453 = zext i1 %2452 to i8
  %2454 = icmp ne i8 %2433, 0
  %2455 = icmp ne i8 %2453, 0
  %2456 = and i1 %2454, %2455
  %2457 = zext i1 %2456 to i8
  %2458 = icmp ne i8 %2457, 0
  br i1 %2458, label %then831, label %else832

then831:                                          ; preds = %ifcont830
  %2459 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2459, align 1
  %2460 = load ptr, ptr %__panic1, align 8
  %2461 = getelementptr i8, ptr %2460, i64 4
  store i32 %2435, ptr %2461, align 4
  br label %ifcont833

else832:                                          ; preds = %ifcont830
  br label %ifcont833

ifcont833:                                        ; preds = %else832, %then831
  %2462 = phi {} [ zeroinitializer, %then831 ], [ zeroinitializer, %else832 ]
  %2463 = icmp ne i8 %2433, 0
  %2464 = icmp ne i8 %2441, 0
  %2465 = or i1 %2463, %2464
  %2466 = zext i1 %2465 to i8
  %2467 = icmp ne i8 %2441, 0
  br i1 %2467, label %then834, label %else835

then834:                                          ; preds = %ifcont833
  br label %ifcont836

else835:                                          ; preds = %ifcont833
  br label %ifcont836

ifcont836:                                        ; preds = %else835, %then834
  %2468 = phi i32 [ %2444, %then834 ], [ %2435, %else835 ]
  ret i32 0

panic_cont837:                                    ; preds = %panic_ok815
  %2469 = load { ptr, i64 }, ptr %status_msg, align 8
  store { ptr, i64 } %2469, ptr %byref_arg838, align 8
  %2470 = call i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(ptr %ctx, ptr %byref_arg838)
  store i16 %2470, ptr %match_val839, align 2
  %2471 = load i8, ptr %match_val839, align 1
  %2472 = icmp eq i8 %2471, 0
  br i1 %2472, label %match_arm842, label %match_next843

match_merge840:                                   ; preds = %panic_cont904, %panic_cont868
  %2473 = phi {} [ zeroinitializer, %panic_cont868 ], [ zeroinitializer, %panic_cont904 ]
  %2474 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2474, align 1
  %2475 = getelementptr i8, ptr %2474, i64 4
  store i32 0, ptr %2475, align 4
  store i16 %2470, ptr %match_val905, align 2
  %2476 = load i8, ptr %match_val905, align 1
  %2477 = icmp eq i8 %2476, 0
  br i1 %2477, label %match_arm908, label %match_next909

match_fail841:                                    ; preds = %match_next843
  unreachable

match_arm842:                                     ; preds = %panic_cont837
  store i16 %2470, ptr %tmp_addr844, align 2
  store {} zeroinitializer, ptr %ok845, align 1
  store {} zeroinitializer, ptr %ok845, align 1
  %2478 = load ptr, ptr %__panic1, align 8
  %2479 = load i8, ptr %2478, align 1
  %2480 = icmp ne i8 %2479, 0
  br i1 %2480, label %panic_fail847, label %panic_ok846

match_next843:                                    ; preds = %panic_cont837
  %2481 = load i8, ptr %match_val839, align 1
  %2482 = icmp eq i8 %2481, 1
  br i1 %2482, label %match_arm869, label %match_fail841

panic_ok846:                                      ; preds = %match_arm842
  br label %panic_cont868

panic_fail847:                                    ; preds = %match_arm842
  %2483 = load ptr, ptr %__panic1, align 8
  %2484 = load i8, ptr %2483, align 1
  %2485 = load ptr, ptr %__panic1, align 8
  %2486 = getelementptr i8, ptr %2485, i64 4
  %2487 = load i32, ptr %2486, align 4
  %2488 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2488, align 1
  %2489 = getelementptr i8, ptr %2488, i64 4
  store i32 0, ptr %2489, align 4
  %2490 = load { i32 }, ptr %next, align 4
  %2491 = load ptr, ptr %__panic1, align 8
  store { i32 } %2490, ptr %ptr_arg848, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg848, ptr %2491)
  %2492 = load ptr, ptr %__panic1, align 8
  %2493 = load i8, ptr %2492, align 1
  %2494 = load ptr, ptr %__panic1, align 8
  %2495 = getelementptr i8, ptr %2494, i64 4
  %2496 = load i32, ptr %2495, align 4
  %2497 = icmp ne i8 %2493, 0
  %2498 = and i1 true, %2497
  %2499 = zext i1 %2498 to i8
  %2500 = icmp ne i8 %2499, 0
  br i1 %2500, label %then849, label %else850

then849:                                          ; preds = %panic_fail847
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2496)
  br label %ifcont851

else850:                                          ; preds = %panic_fail847
  br label %ifcont851

ifcont851:                                        ; preds = %else850, %then849
  %2501 = phi {} [ zeroinitializer, %then849 ], [ zeroinitializer, %else850 ]
  %2502 = icmp ne i8 %2493, 0
  %2503 = xor i1 %2502, true
  %2504 = zext i1 %2503 to i8
  %2505 = icmp ne i8 %2504, 0
  %2506 = and i1 true, %2505
  %2507 = zext i1 %2506 to i8
  %2508 = icmp ne i8 %2507, 0
  br i1 %2508, label %then852, label %else853

then852:                                          ; preds = %ifcont851
  %2509 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2509, align 1
  %2510 = load ptr, ptr %__panic1, align 8
  %2511 = getelementptr i8, ptr %2510, i64 4
  store i32 %2487, ptr %2511, align 4
  br label %ifcont854

else853:                                          ; preds = %ifcont851
  br label %ifcont854

ifcont854:                                        ; preds = %else853, %then852
  %2512 = phi {} [ zeroinitializer, %then852 ], [ zeroinitializer, %else853 ]
  %2513 = icmp ne i8 %2493, 0
  %2514 = or i1 true, %2513
  %2515 = zext i1 %2514 to i8
  %2516 = icmp ne i8 %2493, 0
  br i1 %2516, label %then855, label %else856

then855:                                          ; preds = %ifcont854
  br label %ifcont857

else856:                                          ; preds = %ifcont854
  br label %ifcont857

ifcont857:                                        ; preds = %else856, %then855
  %2517 = phi i32 [ %2496, %then855 ], [ %2487, %else856 ]
  %2518 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2518, align 1
  %2519 = getelementptr i8, ptr %2518, i64 4
  store i32 0, ptr %2519, align 4
  %2520 = load { i32 }, ptr %counter, align 4
  %2521 = load ptr, ptr %__panic1, align 8
  store { i32 } %2520, ptr %ptr_arg858, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg858, ptr %2521)
  %2522 = load ptr, ptr %__panic1, align 8
  %2523 = load i8, ptr %2522, align 1
  %2524 = load ptr, ptr %__panic1, align 8
  %2525 = getelementptr i8, ptr %2524, i64 4
  %2526 = load i32, ptr %2525, align 4
  %2527 = icmp ne i8 %2515, 0
  %2528 = icmp ne i8 %2523, 0
  %2529 = and i1 %2527, %2528
  %2530 = zext i1 %2529 to i8
  %2531 = icmp ne i8 %2530, 0
  br i1 %2531, label %then859, label %else860

then859:                                          ; preds = %ifcont857
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2526)
  br label %ifcont861

else860:                                          ; preds = %ifcont857
  br label %ifcont861

ifcont861:                                        ; preds = %else860, %then859
  %2532 = phi {} [ zeroinitializer, %then859 ], [ zeroinitializer, %else860 ]
  %2533 = icmp ne i8 %2523, 0
  %2534 = xor i1 %2533, true
  %2535 = zext i1 %2534 to i8
  %2536 = icmp ne i8 %2515, 0
  %2537 = icmp ne i8 %2535, 0
  %2538 = and i1 %2536, %2537
  %2539 = zext i1 %2538 to i8
  %2540 = icmp ne i8 %2539, 0
  br i1 %2540, label %then862, label %else863

then862:                                          ; preds = %ifcont861
  %2541 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2541, align 1
  %2542 = load ptr, ptr %__panic1, align 8
  %2543 = getelementptr i8, ptr %2542, i64 4
  store i32 %2517, ptr %2543, align 4
  br label %ifcont864

else863:                                          ; preds = %ifcont861
  br label %ifcont864

ifcont864:                                        ; preds = %else863, %then862
  %2544 = phi {} [ zeroinitializer, %then862 ], [ zeroinitializer, %else863 ]
  %2545 = icmp ne i8 %2515, 0
  %2546 = icmp ne i8 %2523, 0
  %2547 = or i1 %2545, %2546
  %2548 = zext i1 %2547 to i8
  %2549 = icmp ne i8 %2523, 0
  br i1 %2549, label %then865, label %else866

then865:                                          ; preds = %ifcont864
  br label %ifcont867

else866:                                          ; preds = %ifcont864
  br label %ifcont867

ifcont867:                                        ; preds = %else866, %then865
  %2550 = phi i32 [ %2526, %then865 ], [ %2517, %else866 ]
  ret i32 0

panic_cont868:                                    ; preds = %panic_ok846
  br label %match_merge840

match_arm869:                                     ; preds = %match_next843
  store i16 %2470, ptr %tmp_addr870, align 2
  %2551 = getelementptr i8, ptr %tmp_addr870, i64 1
  %2552 = load i8, ptr %2551, align 1
  store i8 0, ptr %err871, align 1
  store i8 %2552, ptr %err871, align 1
  %2553 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2553, align 1
  %2554 = getelementptr i8, ptr %2553, i64 4
  store i32 0, ptr %2554, align 4
  %2555 = load i8, ptr %err871, align 1
  %2556 = load ptr, ptr %__panic1, align 8
  store i8 %2555, ptr %ptr_arg872, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg872, ptr %2556)
  %2557 = load ptr, ptr %__panic1, align 8
  %2558 = load i8, ptr %2557, align 1
  %2559 = load ptr, ptr %__panic1, align 8
  %2560 = getelementptr i8, ptr %2559, i64 4
  %2561 = load i32, ptr %2560, align 4
  %2562 = icmp ne i8 %2558, 0
  %2563 = and i1 false, %2562
  %2564 = zext i1 %2563 to i8
  %2565 = icmp ne i8 %2564, 0
  br i1 %2565, label %then873, label %else874

then873:                                          ; preds = %match_arm869
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2561)
  br label %ifcont875

else874:                                          ; preds = %match_arm869
  br label %ifcont875

ifcont875:                                        ; preds = %else874, %then873
  %2566 = phi {} [ zeroinitializer, %then873 ], [ zeroinitializer, %else874 ]
  %2567 = icmp ne i8 %2558, 0
  %2568 = xor i1 %2567, true
  %2569 = zext i1 %2568 to i8
  %2570 = icmp ne i8 %2569, 0
  %2571 = and i1 false, %2570
  %2572 = zext i1 %2571 to i8
  %2573 = icmp ne i8 %2572, 0
  br i1 %2573, label %then876, label %else877

then876:                                          ; preds = %ifcont875
  %2574 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2574, align 1
  %2575 = load ptr, ptr %__panic1, align 8
  %2576 = getelementptr i8, ptr %2575, i64 4
  store i32 0, ptr %2576, align 4
  br label %ifcont878

else877:                                          ; preds = %ifcont875
  br label %ifcont878

ifcont878:                                        ; preds = %else877, %then876
  %2577 = phi {} [ zeroinitializer, %then876 ], [ zeroinitializer, %else877 ]
  %2578 = icmp ne i8 %2558, 0
  %2579 = or i1 false, %2578
  %2580 = zext i1 %2579 to i8
  %2581 = icmp ne i8 %2558, 0
  br i1 %2581, label %then879, label %else880

then879:                                          ; preds = %ifcont878
  br label %ifcont881

else880:                                          ; preds = %ifcont878
  br label %ifcont881

ifcont881:                                        ; preds = %else880, %then879
  %2582 = phi i32 [ %2561, %then879 ], [ 0, %else880 ]
  %2583 = load ptr, ptr %__panic1, align 8
  %2584 = load i8, ptr %2583, align 1
  %2585 = icmp ne i8 %2584, 0
  br i1 %2585, label %panic_fail883, label %panic_ok882

panic_ok882:                                      ; preds = %ifcont881
  br label %panic_cont904

panic_fail883:                                    ; preds = %ifcont881
  %2586 = load ptr, ptr %__panic1, align 8
  %2587 = load i8, ptr %2586, align 1
  %2588 = load ptr, ptr %__panic1, align 8
  %2589 = getelementptr i8, ptr %2588, i64 4
  %2590 = load i32, ptr %2589, align 4
  %2591 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2591, align 1
  %2592 = getelementptr i8, ptr %2591, i64 4
  store i32 0, ptr %2592, align 4
  %2593 = load { i32 }, ptr %next, align 4
  %2594 = load ptr, ptr %__panic1, align 8
  store { i32 } %2593, ptr %ptr_arg884, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg884, ptr %2594)
  %2595 = load ptr, ptr %__panic1, align 8
  %2596 = load i8, ptr %2595, align 1
  %2597 = load ptr, ptr %__panic1, align 8
  %2598 = getelementptr i8, ptr %2597, i64 4
  %2599 = load i32, ptr %2598, align 4
  %2600 = icmp ne i8 %2596, 0
  %2601 = and i1 true, %2600
  %2602 = zext i1 %2601 to i8
  %2603 = icmp ne i8 %2602, 0
  br i1 %2603, label %then885, label %else886

then885:                                          ; preds = %panic_fail883
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2599)
  br label %ifcont887

else886:                                          ; preds = %panic_fail883
  br label %ifcont887

ifcont887:                                        ; preds = %else886, %then885
  %2604 = phi {} [ zeroinitializer, %then885 ], [ zeroinitializer, %else886 ]
  %2605 = icmp ne i8 %2596, 0
  %2606 = xor i1 %2605, true
  %2607 = zext i1 %2606 to i8
  %2608 = icmp ne i8 %2607, 0
  %2609 = and i1 true, %2608
  %2610 = zext i1 %2609 to i8
  %2611 = icmp ne i8 %2610, 0
  br i1 %2611, label %then888, label %else889

then888:                                          ; preds = %ifcont887
  %2612 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2612, align 1
  %2613 = load ptr, ptr %__panic1, align 8
  %2614 = getelementptr i8, ptr %2613, i64 4
  store i32 %2590, ptr %2614, align 4
  br label %ifcont890

else889:                                          ; preds = %ifcont887
  br label %ifcont890

ifcont890:                                        ; preds = %else889, %then888
  %2615 = phi {} [ zeroinitializer, %then888 ], [ zeroinitializer, %else889 ]
  %2616 = icmp ne i8 %2596, 0
  %2617 = or i1 true, %2616
  %2618 = zext i1 %2617 to i8
  %2619 = icmp ne i8 %2596, 0
  br i1 %2619, label %then891, label %else892

then891:                                          ; preds = %ifcont890
  br label %ifcont893

else892:                                          ; preds = %ifcont890
  br label %ifcont893

ifcont893:                                        ; preds = %else892, %then891
  %2620 = phi i32 [ %2599, %then891 ], [ %2590, %else892 ]
  %2621 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2621, align 1
  %2622 = getelementptr i8, ptr %2621, i64 4
  store i32 0, ptr %2622, align 4
  %2623 = load { i32 }, ptr %counter, align 4
  %2624 = load ptr, ptr %__panic1, align 8
  store { i32 } %2623, ptr %ptr_arg894, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg894, ptr %2624)
  %2625 = load ptr, ptr %__panic1, align 8
  %2626 = load i8, ptr %2625, align 1
  %2627 = load ptr, ptr %__panic1, align 8
  %2628 = getelementptr i8, ptr %2627, i64 4
  %2629 = load i32, ptr %2628, align 4
  %2630 = icmp ne i8 %2618, 0
  %2631 = icmp ne i8 %2626, 0
  %2632 = and i1 %2630, %2631
  %2633 = zext i1 %2632 to i8
  %2634 = icmp ne i8 %2633, 0
  br i1 %2634, label %then895, label %else896

then895:                                          ; preds = %ifcont893
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2629)
  br label %ifcont897

else896:                                          ; preds = %ifcont893
  br label %ifcont897

ifcont897:                                        ; preds = %else896, %then895
  %2635 = phi {} [ zeroinitializer, %then895 ], [ zeroinitializer, %else896 ]
  %2636 = icmp ne i8 %2626, 0
  %2637 = xor i1 %2636, true
  %2638 = zext i1 %2637 to i8
  %2639 = icmp ne i8 %2618, 0
  %2640 = icmp ne i8 %2638, 0
  %2641 = and i1 %2639, %2640
  %2642 = zext i1 %2641 to i8
  %2643 = icmp ne i8 %2642, 0
  br i1 %2643, label %then898, label %else899

then898:                                          ; preds = %ifcont897
  %2644 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2644, align 1
  %2645 = load ptr, ptr %__panic1, align 8
  %2646 = getelementptr i8, ptr %2645, i64 4
  store i32 %2620, ptr %2646, align 4
  br label %ifcont900

else899:                                          ; preds = %ifcont897
  br label %ifcont900

ifcont900:                                        ; preds = %else899, %then898
  %2647 = phi {} [ zeroinitializer, %then898 ], [ zeroinitializer, %else899 ]
  %2648 = icmp ne i8 %2618, 0
  %2649 = icmp ne i8 %2626, 0
  %2650 = or i1 %2648, %2649
  %2651 = zext i1 %2650 to i8
  %2652 = icmp ne i8 %2626, 0
  br i1 %2652, label %then901, label %else902

then901:                                          ; preds = %ifcont900
  br label %ifcont903

else902:                                          ; preds = %ifcont900
  br label %ifcont903

ifcont903:                                        ; preds = %else902, %then901
  %2653 = phi i32 [ %2629, %then901 ], [ %2620, %else902 ]
  ret i32 0

panic_cont904:                                    ; preds = %panic_ok882
  br label %match_merge840

match_merge906:                                   ; preds = %match_arm910, %match_arm908
  %2654 = phi {} [ zeroinitializer, %match_arm908 ], [ zeroinitializer, %match_arm910 ]
  %2655 = load ptr, ptr %__panic1, align 8
  %2656 = load i8, ptr %2655, align 1
  %2657 = load ptr, ptr %__panic1, align 8
  %2658 = getelementptr i8, ptr %2657, i64 4
  %2659 = load i32, ptr %2658, align 4
  %2660 = icmp ne i8 %2656, 0
  %2661 = and i1 false, %2660
  %2662 = zext i1 %2661 to i8
  %2663 = icmp ne i8 %2662, 0
  br i1 %2663, label %then913, label %else914

match_fail907:                                    ; preds = %match_next909
  unreachable

match_arm908:                                     ; preds = %match_merge840
  br label %match_merge906

match_next909:                                    ; preds = %match_merge840
  %2664 = load i8, ptr %match_val905, align 1
  %2665 = icmp eq i8 %2664, 1
  br i1 %2665, label %match_arm910, label %match_fail907

match_arm910:                                     ; preds = %match_next909
  store i16 %2470, ptr %tmp_addr911, align 2
  %2666 = getelementptr i8, ptr %tmp_addr911, i64 1
  %2667 = load i8, ptr %2666, align 1
  %2668 = load ptr, ptr %__panic1, align 8
  store i8 %2667, ptr %ptr_arg912, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg912, ptr %2668)
  br label %match_merge906

then913:                                          ; preds = %match_merge906
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2659)
  br label %ifcont915

else914:                                          ; preds = %match_merge906
  br label %ifcont915

ifcont915:                                        ; preds = %else914, %then913
  %2669 = phi {} [ zeroinitializer, %then913 ], [ zeroinitializer, %else914 ]
  %2670 = icmp ne i8 %2656, 0
  %2671 = xor i1 %2670, true
  %2672 = zext i1 %2671 to i8
  %2673 = icmp ne i8 %2672, 0
  %2674 = and i1 false, %2673
  %2675 = zext i1 %2674 to i8
  %2676 = icmp ne i8 %2675, 0
  br i1 %2676, label %then916, label %else917

then916:                                          ; preds = %ifcont915
  %2677 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2677, align 1
  %2678 = load ptr, ptr %__panic1, align 8
  %2679 = getelementptr i8, ptr %2678, i64 4
  store i32 0, ptr %2679, align 4
  br label %ifcont918

else917:                                          ; preds = %ifcont915
  br label %ifcont918

ifcont918:                                        ; preds = %else917, %then916
  %2680 = phi {} [ zeroinitializer, %then916 ], [ zeroinitializer, %else917 ]
  %2681 = icmp ne i8 %2656, 0
  %2682 = or i1 false, %2681
  %2683 = zext i1 %2682 to i8
  %2684 = icmp ne i8 %2656, 0
  br i1 %2684, label %then919, label %else920

then919:                                          ; preds = %ifcont918
  br label %ifcont921

else920:                                          ; preds = %ifcont918
  br label %ifcont921

ifcont921:                                        ; preds = %else920, %then919
  %2685 = phi i32 [ %2659, %then919 ], [ 0, %else920 ]
  %2686 = load ptr, ptr %__panic1, align 8
  %2687 = load i8, ptr %2686, align 1
  %2688 = icmp ne i8 %2687, 0
  br i1 %2688, label %panic_fail923, label %panic_ok922

panic_ok922:                                      ; preds = %ifcont921
  br label %panic_cont944

panic_fail923:                                    ; preds = %ifcont921
  %2689 = load ptr, ptr %__panic1, align 8
  %2690 = load i8, ptr %2689, align 1
  %2691 = load ptr, ptr %__panic1, align 8
  %2692 = getelementptr i8, ptr %2691, i64 4
  %2693 = load i32, ptr %2692, align 4
  %2694 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2694, align 1
  %2695 = getelementptr i8, ptr %2694, i64 4
  store i32 0, ptr %2695, align 4
  %2696 = load { i32 }, ptr %next, align 4
  %2697 = load ptr, ptr %__panic1, align 8
  store { i32 } %2696, ptr %ptr_arg924, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg924, ptr %2697)
  %2698 = load ptr, ptr %__panic1, align 8
  %2699 = load i8, ptr %2698, align 1
  %2700 = load ptr, ptr %__panic1, align 8
  %2701 = getelementptr i8, ptr %2700, i64 4
  %2702 = load i32, ptr %2701, align 4
  %2703 = icmp ne i8 %2699, 0
  %2704 = and i1 true, %2703
  %2705 = zext i1 %2704 to i8
  %2706 = icmp ne i8 %2705, 0
  br i1 %2706, label %then925, label %else926

then925:                                          ; preds = %panic_fail923
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2702)
  br label %ifcont927

else926:                                          ; preds = %panic_fail923
  br label %ifcont927

ifcont927:                                        ; preds = %else926, %then925
  %2707 = phi {} [ zeroinitializer, %then925 ], [ zeroinitializer, %else926 ]
  %2708 = icmp ne i8 %2699, 0
  %2709 = xor i1 %2708, true
  %2710 = zext i1 %2709 to i8
  %2711 = icmp ne i8 %2710, 0
  %2712 = and i1 true, %2711
  %2713 = zext i1 %2712 to i8
  %2714 = icmp ne i8 %2713, 0
  br i1 %2714, label %then928, label %else929

then928:                                          ; preds = %ifcont927
  %2715 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2715, align 1
  %2716 = load ptr, ptr %__panic1, align 8
  %2717 = getelementptr i8, ptr %2716, i64 4
  store i32 %2693, ptr %2717, align 4
  br label %ifcont930

else929:                                          ; preds = %ifcont927
  br label %ifcont930

ifcont930:                                        ; preds = %else929, %then928
  %2718 = phi {} [ zeroinitializer, %then928 ], [ zeroinitializer, %else929 ]
  %2719 = icmp ne i8 %2699, 0
  %2720 = or i1 true, %2719
  %2721 = zext i1 %2720 to i8
  %2722 = icmp ne i8 %2699, 0
  br i1 %2722, label %then931, label %else932

then931:                                          ; preds = %ifcont930
  br label %ifcont933

else932:                                          ; preds = %ifcont930
  br label %ifcont933

ifcont933:                                        ; preds = %else932, %then931
  %2723 = phi i32 [ %2702, %then931 ], [ %2693, %else932 ]
  %2724 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2724, align 1
  %2725 = getelementptr i8, ptr %2724, i64 4
  store i32 0, ptr %2725, align 4
  %2726 = load { i32 }, ptr %counter, align 4
  %2727 = load ptr, ptr %__panic1, align 8
  store { i32 } %2726, ptr %ptr_arg934, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg934, ptr %2727)
  %2728 = load ptr, ptr %__panic1, align 8
  %2729 = load i8, ptr %2728, align 1
  %2730 = load ptr, ptr %__panic1, align 8
  %2731 = getelementptr i8, ptr %2730, i64 4
  %2732 = load i32, ptr %2731, align 4
  %2733 = icmp ne i8 %2721, 0
  %2734 = icmp ne i8 %2729, 0
  %2735 = and i1 %2733, %2734
  %2736 = zext i1 %2735 to i8
  %2737 = icmp ne i8 %2736, 0
  br i1 %2737, label %then935, label %else936

then935:                                          ; preds = %ifcont933
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2732)
  br label %ifcont937

else936:                                          ; preds = %ifcont933
  br label %ifcont937

ifcont937:                                        ; preds = %else936, %then935
  %2738 = phi {} [ zeroinitializer, %then935 ], [ zeroinitializer, %else936 ]
  %2739 = icmp ne i8 %2729, 0
  %2740 = xor i1 %2739, true
  %2741 = zext i1 %2740 to i8
  %2742 = icmp ne i8 %2721, 0
  %2743 = icmp ne i8 %2741, 0
  %2744 = and i1 %2742, %2743
  %2745 = zext i1 %2744 to i8
  %2746 = icmp ne i8 %2745, 0
  br i1 %2746, label %then938, label %else939

then938:                                          ; preds = %ifcont937
  %2747 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2747, align 1
  %2748 = load ptr, ptr %__panic1, align 8
  %2749 = getelementptr i8, ptr %2748, i64 4
  store i32 %2723, ptr %2749, align 4
  br label %ifcont940

else939:                                          ; preds = %ifcont937
  br label %ifcont940

ifcont940:                                        ; preds = %else939, %then938
  %2750 = phi {} [ zeroinitializer, %then938 ], [ zeroinitializer, %else939 ]
  %2751 = icmp ne i8 %2721, 0
  %2752 = icmp ne i8 %2729, 0
  %2753 = or i1 %2751, %2752
  %2754 = zext i1 %2753 to i8
  %2755 = icmp ne i8 %2729, 0
  br i1 %2755, label %then941, label %else942

then941:                                          ; preds = %ifcont940
  br label %ifcont943

else942:                                          ; preds = %ifcont940
  br label %ifcont943

ifcont943:                                        ; preds = %else942, %then941
  %2756 = phi i32 [ %2732, %then941 ], [ %2723, %else942 ]
  ret i32 0

panic_cont944:                                    ; preds = %panic_ok922
  %2757 = load { ptr, i64 }, ptr %ptr_msg, align 8
  store { ptr, i64 } %2757, ptr %byref_arg945, align 8
  %2758 = call i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(ptr %ctx, ptr %byref_arg945)
  store i16 %2758, ptr %match_val946, align 2
  %2759 = load i8, ptr %match_val946, align 1
  %2760 = icmp eq i8 %2759, 0
  br i1 %2760, label %match_arm949, label %match_next950

match_merge947:                                   ; preds = %panic_cont1011, %panic_cont975
  %2761 = phi {} [ zeroinitializer, %panic_cont975 ], [ zeroinitializer, %panic_cont1011 ]
  %2762 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2762, align 1
  %2763 = getelementptr i8, ptr %2762, i64 4
  store i32 0, ptr %2763, align 4
  store i16 %2758, ptr %match_val1012, align 2
  %2764 = load i8, ptr %match_val1012, align 1
  %2765 = icmp eq i8 %2764, 0
  br i1 %2765, label %match_arm1015, label %match_next1016

match_fail948:                                    ; preds = %match_next950
  unreachable

match_arm949:                                     ; preds = %panic_cont944
  store i16 %2758, ptr %tmp_addr951, align 2
  store {} zeroinitializer, ptr %ok952, align 1
  store {} zeroinitializer, ptr %ok952, align 1
  %2766 = load ptr, ptr %__panic1, align 8
  %2767 = load i8, ptr %2766, align 1
  %2768 = icmp ne i8 %2767, 0
  br i1 %2768, label %panic_fail954, label %panic_ok953

match_next950:                                    ; preds = %panic_cont944
  %2769 = load i8, ptr %match_val946, align 1
  %2770 = icmp eq i8 %2769, 1
  br i1 %2770, label %match_arm976, label %match_fail948

panic_ok953:                                      ; preds = %match_arm949
  br label %panic_cont975

panic_fail954:                                    ; preds = %match_arm949
  %2771 = load ptr, ptr %__panic1, align 8
  %2772 = load i8, ptr %2771, align 1
  %2773 = load ptr, ptr %__panic1, align 8
  %2774 = getelementptr i8, ptr %2773, i64 4
  %2775 = load i32, ptr %2774, align 4
  %2776 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2776, align 1
  %2777 = getelementptr i8, ptr %2776, i64 4
  store i32 0, ptr %2777, align 4
  %2778 = load { i32 }, ptr %next, align 4
  %2779 = load ptr, ptr %__panic1, align 8
  store { i32 } %2778, ptr %ptr_arg955, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg955, ptr %2779)
  %2780 = load ptr, ptr %__panic1, align 8
  %2781 = load i8, ptr %2780, align 1
  %2782 = load ptr, ptr %__panic1, align 8
  %2783 = getelementptr i8, ptr %2782, i64 4
  %2784 = load i32, ptr %2783, align 4
  %2785 = icmp ne i8 %2781, 0
  %2786 = and i1 true, %2785
  %2787 = zext i1 %2786 to i8
  %2788 = icmp ne i8 %2787, 0
  br i1 %2788, label %then956, label %else957

then956:                                          ; preds = %panic_fail954
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2784)
  br label %ifcont958

else957:                                          ; preds = %panic_fail954
  br label %ifcont958

ifcont958:                                        ; preds = %else957, %then956
  %2789 = phi {} [ zeroinitializer, %then956 ], [ zeroinitializer, %else957 ]
  %2790 = icmp ne i8 %2781, 0
  %2791 = xor i1 %2790, true
  %2792 = zext i1 %2791 to i8
  %2793 = icmp ne i8 %2792, 0
  %2794 = and i1 true, %2793
  %2795 = zext i1 %2794 to i8
  %2796 = icmp ne i8 %2795, 0
  br i1 %2796, label %then959, label %else960

then959:                                          ; preds = %ifcont958
  %2797 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2797, align 1
  %2798 = load ptr, ptr %__panic1, align 8
  %2799 = getelementptr i8, ptr %2798, i64 4
  store i32 %2775, ptr %2799, align 4
  br label %ifcont961

else960:                                          ; preds = %ifcont958
  br label %ifcont961

ifcont961:                                        ; preds = %else960, %then959
  %2800 = phi {} [ zeroinitializer, %then959 ], [ zeroinitializer, %else960 ]
  %2801 = icmp ne i8 %2781, 0
  %2802 = or i1 true, %2801
  %2803 = zext i1 %2802 to i8
  %2804 = icmp ne i8 %2781, 0
  br i1 %2804, label %then962, label %else963

then962:                                          ; preds = %ifcont961
  br label %ifcont964

else963:                                          ; preds = %ifcont961
  br label %ifcont964

ifcont964:                                        ; preds = %else963, %then962
  %2805 = phi i32 [ %2784, %then962 ], [ %2775, %else963 ]
  %2806 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2806, align 1
  %2807 = getelementptr i8, ptr %2806, i64 4
  store i32 0, ptr %2807, align 4
  %2808 = load { i32 }, ptr %counter, align 4
  %2809 = load ptr, ptr %__panic1, align 8
  store { i32 } %2808, ptr %ptr_arg965, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg965, ptr %2809)
  %2810 = load ptr, ptr %__panic1, align 8
  %2811 = load i8, ptr %2810, align 1
  %2812 = load ptr, ptr %__panic1, align 8
  %2813 = getelementptr i8, ptr %2812, i64 4
  %2814 = load i32, ptr %2813, align 4
  %2815 = icmp ne i8 %2803, 0
  %2816 = icmp ne i8 %2811, 0
  %2817 = and i1 %2815, %2816
  %2818 = zext i1 %2817 to i8
  %2819 = icmp ne i8 %2818, 0
  br i1 %2819, label %then966, label %else967

then966:                                          ; preds = %ifcont964
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2814)
  br label %ifcont968

else967:                                          ; preds = %ifcont964
  br label %ifcont968

ifcont968:                                        ; preds = %else967, %then966
  %2820 = phi {} [ zeroinitializer, %then966 ], [ zeroinitializer, %else967 ]
  %2821 = icmp ne i8 %2811, 0
  %2822 = xor i1 %2821, true
  %2823 = zext i1 %2822 to i8
  %2824 = icmp ne i8 %2803, 0
  %2825 = icmp ne i8 %2823, 0
  %2826 = and i1 %2824, %2825
  %2827 = zext i1 %2826 to i8
  %2828 = icmp ne i8 %2827, 0
  br i1 %2828, label %then969, label %else970

then969:                                          ; preds = %ifcont968
  %2829 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2829, align 1
  %2830 = load ptr, ptr %__panic1, align 8
  %2831 = getelementptr i8, ptr %2830, i64 4
  store i32 %2805, ptr %2831, align 4
  br label %ifcont971

else970:                                          ; preds = %ifcont968
  br label %ifcont971

ifcont971:                                        ; preds = %else970, %then969
  %2832 = phi {} [ zeroinitializer, %then969 ], [ zeroinitializer, %else970 ]
  %2833 = icmp ne i8 %2803, 0
  %2834 = icmp ne i8 %2811, 0
  %2835 = or i1 %2833, %2834
  %2836 = zext i1 %2835 to i8
  %2837 = icmp ne i8 %2811, 0
  br i1 %2837, label %then972, label %else973

then972:                                          ; preds = %ifcont971
  br label %ifcont974

else973:                                          ; preds = %ifcont971
  br label %ifcont974

ifcont974:                                        ; preds = %else973, %then972
  %2838 = phi i32 [ %2814, %then972 ], [ %2805, %else973 ]
  ret i32 0

panic_cont975:                                    ; preds = %panic_ok953
  br label %match_merge947

match_arm976:                                     ; preds = %match_next950
  store i16 %2758, ptr %tmp_addr977, align 2
  %2839 = getelementptr i8, ptr %tmp_addr977, i64 1
  %2840 = load i8, ptr %2839, align 1
  store i8 0, ptr %err978, align 1
  store i8 %2840, ptr %err978, align 1
  %2841 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2841, align 1
  %2842 = getelementptr i8, ptr %2841, i64 4
  store i32 0, ptr %2842, align 4
  %2843 = load i8, ptr %err978, align 1
  %2844 = load ptr, ptr %__panic1, align 8
  store i8 %2843, ptr %ptr_arg979, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg979, ptr %2844)
  %2845 = load ptr, ptr %__panic1, align 8
  %2846 = load i8, ptr %2845, align 1
  %2847 = load ptr, ptr %__panic1, align 8
  %2848 = getelementptr i8, ptr %2847, i64 4
  %2849 = load i32, ptr %2848, align 4
  %2850 = icmp ne i8 %2846, 0
  %2851 = and i1 false, %2850
  %2852 = zext i1 %2851 to i8
  %2853 = icmp ne i8 %2852, 0
  br i1 %2853, label %then980, label %else981

then980:                                          ; preds = %match_arm976
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2849)
  br label %ifcont982

else981:                                          ; preds = %match_arm976
  br label %ifcont982

ifcont982:                                        ; preds = %else981, %then980
  %2854 = phi {} [ zeroinitializer, %then980 ], [ zeroinitializer, %else981 ]
  %2855 = icmp ne i8 %2846, 0
  %2856 = xor i1 %2855, true
  %2857 = zext i1 %2856 to i8
  %2858 = icmp ne i8 %2857, 0
  %2859 = and i1 false, %2858
  %2860 = zext i1 %2859 to i8
  %2861 = icmp ne i8 %2860, 0
  br i1 %2861, label %then983, label %else984

then983:                                          ; preds = %ifcont982
  %2862 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2862, align 1
  %2863 = load ptr, ptr %__panic1, align 8
  %2864 = getelementptr i8, ptr %2863, i64 4
  store i32 0, ptr %2864, align 4
  br label %ifcont985

else984:                                          ; preds = %ifcont982
  br label %ifcont985

ifcont985:                                        ; preds = %else984, %then983
  %2865 = phi {} [ zeroinitializer, %then983 ], [ zeroinitializer, %else984 ]
  %2866 = icmp ne i8 %2846, 0
  %2867 = or i1 false, %2866
  %2868 = zext i1 %2867 to i8
  %2869 = icmp ne i8 %2846, 0
  br i1 %2869, label %then986, label %else987

then986:                                          ; preds = %ifcont985
  br label %ifcont988

else987:                                          ; preds = %ifcont985
  br label %ifcont988

ifcont988:                                        ; preds = %else987, %then986
  %2870 = phi i32 [ %2849, %then986 ], [ 0, %else987 ]
  %2871 = load ptr, ptr %__panic1, align 8
  %2872 = load i8, ptr %2871, align 1
  %2873 = icmp ne i8 %2872, 0
  br i1 %2873, label %panic_fail990, label %panic_ok989

panic_ok989:                                      ; preds = %ifcont988
  br label %panic_cont1011

panic_fail990:                                    ; preds = %ifcont988
  %2874 = load ptr, ptr %__panic1, align 8
  %2875 = load i8, ptr %2874, align 1
  %2876 = load ptr, ptr %__panic1, align 8
  %2877 = getelementptr i8, ptr %2876, i64 4
  %2878 = load i32, ptr %2877, align 4
  %2879 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2879, align 1
  %2880 = getelementptr i8, ptr %2879, i64 4
  store i32 0, ptr %2880, align 4
  %2881 = load { i32 }, ptr %next, align 4
  %2882 = load ptr, ptr %__panic1, align 8
  store { i32 } %2881, ptr %ptr_arg991, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg991, ptr %2882)
  %2883 = load ptr, ptr %__panic1, align 8
  %2884 = load i8, ptr %2883, align 1
  %2885 = load ptr, ptr %__panic1, align 8
  %2886 = getelementptr i8, ptr %2885, i64 4
  %2887 = load i32, ptr %2886, align 4
  %2888 = icmp ne i8 %2884, 0
  %2889 = and i1 true, %2888
  %2890 = zext i1 %2889 to i8
  %2891 = icmp ne i8 %2890, 0
  br i1 %2891, label %then992, label %else993

then992:                                          ; preds = %panic_fail990
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2887)
  br label %ifcont994

else993:                                          ; preds = %panic_fail990
  br label %ifcont994

ifcont994:                                        ; preds = %else993, %then992
  %2892 = phi {} [ zeroinitializer, %then992 ], [ zeroinitializer, %else993 ]
  %2893 = icmp ne i8 %2884, 0
  %2894 = xor i1 %2893, true
  %2895 = zext i1 %2894 to i8
  %2896 = icmp ne i8 %2895, 0
  %2897 = and i1 true, %2896
  %2898 = zext i1 %2897 to i8
  %2899 = icmp ne i8 %2898, 0
  br i1 %2899, label %then995, label %else996

then995:                                          ; preds = %ifcont994
  %2900 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2900, align 1
  %2901 = load ptr, ptr %__panic1, align 8
  %2902 = getelementptr i8, ptr %2901, i64 4
  store i32 %2878, ptr %2902, align 4
  br label %ifcont997

else996:                                          ; preds = %ifcont994
  br label %ifcont997

ifcont997:                                        ; preds = %else996, %then995
  %2903 = phi {} [ zeroinitializer, %then995 ], [ zeroinitializer, %else996 ]
  %2904 = icmp ne i8 %2884, 0
  %2905 = or i1 true, %2904
  %2906 = zext i1 %2905 to i8
  %2907 = icmp ne i8 %2884, 0
  br i1 %2907, label %then998, label %else999

then998:                                          ; preds = %ifcont997
  br label %ifcont1000

else999:                                          ; preds = %ifcont997
  br label %ifcont1000

ifcont1000:                                       ; preds = %else999, %then998
  %2908 = phi i32 [ %2887, %then998 ], [ %2878, %else999 ]
  %2909 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2909, align 1
  %2910 = getelementptr i8, ptr %2909, i64 4
  store i32 0, ptr %2910, align 4
  %2911 = load { i32 }, ptr %counter, align 4
  %2912 = load ptr, ptr %__panic1, align 8
  store { i32 } %2911, ptr %ptr_arg1001, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1001, ptr %2912)
  %2913 = load ptr, ptr %__panic1, align 8
  %2914 = load i8, ptr %2913, align 1
  %2915 = load ptr, ptr %__panic1, align 8
  %2916 = getelementptr i8, ptr %2915, i64 4
  %2917 = load i32, ptr %2916, align 4
  %2918 = icmp ne i8 %2906, 0
  %2919 = icmp ne i8 %2914, 0
  %2920 = and i1 %2918, %2919
  %2921 = zext i1 %2920 to i8
  %2922 = icmp ne i8 %2921, 0
  br i1 %2922, label %then1002, label %else1003

then1002:                                         ; preds = %ifcont1000
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2917)
  br label %ifcont1004

else1003:                                         ; preds = %ifcont1000
  br label %ifcont1004

ifcont1004:                                       ; preds = %else1003, %then1002
  %2923 = phi {} [ zeroinitializer, %then1002 ], [ zeroinitializer, %else1003 ]
  %2924 = icmp ne i8 %2914, 0
  %2925 = xor i1 %2924, true
  %2926 = zext i1 %2925 to i8
  %2927 = icmp ne i8 %2906, 0
  %2928 = icmp ne i8 %2926, 0
  %2929 = and i1 %2927, %2928
  %2930 = zext i1 %2929 to i8
  %2931 = icmp ne i8 %2930, 0
  br i1 %2931, label %then1005, label %else1006

then1005:                                         ; preds = %ifcont1004
  %2932 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2932, align 1
  %2933 = load ptr, ptr %__panic1, align 8
  %2934 = getelementptr i8, ptr %2933, i64 4
  store i32 %2908, ptr %2934, align 4
  br label %ifcont1007

else1006:                                         ; preds = %ifcont1004
  br label %ifcont1007

ifcont1007:                                       ; preds = %else1006, %then1005
  %2935 = phi {} [ zeroinitializer, %then1005 ], [ zeroinitializer, %else1006 ]
  %2936 = icmp ne i8 %2906, 0
  %2937 = icmp ne i8 %2914, 0
  %2938 = or i1 %2936, %2937
  %2939 = zext i1 %2938 to i8
  %2940 = icmp ne i8 %2914, 0
  br i1 %2940, label %then1008, label %else1009

then1008:                                         ; preds = %ifcont1007
  br label %ifcont1010

else1009:                                         ; preds = %ifcont1007
  br label %ifcont1010

ifcont1010:                                       ; preds = %else1009, %then1008
  %2941 = phi i32 [ %2917, %then1008 ], [ %2908, %else1009 ]
  ret i32 0

panic_cont1011:                                   ; preds = %panic_ok989
  br label %match_merge947

match_merge1013:                                  ; preds = %match_arm1017, %match_arm1015
  %2942 = phi {} [ zeroinitializer, %match_arm1015 ], [ zeroinitializer, %match_arm1017 ]
  %2943 = load ptr, ptr %__panic1, align 8
  %2944 = load i8, ptr %2943, align 1
  %2945 = load ptr, ptr %__panic1, align 8
  %2946 = getelementptr i8, ptr %2945, i64 4
  %2947 = load i32, ptr %2946, align 4
  %2948 = icmp ne i8 %2944, 0
  %2949 = and i1 false, %2948
  %2950 = zext i1 %2949 to i8
  %2951 = icmp ne i8 %2950, 0
  br i1 %2951, label %then1020, label %else1021

match_fail1014:                                   ; preds = %match_next1016
  unreachable

match_arm1015:                                    ; preds = %match_merge947
  br label %match_merge1013

match_next1016:                                   ; preds = %match_merge947
  %2952 = load i8, ptr %match_val1012, align 1
  %2953 = icmp eq i8 %2952, 1
  br i1 %2953, label %match_arm1017, label %match_fail1014

match_arm1017:                                    ; preds = %match_next1016
  store i16 %2758, ptr %tmp_addr1018, align 2
  %2954 = getelementptr i8, ptr %tmp_addr1018, i64 1
  %2955 = load i8, ptr %2954, align 1
  %2956 = load ptr, ptr %__panic1, align 8
  store i8 %2955, ptr %ptr_arg1019, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg1019, ptr %2956)
  br label %match_merge1013

then1020:                                         ; preds = %match_merge1013
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2947)
  br label %ifcont1022

else1021:                                         ; preds = %match_merge1013
  br label %ifcont1022

ifcont1022:                                       ; preds = %else1021, %then1020
  %2957 = phi {} [ zeroinitializer, %then1020 ], [ zeroinitializer, %else1021 ]
  %2958 = icmp ne i8 %2944, 0
  %2959 = xor i1 %2958, true
  %2960 = zext i1 %2959 to i8
  %2961 = icmp ne i8 %2960, 0
  %2962 = and i1 false, %2961
  %2963 = zext i1 %2962 to i8
  %2964 = icmp ne i8 %2963, 0
  br i1 %2964, label %then1023, label %else1024

then1023:                                         ; preds = %ifcont1022
  %2965 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %2965, align 1
  %2966 = load ptr, ptr %__panic1, align 8
  %2967 = getelementptr i8, ptr %2966, i64 4
  store i32 0, ptr %2967, align 4
  br label %ifcont1025

else1024:                                         ; preds = %ifcont1022
  br label %ifcont1025

ifcont1025:                                       ; preds = %else1024, %then1023
  %2968 = phi {} [ zeroinitializer, %then1023 ], [ zeroinitializer, %else1024 ]
  %2969 = icmp ne i8 %2944, 0
  %2970 = or i1 false, %2969
  %2971 = zext i1 %2970 to i8
  %2972 = icmp ne i8 %2944, 0
  br i1 %2972, label %then1026, label %else1027

then1026:                                         ; preds = %ifcont1025
  br label %ifcont1028

else1027:                                         ; preds = %ifcont1025
  br label %ifcont1028

ifcont1028:                                       ; preds = %else1027, %then1026
  %2973 = phi i32 [ %2947, %then1026 ], [ 0, %else1027 ]
  %2974 = load ptr, ptr %__panic1, align 8
  %2975 = load i8, ptr %2974, align 1
  %2976 = icmp ne i8 %2975, 0
  br i1 %2976, label %panic_fail1030, label %panic_ok1029

panic_ok1029:                                     ; preds = %ifcont1028
  br label %panic_cont1051

panic_fail1030:                                   ; preds = %ifcont1028
  %2977 = load ptr, ptr %__panic1, align 8
  %2978 = load i8, ptr %2977, align 1
  %2979 = load ptr, ptr %__panic1, align 8
  %2980 = getelementptr i8, ptr %2979, i64 4
  %2981 = load i32, ptr %2980, align 4
  %2982 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %2982, align 1
  %2983 = getelementptr i8, ptr %2982, i64 4
  store i32 0, ptr %2983, align 4
  %2984 = load { i32 }, ptr %next, align 4
  %2985 = load ptr, ptr %__panic1, align 8
  store { i32 } %2984, ptr %ptr_arg1031, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1031, ptr %2985)
  %2986 = load ptr, ptr %__panic1, align 8
  %2987 = load i8, ptr %2986, align 1
  %2988 = load ptr, ptr %__panic1, align 8
  %2989 = getelementptr i8, ptr %2988, i64 4
  %2990 = load i32, ptr %2989, align 4
  %2991 = icmp ne i8 %2987, 0
  %2992 = and i1 true, %2991
  %2993 = zext i1 %2992 to i8
  %2994 = icmp ne i8 %2993, 0
  br i1 %2994, label %then1032, label %else1033

then1032:                                         ; preds = %panic_fail1030
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %2990)
  br label %ifcont1034

else1033:                                         ; preds = %panic_fail1030
  br label %ifcont1034

ifcont1034:                                       ; preds = %else1033, %then1032
  %2995 = phi {} [ zeroinitializer, %then1032 ], [ zeroinitializer, %else1033 ]
  %2996 = icmp ne i8 %2987, 0
  %2997 = xor i1 %2996, true
  %2998 = zext i1 %2997 to i8
  %2999 = icmp ne i8 %2998, 0
  %3000 = and i1 true, %2999
  %3001 = zext i1 %3000 to i8
  %3002 = icmp ne i8 %3001, 0
  br i1 %3002, label %then1035, label %else1036

then1035:                                         ; preds = %ifcont1034
  %3003 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3003, align 1
  %3004 = load ptr, ptr %__panic1, align 8
  %3005 = getelementptr i8, ptr %3004, i64 4
  store i32 %2981, ptr %3005, align 4
  br label %ifcont1037

else1036:                                         ; preds = %ifcont1034
  br label %ifcont1037

ifcont1037:                                       ; preds = %else1036, %then1035
  %3006 = phi {} [ zeroinitializer, %then1035 ], [ zeroinitializer, %else1036 ]
  %3007 = icmp ne i8 %2987, 0
  %3008 = or i1 true, %3007
  %3009 = zext i1 %3008 to i8
  %3010 = icmp ne i8 %2987, 0
  br i1 %3010, label %then1038, label %else1039

then1038:                                         ; preds = %ifcont1037
  br label %ifcont1040

else1039:                                         ; preds = %ifcont1037
  br label %ifcont1040

ifcont1040:                                       ; preds = %else1039, %then1038
  %3011 = phi i32 [ %2990, %then1038 ], [ %2981, %else1039 ]
  %3012 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3012, align 1
  %3013 = getelementptr i8, ptr %3012, i64 4
  store i32 0, ptr %3013, align 4
  %3014 = load { i32 }, ptr %counter, align 4
  %3015 = load ptr, ptr %__panic1, align 8
  store { i32 } %3014, ptr %ptr_arg1041, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1041, ptr %3015)
  %3016 = load ptr, ptr %__panic1, align 8
  %3017 = load i8, ptr %3016, align 1
  %3018 = load ptr, ptr %__panic1, align 8
  %3019 = getelementptr i8, ptr %3018, i64 4
  %3020 = load i32, ptr %3019, align 4
  %3021 = icmp ne i8 %3009, 0
  %3022 = icmp ne i8 %3017, 0
  %3023 = and i1 %3021, %3022
  %3024 = zext i1 %3023 to i8
  %3025 = icmp ne i8 %3024, 0
  br i1 %3025, label %then1042, label %else1043

then1042:                                         ; preds = %ifcont1040
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3020)
  br label %ifcont1044

else1043:                                         ; preds = %ifcont1040
  br label %ifcont1044

ifcont1044:                                       ; preds = %else1043, %then1042
  %3026 = phi {} [ zeroinitializer, %then1042 ], [ zeroinitializer, %else1043 ]
  %3027 = icmp ne i8 %3017, 0
  %3028 = xor i1 %3027, true
  %3029 = zext i1 %3028 to i8
  %3030 = icmp ne i8 %3009, 0
  %3031 = icmp ne i8 %3029, 0
  %3032 = and i1 %3030, %3031
  %3033 = zext i1 %3032 to i8
  %3034 = icmp ne i8 %3033, 0
  br i1 %3034, label %then1045, label %else1046

then1045:                                         ; preds = %ifcont1044
  %3035 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3035, align 1
  %3036 = load ptr, ptr %__panic1, align 8
  %3037 = getelementptr i8, ptr %3036, i64 4
  store i32 %3011, ptr %3037, align 4
  br label %ifcont1047

else1046:                                         ; preds = %ifcont1044
  br label %ifcont1047

ifcont1047:                                       ; preds = %else1046, %then1045
  %3038 = phi {} [ zeroinitializer, %then1045 ], [ zeroinitializer, %else1046 ]
  %3039 = icmp ne i8 %3009, 0
  %3040 = icmp ne i8 %3017, 0
  %3041 = or i1 %3039, %3040
  %3042 = zext i1 %3041 to i8
  %3043 = icmp ne i8 %3017, 0
  br i1 %3043, label %then1048, label %else1049

then1048:                                         ; preds = %ifcont1047
  br label %ifcont1050

else1049:                                         ; preds = %ifcont1047
  br label %ifcont1050

ifcont1050:                                       ; preds = %else1049, %then1048
  %3044 = phi i32 [ %3020, %then1048 ], [ %3011, %else1049 ]
  ret i32 0

panic_cont1051:                                   ; preds = %panic_ok1029
  %3045 = load { ptr, i64 }, ptr %region_msg, align 8
  store { ptr, i64 } %3045, ptr %byref_arg1052, align 8
  %3046 = call i16 @cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout(ptr %ctx, ptr %byref_arg1052)
  store i16 %3046, ptr %match_val1053, align 2
  %3047 = load i8, ptr %match_val1053, align 1
  %3048 = icmp eq i8 %3047, 0
  br i1 %3048, label %match_arm1056, label %match_next1057

match_merge1054:                                  ; preds = %panic_cont1118, %panic_cont1082
  %3049 = phi {} [ zeroinitializer, %panic_cont1082 ], [ zeroinitializer, %panic_cont1118 ]
  %3050 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3050, align 1
  %3051 = getelementptr i8, ptr %3050, i64 4
  store i32 0, ptr %3051, align 4
  store i16 %3046, ptr %match_val1119, align 2
  %3052 = load i8, ptr %match_val1119, align 1
  %3053 = icmp eq i8 %3052, 0
  br i1 %3053, label %match_arm1122, label %match_next1123

match_fail1055:                                   ; preds = %match_next1057
  unreachable

match_arm1056:                                    ; preds = %panic_cont1051
  store i16 %3046, ptr %tmp_addr1058, align 2
  store {} zeroinitializer, ptr %ok1059, align 1
  store {} zeroinitializer, ptr %ok1059, align 1
  %3054 = load ptr, ptr %__panic1, align 8
  %3055 = load i8, ptr %3054, align 1
  %3056 = icmp ne i8 %3055, 0
  br i1 %3056, label %panic_fail1061, label %panic_ok1060

match_next1057:                                   ; preds = %panic_cont1051
  %3057 = load i8, ptr %match_val1053, align 1
  %3058 = icmp eq i8 %3057, 1
  br i1 %3058, label %match_arm1083, label %match_fail1055

panic_ok1060:                                     ; preds = %match_arm1056
  br label %panic_cont1082

panic_fail1061:                                   ; preds = %match_arm1056
  %3059 = load ptr, ptr %__panic1, align 8
  %3060 = load i8, ptr %3059, align 1
  %3061 = load ptr, ptr %__panic1, align 8
  %3062 = getelementptr i8, ptr %3061, i64 4
  %3063 = load i32, ptr %3062, align 4
  %3064 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3064, align 1
  %3065 = getelementptr i8, ptr %3064, i64 4
  store i32 0, ptr %3065, align 4
  %3066 = load { i32 }, ptr %next, align 4
  %3067 = load ptr, ptr %__panic1, align 8
  store { i32 } %3066, ptr %ptr_arg1062, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1062, ptr %3067)
  %3068 = load ptr, ptr %__panic1, align 8
  %3069 = load i8, ptr %3068, align 1
  %3070 = load ptr, ptr %__panic1, align 8
  %3071 = getelementptr i8, ptr %3070, i64 4
  %3072 = load i32, ptr %3071, align 4
  %3073 = icmp ne i8 %3069, 0
  %3074 = and i1 true, %3073
  %3075 = zext i1 %3074 to i8
  %3076 = icmp ne i8 %3075, 0
  br i1 %3076, label %then1063, label %else1064

then1063:                                         ; preds = %panic_fail1061
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3072)
  br label %ifcont1065

else1064:                                         ; preds = %panic_fail1061
  br label %ifcont1065

ifcont1065:                                       ; preds = %else1064, %then1063
  %3077 = phi {} [ zeroinitializer, %then1063 ], [ zeroinitializer, %else1064 ]
  %3078 = icmp ne i8 %3069, 0
  %3079 = xor i1 %3078, true
  %3080 = zext i1 %3079 to i8
  %3081 = icmp ne i8 %3080, 0
  %3082 = and i1 true, %3081
  %3083 = zext i1 %3082 to i8
  %3084 = icmp ne i8 %3083, 0
  br i1 %3084, label %then1066, label %else1067

then1066:                                         ; preds = %ifcont1065
  %3085 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3085, align 1
  %3086 = load ptr, ptr %__panic1, align 8
  %3087 = getelementptr i8, ptr %3086, i64 4
  store i32 %3063, ptr %3087, align 4
  br label %ifcont1068

else1067:                                         ; preds = %ifcont1065
  br label %ifcont1068

ifcont1068:                                       ; preds = %else1067, %then1066
  %3088 = phi {} [ zeroinitializer, %then1066 ], [ zeroinitializer, %else1067 ]
  %3089 = icmp ne i8 %3069, 0
  %3090 = or i1 true, %3089
  %3091 = zext i1 %3090 to i8
  %3092 = icmp ne i8 %3069, 0
  br i1 %3092, label %then1069, label %else1070

then1069:                                         ; preds = %ifcont1068
  br label %ifcont1071

else1070:                                         ; preds = %ifcont1068
  br label %ifcont1071

ifcont1071:                                       ; preds = %else1070, %then1069
  %3093 = phi i32 [ %3072, %then1069 ], [ %3063, %else1070 ]
  %3094 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3094, align 1
  %3095 = getelementptr i8, ptr %3094, i64 4
  store i32 0, ptr %3095, align 4
  %3096 = load { i32 }, ptr %counter, align 4
  %3097 = load ptr, ptr %__panic1, align 8
  store { i32 } %3096, ptr %ptr_arg1072, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1072, ptr %3097)
  %3098 = load ptr, ptr %__panic1, align 8
  %3099 = load i8, ptr %3098, align 1
  %3100 = load ptr, ptr %__panic1, align 8
  %3101 = getelementptr i8, ptr %3100, i64 4
  %3102 = load i32, ptr %3101, align 4
  %3103 = icmp ne i8 %3091, 0
  %3104 = icmp ne i8 %3099, 0
  %3105 = and i1 %3103, %3104
  %3106 = zext i1 %3105 to i8
  %3107 = icmp ne i8 %3106, 0
  br i1 %3107, label %then1073, label %else1074

then1073:                                         ; preds = %ifcont1071
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3102)
  br label %ifcont1075

else1074:                                         ; preds = %ifcont1071
  br label %ifcont1075

ifcont1075:                                       ; preds = %else1074, %then1073
  %3108 = phi {} [ zeroinitializer, %then1073 ], [ zeroinitializer, %else1074 ]
  %3109 = icmp ne i8 %3099, 0
  %3110 = xor i1 %3109, true
  %3111 = zext i1 %3110 to i8
  %3112 = icmp ne i8 %3091, 0
  %3113 = icmp ne i8 %3111, 0
  %3114 = and i1 %3112, %3113
  %3115 = zext i1 %3114 to i8
  %3116 = icmp ne i8 %3115, 0
  br i1 %3116, label %then1076, label %else1077

then1076:                                         ; preds = %ifcont1075
  %3117 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3117, align 1
  %3118 = load ptr, ptr %__panic1, align 8
  %3119 = getelementptr i8, ptr %3118, i64 4
  store i32 %3093, ptr %3119, align 4
  br label %ifcont1078

else1077:                                         ; preds = %ifcont1075
  br label %ifcont1078

ifcont1078:                                       ; preds = %else1077, %then1076
  %3120 = phi {} [ zeroinitializer, %then1076 ], [ zeroinitializer, %else1077 ]
  %3121 = icmp ne i8 %3091, 0
  %3122 = icmp ne i8 %3099, 0
  %3123 = or i1 %3121, %3122
  %3124 = zext i1 %3123 to i8
  %3125 = icmp ne i8 %3099, 0
  br i1 %3125, label %then1079, label %else1080

then1079:                                         ; preds = %ifcont1078
  br label %ifcont1081

else1080:                                         ; preds = %ifcont1078
  br label %ifcont1081

ifcont1081:                                       ; preds = %else1080, %then1079
  %3126 = phi i32 [ %3102, %then1079 ], [ %3093, %else1080 ]
  ret i32 0

panic_cont1082:                                   ; preds = %panic_ok1060
  br label %match_merge1054

match_arm1083:                                    ; preds = %match_next1057
  store i16 %3046, ptr %tmp_addr1084, align 2
  %3127 = getelementptr i8, ptr %tmp_addr1084, i64 1
  %3128 = load i8, ptr %3127, align 1
  store i8 0, ptr %err1085, align 1
  store i8 %3128, ptr %err1085, align 1
  %3129 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3129, align 1
  %3130 = getelementptr i8, ptr %3129, i64 4
  store i32 0, ptr %3130, align 4
  %3131 = load i8, ptr %err1085, align 1
  %3132 = load ptr, ptr %__panic1, align 8
  store i8 %3131, ptr %ptr_arg1086, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg1086, ptr %3132)
  %3133 = load ptr, ptr %__panic1, align 8
  %3134 = load i8, ptr %3133, align 1
  %3135 = load ptr, ptr %__panic1, align 8
  %3136 = getelementptr i8, ptr %3135, i64 4
  %3137 = load i32, ptr %3136, align 4
  %3138 = icmp ne i8 %3134, 0
  %3139 = and i1 false, %3138
  %3140 = zext i1 %3139 to i8
  %3141 = icmp ne i8 %3140, 0
  br i1 %3141, label %then1087, label %else1088

then1087:                                         ; preds = %match_arm1083
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3137)
  br label %ifcont1089

else1088:                                         ; preds = %match_arm1083
  br label %ifcont1089

ifcont1089:                                       ; preds = %else1088, %then1087
  %3142 = phi {} [ zeroinitializer, %then1087 ], [ zeroinitializer, %else1088 ]
  %3143 = icmp ne i8 %3134, 0
  %3144 = xor i1 %3143, true
  %3145 = zext i1 %3144 to i8
  %3146 = icmp ne i8 %3145, 0
  %3147 = and i1 false, %3146
  %3148 = zext i1 %3147 to i8
  %3149 = icmp ne i8 %3148, 0
  br i1 %3149, label %then1090, label %else1091

then1090:                                         ; preds = %ifcont1089
  %3150 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3150, align 1
  %3151 = load ptr, ptr %__panic1, align 8
  %3152 = getelementptr i8, ptr %3151, i64 4
  store i32 0, ptr %3152, align 4
  br label %ifcont1092

else1091:                                         ; preds = %ifcont1089
  br label %ifcont1092

ifcont1092:                                       ; preds = %else1091, %then1090
  %3153 = phi {} [ zeroinitializer, %then1090 ], [ zeroinitializer, %else1091 ]
  %3154 = icmp ne i8 %3134, 0
  %3155 = or i1 false, %3154
  %3156 = zext i1 %3155 to i8
  %3157 = icmp ne i8 %3134, 0
  br i1 %3157, label %then1093, label %else1094

then1093:                                         ; preds = %ifcont1092
  br label %ifcont1095

else1094:                                         ; preds = %ifcont1092
  br label %ifcont1095

ifcont1095:                                       ; preds = %else1094, %then1093
  %3158 = phi i32 [ %3137, %then1093 ], [ 0, %else1094 ]
  %3159 = load ptr, ptr %__panic1, align 8
  %3160 = load i8, ptr %3159, align 1
  %3161 = icmp ne i8 %3160, 0
  br i1 %3161, label %panic_fail1097, label %panic_ok1096

panic_ok1096:                                     ; preds = %ifcont1095
  br label %panic_cont1118

panic_fail1097:                                   ; preds = %ifcont1095
  %3162 = load ptr, ptr %__panic1, align 8
  %3163 = load i8, ptr %3162, align 1
  %3164 = load ptr, ptr %__panic1, align 8
  %3165 = getelementptr i8, ptr %3164, i64 4
  %3166 = load i32, ptr %3165, align 4
  %3167 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3167, align 1
  %3168 = getelementptr i8, ptr %3167, i64 4
  store i32 0, ptr %3168, align 4
  %3169 = load { i32 }, ptr %next, align 4
  %3170 = load ptr, ptr %__panic1, align 8
  store { i32 } %3169, ptr %ptr_arg1098, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1098, ptr %3170)
  %3171 = load ptr, ptr %__panic1, align 8
  %3172 = load i8, ptr %3171, align 1
  %3173 = load ptr, ptr %__panic1, align 8
  %3174 = getelementptr i8, ptr %3173, i64 4
  %3175 = load i32, ptr %3174, align 4
  %3176 = icmp ne i8 %3172, 0
  %3177 = and i1 true, %3176
  %3178 = zext i1 %3177 to i8
  %3179 = icmp ne i8 %3178, 0
  br i1 %3179, label %then1099, label %else1100

then1099:                                         ; preds = %panic_fail1097
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3175)
  br label %ifcont1101

else1100:                                         ; preds = %panic_fail1097
  br label %ifcont1101

ifcont1101:                                       ; preds = %else1100, %then1099
  %3180 = phi {} [ zeroinitializer, %then1099 ], [ zeroinitializer, %else1100 ]
  %3181 = icmp ne i8 %3172, 0
  %3182 = xor i1 %3181, true
  %3183 = zext i1 %3182 to i8
  %3184 = icmp ne i8 %3183, 0
  %3185 = and i1 true, %3184
  %3186 = zext i1 %3185 to i8
  %3187 = icmp ne i8 %3186, 0
  br i1 %3187, label %then1102, label %else1103

then1102:                                         ; preds = %ifcont1101
  %3188 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3188, align 1
  %3189 = load ptr, ptr %__panic1, align 8
  %3190 = getelementptr i8, ptr %3189, i64 4
  store i32 %3166, ptr %3190, align 4
  br label %ifcont1104

else1103:                                         ; preds = %ifcont1101
  br label %ifcont1104

ifcont1104:                                       ; preds = %else1103, %then1102
  %3191 = phi {} [ zeroinitializer, %then1102 ], [ zeroinitializer, %else1103 ]
  %3192 = icmp ne i8 %3172, 0
  %3193 = or i1 true, %3192
  %3194 = zext i1 %3193 to i8
  %3195 = icmp ne i8 %3172, 0
  br i1 %3195, label %then1105, label %else1106

then1105:                                         ; preds = %ifcont1104
  br label %ifcont1107

else1106:                                         ; preds = %ifcont1104
  br label %ifcont1107

ifcont1107:                                       ; preds = %else1106, %then1105
  %3196 = phi i32 [ %3175, %then1105 ], [ %3166, %else1106 ]
  %3197 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3197, align 1
  %3198 = getelementptr i8, ptr %3197, i64 4
  store i32 0, ptr %3198, align 4
  %3199 = load { i32 }, ptr %counter, align 4
  %3200 = load ptr, ptr %__panic1, align 8
  store { i32 } %3199, ptr %ptr_arg1108, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1108, ptr %3200)
  %3201 = load ptr, ptr %__panic1, align 8
  %3202 = load i8, ptr %3201, align 1
  %3203 = load ptr, ptr %__panic1, align 8
  %3204 = getelementptr i8, ptr %3203, i64 4
  %3205 = load i32, ptr %3204, align 4
  %3206 = icmp ne i8 %3194, 0
  %3207 = icmp ne i8 %3202, 0
  %3208 = and i1 %3206, %3207
  %3209 = zext i1 %3208 to i8
  %3210 = icmp ne i8 %3209, 0
  br i1 %3210, label %then1109, label %else1110

then1109:                                         ; preds = %ifcont1107
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3205)
  br label %ifcont1111

else1110:                                         ; preds = %ifcont1107
  br label %ifcont1111

ifcont1111:                                       ; preds = %else1110, %then1109
  %3211 = phi {} [ zeroinitializer, %then1109 ], [ zeroinitializer, %else1110 ]
  %3212 = icmp ne i8 %3202, 0
  %3213 = xor i1 %3212, true
  %3214 = zext i1 %3213 to i8
  %3215 = icmp ne i8 %3194, 0
  %3216 = icmp ne i8 %3214, 0
  %3217 = and i1 %3215, %3216
  %3218 = zext i1 %3217 to i8
  %3219 = icmp ne i8 %3218, 0
  br i1 %3219, label %then1112, label %else1113

then1112:                                         ; preds = %ifcont1111
  %3220 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3220, align 1
  %3221 = load ptr, ptr %__panic1, align 8
  %3222 = getelementptr i8, ptr %3221, i64 4
  store i32 %3196, ptr %3222, align 4
  br label %ifcont1114

else1113:                                         ; preds = %ifcont1111
  br label %ifcont1114

ifcont1114:                                       ; preds = %else1113, %then1112
  %3223 = phi {} [ zeroinitializer, %then1112 ], [ zeroinitializer, %else1113 ]
  %3224 = icmp ne i8 %3194, 0
  %3225 = icmp ne i8 %3202, 0
  %3226 = or i1 %3224, %3225
  %3227 = zext i1 %3226 to i8
  %3228 = icmp ne i8 %3202, 0
  br i1 %3228, label %then1115, label %else1116

then1115:                                         ; preds = %ifcont1114
  br label %ifcont1117

else1116:                                         ; preds = %ifcont1114
  br label %ifcont1117

ifcont1117:                                       ; preds = %else1116, %then1115
  %3229 = phi i32 [ %3205, %then1115 ], [ %3196, %else1116 ]
  ret i32 0

panic_cont1118:                                   ; preds = %panic_ok1096
  br label %match_merge1054

match_merge1120:                                  ; preds = %match_arm1124, %match_arm1122
  %3230 = phi {} [ zeroinitializer, %match_arm1122 ], [ zeroinitializer, %match_arm1124 ]
  %3231 = load ptr, ptr %__panic1, align 8
  %3232 = load i8, ptr %3231, align 1
  %3233 = load ptr, ptr %__panic1, align 8
  %3234 = getelementptr i8, ptr %3233, i64 4
  %3235 = load i32, ptr %3234, align 4
  %3236 = icmp ne i8 %3232, 0
  %3237 = and i1 false, %3236
  %3238 = zext i1 %3237 to i8
  %3239 = icmp ne i8 %3238, 0
  br i1 %3239, label %then1127, label %else1128

match_fail1121:                                   ; preds = %match_next1123
  unreachable

match_arm1122:                                    ; preds = %match_merge1054
  br label %match_merge1120

match_next1123:                                   ; preds = %match_merge1054
  %3240 = load i8, ptr %match_val1119, align 1
  %3241 = icmp eq i8 %3240, 1
  br i1 %3241, label %match_arm1124, label %match_fail1121

match_arm1124:                                    ; preds = %match_next1123
  store i16 %3046, ptr %tmp_addr1125, align 2
  %3242 = getelementptr i8, ptr %tmp_addr1125, i64 1
  %3243 = load i8, ptr %3242, align 1
  %3244 = load ptr, ptr %__panic1, align 8
  store i8 %3243, ptr %ptr_arg1126, align 1
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError(ptr %ptr_arg1126, ptr %3244)
  br label %match_merge1120

then1127:                                         ; preds = %match_merge1120
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3235)
  br label %ifcont1129

else1128:                                         ; preds = %match_merge1120
  br label %ifcont1129

ifcont1129:                                       ; preds = %else1128, %then1127
  %3245 = phi {} [ zeroinitializer, %then1127 ], [ zeroinitializer, %else1128 ]
  %3246 = icmp ne i8 %3232, 0
  %3247 = xor i1 %3246, true
  %3248 = zext i1 %3247 to i8
  %3249 = icmp ne i8 %3248, 0
  %3250 = and i1 false, %3249
  %3251 = zext i1 %3250 to i8
  %3252 = icmp ne i8 %3251, 0
  br i1 %3252, label %then1130, label %else1131

then1130:                                         ; preds = %ifcont1129
  %3253 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3253, align 1
  %3254 = load ptr, ptr %__panic1, align 8
  %3255 = getelementptr i8, ptr %3254, i64 4
  store i32 0, ptr %3255, align 4
  br label %ifcont1132

else1131:                                         ; preds = %ifcont1129
  br label %ifcont1132

ifcont1132:                                       ; preds = %else1131, %then1130
  %3256 = phi {} [ zeroinitializer, %then1130 ], [ zeroinitializer, %else1131 ]
  %3257 = icmp ne i8 %3232, 0
  %3258 = or i1 false, %3257
  %3259 = zext i1 %3258 to i8
  %3260 = icmp ne i8 %3232, 0
  br i1 %3260, label %then1133, label %else1134

then1133:                                         ; preds = %ifcont1132
  br label %ifcont1135

else1134:                                         ; preds = %ifcont1132
  br label %ifcont1135

ifcont1135:                                       ; preds = %else1134, %then1133
  %3261 = phi i32 [ %3235, %then1133 ], [ 0, %else1134 ]
  %3262 = load ptr, ptr %__panic1, align 8
  %3263 = load i8, ptr %3262, align 1
  %3264 = icmp ne i8 %3263, 0
  br i1 %3264, label %panic_fail1137, label %panic_ok1136

panic_ok1136:                                     ; preds = %ifcont1135
  br label %panic_cont1158

panic_fail1137:                                   ; preds = %ifcont1135
  %3265 = load ptr, ptr %__panic1, align 8
  %3266 = load i8, ptr %3265, align 1
  %3267 = load ptr, ptr %__panic1, align 8
  %3268 = getelementptr i8, ptr %3267, i64 4
  %3269 = load i32, ptr %3268, align 4
  %3270 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3270, align 1
  %3271 = getelementptr i8, ptr %3270, i64 4
  store i32 0, ptr %3271, align 4
  %3272 = load { i32 }, ptr %next, align 4
  %3273 = load ptr, ptr %__panic1, align 8
  store { i32 } %3272, ptr %ptr_arg1138, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1138, ptr %3273)
  %3274 = load ptr, ptr %__panic1, align 8
  %3275 = load i8, ptr %3274, align 1
  %3276 = load ptr, ptr %__panic1, align 8
  %3277 = getelementptr i8, ptr %3276, i64 4
  %3278 = load i32, ptr %3277, align 4
  %3279 = icmp ne i8 %3275, 0
  %3280 = and i1 true, %3279
  %3281 = zext i1 %3280 to i8
  %3282 = icmp ne i8 %3281, 0
  br i1 %3282, label %then1139, label %else1140

then1139:                                         ; preds = %panic_fail1137
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3278)
  br label %ifcont1141

else1140:                                         ; preds = %panic_fail1137
  br label %ifcont1141

ifcont1141:                                       ; preds = %else1140, %then1139
  %3283 = phi {} [ zeroinitializer, %then1139 ], [ zeroinitializer, %else1140 ]
  %3284 = icmp ne i8 %3275, 0
  %3285 = xor i1 %3284, true
  %3286 = zext i1 %3285 to i8
  %3287 = icmp ne i8 %3286, 0
  %3288 = and i1 true, %3287
  %3289 = zext i1 %3288 to i8
  %3290 = icmp ne i8 %3289, 0
  br i1 %3290, label %then1142, label %else1143

then1142:                                         ; preds = %ifcont1141
  %3291 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3291, align 1
  %3292 = load ptr, ptr %__panic1, align 8
  %3293 = getelementptr i8, ptr %3292, i64 4
  store i32 %3269, ptr %3293, align 4
  br label %ifcont1144

else1143:                                         ; preds = %ifcont1141
  br label %ifcont1144

ifcont1144:                                       ; preds = %else1143, %then1142
  %3294 = phi {} [ zeroinitializer, %then1142 ], [ zeroinitializer, %else1143 ]
  %3295 = icmp ne i8 %3275, 0
  %3296 = or i1 true, %3295
  %3297 = zext i1 %3296 to i8
  %3298 = icmp ne i8 %3275, 0
  br i1 %3298, label %then1145, label %else1146

then1145:                                         ; preds = %ifcont1144
  br label %ifcont1147

else1146:                                         ; preds = %ifcont1144
  br label %ifcont1147

ifcont1147:                                       ; preds = %else1146, %then1145
  %3299 = phi i32 [ %3278, %then1145 ], [ %3269, %else1146 ]
  %3300 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3300, align 1
  %3301 = getelementptr i8, ptr %3300, i64 4
  store i32 0, ptr %3301, align 4
  %3302 = load { i32 }, ptr %counter, align 4
  %3303 = load ptr, ptr %__panic1, align 8
  store { i32 } %3302, ptr %ptr_arg1148, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1148, ptr %3303)
  %3304 = load ptr, ptr %__panic1, align 8
  %3305 = load i8, ptr %3304, align 1
  %3306 = load ptr, ptr %__panic1, align 8
  %3307 = getelementptr i8, ptr %3306, i64 4
  %3308 = load i32, ptr %3307, align 4
  %3309 = icmp ne i8 %3297, 0
  %3310 = icmp ne i8 %3305, 0
  %3311 = and i1 %3309, %3310
  %3312 = zext i1 %3311 to i8
  %3313 = icmp ne i8 %3312, 0
  br i1 %3313, label %then1149, label %else1150

then1149:                                         ; preds = %ifcont1147
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3308)
  br label %ifcont1151

else1150:                                         ; preds = %ifcont1147
  br label %ifcont1151

ifcont1151:                                       ; preds = %else1150, %then1149
  %3314 = phi {} [ zeroinitializer, %then1149 ], [ zeroinitializer, %else1150 ]
  %3315 = icmp ne i8 %3305, 0
  %3316 = xor i1 %3315, true
  %3317 = zext i1 %3316 to i8
  %3318 = icmp ne i8 %3297, 0
  %3319 = icmp ne i8 %3317, 0
  %3320 = and i1 %3318, %3319
  %3321 = zext i1 %3320 to i8
  %3322 = icmp ne i8 %3321, 0
  br i1 %3322, label %then1152, label %else1153

then1152:                                         ; preds = %ifcont1151
  %3323 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3323, align 1
  %3324 = load ptr, ptr %__panic1, align 8
  %3325 = getelementptr i8, ptr %3324, i64 4
  store i32 %3299, ptr %3325, align 4
  br label %ifcont1154

else1153:                                         ; preds = %ifcont1151
  br label %ifcont1154

ifcont1154:                                       ; preds = %else1153, %then1152
  %3326 = phi {} [ zeroinitializer, %then1152 ], [ zeroinitializer, %else1153 ]
  %3327 = icmp ne i8 %3297, 0
  %3328 = icmp ne i8 %3305, 0
  %3329 = or i1 %3327, %3328
  %3330 = zext i1 %3329 to i8
  %3331 = icmp ne i8 %3305, 0
  br i1 %3331, label %then1155, label %else1156

then1155:                                         ; preds = %ifcont1154
  br label %ifcont1157

else1156:                                         ; preds = %ifcont1154
  br label %ifcont1157

ifcont1157:                                       ; preds = %else1156, %then1155
  %3332 = phi i32 [ %3308, %then1155 ], [ %3299, %else1156 ]
  ret i32 0

panic_cont1158:                                   ; preds = %panic_ok1136
  %3333 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3333, align 1
  %3334 = getelementptr i8, ptr %3333, i64 4
  store i32 0, ptr %3334, align 4
  %3335 = load { i32 }, ptr %next, align 4
  %3336 = load ptr, ptr %__panic1, align 8
  store { i32 } %3335, ptr %ptr_arg1159, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1159, ptr %3336)
  %3337 = load ptr, ptr %__panic1, align 8
  %3338 = load i8, ptr %3337, align 1
  %3339 = load ptr, ptr %__panic1, align 8
  %3340 = getelementptr i8, ptr %3339, i64 4
  %3341 = load i32, ptr %3340, align 4
  %3342 = icmp ne i8 %3338, 0
  %3343 = and i1 false, %3342
  %3344 = zext i1 %3343 to i8
  %3345 = icmp ne i8 %3344, 0
  br i1 %3345, label %then1160, label %else1161

then1160:                                         ; preds = %panic_cont1158
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3341)
  br label %ifcont1162

else1161:                                         ; preds = %panic_cont1158
  br label %ifcont1162

ifcont1162:                                       ; preds = %else1161, %then1160
  %3346 = phi {} [ zeroinitializer, %then1160 ], [ zeroinitializer, %else1161 ]
  %3347 = icmp ne i8 %3338, 0
  %3348 = xor i1 %3347, true
  %3349 = zext i1 %3348 to i8
  %3350 = icmp ne i8 %3349, 0
  %3351 = and i1 false, %3350
  %3352 = zext i1 %3351 to i8
  %3353 = icmp ne i8 %3352, 0
  br i1 %3353, label %then1163, label %else1164

then1163:                                         ; preds = %ifcont1162
  %3354 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3354, align 1
  %3355 = load ptr, ptr %__panic1, align 8
  %3356 = getelementptr i8, ptr %3355, i64 4
  store i32 0, ptr %3356, align 4
  br label %ifcont1165

else1164:                                         ; preds = %ifcont1162
  br label %ifcont1165

ifcont1165:                                       ; preds = %else1164, %then1163
  %3357 = phi {} [ zeroinitializer, %then1163 ], [ zeroinitializer, %else1164 ]
  %3358 = icmp ne i8 %3338, 0
  %3359 = or i1 false, %3358
  %3360 = zext i1 %3359 to i8
  %3361 = icmp ne i8 %3338, 0
  br i1 %3361, label %then1166, label %else1167

then1166:                                         ; preds = %ifcont1165
  br label %ifcont1168

else1167:                                         ; preds = %ifcont1165
  br label %ifcont1168

ifcont1168:                                       ; preds = %else1167, %then1166
  %3362 = phi i32 [ %3341, %then1166 ], [ 0, %else1167 ]
  %3363 = load ptr, ptr %__panic1, align 8
  store i8 0, ptr %3363, align 1
  %3364 = getelementptr i8, ptr %3363, i64 4
  store i32 0, ptr %3364, align 4
  %3365 = load { i32 }, ptr %counter, align 4
  %3366 = load ptr, ptr %__panic1, align 8
  store { i32 } %3365, ptr %ptr_arg1169, align 4
  call void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %ptr_arg1169, ptr %3366)
  %3367 = load ptr, ptr %__panic1, align 8
  %3368 = load i8, ptr %3367, align 1
  %3369 = load ptr, ptr %__panic1, align 8
  %3370 = getelementptr i8, ptr %3369, i64 4
  %3371 = load i32, ptr %3370, align 4
  %3372 = icmp ne i8 %3360, 0
  %3373 = icmp ne i8 %3368, 0
  %3374 = and i1 %3372, %3373
  %3375 = zext i1 %3374 to i8
  %3376 = icmp ne i8 %3375, 0
  br i1 %3376, label %then1170, label %else1171

then1170:                                         ; preds = %ifcont1168
  call void @cursive_x3a_x3aruntime_x3a_x3apanic(i32 %3371)
  br label %ifcont1172

else1171:                                         ; preds = %ifcont1168
  br label %ifcont1172

ifcont1172:                                       ; preds = %else1171, %then1170
  %3377 = phi {} [ zeroinitializer, %then1170 ], [ zeroinitializer, %else1171 ]
  %3378 = icmp ne i8 %3368, 0
  %3379 = xor i1 %3378, true
  %3380 = zext i1 %3379 to i8
  %3381 = icmp ne i8 %3360, 0
  %3382 = icmp ne i8 %3380, 0
  %3383 = and i1 %3381, %3382
  %3384 = zext i1 %3383 to i8
  %3385 = icmp ne i8 %3384, 0
  br i1 %3385, label %then1173, label %else1174

then1173:                                         ; preds = %ifcont1172
  %3386 = load ptr, ptr %__panic1, align 8
  store i8 1, ptr %3386, align 1
  %3387 = load ptr, ptr %__panic1, align 8
  %3388 = getelementptr i8, ptr %3387, i64 4
  store i32 %3362, ptr %3388, align 4
  br label %ifcont1175

else1174:                                         ; preds = %ifcont1172
  br label %ifcont1175

ifcont1175:                                       ; preds = %else1174, %then1173
  %3389 = phi {} [ zeroinitializer, %then1173 ], [ zeroinitializer, %else1174 ]
  %3390 = icmp ne i8 %3360, 0
  %3391 = icmp ne i8 %3368, 0
  %3392 = or i1 %3390, %3391
  %3393 = zext i1 %3392 to i8
  %3394 = icmp ne i8 %3368, 0
  br i1 %3394, label %then1176, label %else1177

then1176:                                         ; preds = %ifcont1175
  br label %ifcont1178

else1177:                                         ; preds = %ifcont1175
  br label %ifcont1178

ifcont1178:                                       ; preds = %else1177, %then1176
  %3395 = phi i32 [ %3371, %then1176 ], [ %3362, %else1177 ]
  %3396 = load ptr, ptr %__panic1, align 8
  %3397 = load i8, ptr %3396, align 1
  %3398 = icmp ne i8 %3397, 0
  br i1 %3398, label %panic_fail1180, label %panic_ok1179

panic_ok1179:                                     ; preds = %ifcont1178
  br label %panic_cont1181

panic_fail1180:                                   ; preds = %ifcont1178
  ret i32 0

panic_cont1181:                                   ; preds = %panic_ok1179
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

define linkonce_odr void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aCounter(ptr %data, ptr %__panic) unnamed_addr comdat {
entry:
  %__panic2 = alloca ptr, align 8
  %data1 = alloca ptr, align 8
  store ptr %data, ptr %data1, align 8
  store ptr %__panic, ptr %__panic2, align 8
  %0 = load ptr, ptr %__panic2, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %data1, align 8
  %3 = load { i32 }, ptr %2, align 4
  ret void
}

define linkonce_odr void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aStatus(ptr %data, ptr %__panic) unnamed_addr comdat {
entry:
  %match_val = alloca { [12 x i8], [0 x i32] }, align 8
  %__panic2 = alloca ptr, align 8
  %data1 = alloca ptr, align 8
  store ptr %data, ptr %data1, align 8
  store ptr %__panic, ptr %__panic2, align 8
  %0 = load ptr, ptr %__panic2, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %data1, align 8
  %3 = load { [12 x i8], [0 x i32] }, ptr %2, align 4
  store { [12 x i8], [0 x i32] } %3, ptr %match_val, align 4
  %4 = load i8, ptr %match_val, align 1
  %5 = icmp eq i8 %4, 0
  br i1 %5, label %match_arm, label %match_next

match_merge:                                      ; preds = %match_arm3, %match_arm
  %6 = phi {} [ zeroinitializer, %match_arm ], [ zeroinitializer, %match_arm3 ]
  ret void

match_fail:                                       ; preds = %match_next
  unreachable

match_arm:                                        ; preds = %entry
  br label %match_merge

match_next:                                       ; preds = %entry
  %7 = load i8, ptr %match_val, align 1
  %8 = icmp eq i8 %7, 1
  br i1 %8, label %match_arm3, label %match_fail

match_arm3:                                       ; preds = %match_next
  br label %match_merge
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

define linkonce_odr void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aStatus_x3a_x3aIdle(ptr %data, ptr %__panic) unnamed_addr comdat {
entry:
  %__panic2 = alloca ptr, align 8
  %data1 = alloca ptr, align 8
  store ptr %data, ptr %data1, align 8
  store ptr %__panic, ptr %__panic2, align 8
  %0 = load ptr, ptr %__panic2, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %data1, align 8
  %3 = load { i32 }, ptr %2, align 4
  ret void
}

define linkonce_odr void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aStatus_x3a_x3aBusy(ptr %data, ptr %__panic) unnamed_addr comdat {
entry:
  %__panic2 = alloca ptr, align 8
  %data1 = alloca ptr, align 8
  store ptr %data, ptr %data1, align 8
  store ptr %__panic, ptr %__panic2, align 8
  %0 = load ptr, ptr %__panic2, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %data1, align 8
  %3 = load { i32, i32 }, ptr %2, align 4
  ret void
}

define linkonce_odr void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aOptPtr(ptr %data, ptr %__panic) unnamed_addr comdat {
entry:
  %match_val = alloca { [16 x i8], [0 x i64] }, align 8
  %__panic2 = alloca ptr, align 8
  %data1 = alloca ptr, align 8
  store ptr %data, ptr %data1, align 8
  store ptr %__panic, ptr %__panic2, align 8
  %0 = load ptr, ptr %__panic2, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %data1, align 8
  %3 = load { [16 x i8], [0 x i64] }, ptr %2, align 8
  store { [16 x i8], [0 x i64] } %3, ptr %match_val, align 8
  %4 = load i8, ptr %match_val, align 1
  %5 = icmp eq i8 %4, 0
  br i1 %5, label %match_arm, label %match_next

match_merge:                                      ; preds = %match_arm3, %match_arm
  %6 = phi {} [ zeroinitializer, %match_arm ], [ zeroinitializer, %match_arm3 ]
  ret void

match_fail:                                       ; preds = %match_next
  unreachable

match_arm:                                        ; preds = %entry
  br label %match_merge

match_next:                                       ; preds = %entry
  %7 = load i8, ptr %match_val, align 1
  %8 = icmp eq i8 %7, 1
  br i1 %8, label %match_arm3, label %match_fail

match_arm3:                                       ; preds = %match_next
  br label %match_merge
}

define linkonce_odr void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aOptPtr_x3a_x3aValid(ptr %data, ptr %__panic) unnamed_addr comdat {
entry:
  %__panic2 = alloca ptr, align 8
  %data1 = alloca ptr, align 8
  store ptr %data, ptr %data1, align 8
  store ptr %__panic, ptr %__panic2, align 8
  %0 = load ptr, ptr %__panic2, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %data1, align 8
  %3 = load { ptr }, ptr %2, align 8
  ret void
}

define linkonce_odr void @cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3atest_x3a_x3aOptPtr_x3a_x3aNull(ptr %data, ptr %__panic) unnamed_addr comdat {
entry:
  %__panic2 = alloca ptr, align 8
  %data1 = alloca ptr, align 8
  store ptr %data, ptr %data1, align 8
  store ptr %__panic, ptr %__panic2, align 8
  %0 = load ptr, ptr %__panic2, align 8
  store i8 0, ptr %0, align 1
  %1 = getelementptr i8, ptr %0, i64 4
  store i32 0, ptr %1, align 4
  %2 = load ptr, ptr %data1, align 8
  %3 = load { i8 }, ptr %2, align 1
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32) #2

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
attributes #2 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }


.\cursive-bootstrap\build\runtime\cursive0_rt.dir\Debug\filesystem.obj:	file format coff-x86-64

Disassembly of section .text$mn:

0000000000000000 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread>:
       0: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
       5: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
       a: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
       f: 56                           	pushq	%rsi
      10: 57                           	pushq	%rdi
      11: 48 81 ec 08 01 00 00         	subq	$0x108, %rsp            # imm = 0x108
      18: 48 8d 7c 24 40               	leaq	0x40(%rsp), %rdi
      1d: b9 32 00 00 00               	movl	$0x32, %ecx
      22: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
      27: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
      29: 48 8b 8c 24 20 01 00 00      	movq	0x120(%rsp), %rcx
      31: 48 8b 8c 24 28 01 00 00      	movq	0x128(%rsp), %rcx
      39: e8 00 00 00 00               	callq	0x3e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x3e>
      3e: 48 89 44 24 40               	movq	%rax, 0x40(%rsp)
      43: 48 c7 44 24 58 00 00 00 00   	movq	$0x0, 0x58(%rsp)
      4c: c7 44 24 74 00 00 00 00      	movl	$0x0, 0x74(%rsp)
      54: 4c 8d 4c 24 74               	leaq	0x74(%rsp), %r9
      59: 4c 8d 44 24 58               	leaq	0x58(%rsp), %r8
      5e: 48 8b 94 24 30 01 00 00      	movq	0x130(%rsp), %rdx
      66: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
      6b: e8 00 00 00 00               	callq	0x70 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x70>
      70: 85 c0                        	testl	%eax, %eax
      72: 75 2e                        	jne	0xa2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0xa2>
      74: b2 03                        	movb	$0x3, %dl
      76: 48 8d 8c 24 c8 00 00 00      	leaq	0xc8(%rsp), %rcx
      7e: e8 00 00 00 00               	callq	0x83 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x83>
      83: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
      8b: 48 8b f0                     	movq	%rax, %rsi
      8e: b9 10 00 00 00               	movl	$0x10, %ecx
      93: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
      95: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
      9d: e9 1c 01 00 00               	jmp	0x1be <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x1be>
      a2: 45 33 c0                     	xorl	%r8d, %r8d
      a5: 8b 54 24 74                  	movl	0x74(%rsp), %edx
      a9: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
      ae: e8 00 00 00 00               	callq	0xb3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0xb3>
      b3: 48 89 84 24 88 00 00 00      	movq	%rax, 0x88(%rsp)
      bb: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
      c0: e8 00 00 00 00               	callq	0xc5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0xc5>
      c5: 48 83 bc 24 88 00 00 00 00   	cmpq	$0x0, 0x88(%rsp)
      ce: 75 2e                        	jne	0xfe <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0xfe>
      d0: b2 03                        	movb	$0x3, %dl
      d2: 48 8d 8c 24 d8 00 00 00      	leaq	0xd8(%rsp), %rcx
      da: e8 00 00 00 00               	callq	0xdf <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0xdf>
      df: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
      e7: 48 8b f0                     	movq	%rax, %rsi
      ea: b9 10 00 00 00               	movl	$0x10, %ecx
      ef: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
      f1: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
      f9: e9 c0 00 00 00               	jmp	0x1be <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x1be>
      fe: 48 c7 44 24 30 00 00 00 00   	movq	$0x0, 0x30(%rsp)
     107: c7 44 24 28 80 00 00 00      	movl	$0x80, 0x28(%rsp)
     10f: c7 44 24 20 03 00 00 00      	movl	$0x3, 0x20(%rsp)
     117: 45 33 c9                     	xorl	%r9d, %r9d
     11a: 41 b8 07 00 00 00            	movl	$0x7, %r8d
     120: ba 00 00 00 80               	movl	$0x80000000, %edx       # imm = 0x80000000
     125: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
     12d: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x133 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x133>
     133: 48 89 84 24 90 00 00 00      	movq	%rax, 0x90(%rsp)
     13b: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
     143: e8 00 00 00 00               	callq	0x148 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x148>
     148: 48 83 bc 24 90 00 00 00 ff   	cmpq	$-0x1, 0x90(%rsp)
     151: 75 31                        	jne	0x184 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x184>
     153: e8 00 00 00 00               	callq	0x158 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x158>
     158: 0f b6 d0                     	movzbl	%al, %edx
     15b: 48 8d 8c 24 e8 00 00 00      	leaq	0xe8(%rsp), %rcx
     163: e8 00 00 00 00               	callq	0x168 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x168>
     168: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     170: 48 8b f0                     	movq	%rax, %rsi
     173: b9 10 00 00 00               	movl	$0x10, %ecx
     178: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     17a: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     182: eb 3a                        	jmp	0x1be <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x1be>
     184: c6 84 24 a8 00 00 00 01      	movb	$0x1, 0xa8(%rsp)
     18c: 48 8b 84 24 90 00 00 00      	movq	0x90(%rsp), %rax
     194: 48 89 84 24 b0 00 00 00      	movq	%rax, 0xb0(%rsp)
     19c: 48 8d 84 24 a8 00 00 00      	leaq	0xa8(%rsp), %rax
     1a4: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     1ac: 48 8b f0                     	movq	%rax, %rsi
     1af: b9 10 00 00 00               	movl	$0x10, %ecx
     1b4: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     1b6: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     1be: 48 8b f8                     	movq	%rax, %rdi
     1c1: 48 8b cc                     	movq	%rsp, %rcx
     1c4: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x1cb <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x1cb>
     1cb: e8 00 00 00 00               	callq	0x1d0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fread+0x1d0>
     1d0: 48 8b c7                     	movq	%rdi, %rax
     1d3: 48 81 c4 08 01 00 00         	addq	$0x108, %rsp            # imm = 0x108
     1da: 5f                           	popq	%rdi
     1db: 5e                           	popq	%rsi
     1dc: c3                           	retq
     1dd: cc                           	int3
     1de: cc                           	int3
     1df: cc                           	int3
     1e0: cc                           	int3
     1e1: cc                           	int3
     1e2: cc                           	int3
     1e3: cc                           	int3
     1e4: cc                           	int3
     1e5: cc                           	int3
     1e6: cc                           	int3
     1e7: cc                           	int3
     1e8: cc                           	int3
     1e9: cc                           	int3
     1ea: cc                           	int3
     1eb: cc                           	int3
     1ec: cc                           	int3
     1ed: cc                           	int3
     1ee: cc                           	int3
     1ef: cc                           	int3

00000000000001f0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite>:
     1f0: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
     1f5: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
     1fa: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
     1ff: 56                           	pushq	%rsi
     200: 57                           	pushq	%rdi
     201: 48 81 ec 08 01 00 00         	subq	$0x108, %rsp            # imm = 0x108
     208: 48 8d 7c 24 40               	leaq	0x40(%rsp), %rdi
     20d: b9 32 00 00 00               	movl	$0x32, %ecx
     212: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
     217: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
     219: 48 8b 8c 24 20 01 00 00      	movq	0x120(%rsp), %rcx
     221: 48 8b 8c 24 28 01 00 00      	movq	0x128(%rsp), %rcx
     229: e8 00 00 00 00               	callq	0x22e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x3e>
     22e: 48 89 44 24 40               	movq	%rax, 0x40(%rsp)
     233: 48 c7 44 24 58 00 00 00 00   	movq	$0x0, 0x58(%rsp)
     23c: c7 44 24 74 00 00 00 00      	movl	$0x0, 0x74(%rsp)
     244: 4c 8d 4c 24 74               	leaq	0x74(%rsp), %r9
     249: 4c 8d 44 24 58               	leaq	0x58(%rsp), %r8
     24e: 48 8b 94 24 30 01 00 00      	movq	0x130(%rsp), %rdx
     256: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
     25b: e8 00 00 00 00               	callq	0x260 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x70>
     260: 85 c0                        	testl	%eax, %eax
     262: 75 2e                        	jne	0x292 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0xa2>
     264: b2 03                        	movb	$0x3, %dl
     266: 48 8d 8c 24 c8 00 00 00      	leaq	0xc8(%rsp), %rcx
     26e: e8 00 00 00 00               	callq	0x273 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x83>
     273: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     27b: 48 8b f0                     	movq	%rax, %rsi
     27e: b9 10 00 00 00               	movl	$0x10, %ecx
     283: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     285: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     28d: e9 1c 01 00 00               	jmp	0x3ae <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x1be>
     292: 45 33 c0                     	xorl	%r8d, %r8d
     295: 8b 54 24 74                  	movl	0x74(%rsp), %edx
     299: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
     29e: e8 00 00 00 00               	callq	0x2a3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0xb3>
     2a3: 48 89 84 24 88 00 00 00      	movq	%rax, 0x88(%rsp)
     2ab: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
     2b0: e8 00 00 00 00               	callq	0x2b5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0xc5>
     2b5: 48 83 bc 24 88 00 00 00 00   	cmpq	$0x0, 0x88(%rsp)
     2be: 75 2e                        	jne	0x2ee <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0xfe>
     2c0: b2 03                        	movb	$0x3, %dl
     2c2: 48 8d 8c 24 d8 00 00 00      	leaq	0xd8(%rsp), %rcx
     2ca: e8 00 00 00 00               	callq	0x2cf <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0xdf>
     2cf: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     2d7: 48 8b f0                     	movq	%rax, %rsi
     2da: b9 10 00 00 00               	movl	$0x10, %ecx
     2df: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     2e1: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     2e9: e9 c0 00 00 00               	jmp	0x3ae <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x1be>
     2ee: 48 c7 44 24 30 00 00 00 00   	movq	$0x0, 0x30(%rsp)
     2f7: c7 44 24 28 80 00 00 00      	movl	$0x80, 0x28(%rsp)
     2ff: c7 44 24 20 03 00 00 00      	movl	$0x3, 0x20(%rsp)
     307: 45 33 c9                     	xorl	%r9d, %r9d
     30a: 41 b8 01 00 00 00            	movl	$0x1, %r8d
     310: ba 00 00 00 40               	movl	$0x40000000, %edx       # imm = 0x40000000
     315: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
     31d: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x323 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x133>
     323: 48 89 84 24 90 00 00 00      	movq	%rax, 0x90(%rsp)
     32b: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
     333: e8 00 00 00 00               	callq	0x338 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x148>
     338: 48 83 bc 24 90 00 00 00 ff   	cmpq	$-0x1, 0x90(%rsp)
     341: 75 31                        	jne	0x374 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x184>
     343: e8 00 00 00 00               	callq	0x348 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x158>
     348: 0f b6 d0                     	movzbl	%al, %edx
     34b: 48 8d 8c 24 e8 00 00 00      	leaq	0xe8(%rsp), %rcx
     353: e8 00 00 00 00               	callq	0x358 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x168>
     358: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     360: 48 8b f0                     	movq	%rax, %rsi
     363: b9 10 00 00 00               	movl	$0x10, %ecx
     368: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     36a: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     372: eb 3a                        	jmp	0x3ae <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x1be>
     374: c6 84 24 a8 00 00 00 01      	movb	$0x1, 0xa8(%rsp)
     37c: 48 8b 84 24 90 00 00 00      	movq	0x90(%rsp), %rax
     384: 48 89 84 24 b0 00 00 00      	movq	%rax, 0xb0(%rsp)
     38c: 48 8d 84 24 a8 00 00 00      	leaq	0xa8(%rsp), %rax
     394: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     39c: 48 8b f0                     	movq	%rax, %rsi
     39f: b9 10 00 00 00               	movl	$0x10, %ecx
     3a4: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     3a6: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     3ae: 48 8b f8                     	movq	%rax, %rdi
     3b1: 48 8b cc                     	movq	%rsp, %rcx
     3b4: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x3bb <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x1cb>
     3bb: e8 00 00 00 00               	callq	0x3c0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fwrite+0x1d0>
     3c0: 48 8b c7                     	movq	%rdi, %rax
     3c3: 48 81 c4 08 01 00 00         	addq	$0x108, %rsp            # imm = 0x108
     3ca: 5f                           	popq	%rdi
     3cb: 5e                           	popq	%rsi
     3cc: c3                           	retq
     3cd: cc                           	int3
     3ce: cc                           	int3
     3cf: cc                           	int3
     3d0: cc                           	int3
     3d1: cc                           	int3
     3d2: cc                           	int3
     3d3: cc                           	int3
     3d4: cc                           	int3
     3d5: cc                           	int3
     3d6: cc                           	int3
     3d7: cc                           	int3
     3d8: cc                           	int3
     3d9: cc                           	int3
     3da: cc                           	int3
     3db: cc                           	int3
     3dc: cc                           	int3
     3dd: cc                           	int3
     3de: cc                           	int3
     3df: cc                           	int3

00000000000003e0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend>:
     3e0: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
     3e5: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
     3ea: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
     3ef: 56                           	pushq	%rsi
     3f0: 57                           	pushq	%rdi
     3f1: 48 81 ec 08 01 00 00         	subq	$0x108, %rsp            # imm = 0x108
     3f8: 48 8d 7c 24 40               	leaq	0x40(%rsp), %rdi
     3fd: b9 32 00 00 00               	movl	$0x32, %ecx
     402: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
     407: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
     409: 48 8b 8c 24 20 01 00 00      	movq	0x120(%rsp), %rcx
     411: 48 8b 8c 24 28 01 00 00      	movq	0x128(%rsp), %rcx
     419: e8 00 00 00 00               	callq	0x41e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x3e>
     41e: 48 89 44 24 40               	movq	%rax, 0x40(%rsp)
     423: 48 c7 44 24 58 00 00 00 00   	movq	$0x0, 0x58(%rsp)
     42c: c7 44 24 74 00 00 00 00      	movl	$0x0, 0x74(%rsp)
     434: 4c 8d 4c 24 74               	leaq	0x74(%rsp), %r9
     439: 4c 8d 44 24 58               	leaq	0x58(%rsp), %r8
     43e: 48 8b 94 24 30 01 00 00      	movq	0x130(%rsp), %rdx
     446: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
     44b: e8 00 00 00 00               	callq	0x450 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x70>
     450: 85 c0                        	testl	%eax, %eax
     452: 75 2e                        	jne	0x482 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0xa2>
     454: b2 03                        	movb	$0x3, %dl
     456: 48 8d 8c 24 c8 00 00 00      	leaq	0xc8(%rsp), %rcx
     45e: e8 00 00 00 00               	callq	0x463 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x83>
     463: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     46b: 48 8b f0                     	movq	%rax, %rsi
     46e: b9 10 00 00 00               	movl	$0x10, %ecx
     473: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     475: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     47d: e9 1c 01 00 00               	jmp	0x59e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x1be>
     482: 45 33 c0                     	xorl	%r8d, %r8d
     485: 8b 54 24 74                  	movl	0x74(%rsp), %edx
     489: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
     48e: e8 00 00 00 00               	callq	0x493 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0xb3>
     493: 48 89 84 24 88 00 00 00      	movq	%rax, 0x88(%rsp)
     49b: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
     4a0: e8 00 00 00 00               	callq	0x4a5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0xc5>
     4a5: 48 83 bc 24 88 00 00 00 00   	cmpq	$0x0, 0x88(%rsp)
     4ae: 75 2e                        	jne	0x4de <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0xfe>
     4b0: b2 03                        	movb	$0x3, %dl
     4b2: 48 8d 8c 24 d8 00 00 00      	leaq	0xd8(%rsp), %rcx
     4ba: e8 00 00 00 00               	callq	0x4bf <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0xdf>
     4bf: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     4c7: 48 8b f0                     	movq	%rax, %rsi
     4ca: b9 10 00 00 00               	movl	$0x10, %ecx
     4cf: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     4d1: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     4d9: e9 c0 00 00 00               	jmp	0x59e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x1be>
     4de: 48 c7 44 24 30 00 00 00 00   	movq	$0x0, 0x30(%rsp)
     4e7: c7 44 24 28 80 00 00 00      	movl	$0x80, 0x28(%rsp)
     4ef: c7 44 24 20 03 00 00 00      	movl	$0x3, 0x20(%rsp)
     4f7: 45 33 c9                     	xorl	%r9d, %r9d
     4fa: 41 b8 01 00 00 00            	movl	$0x1, %r8d
     500: ba 04 00 00 00               	movl	$0x4, %edx
     505: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
     50d: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x513 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x133>
     513: 48 89 84 24 90 00 00 00      	movq	%rax, 0x90(%rsp)
     51b: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
     523: e8 00 00 00 00               	callq	0x528 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x148>
     528: 48 83 bc 24 90 00 00 00 ff   	cmpq	$-0x1, 0x90(%rsp)
     531: 75 31                        	jne	0x564 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x184>
     533: e8 00 00 00 00               	callq	0x538 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x158>
     538: 0f b6 d0                     	movzbl	%al, %edx
     53b: 48 8d 8c 24 e8 00 00 00      	leaq	0xe8(%rsp), %rcx
     543: e8 00 00 00 00               	callq	0x548 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x168>
     548: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     550: 48 8b f0                     	movq	%rax, %rsi
     553: b9 10 00 00 00               	movl	$0x10, %ecx
     558: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     55a: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     562: eb 3a                        	jmp	0x59e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x1be>
     564: c6 84 24 a8 00 00 00 01      	movb	$0x1, 0xa8(%rsp)
     56c: 48 8b 84 24 90 00 00 00      	movq	0x90(%rsp), %rax
     574: 48 89 84 24 b0 00 00 00      	movq	%rax, 0xb0(%rsp)
     57c: 48 8d 84 24 a8 00 00 00      	leaq	0xa8(%rsp), %rax
     584: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     58c: 48 8b f0                     	movq	%rax, %rsi
     58f: b9 10 00 00 00               	movl	$0x10, %ecx
     594: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     596: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     59e: 48 8b f8                     	movq	%rax, %rdi
     5a1: 48 8b cc                     	movq	%rsp, %rcx
     5a4: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x5ab <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x1cb>
     5ab: e8 00 00 00 00               	callq	0x5b0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fappend+0x1d0>
     5b0: 48 8b c7                     	movq	%rdi, %rax
     5b3: 48 81 c4 08 01 00 00         	addq	$0x108, %rsp            # imm = 0x108
     5ba: 5f                           	popq	%rdi
     5bb: 5e                           	popq	%rsi
     5bc: c3                           	retq
     5bd: cc                           	int3
     5be: cc                           	int3
     5bf: cc                           	int3
     5c0: cc                           	int3
     5c1: cc                           	int3
     5c2: cc                           	int3
     5c3: cc                           	int3
     5c4: cc                           	int3
     5c5: cc                           	int3
     5c6: cc                           	int3
     5c7: cc                           	int3
     5c8: cc                           	int3
     5c9: cc                           	int3
     5ca: cc                           	int3
     5cb: cc                           	int3
     5cc: cc                           	int3
     5cd: cc                           	int3
     5ce: cc                           	int3
     5cf: cc                           	int3

00000000000005d0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite>:
     5d0: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
     5d5: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
     5da: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
     5df: 56                           	pushq	%rsi
     5e0: 57                           	pushq	%rdi
     5e1: 48 81 ec 08 01 00 00         	subq	$0x108, %rsp            # imm = 0x108
     5e8: 48 8d 7c 24 40               	leaq	0x40(%rsp), %rdi
     5ed: b9 32 00 00 00               	movl	$0x32, %ecx
     5f2: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
     5f7: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
     5f9: 48 8b 8c 24 20 01 00 00      	movq	0x120(%rsp), %rcx
     601: 48 8b 8c 24 28 01 00 00      	movq	0x128(%rsp), %rcx
     609: e8 00 00 00 00               	callq	0x60e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x3e>
     60e: 48 89 44 24 40               	movq	%rax, 0x40(%rsp)
     613: 48 c7 44 24 58 00 00 00 00   	movq	$0x0, 0x58(%rsp)
     61c: c7 44 24 74 00 00 00 00      	movl	$0x0, 0x74(%rsp)
     624: 4c 8d 4c 24 74               	leaq	0x74(%rsp), %r9
     629: 4c 8d 44 24 58               	leaq	0x58(%rsp), %r8
     62e: 48 8b 94 24 30 01 00 00      	movq	0x130(%rsp), %rdx
     636: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
     63b: e8 00 00 00 00               	callq	0x640 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x70>
     640: 85 c0                        	testl	%eax, %eax
     642: 75 2e                        	jne	0x672 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0xa2>
     644: b2 03                        	movb	$0x3, %dl
     646: 48 8d 8c 24 c8 00 00 00      	leaq	0xc8(%rsp), %rcx
     64e: e8 00 00 00 00               	callq	0x653 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x83>
     653: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     65b: 48 8b f0                     	movq	%rax, %rsi
     65e: b9 10 00 00 00               	movl	$0x10, %ecx
     663: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     665: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     66d: e9 1c 01 00 00               	jmp	0x78e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x1be>
     672: 45 33 c0                     	xorl	%r8d, %r8d
     675: 8b 54 24 74                  	movl	0x74(%rsp), %edx
     679: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
     67e: e8 00 00 00 00               	callq	0x683 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0xb3>
     683: 48 89 84 24 88 00 00 00      	movq	%rax, 0x88(%rsp)
     68b: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
     690: e8 00 00 00 00               	callq	0x695 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0xc5>
     695: 48 83 bc 24 88 00 00 00 00   	cmpq	$0x0, 0x88(%rsp)
     69e: 75 2e                        	jne	0x6ce <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0xfe>
     6a0: b2 03                        	movb	$0x3, %dl
     6a2: 48 8d 8c 24 d8 00 00 00      	leaq	0xd8(%rsp), %rcx
     6aa: e8 00 00 00 00               	callq	0x6af <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0xdf>
     6af: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     6b7: 48 8b f0                     	movq	%rax, %rsi
     6ba: b9 10 00 00 00               	movl	$0x10, %ecx
     6bf: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     6c1: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     6c9: e9 c0 00 00 00               	jmp	0x78e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x1be>
     6ce: 48 c7 44 24 30 00 00 00 00   	movq	$0x0, 0x30(%rsp)
     6d7: c7 44 24 28 80 00 00 00      	movl	$0x80, 0x28(%rsp)
     6df: c7 44 24 20 01 00 00 00      	movl	$0x1, 0x20(%rsp)
     6e7: 45 33 c9                     	xorl	%r9d, %r9d
     6ea: 41 b8 01 00 00 00            	movl	$0x1, %r8d
     6f0: ba 00 00 00 40               	movl	$0x40000000, %edx       # imm = 0x40000000
     6f5: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
     6fd: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x703 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x133>
     703: 48 89 84 24 90 00 00 00      	movq	%rax, 0x90(%rsp)
     70b: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
     713: e8 00 00 00 00               	callq	0x718 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x148>
     718: 48 83 bc 24 90 00 00 00 ff   	cmpq	$-0x1, 0x90(%rsp)
     721: 75 31                        	jne	0x754 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x184>
     723: e8 00 00 00 00               	callq	0x728 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x158>
     728: 0f b6 d0                     	movzbl	%al, %edx
     72b: 48 8d 8c 24 e8 00 00 00      	leaq	0xe8(%rsp), %rcx
     733: e8 00 00 00 00               	callq	0x738 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x168>
     738: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     740: 48 8b f0                     	movq	%rax, %rsi
     743: b9 10 00 00 00               	movl	$0x10, %ecx
     748: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     74a: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     752: eb 3a                        	jmp	0x78e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x1be>
     754: c6 84 24 a8 00 00 00 01      	movb	$0x1, 0xa8(%rsp)
     75c: 48 8b 84 24 90 00 00 00      	movq	0x90(%rsp), %rax
     764: 48 89 84 24 b0 00 00 00      	movq	%rax, 0xb0(%rsp)
     76c: 48 8d 84 24 a8 00 00 00      	leaq	0xa8(%rsp), %rax
     774: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
     77c: 48 8b f0                     	movq	%rax, %rsi
     77f: b9 10 00 00 00               	movl	$0x10, %ecx
     784: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     786: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     78e: 48 8b f8                     	movq	%rax, %rdi
     791: 48 8b cc                     	movq	%rsp, %rcx
     794: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x79b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x1cb>
     79b: e8 00 00 00 00               	callq	0x7a0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fwrite+0x1d0>
     7a0: 48 8b c7                     	movq	%rdi, %rax
     7a3: 48 81 c4 08 01 00 00         	addq	$0x108, %rsp            # imm = 0x108
     7aa: 5f                           	popq	%rdi
     7ab: 5e                           	popq	%rsi
     7ac: c3                           	retq
     7ad: cc                           	int3
     7ae: cc                           	int3
     7af: cc                           	int3
     7b0: cc                           	int3
     7b1: cc                           	int3
     7b2: cc                           	int3
     7b3: cc                           	int3
     7b4: cc                           	int3
     7b5: cc                           	int3
     7b6: cc                           	int3
     7b7: cc                           	int3
     7b8: cc                           	int3
     7b9: cc                           	int3
     7ba: cc                           	int3
     7bb: cc                           	int3
     7bc: cc                           	int3
     7bd: cc                           	int3
     7be: cc                           	int3
     7bf: cc                           	int3

00000000000007c0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile>:
     7c0: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
     7c5: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
     7ca: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
     7cf: 56                           	pushq	%rsi
     7d0: 57                           	pushq	%rdi
     7d1: 48 81 ec e8 00 00 00         	subq	$0xe8, %rsp
     7d8: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
     7dd: b9 32 00 00 00               	movl	$0x32, %ecx
     7e2: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
     7e7: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
     7e9: 48 8b 8c 24 00 01 00 00      	movq	0x100(%rsp), %rcx
     7f1: 48 83 bc 24 00 01 00 00 00   	cmpq	$0x0, 0x100(%rsp)
     7fa: 75 05                        	jne	0x801 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile+0x41>
     7fc: e9 ae 00 00 00               	jmp	0x8af <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile+0xef>
     801: 4c 8b 84 24 10 01 00 00      	movq	0x110(%rsp), %r8
     809: 48 8b 94 24 08 01 00 00      	movq	0x108(%rsp), %rdx
     811: 48 8d 8c 24 88 00 00 00      	leaq	0x88(%rsp), %rcx
     819: e8 00 00 00 00               	callq	0x81e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile+0x5e>
     81e: 48 8d 4c 24 28               	leaq	0x28(%rsp), %rcx
     823: 48 8b f9                     	movq	%rcx, %rdi
     826: 48 8b f0                     	movq	%rax, %rsi
     829: b9 10 00 00 00               	movl	$0x10, %ecx
     82e: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     830: 0f b6 44 24 28               	movzbl	0x28(%rsp), %eax
     835: 85 c0                        	testl	%eax, %eax
     837: 75 26                        	jne	0x85f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile+0x9f>
     839: 0f b6 54 24 30               	movzbl	0x30(%rsp), %edx
     83e: 48 8d 8c 24 98 00 00 00      	leaq	0x98(%rsp), %rcx
     846: e8 00 00 00 00               	callq	0x84b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile+0x8b>
     84b: 48 8b bc 24 00 01 00 00      	movq	0x100(%rsp), %rdi
     853: 48 8b f0                     	movq	%rax, %rsi
     856: b9 20 00 00 00               	movl	$0x20, %ecx
     85b: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     85d: eb 50                        	jmp	0x8af <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile+0xef>
     85f: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
     864: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
     869: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
     86e: 48 8d 8c 24 b8 00 00 00      	leaq	0xb8(%rsp), %rcx
     876: e8 00 00 00 00               	callq	0x87b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile+0xbb>
     87b: 48 8d 4c 24 58               	leaq	0x58(%rsp), %rcx
     880: 48 8b f9                     	movq	%rcx, %rdi
     883: 48 8b f0                     	movq	%rax, %rsi
     886: b9 20 00 00 00               	movl	$0x20, %ecx
     88b: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     88d: 48 8b 4c 24 48               	movq	0x48(%rsp), %rcx
     892: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x898 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile+0xd8>
     898: 48 8d 44 24 58               	leaq	0x58(%rsp), %rax
     89d: 48 8b bc 24 00 01 00 00      	movq	0x100(%rsp), %rdi
     8a5: 48 8b f0                     	movq	%rax, %rsi
     8a8: b9 20 00 00 00               	movl	$0x20, %ecx
     8ad: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     8af: 48 8b cc                     	movq	%rsp, %rcx
     8b2: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x8b9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile+0xf9>
     8b9: e8 00 00 00 00               	callq	0x8be <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5ffile+0xfe>
     8be: 48 81 c4 e8 00 00 00         	addq	$0xe8, %rsp
     8c5: 5f                           	popq	%rdi
     8c6: 5e                           	popq	%rsi
     8c7: c3                           	retq
     8c8: cc                           	int3
     8c9: cc                           	int3
     8ca: cc                           	int3
     8cb: cc                           	int3
     8cc: cc                           	int3
     8cd: cc                           	int3
     8ce: cc                           	int3
     8cf: cc                           	int3

00000000000008d0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes>:
     8d0: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
     8d5: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
     8da: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
     8df: 56                           	pushq	%rsi
     8e0: 57                           	pushq	%rdi
     8e1: 48 81 ec e8 00 00 00         	subq	$0xe8, %rsp
     8e8: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
     8ed: b9 32 00 00 00               	movl	$0x32, %ecx
     8f2: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
     8f7: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
     8f9: 48 8b 8c 24 00 01 00 00      	movq	0x100(%rsp), %rcx
     901: 48 83 bc 24 00 01 00 00 00   	cmpq	$0x0, 0x100(%rsp)
     90a: 75 05                        	jne	0x911 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes+0x41>
     90c: e9 ae 00 00 00               	jmp	0x9bf <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes+0xef>
     911: 4c 8b 84 24 10 01 00 00      	movq	0x110(%rsp), %r8
     919: 48 8b 94 24 08 01 00 00      	movq	0x108(%rsp), %rdx
     921: 48 8d 8c 24 88 00 00 00      	leaq	0x88(%rsp), %rcx
     929: e8 00 00 00 00               	callq	0x92e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes+0x5e>
     92e: 48 8d 4c 24 28               	leaq	0x28(%rsp), %rcx
     933: 48 8b f9                     	movq	%rcx, %rdi
     936: 48 8b f0                     	movq	%rax, %rsi
     939: b9 10 00 00 00               	movl	$0x10, %ecx
     93e: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     940: 0f b6 44 24 28               	movzbl	0x28(%rsp), %eax
     945: 85 c0                        	testl	%eax, %eax
     947: 75 26                        	jne	0x96f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes+0x9f>
     949: 0f b6 54 24 30               	movzbl	0x30(%rsp), %edx
     94e: 48 8d 8c 24 98 00 00 00      	leaq	0x98(%rsp), %rcx
     956: e8 00 00 00 00               	callq	0x95b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes+0x8b>
     95b: 48 8b bc 24 00 01 00 00      	movq	0x100(%rsp), %rdi
     963: 48 8b f0                     	movq	%rax, %rsi
     966: b9 20 00 00 00               	movl	$0x20, %ecx
     96b: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     96d: eb 50                        	jmp	0x9bf <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes+0xef>
     96f: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
     974: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
     979: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
     97e: 48 8d 8c 24 b8 00 00 00      	leaq	0xb8(%rsp), %rcx
     986: e8 00 00 00 00               	callq	0x98b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes+0xbb>
     98b: 48 8d 4c 24 58               	leaq	0x58(%rsp), %rcx
     990: 48 8b f9                     	movq	%rcx, %rdi
     993: 48 8b f0                     	movq	%rax, %rsi
     996: b9 20 00 00 00               	movl	$0x20, %ecx
     99b: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     99d: 48 8b 4c 24 48               	movq	0x48(%rsp), %rcx
     9a2: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x9a8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes+0xd8>
     9a8: 48 8d 44 24 58               	leaq	0x58(%rsp), %rax
     9ad: 48 8b bc 24 00 01 00 00      	movq	0x100(%rsp), %rdi
     9b5: 48 8b f0                     	movq	%rax, %rsi
     9b8: b9 20 00 00 00               	movl	$0x20, %ecx
     9bd: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
     9bf: 48 8b cc                     	movq	%rsp, %rcx
     9c2: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x9c9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes+0xf9>
     9c9: e8 00 00 00 00               	callq	0x9ce <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aread_x5fbytes+0xfe>
     9ce: 48 81 c4 e8 00 00 00         	addq	$0xe8, %rsp
     9d5: 5f                           	popq	%rdi
     9d6: 5e                           	popq	%rsi
     9d7: c3                           	retq
     9d8: cc                           	int3
     9d9: cc                           	int3
     9da: cc                           	int3
     9db: cc                           	int3
     9dc: cc                           	int3
     9dd: cc                           	int3
     9de: cc                           	int3
     9df: cc                           	int3

00000000000009e0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile>:
     9e0: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
     9e5: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
     9ea: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
     9ef: 57                           	pushq	%rdi
     9f0: 48 81 ec 90 01 00 00         	subq	$0x190, %rsp            # imm = 0x190
     9f7: 48 8d 7c 24 40               	leaq	0x40(%rsp), %rdi
     9fc: b9 54 00 00 00               	movl	$0x54, %ecx
     a01: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
     a06: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
     a08: 48 8b 8c 24 a0 01 00 00      	movq	0x1a0(%rsp), %rcx
     a10: b2 57                        	movb	$0x57, %dl
     a12: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xa19 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x39>
     a19: e8 00 00 00 00               	callq	0xa1e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x3e>
     a1e: 90                           	nop
     a1f: e8 00 00 00 00               	callq	0xa24 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x44>
     a24: 66 89 44 24 44               	movw	%ax, 0x44(%rsp)
     a29: 0f b6 54 24 44               	movzbl	0x44(%rsp), %edx
     a2e: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xa35 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x55>
     a35: e8 00 00 00 00               	callq	0xa3a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x5a>
     a3a: 0f b6 54 24 45               	movzbl	0x45(%rsp), %edx
     a3f: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xa46 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x66>
     a46: e8 00 00 00 00               	callq	0xa4b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x6b>
     a4b: 0f b7 44 24 44               	movzwl	0x44(%rsp), %eax
     a50: e9 9a 03 00 00               	jmp	0xdef <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x40f>
     a55: 48 8b 8c 24 a0 01 00 00      	movq	0x1a0(%rsp), %rcx
     a5d: e8 00 00 00 00               	callq	0xa62 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x82>
     a62: 48 89 44 24 58               	movq	%rax, 0x58(%rsp)
     a67: 48 c7 44 24 68 00 00 00 00   	movq	$0x0, 0x68(%rsp)
     a70: c7 84 24 84 00 00 00 00 00 00 00     	movl	$0x0, 0x84(%rsp)
     a7b: 4c 8d 8c 24 84 00 00 00      	leaq	0x84(%rsp), %r9
     a83: 4c 8d 44 24 68               	leaq	0x68(%rsp), %r8
     a88: 48 8b 94 24 a8 01 00 00      	movq	0x1a8(%rsp), %rdx
     a90: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
     a95: e8 00 00 00 00               	callq	0xa9a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0xba>
     a9a: 85 c0                        	testl	%eax, %eax
     a9c: 75 44                        	jne	0xae2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x102>
     a9e: b1 03                        	movb	$0x3, %cl
     aa0: e8 00 00 00 00               	callq	0xaa5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0xc5>
     aa5: 66 89 84 24 a4 00 00 00      	movw	%ax, 0xa4(%rsp)
     aad: 0f b6 94 24 a4 00 00 00      	movzbl	0xa4(%rsp), %edx
     ab5: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xabc <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0xdc>
     abc: e8 00 00 00 00               	callq	0xac1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0xe1>
     ac1: 0f b6 94 24 a5 00 00 00      	movzbl	0xa5(%rsp), %edx
     ac9: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xad0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0xf0>
     ad0: e8 00 00 00 00               	callq	0xad5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0xf5>
     ad5: 0f b7 84 24 a4 00 00 00      	movzwl	0xa4(%rsp), %eax
     add: e9 0d 03 00 00               	jmp	0xdef <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x40f>
     ae2: 45 33 c0                     	xorl	%r8d, %r8d
     ae5: 8b 94 24 84 00 00 00         	movl	0x84(%rsp), %edx
     aec: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
     af1: e8 00 00 00 00               	callq	0xaf6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x116>
     af6: 48 89 84 24 b8 00 00 00      	movq	%rax, 0xb8(%rsp)
     afe: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
     b03: e8 00 00 00 00               	callq	0xb08 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x128>
     b08: 48 83 bc 24 b8 00 00 00 00   	cmpq	$0x0, 0xb8(%rsp)
     b11: 75 44                        	jne	0xb57 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x177>
     b13: b1 03                        	movb	$0x3, %cl
     b15: e8 00 00 00 00               	callq	0xb1a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x13a>
     b1a: 66 89 84 24 c4 00 00 00      	movw	%ax, 0xc4(%rsp)
     b22: 0f b6 94 24 c4 00 00 00      	movzbl	0xc4(%rsp), %edx
     b2a: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xb31 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x151>
     b31: e8 00 00 00 00               	callq	0xb36 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x156>
     b36: 0f b6 94 24 c5 00 00 00      	movzbl	0xc5(%rsp), %edx
     b3e: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xb45 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x165>
     b45: e8 00 00 00 00               	callq	0xb4a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x16a>
     b4a: 0f b7 84 24 c4 00 00 00      	movzwl	0xc4(%rsp), %eax
     b52: e9 98 02 00 00               	jmp	0xdef <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x40f>
     b57: 48 c7 44 24 30 00 00 00 00   	movq	$0x0, 0x30(%rsp)
     b60: c7 44 24 28 80 00 00 00      	movl	$0x80, 0x28(%rsp)
     b68: c7 44 24 20 02 00 00 00      	movl	$0x2, 0x20(%rsp)
     b70: 45 33 c9                     	xorl	%r9d, %r9d
     b73: 41 b8 01 00 00 00            	movl	$0x1, %r8d
     b79: ba 00 00 00 40               	movl	$0x40000000, %edx       # imm = 0x40000000
     b7e: 48 8b 8c 24 b8 00 00 00      	movq	0xb8(%rsp), %rcx
     b86: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xb8c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x1ac>
     b8c: 48 89 84 24 d8 00 00 00      	movq	%rax, 0xd8(%rsp)
     b94: 48 8b 8c 24 b8 00 00 00      	movq	0xb8(%rsp), %rcx
     b9c: e8 00 00 00 00               	callq	0xba1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x1c1>
     ba1: 48 83 bc 24 d8 00 00 00 ff   	cmpq	$-0x1, 0xd8(%rsp)
     baa: 75 4a                        	jne	0xbf6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x216>
     bac: e8 00 00 00 00               	callq	0xbb1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x1d1>
     bb1: 0f b6 c8                     	movzbl	%al, %ecx
     bb4: e8 00 00 00 00               	callq	0xbb9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x1d9>
     bb9: 66 89 84 24 e4 00 00 00      	movw	%ax, 0xe4(%rsp)
     bc1: 0f b6 94 24 e4 00 00 00      	movzbl	0xe4(%rsp), %edx
     bc9: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xbd0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x1f0>
     bd0: e8 00 00 00 00               	callq	0xbd5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x1f5>
     bd5: 0f b6 94 24 e5 00 00 00      	movzbl	0xe5(%rsp), %edx
     bdd: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xbe4 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x204>
     be4: e8 00 00 00 00               	callq	0xbe9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x209>
     be9: 0f b7 84 24 e4 00 00 00      	movzwl	0xe4(%rsp), %eax
     bf1: e9 f9 01 00 00               	jmp	0xdef <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x40f>
     bf6: 48 83 bc 24 b0 01 00 00 00   	cmpq	$0x0, 0x1b0(%rsp)
     bff: 74 16                        	je	0xc17 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x237>
     c01: 48 8b 84 24 b0 01 00 00      	movq	0x1b0(%rsp), %rax
     c09: 48 8b 40 08                  	movq	0x8(%rax), %rax
     c0d: 48 89 84 24 88 01 00 00      	movq	%rax, 0x188(%rsp)
     c15: eb 0c                        	jmp	0xc23 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x243>
     c17: 48 c7 84 24 88 01 00 00 00 00 00 00  	movq	$0x0, 0x188(%rsp)
     c23: 48 8b 84 24 88 01 00 00      	movq	0x188(%rsp), %rax
     c2b: 48 89 84 24 f8 00 00 00      	movq	%rax, 0xf8(%rsp)
     c33: 48 c7 84 24 00 01 00 00 00 00 00 00  	movq	$0x0, 0x100(%rsp)
     c3f: 48 8b 84 24 f8 00 00 00      	movq	0xf8(%rsp), %rax
     c47: 48 39 84 24 00 01 00 00      	cmpq	%rax, 0x100(%rsp)
     c4f: 0f 83 fc 00 00 00            	jae	0xd51 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x371>
     c55: c7 84 24 14 01 00 00 00 00 00 00     	movl	$0x0, 0x114(%rsp)
     c60: 48 8b 84 24 00 01 00 00      	movq	0x100(%rsp), %rax
     c68: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
     c70: 48 2b c8                     	subq	%rax, %rcx
     c73: 48 8b c1                     	movq	%rcx, %rax
     c76: ba ff ff ff 7f               	movl	$0x7fffffff, %edx       # imm = 0x7FFFFFFF
     c7b: 48 8b c8                     	movq	%rax, %rcx
     c7e: e8 00 00 00 00               	callq	0xc83 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x2a3>
     c83: 89 84 24 24 01 00 00         	movl	%eax, 0x124(%rsp)
     c8a: 48 8b 84 24 b0 01 00 00      	movq	0x1b0(%rsp), %rax
     c92: 48 8b 00                     	movq	(%rax), %rax
     c95: 48 03 84 24 00 01 00 00      	addq	0x100(%rsp), %rax
     c9d: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
     ca6: 4c 8d 8c 24 14 01 00 00      	leaq	0x114(%rsp), %r9
     cae: 44 8b 84 24 24 01 00 00      	movl	0x124(%rsp), %r8d
     cb6: 48 8b d0                     	movq	%rax, %rdx
     cb9: 48 8b 8c 24 d8 00 00 00      	movq	0xd8(%rsp), %rcx
     cc1: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xcc7 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x2e7>
     cc7: 85 c0                        	testl	%eax, %eax
     cc9: 75 58                        	jne	0xd23 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x343>
     ccb: 48 8b 8c 24 d8 00 00 00      	movq	0xd8(%rsp), %rcx
     cd3: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xcd9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x2f9>
     cd9: e8 00 00 00 00               	callq	0xcde <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x2fe>
     cde: 0f b6 c8                     	movzbl	%al, %ecx
     ce1: e8 00 00 00 00               	callq	0xce6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x306>
     ce6: 66 89 84 24 34 01 00 00      	movw	%ax, 0x134(%rsp)
     cee: 0f b6 94 24 34 01 00 00      	movzbl	0x134(%rsp), %edx
     cf6: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xcfd <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x31d>
     cfd: e8 00 00 00 00               	callq	0xd02 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x322>
     d02: 0f b6 94 24 35 01 00 00      	movzbl	0x135(%rsp), %edx
     d0a: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xd11 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x331>
     d11: e8 00 00 00 00               	callq	0xd16 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x336>
     d16: 0f b7 84 24 34 01 00 00      	movzwl	0x134(%rsp), %eax
     d1e: e9 cc 00 00 00               	jmp	0xdef <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x40f>
     d23: 83 bc 24 14 01 00 00 00      	cmpl	$0x0, 0x114(%rsp)
     d2b: 75 02                        	jne	0xd2f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x34f>
     d2d: eb 22                        	jmp	0xd51 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x371>
     d2f: 8b 84 24 14 01 00 00         	movl	0x114(%rsp), %eax
     d36: 48 8b 8c 24 00 01 00 00      	movq	0x100(%rsp), %rcx
     d3e: 48 03 c8                     	addq	%rax, %rcx
     d41: 48 8b c1                     	movq	%rcx, %rax
     d44: 48 89 84 24 00 01 00 00      	movq	%rax, 0x100(%rsp)
     d4c: e9 ee fe ff ff               	jmp	0xc3f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x25f>
     d51: 48 8b 8c 24 d8 00 00 00      	movq	0xd8(%rsp), %rcx
     d59: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xd5f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x37f>
     d5f: 48 8b 84 24 f8 00 00 00      	movq	0xf8(%rsp), %rax
     d67: 48 39 84 24 00 01 00 00      	cmpq	%rax, 0x100(%rsp)
     d6f: 74 41                        	je	0xdb2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x3d2>
     d71: b1 05                        	movb	$0x5, %cl
     d73: e8 00 00 00 00               	callq	0xd78 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x398>
     d78: 66 89 84 24 54 01 00 00      	movw	%ax, 0x154(%rsp)
     d80: 0f b6 94 24 54 01 00 00      	movzbl	0x154(%rsp), %edx
     d88: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xd8f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x3af>
     d8f: e8 00 00 00 00               	callq	0xd94 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x3b4>
     d94: 0f b6 94 24 55 01 00 00      	movzbl	0x155(%rsp), %edx
     d9c: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xda3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x3c3>
     da3: e8 00 00 00 00               	callq	0xda8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x3c8>
     da8: 0f b7 84 24 54 01 00 00      	movzwl	0x154(%rsp), %eax
     db0: eb 3d                        	jmp	0xdef <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x40f>
     db2: e8 00 00 00 00               	callq	0xdb7 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x3d7>
     db7: 66 89 84 24 74 01 00 00      	movw	%ax, 0x174(%rsp)
     dbf: 0f b6 94 24 74 01 00 00      	movzbl	0x174(%rsp), %edx
     dc7: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xdce <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x3ee>
     dce: e8 00 00 00 00               	callq	0xdd3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x3f3>
     dd3: 0f b6 94 24 75 01 00 00      	movzbl	0x175(%rsp), %edx
     ddb: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xde2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x402>
     de2: e8 00 00 00 00               	callq	0xde7 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x407>
     de7: 0f b7 84 24 74 01 00 00      	movzwl	0x174(%rsp), %eax
     def: 48 8b f8                     	movq	%rax, %rdi
     df2: 48 8b cc                     	movq	%rsp, %rcx
     df5: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0xdfc <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x41c>
     dfc: e8 00 00 00 00               	callq	0xe01 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile+0x421>
     e01: 48 8b c7                     	movq	%rdi, %rax
     e04: 48 81 c4 90 01 00 00         	addq	$0x190, %rsp            # imm = 0x190
     e0b: 5f                           	popq	%rdi
     e0c: c3                           	retq
     e0d: cc                           	int3
     e0e: cc                           	int3
     e0f: cc                           	int3
     e10: cc                           	int3
     e11: cc                           	int3
     e12: cc                           	int3
     e13: cc                           	int3
     e14: cc                           	int3
     e15: cc                           	int3
     e16: cc                           	int3
     e17: cc                           	int3
     e18: cc                           	int3
     e19: cc                           	int3
     e1a: cc                           	int3
     e1b: cc                           	int3
     e1c: cc                           	int3
     e1d: cc                           	int3
     e1e: cc                           	int3
     e1f: cc                           	int3

0000000000000e20 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout>:
     e20: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
     e25: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
     e2a: 57                           	pushq	%rdi
     e2b: 48 81 ec 10 01 00 00         	subq	$0x110, %rsp            # imm = 0x110
     e32: 48 8d 7c 24 40               	leaq	0x40(%rsp), %rdi
     e37: b9 34 00 00 00               	movl	$0x34, %ecx
     e3c: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
     e41: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
     e43: 48 8b 8c 24 20 01 00 00      	movq	0x120(%rsp), %rcx
     e4b: 48 83 bc 24 28 01 00 00 00   	cmpq	$0x0, 0x128(%rsp)
     e54: 74 16                        	je	0xe6c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x4c>
     e56: 48 8b 84 24 28 01 00 00      	movq	0x128(%rsp), %rax
     e5e: 48 8b 40 08                  	movq	0x8(%rax), %rax
     e62: 48 89 84 24 f8 00 00 00      	movq	%rax, 0xf8(%rsp)
     e6a: eb 0c                        	jmp	0xe78 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x58>
     e6c: 48 c7 84 24 f8 00 00 00 00 00 00 00  	movq	$0x0, 0xf8(%rsp)
     e78: 48 8b 84 24 f8 00 00 00      	movq	0xf8(%rsp), %rax
     e80: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
     e85: 48 83 bc 24 28 01 00 00 00   	cmpq	$0x0, 0x128(%rsp)
     e8e: 74 15                        	je	0xea5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x85>
     e90: 48 8b 84 24 28 01 00 00      	movq	0x128(%rsp), %rax
     e98: 48 8b 00                     	movq	(%rax), %rax
     e9b: 48 89 84 24 00 01 00 00      	movq	%rax, 0x100(%rsp)
     ea3: eb 0c                        	jmp	0xeb1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x91>
     ea5: 48 c7 84 24 00 01 00 00 00 00 00 00  	movq	$0x0, 0x100(%rsp)
     eb1: 48 8b 84 24 00 01 00 00      	movq	0x100(%rsp), %rax
     eb9: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
     ebe: b9 f5 ff ff ff               	movl	$0xfffffff5, %ecx       # imm = 0xFFFFFFF5
     ec3: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xec9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0xa9>
     ec9: 48 89 44 24 78               	movq	%rax, 0x78(%rsp)
     ece: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xed4 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0xb4>
     ed4: 89 84 24 84 00 00 00         	movl	%eax, 0x84(%rsp)
     edb: 48 c7 44 24 30 00 00 00 00   	movq	$0x0, 0x30(%rsp)
     ee4: c7 44 24 28 80 00 00 00      	movl	$0x80, 0x28(%rsp)
     eec: c7 44 24 20 04 00 00 00      	movl	$0x4, 0x20(%rsp)
     ef4: 45 33 c9                     	xorl	%r9d, %r9d
     ef7: 41 b8 01 00 00 00            	movl	$0x1, %r8d
     efd: ba 04 00 00 00               	movl	$0x4, %edx
     f02: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0xf09 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0xe9>
     f09: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xf0f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0xef>
     f0f: 48 89 84 24 98 00 00 00      	movq	%rax, 0x98(%rsp)
     f17: 48 83 bc 24 98 00 00 00 ff   	cmpq	$-0x1, 0x98(%rsp)
     f20: 0f 84 d5 00 00 00            	je	0xffb <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x1db>
     f26: c7 84 24 a4 00 00 00 00 00 00 00     	movl	$0x0, 0xa4(%rsp)
     f31: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
     f3a: 4c 8d 8c 24 a4 00 00 00      	leaq	0xa4(%rsp), %r9
     f42: 41 b8 08 00 00 00            	movl	$0x8, %r8d
     f48: 48 8d 54 24 48               	leaq	0x48(%rsp), %rdx
     f4d: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
     f55: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xf5b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x13b>
     f5b: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
     f64: 4c 8d 8c 24 a4 00 00 00      	leaq	0xa4(%rsp), %r9
     f6c: 41 b8 08 00 00 00            	movl	$0x8, %r8d
     f72: 48 8d 54 24 68               	leaq	0x68(%rsp), %rdx
     f77: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
     f7f: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xf85 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x165>
     f85: 48 8b 44 24 78               	movq	0x78(%rsp), %rax
     f8a: 48 89 84 24 c8 00 00 00      	movq	%rax, 0xc8(%rsp)
     f92: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
     f9b: 4c 8d 8c 24 a4 00 00 00      	leaq	0xa4(%rsp), %r9
     fa3: 41 b8 08 00 00 00            	movl	$0x8, %r8d
     fa9: 48 8d 94 24 c8 00 00 00      	leaq	0xc8(%rsp), %rdx
     fb1: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
     fb9: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xfbf <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x19f>
     fbf: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
     fc8: 4c 8d 8c 24 a4 00 00 00      	leaq	0xa4(%rsp), %r9
     fd0: 41 b8 04 00 00 00            	movl	$0x4, %r8d
     fd6: 48 8d 94 24 84 00 00 00      	leaq	0x84(%rsp), %rdx
     fde: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
     fe6: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xfec <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x1cc>
     fec: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
     ff4: ff 15 00 00 00 00            	callq	*(%rip)                 # 0xffa <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x1da>
     ffa: 90                           	nop
     ffb: 48 83 7c 24 78 00            	cmpq	$0x0, 0x78(%rsp)
    1001: 74 08                        	je	0x100b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x1eb>
    1003: 48 83 7c 24 78 ff            	cmpq	$-0x1, 0x78(%rsp)
    1009: 75 0c                        	jne	0x1017 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x1f7>
    100b: b1 05                        	movb	$0x5, %cl
    100d: e8 00 00 00 00               	callq	0x1012 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x1f2>
    1012: e9 e9 00 00 00               	jmp	0x1100 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2e0>
    1017: 48 c7 84 24 d8 00 00 00 00 00 00 00  	movq	$0x0, 0xd8(%rsp)
    1023: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    1028: 48 39 84 24 d8 00 00 00      	cmpq	%rax, 0xd8(%rsp)
    1030: 0f 83 ad 00 00 00            	jae	0x10e3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2c3>
    1036: c7 84 24 e4 00 00 00 00 00 00 00     	movl	$0x0, 0xe4(%rsp)
    1041: 48 8b 84 24 d8 00 00 00      	movq	0xd8(%rsp), %rax
    1049: 48 8b 4c 24 48               	movq	0x48(%rsp), %rcx
    104e: 48 2b c8                     	subq	%rax, %rcx
    1051: 48 8b c1                     	movq	%rcx, %rax
    1054: ba ff ff ff 7f               	movl	$0x7fffffff, %edx       # imm = 0x7FFFFFFF
    1059: 48 8b c8                     	movq	%rax, %rcx
    105c: e8 00 00 00 00               	callq	0x1061 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x241>
    1061: 89 84 24 f4 00 00 00         	movl	%eax, 0xf4(%rsp)
    1068: 48 8b 84 24 28 01 00 00      	movq	0x128(%rsp), %rax
    1070: 48 8b 00                     	movq	(%rax), %rax
    1073: 48 03 84 24 d8 00 00 00      	addq	0xd8(%rsp), %rax
    107b: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
    1084: 4c 8d 8c 24 e4 00 00 00      	leaq	0xe4(%rsp), %r9
    108c: 44 8b 84 24 f4 00 00 00      	movl	0xf4(%rsp), %r8d
    1094: 48 8b d0                     	movq	%rax, %rdx
    1097: 48 8b 4c 24 78               	movq	0x78(%rsp), %rcx
    109c: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x10a2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x282>
    10a2: 85 c0                        	testl	%eax, %eax
    10a4: 75 0f                        	jne	0x10b5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x295>
    10a6: e8 00 00 00 00               	callq	0x10ab <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x28b>
    10ab: 0f b6 c8                     	movzbl	%al, %ecx
    10ae: e8 00 00 00 00               	callq	0x10b3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x293>
    10b3: eb 4b                        	jmp	0x1100 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2e0>
    10b5: 83 bc 24 e4 00 00 00 00      	cmpl	$0x0, 0xe4(%rsp)
    10bd: 75 02                        	jne	0x10c1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2a1>
    10bf: eb 22                        	jmp	0x10e3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2c3>
    10c1: 8b 84 24 e4 00 00 00         	movl	0xe4(%rsp), %eax
    10c8: 48 8b 8c 24 d8 00 00 00      	movq	0xd8(%rsp), %rcx
    10d0: 48 03 c8                     	addq	%rax, %rcx
    10d3: 48 8b c1                     	movq	%rcx, %rax
    10d6: 48 89 84 24 d8 00 00 00      	movq	%rax, 0xd8(%rsp)
    10de: e9 40 ff ff ff               	jmp	0x1023 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x203>
    10e3: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    10e8: 48 39 84 24 d8 00 00 00      	cmpq	%rax, 0xd8(%rsp)
    10f0: 74 09                        	je	0x10fb <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2db>
    10f2: b1 05                        	movb	$0x5, %cl
    10f4: e8 00 00 00 00               	callq	0x10f9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2d9>
    10f9: eb 05                        	jmp	0x1100 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2e0>
    10fb: e8 00 00 00 00               	callq	0x1100 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2e0>
    1100: 48 8b f8                     	movq	%rax, %rdi
    1103: 48 8b cc                     	movq	%rsp, %rcx
    1106: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x110d <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2ed>
    110d: e8 00 00 00 00               	callq	0x1112 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout+0x2f2>
    1112: 48 8b c7                     	movq	%rdi, %rax
    1115: 48 81 c4 10 01 00 00         	addq	$0x110, %rsp            # imm = 0x110
    111c: 5f                           	popq	%rdi
    111d: c3                           	retq
    111e: cc                           	int3
    111f: cc                           	int3
    1120: cc                           	int3
    1121: cc                           	int3
    1122: cc                           	int3
    1123: cc                           	int3
    1124: cc                           	int3
    1125: cc                           	int3
    1126: cc                           	int3
    1127: cc                           	int3
    1128: cc                           	int3
    1129: cc                           	int3
    112a: cc                           	int3
    112b: cc                           	int3
    112c: cc                           	int3
    112d: cc                           	int3
    112e: cc                           	int3
    112f: cc                           	int3

0000000000001130 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr>:
    1130: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    1135: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    113a: 57                           	pushq	%rdi
    113b: 48 83 ec 70                  	subq	$0x70, %rsp
    113f: 48 8d 7c 24 30               	leaq	0x30(%rsp), %rdi
    1144: b9 10 00 00 00               	movl	$0x10, %ecx
    1149: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    114e: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    1150: 48 8b 8c 24 80 00 00 00      	movq	0x80(%rsp), %rcx
    1158: b9 f4 ff ff ff               	movl	$0xfffffff4, %ecx       # imm = 0xFFFFFFF4
    115d: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1163 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x33>
    1163: 48 89 44 24 30               	movq	%rax, 0x30(%rsp)
    1168: 48 83 7c 24 30 00            	cmpq	$0x0, 0x30(%rsp)
    116e: 74 08                        	je	0x1178 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x48>
    1170: 48 83 7c 24 30 ff            	cmpq	$-0x1, 0x30(%rsp)
    1176: 75 0c                        	jne	0x1184 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x54>
    1178: b1 05                        	movb	$0x5, %cl
    117a: e8 00 00 00 00               	callq	0x117f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x4f>
    117f: e9 f3 00 00 00               	jmp	0x1277 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x147>
    1184: 48 83 bc 24 88 00 00 00 00   	cmpq	$0x0, 0x88(%rsp)
    118d: 74 13                        	je	0x11a2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x72>
    118f: 48 8b 84 24 88 00 00 00      	movq	0x88(%rsp), %rax
    1197: 48 8b 40 08                  	movq	0x8(%rax), %rax
    119b: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    11a0: eb 09                        	jmp	0x11ab <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x7b>
    11a2: 48 c7 44 24 68 00 00 00 00   	movq	$0x0, 0x68(%rsp)
    11ab: 48 8b 44 24 68               	movq	0x68(%rsp), %rax
    11b0: 48 89 44 24 38               	movq	%rax, 0x38(%rsp)
    11b5: 48 c7 44 24 40 00 00 00 00   	movq	$0x0, 0x40(%rsp)
    11be: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    11c3: 48 39 44 24 40               	cmpq	%rax, 0x40(%rsp)
    11c8: 0f 83 8f 00 00 00            	jae	0x125d <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x12d>
    11ce: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%rsp)
    11d6: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    11db: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    11e0: 48 2b c8                     	subq	%rax, %rcx
    11e3: 48 8b c1                     	movq	%rcx, %rax
    11e6: ba ff ff ff 7f               	movl	$0x7fffffff, %edx       # imm = 0x7FFFFFFF
    11eb: 48 8b c8                     	movq	%rax, %rcx
    11ee: e8 00 00 00 00               	callq	0x11f3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0xc3>
    11f3: 89 44 24 64                  	movl	%eax, 0x64(%rsp)
    11f7: 48 8b 84 24 88 00 00 00      	movq	0x88(%rsp), %rax
    11ff: 48 8b 00                     	movq	(%rax), %rax
    1202: 48 03 44 24 40               	addq	0x40(%rsp), %rax
    1207: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
    1210: 4c 8d 4c 24 54               	leaq	0x54(%rsp), %r9
    1215: 44 8b 44 24 64               	movl	0x64(%rsp), %r8d
    121a: 48 8b d0                     	movq	%rax, %rdx
    121d: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
    1222: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1228 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0xf8>
    1228: 85 c0                        	testl	%eax, %eax
    122a: 75 0f                        	jne	0x123b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x10b>
    122c: e8 00 00 00 00               	callq	0x1231 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x101>
    1231: 0f b6 c8                     	movzbl	%al, %ecx
    1234: e8 00 00 00 00               	callq	0x1239 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x109>
    1239: eb 3c                        	jmp	0x1277 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x147>
    123b: 83 7c 24 54 00               	cmpl	$0x0, 0x54(%rsp)
    1240: 75 02                        	jne	0x1244 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x114>
    1242: eb 19                        	jmp	0x125d <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x12d>
    1244: 8b 44 24 54                  	movl	0x54(%rsp), %eax
    1248: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    124d: 48 03 c8                     	addq	%rax, %rcx
    1250: 48 8b c1                     	movq	%rcx, %rax
    1253: 48 89 44 24 40               	movq	%rax, 0x40(%rsp)
    1258: e9 61 ff ff ff               	jmp	0x11be <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x8e>
    125d: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    1262: 48 39 44 24 40               	cmpq	%rax, 0x40(%rsp)
    1267: 74 09                        	je	0x1272 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x142>
    1269: b1 05                        	movb	$0x5, %cl
    126b: e8 00 00 00 00               	callq	0x1270 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x140>
    1270: eb 05                        	jmp	0x1277 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x147>
    1272: e8 00 00 00 00               	callq	0x1277 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x147>
    1277: 48 8b f8                     	movq	%rax, %rdi
    127a: 48 8b cc                     	movq	%rsp, %rcx
    127d: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x1284 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x154>
    1284: e8 00 00 00 00               	callq	0x1289 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstderr+0x159>
    1289: 48 8b c7                     	movq	%rdi, %rax
    128c: 48 83 c4 70                  	addq	$0x70, %rsp
    1290: 5f                           	popq	%rdi
    1291: c3                           	retq
    1292: cc                           	int3
    1293: cc                           	int3
    1294: cc                           	int3
    1295: cc                           	int3
    1296: cc                           	int3
    1297: cc                           	int3
    1298: cc                           	int3
    1299: cc                           	int3
    129a: cc                           	int3
    129b: cc                           	int3
    129c: cc                           	int3
    129d: cc                           	int3
    129e: cc                           	int3
    129f: cc                           	int3

00000000000012a0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists>:
    12a0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    12a5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    12aa: 57                           	pushq	%rdi
    12ab: 48 81 ec 80 00 00 00         	subq	$0x80, %rsp
    12b2: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    12b7: b9 18 00 00 00               	movl	$0x18, %ecx
    12bc: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    12c1: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    12c3: 48 8b 8c 24 90 00 00 00      	movq	0x90(%rsp), %rcx
    12cb: b2 58                        	movb	$0x58, %dl
    12cd: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x12d4 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0x34>
    12d4: e8 00 00 00 00               	callq	0x12d9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0x39>
    12d9: 48 8b 8c 24 90 00 00 00      	movq	0x90(%rsp), %rcx
    12e1: e8 00 00 00 00               	callq	0x12e6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0x46>
    12e6: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    12eb: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    12f4: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%rsp)
    12fc: 4c 8d 4c 24 54               	leaq	0x54(%rsp), %r9
    1301: 4c 8d 44 24 38               	leaq	0x38(%rsp), %r8
    1306: 48 8b 94 24 98 00 00 00      	movq	0x98(%rsp), %rdx
    130e: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    1313: e8 00 00 00 00               	callq	0x1318 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0x78>
    1318: 85 c0                        	testl	%eax, %eax
    131a: 75 04                        	jne	0x1320 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0x80>
    131c: 32 c0                        	xorb	%al, %al
    131e: eb 52                        	jmp	0x1372 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0xd2>
    1320: 45 33 c0                     	xorl	%r8d, %r8d
    1323: 8b 54 24 54                  	movl	0x54(%rsp), %edx
    1327: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    132c: e8 00 00 00 00               	callq	0x1331 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0x91>
    1331: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    1336: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    133b: e8 00 00 00 00               	callq	0x1340 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0xa0>
    1340: 48 83 7c 24 68 00            	cmpq	$0x0, 0x68(%rsp)
    1346: 75 04                        	jne	0x134c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0xac>
    1348: 32 c0                        	xorb	%al, %al
    134a: eb 26                        	jmp	0x1372 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0xd2>
    134c: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    1351: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1357 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0xb7>
    1357: 89 44 24 70                  	movl	%eax, 0x70(%rsp)
    135b: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    1360: e8 00 00 00 00               	callq	0x1365 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0xc5>
    1365: 83 7c 24 70 ff               	cmpl	$-0x1, 0x70(%rsp)
    136a: 75 04                        	jne	0x1370 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0xd0>
    136c: 32 c0                        	xorb	%al, %al
    136e: eb 02                        	jmp	0x1372 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0xd2>
    1370: b0 01                        	movb	$0x1, %al
    1372: 48 8b f8                     	movq	%rax, %rdi
    1375: 48 8b cc                     	movq	%rsp, %rcx
    1378: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x137f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0xdf>
    137f: e8 00 00 00 00               	callq	0x1384 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists+0xe4>
    1384: 48 8b c7                     	movq	%rdi, %rax
    1387: 48 81 c4 80 00 00 00         	addq	$0x80, %rsp
    138e: 5f                           	popq	%rdi
    138f: c3                           	retq
    1390: cc                           	int3
    1391: cc                           	int3
    1392: cc                           	int3
    1393: cc                           	int3
    1394: cc                           	int3
    1395: cc                           	int3
    1396: cc                           	int3
    1397: cc                           	int3
    1398: cc                           	int3
    1399: cc                           	int3
    139a: cc                           	int3
    139b: cc                           	int3
    139c: cc                           	int3
    139d: cc                           	int3
    139e: cc                           	int3
    139f: cc                           	int3

00000000000013a0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove>:
    13a0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    13a5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    13aa: 57                           	pushq	%rdi
    13ab: 48 81 ec 80 00 00 00         	subq	$0x80, %rsp
    13b2: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    13b7: b9 18 00 00 00               	movl	$0x18, %ecx
    13bc: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    13c1: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    13c3: 48 8b 8c 24 90 00 00 00      	movq	0x90(%rsp), %rcx
    13cb: 48 8b 8c 24 90 00 00 00      	movq	0x90(%rsp), %rcx
    13d3: e8 00 00 00 00               	callq	0x13d8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x38>
    13d8: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    13dd: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    13e6: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%rsp)
    13ee: 4c 8d 4c 24 54               	leaq	0x54(%rsp), %r9
    13f3: 4c 8d 44 24 38               	leaq	0x38(%rsp), %r8
    13f8: 48 8b 94 24 98 00 00 00      	movq	0x98(%rsp), %rdx
    1400: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    1405: e8 00 00 00 00               	callq	0x140a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x6a>
    140a: 85 c0                        	testl	%eax, %eax
    140c: 75 0c                        	jne	0x141a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x7a>
    140e: b1 03                        	movb	$0x3, %cl
    1410: e8 00 00 00 00               	callq	0x1415 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x75>
    1415: e9 cb 00 00 00               	jmp	0x14e5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x145>
    141a: 45 33 c0                     	xorl	%r8d, %r8d
    141d: 8b 54 24 54                  	movl	0x54(%rsp), %edx
    1421: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    1426: e8 00 00 00 00               	callq	0x142b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x8b>
    142b: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    1430: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    1435: e8 00 00 00 00               	callq	0x143a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x9a>
    143a: 48 83 7c 24 68 00            	cmpq	$0x0, 0x68(%rsp)
    1440: 75 0c                        	jne	0x144e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0xae>
    1442: b1 03                        	movb	$0x3, %cl
    1444: e8 00 00 00 00               	callq	0x1449 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0xa9>
    1449: e9 97 00 00 00               	jmp	0x14e5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x145>
    144e: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    1453: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1459 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0xb9>
    1459: 89 44 24 70                  	movl	%eax, 0x70(%rsp)
    145d: 83 7c 24 70 ff               	cmpl	$-0x1, 0x70(%rsp)
    1462: 75 19                        	jne	0x147d <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0xdd>
    1464: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    1469: e8 00 00 00 00               	callq	0x146e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0xce>
    146e: e8 00 00 00 00               	callq	0x1473 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0xd3>
    1473: 0f b6 c8                     	movzbl	%al, %ecx
    1476: e8 00 00 00 00               	callq	0x147b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0xdb>
    147b: eb 68                        	jmp	0x14e5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x145>
    147d: c7 44 24 74 00 00 00 00      	movl	$0x0, 0x74(%rsp)
    1485: 8b 44 24 70                  	movl	0x70(%rsp), %eax
    1489: 83 e0 10                     	andl	$0x10, %eax
    148c: 85 c0                        	testl	%eax, %eax
    148e: 74 11                        	je	0x14a1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x101>
    1490: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    1495: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x149b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0xfb>
    149b: 89 44 24 74                  	movl	%eax, 0x74(%rsp)
    149f: eb 0f                        	jmp	0x14b0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x110>
    14a1: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    14a6: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x14ac <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x10c>
    14ac: 89 44 24 74                  	movl	%eax, 0x74(%rsp)
    14b0: 83 7c 24 74 00               	cmpl	$0x0, 0x74(%rsp)
    14b5: 75 1f                        	jne	0x14d6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x136>
    14b7: e8 00 00 00 00               	callq	0x14bc <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x11c>
    14bc: 88 44 24 78                  	movb	%al, 0x78(%rsp)
    14c0: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    14c5: e8 00 00 00 00               	callq	0x14ca <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x12a>
    14ca: 0f b6 4c 24 78               	movzbl	0x78(%rsp), %ecx
    14cf: e8 00 00 00 00               	callq	0x14d4 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x134>
    14d4: eb 0f                        	jmp	0x14e5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x145>
    14d6: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    14db: e8 00 00 00 00               	callq	0x14e0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x140>
    14e0: e8 00 00 00 00               	callq	0x14e5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x145>
    14e5: 48 8b f8                     	movq	%rax, %rdi
    14e8: 48 8b cc                     	movq	%rsp, %rcx
    14eb: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x14f2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x152>
    14f2: e8 00 00 00 00               	callq	0x14f7 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aremove+0x157>
    14f7: 48 8b c7                     	movq	%rdi, %rax
    14fa: 48 81 c4 80 00 00 00         	addq	$0x80, %rsp
    1501: 5f                           	popq	%rdi
    1502: c3                           	retq
    1503: cc                           	int3
    1504: cc                           	int3
    1505: cc                           	int3
    1506: cc                           	int3
    1507: cc                           	int3
    1508: cc                           	int3
    1509: cc                           	int3
    150a: cc                           	int3
    150b: cc                           	int3
    150c: cc                           	int3
    150d: cc                           	int3
    150e: cc                           	int3
    150f: cc                           	int3

0000000000001510 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir>:
    1510: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    1515: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    151a: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    151f: 56                           	pushq	%rsi
    1520: 57                           	pushq	%rdi
    1521: 48 81 ec e8 04 00 00         	subq	$0x4e8, %rsp            # imm = 0x4E8
    1528: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    152d: b9 32 01 00 00               	movl	$0x132, %ecx            # imm = 0x132
    1532: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    1537: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    1539: 48 8b 8c 24 00 05 00 00      	movq	0x500(%rsp), %rcx
    1541: b2 4f                        	movb	$0x4f, %dl
    1543: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x154a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x3a>
    154a: e8 00 00 00 00               	callq	0x154f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x3f>
    154f: 48 8b 8c 24 08 05 00 00      	movq	0x508(%rsp), %rcx
    1557: e8 00 00 00 00               	callq	0x155c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x4c>
    155c: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    1561: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    156a: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%rsp)
    1572: 4c 8d 4c 24 54               	leaq	0x54(%rsp), %r9
    1577: 4c 8d 44 24 38               	leaq	0x38(%rsp), %r8
    157c: 48 8b 94 24 10 05 00 00      	movq	0x510(%rsp), %rdx
    1584: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    1589: e8 00 00 00 00               	callq	0x158e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x7e>
    158e: 85 c0                        	testl	%eax, %eax
    1590: 75 2e                        	jne	0x15c0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xb0>
    1592: b2 03                        	movb	$0x3, %dl
    1594: 48 8d 8c 24 08 04 00 00      	leaq	0x408(%rsp), %rcx
    159c: e8 00 00 00 00               	callq	0x15a1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x91>
    15a1: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    15a9: 48 8b f0                     	movq	%rax, %rsi
    15ac: b9 10 00 00 00               	movl	$0x10, %ecx
    15b1: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    15b3: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    15bb: e9 d4 0f 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    15c0: c7 44 24 74 00 00 00 00      	movl	$0x0, 0x74(%rsp)
    15c8: 4c 8d 44 24 74               	leaq	0x74(%rsp), %r8
    15cd: 8b 54 24 54                  	movl	0x54(%rsp), %edx
    15d1: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    15d6: e8 00 00 00 00               	callq	0x15db <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xcb>
    15db: 48 89 84 24 88 00 00 00      	movq	%rax, 0x88(%rsp)
    15e3: 48 83 bc 24 88 00 00 00 00   	cmpq	$0x0, 0x88(%rsp)
    15ec: 75 39                        	jne	0x1627 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x117>
    15ee: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    15f3: e8 00 00 00 00               	callq	0x15f8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xe8>
    15f8: 90                           	nop
    15f9: b2 03                        	movb	$0x3, %dl
    15fb: 48 8d 8c 24 18 04 00 00      	leaq	0x418(%rsp), %rcx
    1603: e8 00 00 00 00               	callq	0x1608 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xf8>
    1608: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    1610: 48 8b f0                     	movq	%rax, %rsi
    1613: b9 10 00 00 00               	movl	$0x10, %ecx
    1618: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    161a: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    1622: e9 6d 0f 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    1627: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    162f: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1635 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x125>
    1635: 89 84 24 90 00 00 00         	movl	%eax, 0x90(%rsp)
    163c: 83 bc 24 90 00 00 00 ff      	cmpl	$-0x1, 0x90(%rsp)
    1644: 75 58                        	jne	0x169e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x18e>
    1646: e8 00 00 00 00               	callq	0x164b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x13b>
    164b: 88 84 24 94 00 00 00         	movb	%al, 0x94(%rsp)
    1652: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    165a: e8 00 00 00 00               	callq	0x165f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x14f>
    165f: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    1664: e8 00 00 00 00               	callq	0x1669 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x159>
    1669: 90                           	nop
    166a: 0f b6 94 24 94 00 00 00      	movzbl	0x94(%rsp), %edx
    1672: 48 8d 8c 24 28 04 00 00      	leaq	0x428(%rsp), %rcx
    167a: e8 00 00 00 00               	callq	0x167f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x16f>
    167f: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    1687: 48 8b f0                     	movq	%rax, %rsi
    168a: b9 10 00 00 00               	movl	$0x10, %ecx
    168f: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    1691: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    1699: e9 f6 0e 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    169e: 8b 84 24 90 00 00 00         	movl	0x90(%rsp), %eax
    16a5: 83 e0 10                     	andl	$0x10, %eax
    16a8: 85 c0                        	testl	%eax, %eax
    16aa: 75 46                        	jne	0x16f2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1e2>
    16ac: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    16b4: e8 00 00 00 00               	callq	0x16b9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1a9>
    16b9: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    16be: e8 00 00 00 00               	callq	0x16c3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1b3>
    16c3: 90                           	nop
    16c4: b2 03                        	movb	$0x3, %dl
    16c6: 48 8d 8c 24 38 04 00 00      	leaq	0x438(%rsp), %rcx
    16ce: e8 00 00 00 00               	callq	0x16d3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1c3>
    16d3: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    16db: 48 8b f0                     	movq	%rax, %rsi
    16de: b9 10 00 00 00               	movl	$0x10, %ecx
    16e3: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    16e5: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    16ed: e9 a2 0e 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    16f2: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    16f6: 83 c0 02                     	addl	$0x2, %eax
    16f9: 89 84 24 98 00 00 00         	movl	%eax, 0x98(%rsp)
    1700: 8b 84 24 98 00 00 00         	movl	0x98(%rsp), %eax
    1707: ff c0                        	incl	%eax
    1709: 8b c0                        	movl	%eax, %eax
    170b: 48 d1 e0                     	shlq	%rax
    170e: 48 8b c8                     	movq	%rax, %rcx
    1711: e8 00 00 00 00               	callq	0x1716 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x206>
    1716: 48 89 84 24 a0 00 00 00      	movq	%rax, 0xa0(%rsp)
    171e: 48 83 bc 24 a0 00 00 00 00   	cmpq	$0x0, 0xa0(%rsp)
    1727: 75 46                        	jne	0x176f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x25f>
    1729: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    1731: e8 00 00 00 00               	callq	0x1736 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x226>
    1736: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    173b: e8 00 00 00 00               	callq	0x1740 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x230>
    1740: 90                           	nop
    1741: b2 05                        	movb	$0x5, %dl
    1743: 48 8d 8c 24 48 04 00 00      	leaq	0x448(%rsp), %rcx
    174b: e8 00 00 00 00               	callq	0x1750 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x240>
    1750: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    1758: 48 8b f0                     	movq	%rax, %rsi
    175b: b9 10 00 00 00               	movl	$0x10, %ecx
    1760: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    1762: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    176a: e9 25 0e 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    176f: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    1773: 48 d1 e0                     	shlq	%rax
    1776: 4c 8b c0                     	movq	%rax, %r8
    1779: 48 8b 94 24 88 00 00 00      	movq	0x88(%rsp), %rdx
    1781: 48 8b 8c 24 a0 00 00 00      	movq	0xa0(%rsp), %rcx
    1789: e8 00 00 00 00               	callq	0x178e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x27e>
    178e: 83 7c 24 74 00               	cmpl	$0x0, 0x74(%rsp)
    1793: 76 60                        	jbe	0x17f5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x2e5>
    1795: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    1799: ff c8                        	decl	%eax
    179b: 8b c0                        	movl	%eax, %eax
    179d: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    17a5: 0f b7 04 41                  	movzwl	(%rcx,%rax,2), %eax
    17a9: 83 f8 5c                     	cmpl	$0x5c, %eax
    17ac: 74 47                        	je	0x17f5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x2e5>
    17ae: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    17b2: b9 5c 00 00 00               	movl	$0x5c, %ecx
    17b7: 48 8b 94 24 a0 00 00 00      	movq	0xa0(%rsp), %rdx
    17bf: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    17c3: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    17c7: ff c0                        	incl	%eax
    17c9: 8b c0                        	movl	%eax, %eax
    17cb: b9 2a 00 00 00               	movl	$0x2a, %ecx
    17d0: 48 8b 94 24 a0 00 00 00      	movq	0xa0(%rsp), %rdx
    17d8: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    17dc: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    17e0: 83 c0 02                     	addl	$0x2, %eax
    17e3: 8b c0                        	movl	%eax, %eax
    17e5: 33 c9                        	xorl	%ecx, %ecx
    17e7: 48 8b 94 24 a0 00 00 00      	movq	0xa0(%rsp), %rdx
    17ef: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    17f3: eb 2b                        	jmp	0x1820 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x310>
    17f5: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    17f9: b9 2a 00 00 00               	movl	$0x2a, %ecx
    17fe: 48 8b 94 24 a0 00 00 00      	movq	0xa0(%rsp), %rdx
    1806: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    180a: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    180e: ff c0                        	incl	%eax
    1810: 8b c0                        	movl	%eax, %eax
    1812: 33 c9                        	xorl	%ecx, %ecx
    1814: 48 8b 94 24 a0 00 00 00      	movq	0xa0(%rsp), %rdx
    181c: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    1820: 48 8d 94 24 c0 00 00 00      	leaq	0xc0(%rsp), %rdx
    1828: 48 8b 8c 24 a0 00 00 00      	movq	0xa0(%rsp), %rcx
    1830: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1836 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x326>
    1836: 48 89 84 24 18 03 00 00      	movq	%rax, 0x318(%rsp)
    183e: 48 8b 8c 24 a0 00 00 00      	movq	0xa0(%rsp), %rcx
    1846: e8 00 00 00 00               	callq	0x184b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x33b>
    184b: 48 83 bc 24 18 03 00 00 ff   	cmpq	$-0x1, 0x318(%rsp)
    1854: 75 58                        	jne	0x18ae <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x39e>
    1856: e8 00 00 00 00               	callq	0x185b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x34b>
    185b: 88 84 24 20 03 00 00         	movb	%al, 0x320(%rsp)
    1862: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    186a: e8 00 00 00 00               	callq	0x186f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x35f>
    186f: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    1874: e8 00 00 00 00               	callq	0x1879 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x369>
    1879: 90                           	nop
    187a: 0f b6 94 24 20 03 00 00      	movzbl	0x320(%rsp), %edx
    1882: 48 8d 8c 24 58 04 00 00      	leaq	0x458(%rsp), %rcx
    188a: e8 00 00 00 00               	callq	0x188f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x37f>
    188f: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    1897: 48 8b f0                     	movq	%rax, %rsi
    189a: b9 10 00 00 00               	movl	$0x10, %ecx
    189f: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    18a1: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    18a9: e9 e6 0c 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    18ae: c7 84 24 24 03 00 00 10 00 00 00     	movl	$0x10, 0x324(%rsp)
    18b9: c7 84 24 28 03 00 00 00 00 00 00     	movl	$0x0, 0x328(%rsp)
    18c4: 8b 84 24 24 03 00 00         	movl	0x324(%rsp), %eax
    18cb: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    18cf: 48 8b c8                     	movq	%rax, %rcx
    18d2: e8 00 00 00 00               	callq	0x18d7 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x3c7>
    18d7: 48 89 84 24 30 03 00 00      	movq	%rax, 0x330(%rsp)
    18df: 48 83 bc 24 30 03 00 00 00   	cmpq	$0x0, 0x330(%rsp)
    18e8: 75 54                        	jne	0x193e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x42e>
    18ea: 48 8b 8c 24 18 03 00 00      	movq	0x318(%rsp), %rcx
    18f2: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x18f8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x3e8>
    18f8: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    1900: e8 00 00 00 00               	callq	0x1905 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x3f5>
    1905: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    190a: e8 00 00 00 00               	callq	0x190f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x3ff>
    190f: 90                           	nop
    1910: b2 05                        	movb	$0x5, %dl
    1912: 48 8d 8c 24 68 04 00 00      	leaq	0x468(%rsp), %rcx
    191a: e8 00 00 00 00               	callq	0x191f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x40f>
    191f: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    1927: 48 8b f0                     	movq	%rax, %rsi
    192a: b9 10 00 00 00               	movl	$0x10, %ecx
    192f: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    1931: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    1939: e9 56 0c 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    193e: 48 8d 84 24 ec 00 00 00      	leaq	0xec(%rsp), %rax
    1946: 48 89 84 24 38 03 00 00      	movq	%rax, 0x338(%rsp)
    194e: b8 02 00 00 00               	movl	$0x2, %eax
    1953: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    1957: 48 8b 8c 24 38 03 00 00      	movq	0x338(%rsp), %rcx
    195f: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    1963: 83 f8 2e                     	cmpl	$0x2e, %eax
    1966: 75 1e                        	jne	0x1986 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x476>
    1968: b8 02 00 00 00               	movl	$0x2, %eax
    196d: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    1971: 48 8b 8c 24 38 03 00 00      	movq	0x338(%rsp), %rcx
    1979: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    197d: 85 c0                        	testl	%eax, %eax
    197f: 75 05                        	jne	0x1986 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x476>
    1981: e9 5a 06 00 00               	jmp	0x1fe0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xad0>
    1986: b8 02 00 00 00               	movl	$0x2, %eax
    198b: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    198f: 48 8b 8c 24 38 03 00 00      	movq	0x338(%rsp), %rcx
    1997: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    199b: 83 f8 2e                     	cmpl	$0x2e, %eax
    199e: 75 38                        	jne	0x19d8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x4c8>
    19a0: b8 02 00 00 00               	movl	$0x2, %eax
    19a5: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    19a9: 48 8b 8c 24 38 03 00 00      	movq	0x338(%rsp), %rcx
    19b1: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    19b5: 83 f8 2e                     	cmpl	$0x2e, %eax
    19b8: 75 1e                        	jne	0x19d8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x4c8>
    19ba: b8 02 00 00 00               	movl	$0x2, %eax
    19bf: 48 6b c0 02                  	imulq	$0x2, %rax, %rax
    19c3: 48 8b 8c 24 38 03 00 00      	movq	0x338(%rsp), %rcx
    19cb: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    19cf: 85 c0                        	testl	%eax, %eax
    19d1: 75 05                        	jne	0x19d8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x4c8>
    19d3: e9 08 06 00 00               	jmp	0x1fe0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xad0>
    19d8: 48 8b 8c 24 38 03 00 00      	movq	0x338(%rsp), %rcx
    19e0: e8 00 00 00 00               	callq	0x19e5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x4d5>
    19e5: 89 84 24 40 03 00 00         	movl	%eax, 0x340(%rsp)
    19ec: 8b 84 24 40 03 00 00         	movl	0x340(%rsp), %eax
    19f3: ff c0                        	incl	%eax
    19f5: 8b c0                        	movl	%eax, %eax
    19f7: 48 d1 e0                     	shlq	%rax
    19fa: 48 8b c8                     	movq	%rax, %rcx
    19fd: e8 00 00 00 00               	callq	0x1a02 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x4f2>
    1a02: 48 89 84 24 48 03 00 00      	movq	%rax, 0x348(%rsp)
    1a0a: 48 83 bc 24 48 03 00 00 00   	cmpq	$0x0, 0x348(%rsp)
    1a13: 0f 85 e8 00 00 00            	jne	0x1b01 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x5f1>
    1a19: 48 8b 8c 24 18 03 00 00      	movq	0x318(%rsp), %rcx
    1a21: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1a27 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x517>
    1a27: 90                           	nop
    1a28: c7 84 24 50 03 00 00 00 00 00 00     	movl	$0x0, 0x350(%rsp)
    1a33: eb 10                        	jmp	0x1a45 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x535>
    1a35: 8b 84 24 50 03 00 00         	movl	0x350(%rsp), %eax
    1a3c: ff c0                        	incl	%eax
    1a3e: 89 84 24 50 03 00 00         	movl	%eax, 0x350(%rsp)
    1a45: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1a4c: 39 84 24 50 03 00 00         	cmpl	%eax, 0x350(%rsp)
    1a53: 73 59                        	jae	0x1aae <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x59e>
    1a55: 8b 84 24 50 03 00 00         	movl	0x350(%rsp), %eax
    1a5c: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1a60: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1a68: 48 8b 0c 01                  	movq	(%rcx,%rax), %rcx
    1a6c: e8 00 00 00 00               	callq	0x1a71 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x561>
    1a71: 8b 84 24 50 03 00 00         	movl	0x350(%rsp), %eax
    1a78: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1a7c: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1a84: 48 8b 4c 01 10               	movq	0x10(%rcx,%rax), %rcx
    1a89: e8 00 00 00 00               	callq	0x1a8e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x57e>
    1a8e: 8b 84 24 50 03 00 00         	movl	0x350(%rsp), %eax
    1a95: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1a99: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1aa1: 48 8b 4c 01 20               	movq	0x20(%rcx,%rax), %rcx
    1aa6: e8 00 00 00 00               	callq	0x1aab <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x59b>
    1aab: 90                           	nop
    1aac: eb 87                        	jmp	0x1a35 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x525>
    1aae: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1ab6: e8 00 00 00 00               	callq	0x1abb <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x5ab>
    1abb: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    1ac3: e8 00 00 00 00               	callq	0x1ac8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x5b8>
    1ac8: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    1acd: e8 00 00 00 00               	callq	0x1ad2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x5c2>
    1ad2: 90                           	nop
    1ad3: b2 05                        	movb	$0x5, %dl
    1ad5: 48 8d 8c 24 78 04 00 00      	leaq	0x478(%rsp), %rcx
    1add: e8 00 00 00 00               	callq	0x1ae2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x5d2>
    1ae2: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    1aea: 48 8b f0                     	movq	%rax, %rsi
    1aed: b9 10 00 00 00               	movl	$0x10, %ecx
    1af2: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    1af4: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    1afc: e9 93 0a 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    1b01: 8b 84 24 40 03 00 00         	movl	0x340(%rsp), %eax
    1b08: 48 d1 e0                     	shlq	%rax
    1b0b: 4c 8b c0                     	movq	%rax, %r8
    1b0e: 48 8b 94 24 38 03 00 00      	movq	0x338(%rsp), %rdx
    1b16: 48 8b 8c 24 48 03 00 00      	movq	0x348(%rsp), %rcx
    1b1e: e8 00 00 00 00               	callq	0x1b23 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x613>
    1b23: 8b 84 24 40 03 00 00         	movl	0x340(%rsp), %eax
    1b2a: 33 c9                        	xorl	%ecx, %ecx
    1b2c: 48 8b 94 24 48 03 00 00      	movq	0x348(%rsp), %rdx
    1b34: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    1b38: c7 84 24 64 03 00 00 00 00 00 00     	movl	$0x0, 0x364(%rsp)
    1b43: 4c 8d 84 24 64 03 00 00      	leaq	0x364(%rsp), %r8
    1b4b: 8b 94 24 40 03 00 00         	movl	0x340(%rsp), %edx
    1b52: 48 8b 8c 24 48 03 00 00      	movq	0x348(%rsp), %rcx
    1b5a: e8 00 00 00 00               	callq	0x1b5f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x64f>
    1b5f: 48 89 84 24 78 03 00 00      	movq	%rax, 0x378(%rsp)
    1b67: 48 83 bc 24 78 03 00 00 00   	cmpq	$0x0, 0x378(%rsp)
    1b70: 0f 85 f5 00 00 00            	jne	0x1c6b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x75b>
    1b76: 48 8b 8c 24 48 03 00 00      	movq	0x348(%rsp), %rcx
    1b7e: e8 00 00 00 00               	callq	0x1b83 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x673>
    1b83: 48 8b 8c 24 18 03 00 00      	movq	0x318(%rsp), %rcx
    1b8b: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1b91 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x681>
    1b91: 90                           	nop
    1b92: c7 84 24 80 03 00 00 00 00 00 00     	movl	$0x0, 0x380(%rsp)
    1b9d: eb 10                        	jmp	0x1baf <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x69f>
    1b9f: 8b 84 24 80 03 00 00         	movl	0x380(%rsp), %eax
    1ba6: ff c0                        	incl	%eax
    1ba8: 89 84 24 80 03 00 00         	movl	%eax, 0x380(%rsp)
    1baf: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1bb6: 39 84 24 80 03 00 00         	cmpl	%eax, 0x380(%rsp)
    1bbd: 73 59                        	jae	0x1c18 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x708>
    1bbf: 8b 84 24 80 03 00 00         	movl	0x380(%rsp), %eax
    1bc6: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1bca: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1bd2: 48 8b 0c 01                  	movq	(%rcx,%rax), %rcx
    1bd6: e8 00 00 00 00               	callq	0x1bdb <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x6cb>
    1bdb: 8b 84 24 80 03 00 00         	movl	0x380(%rsp), %eax
    1be2: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1be6: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1bee: 48 8b 4c 01 10               	movq	0x10(%rcx,%rax), %rcx
    1bf3: e8 00 00 00 00               	callq	0x1bf8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x6e8>
    1bf8: 8b 84 24 80 03 00 00         	movl	0x380(%rsp), %eax
    1bff: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1c03: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1c0b: 48 8b 4c 01 20               	movq	0x20(%rcx,%rax), %rcx
    1c10: e8 00 00 00 00               	callq	0x1c15 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x705>
    1c15: 90                           	nop
    1c16: eb 87                        	jmp	0x1b9f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x68f>
    1c18: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1c20: e8 00 00 00 00               	callq	0x1c25 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x715>
    1c25: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    1c2d: e8 00 00 00 00               	callq	0x1c32 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x722>
    1c32: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    1c37: e8 00 00 00 00               	callq	0x1c3c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x72c>
    1c3c: 90                           	nop
    1c3d: b2 05                        	movb	$0x5, %dl
    1c3f: 48 8d 8c 24 88 04 00 00      	leaq	0x488(%rsp), %rcx
    1c47: e8 00 00 00 00               	callq	0x1c4c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x73c>
    1c4c: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    1c54: 48 8b f0                     	movq	%rax, %rsi
    1c57: b9 10 00 00 00               	movl	$0x10, %ecx
    1c5c: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    1c5e: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    1c66: e9 29 09 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    1c6b: c7 84 24 94 03 00 00 00 00 00 00     	movl	$0x0, 0x394(%rsp)
    1c76: 4c 8d 84 24 94 03 00 00      	leaq	0x394(%rsp), %r8
    1c7e: 8b 94 24 40 03 00 00         	movl	0x340(%rsp), %edx
    1c85: 48 8b 8c 24 48 03 00 00      	movq	0x348(%rsp), %rcx
    1c8d: e8 00 00 00 00               	callq	0x1c92 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x782>
    1c92: 48 89 84 24 a8 03 00 00      	movq	%rax, 0x3a8(%rsp)
    1c9a: 48 83 bc 24 a8 03 00 00 00   	cmpq	$0x0, 0x3a8(%rsp)
    1ca3: 75 1e                        	jne	0x1cc3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x7b3>
    1ca5: 48 8b 84 24 78 03 00 00      	movq	0x378(%rsp), %rax
    1cad: 48 89 84 24 a8 03 00 00      	movq	%rax, 0x3a8(%rsp)
    1cb5: 8b 84 24 64 03 00 00         	movl	0x364(%rsp), %eax
    1cbc: 89 84 24 94 03 00 00         	movl	%eax, 0x394(%rsp)
    1cc3: 8b 84 24 24 03 00 00         	movl	0x324(%rsp), %eax
    1cca: 39 84 24 28 03 00 00         	cmpl	%eax, 0x328(%rsp)
    1cd1: 0f 85 fb 01 00 00            	jne	0x1ed2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x9c2>
    1cd7: 8b 84 24 24 03 00 00         	movl	0x324(%rsp), %eax
    1cde: d1 e0                        	shll	%eax
    1ce0: 89 84 24 b0 03 00 00         	movl	%eax, 0x3b0(%rsp)
    1ce7: 8b 84 24 b0 03 00 00         	movl	0x3b0(%rsp), %eax
    1cee: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1cf2: 48 8b c8                     	movq	%rax, %rcx
    1cf5: e8 00 00 00 00               	callq	0x1cfa <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x7ea>
    1cfa: 48 89 84 24 b8 03 00 00      	movq	%rax, 0x3b8(%rsp)
    1d02: 48 83 bc 24 b8 03 00 00 00   	cmpq	$0x0, 0x3b8(%rsp)
    1d0b: 0f 85 22 01 00 00            	jne	0x1e33 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x923>
    1d11: 48 8b 84 24 78 03 00 00      	movq	0x378(%rsp), %rax
    1d19: 48 39 84 24 a8 03 00 00      	cmpq	%rax, 0x3a8(%rsp)
    1d21: 74 0e                        	je	0x1d31 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x821>
    1d23: 48 8b 8c 24 a8 03 00 00      	movq	0x3a8(%rsp), %rcx
    1d2b: e8 00 00 00 00               	callq	0x1d30 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x820>
    1d30: 90                           	nop
    1d31: 48 8b 8c 24 78 03 00 00      	movq	0x378(%rsp), %rcx
    1d39: e8 00 00 00 00               	callq	0x1d3e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x82e>
    1d3e: 48 8b 8c 24 48 03 00 00      	movq	0x348(%rsp), %rcx
    1d46: e8 00 00 00 00               	callq	0x1d4b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x83b>
    1d4b: 48 8b 8c 24 18 03 00 00      	movq	0x318(%rsp), %rcx
    1d53: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1d59 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x849>
    1d59: 90                           	nop
    1d5a: c7 84 24 c0 03 00 00 00 00 00 00     	movl	$0x0, 0x3c0(%rsp)
    1d65: eb 10                        	jmp	0x1d77 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x867>
    1d67: 8b 84 24 c0 03 00 00         	movl	0x3c0(%rsp), %eax
    1d6e: ff c0                        	incl	%eax
    1d70: 89 84 24 c0 03 00 00         	movl	%eax, 0x3c0(%rsp)
    1d77: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1d7e: 39 84 24 c0 03 00 00         	cmpl	%eax, 0x3c0(%rsp)
    1d85: 73 59                        	jae	0x1de0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x8d0>
    1d87: 8b 84 24 c0 03 00 00         	movl	0x3c0(%rsp), %eax
    1d8e: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1d92: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1d9a: 48 8b 0c 01                  	movq	(%rcx,%rax), %rcx
    1d9e: e8 00 00 00 00               	callq	0x1da3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x893>
    1da3: 8b 84 24 c0 03 00 00         	movl	0x3c0(%rsp), %eax
    1daa: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1dae: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1db6: 48 8b 4c 01 10               	movq	0x10(%rcx,%rax), %rcx
    1dbb: e8 00 00 00 00               	callq	0x1dc0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x8b0>
    1dc0: 8b 84 24 c0 03 00 00         	movl	0x3c0(%rsp), %eax
    1dc7: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1dcb: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1dd3: 48 8b 4c 01 20               	movq	0x20(%rcx,%rax), %rcx
    1dd8: e8 00 00 00 00               	callq	0x1ddd <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x8cd>
    1ddd: 90                           	nop
    1dde: eb 87                        	jmp	0x1d67 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x857>
    1de0: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1de8: e8 00 00 00 00               	callq	0x1ded <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x8dd>
    1ded: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    1df5: e8 00 00 00 00               	callq	0x1dfa <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x8ea>
    1dfa: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    1dff: e8 00 00 00 00               	callq	0x1e04 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x8f4>
    1e04: 90                           	nop
    1e05: b2 05                        	movb	$0x5, %dl
    1e07: 48 8d 8c 24 98 04 00 00      	leaq	0x498(%rsp), %rcx
    1e0f: e8 00 00 00 00               	callq	0x1e14 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x904>
    1e14: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    1e1c: 48 8b f0                     	movq	%rax, %rsi
    1e1f: b9 10 00 00 00               	movl	$0x10, %ecx
    1e24: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    1e26: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    1e2e: e9 61 07 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    1e33: c7 84 24 c4 03 00 00 00 00 00 00     	movl	$0x0, 0x3c4(%rsp)
    1e3e: eb 10                        	jmp	0x1e50 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x940>
    1e40: 8b 84 24 c4 03 00 00         	movl	0x3c4(%rsp), %eax
    1e47: ff c0                        	incl	%eax
    1e49: 89 84 24 c4 03 00 00         	movl	%eax, 0x3c4(%rsp)
    1e50: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1e57: 39 84 24 c4 03 00 00         	cmpl	%eax, 0x3c4(%rsp)
    1e5e: 73 47                        	jae	0x1ea7 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x997>
    1e60: 8b 84 24 c4 03 00 00         	movl	0x3c4(%rsp), %eax
    1e67: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1e6b: 8b 8c 24 c4 03 00 00         	movl	0x3c4(%rsp), %ecx
    1e72: 48 6b c9 30                  	imulq	$0x30, %rcx, %rcx
    1e76: 48 8b 94 24 b8 03 00 00      	movq	0x3b8(%rsp), %rdx
    1e7e: 48 8b bc 24 30 03 00 00      	movq	0x330(%rsp), %rdi
    1e86: 48 89 bc 24 c8 04 00 00      	movq	%rdi, 0x4c8(%rsp)
    1e8e: 48 8d 3c 0a                  	leaq	(%rdx,%rcx), %rdi
    1e92: 48 8b 8c 24 c8 04 00 00      	movq	0x4c8(%rsp), %rcx
    1e9a: 48 8d 34 01                  	leaq	(%rcx,%rax), %rsi
    1e9e: b9 30 00 00 00               	movl	$0x30, %ecx
    1ea3: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    1ea5: eb 99                        	jmp	0x1e40 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x930>
    1ea7: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1eaf: e8 00 00 00 00               	callq	0x1eb4 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x9a4>
    1eb4: 48 8b 84 24 b8 03 00 00      	movq	0x3b8(%rsp), %rax
    1ebc: 48 89 84 24 30 03 00 00      	movq	%rax, 0x330(%rsp)
    1ec4: 8b 84 24 b0 03 00 00         	movl	0x3b0(%rsp), %eax
    1ecb: 89 84 24 24 03 00 00         	movl	%eax, 0x324(%rsp)
    1ed2: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1ed9: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1edd: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1ee5: 48 8b 94 24 48 03 00 00      	movq	0x348(%rsp), %rdx
    1eed: 48 89 14 01                  	movq	%rdx, (%rcx,%rax)
    1ef1: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1ef8: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1efc: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1f04: 8b 94 24 40 03 00 00         	movl	0x340(%rsp), %edx
    1f0b: 89 54 01 08                  	movl	%edx, 0x8(%rcx,%rax)
    1f0f: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1f16: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1f1a: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1f22: 48 8b 94 24 78 03 00 00      	movq	0x378(%rsp), %rdx
    1f2a: 48 89 54 01 10               	movq	%rdx, 0x10(%rcx,%rax)
    1f2f: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1f36: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1f3a: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1f42: 8b 94 24 64 03 00 00         	movl	0x364(%rsp), %edx
    1f49: 89 54 01 18                  	movl	%edx, 0x18(%rcx,%rax)
    1f4d: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1f54: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1f58: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1f60: 48 8b 94 24 a8 03 00 00      	movq	0x3a8(%rsp), %rdx
    1f68: 48 89 54 01 20               	movq	%rdx, 0x20(%rcx,%rax)
    1f6d: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1f74: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1f78: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1f80: 8b 94 24 94 03 00 00         	movl	0x394(%rsp), %edx
    1f87: 89 54 01 28                  	movl	%edx, 0x28(%rcx,%rax)
    1f8b: 8b 84 24 c0 00 00 00         	movl	0xc0(%rsp), %eax
    1f92: 83 e0 10                     	andl	$0x10, %eax
    1f95: 85 c0                        	testl	%eax, %eax
    1f97: 74 0d                        	je	0x1fa6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xa96>
    1f99: c7 84 24 d0 04 00 00 01 00 00 00     	movl	$0x1, 0x4d0(%rsp)
    1fa4: eb 0b                        	jmp	0x1fb1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xaa1>
    1fa6: c7 84 24 d0 04 00 00 00 00 00 00     	movl	$0x0, 0x4d0(%rsp)
    1fb1: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1fb8: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    1fbc: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    1fc4: 0f b6 94 24 d0 04 00 00      	movzbl	0x4d0(%rsp), %edx
    1fcc: 88 54 01 2c                  	movb	%dl, 0x2c(%rcx,%rax)
    1fd0: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    1fd7: ff c0                        	incl	%eax
    1fd9: 89 84 24 28 03 00 00         	movl	%eax, 0x328(%rsp)
    1fe0: 48 8d 94 24 c0 00 00 00      	leaq	0xc0(%rsp), %rdx
    1fe8: 48 8b 8c 24 18 03 00 00      	movq	0x318(%rsp), %rcx
    1ff0: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x1ff6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xae6>
    1ff6: 85 c0                        	testl	%eax, %eax
    1ff8: 0f 85 40 f9 ff ff            	jne	0x193e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x42e>
    1ffe: 48 8b 8c 24 18 03 00 00      	movq	0x318(%rsp), %rcx
    2006: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x200c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xafc>
    200c: 83 bc 24 28 03 00 00 01      	cmpl	$0x1, 0x328(%rsp)
    2014: 76 15                        	jbe	0x202b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xb1b>
    2016: 8b 94 24 28 03 00 00         	movl	0x328(%rsp), %edx
    201d: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    2025: e8 00 00 00 00               	callq	0x202a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xb1a>
    202a: 90                           	nop
    202b: b9 40 00 00 00               	movl	$0x40, %ecx
    2030: e8 00 00 00 00               	callq	0x2035 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xb25>
    2035: 48 89 84 24 c8 03 00 00      	movq	%rax, 0x3c8(%rsp)
    203d: 48 83 bc 24 c8 03 00 00 00   	cmpq	$0x0, 0x3c8(%rsp)
    2046: 0f 85 13 01 00 00            	jne	0x215f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xc4f>
    204c: c7 84 24 d0 03 00 00 00 00 00 00     	movl	$0x0, 0x3d0(%rsp)
    2057: eb 10                        	jmp	0x2069 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xb59>
    2059: 8b 84 24 d0 03 00 00         	movl	0x3d0(%rsp), %eax
    2060: ff c0                        	incl	%eax
    2062: 89 84 24 d0 03 00 00         	movl	%eax, 0x3d0(%rsp)
    2069: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    2070: 39 84 24 d0 03 00 00         	cmpl	%eax, 0x3d0(%rsp)
    2077: 0f 83 8f 00 00 00            	jae	0x210c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xbfc>
    207d: 8b 84 24 d0 03 00 00         	movl	0x3d0(%rsp), %eax
    2084: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    2088: 8b 8c 24 d0 03 00 00         	movl	0x3d0(%rsp), %ecx
    208f: 48 6b c9 30                  	imulq	$0x30, %rcx, %rcx
    2093: 48 8b 94 24 30 03 00 00      	movq	0x330(%rsp), %rdx
    209b: 48 8b bc 24 30 03 00 00      	movq	0x330(%rsp), %rdi
    20a3: 48 8b 4c 0f 10               	movq	0x10(%rdi,%rcx), %rcx
    20a8: 48 39 4c 02 20               	cmpq	%rcx, 0x20(%rdx,%rax)
    20ad: 74 1e                        	je	0x20cd <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xbbd>
    20af: 8b 84 24 d0 03 00 00         	movl	0x3d0(%rsp), %eax
    20b6: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    20ba: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    20c2: 48 8b 4c 01 20               	movq	0x20(%rcx,%rax), %rcx
    20c7: e8 00 00 00 00               	callq	0x20cc <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xbbc>
    20cc: 90                           	nop
    20cd: 8b 84 24 d0 03 00 00         	movl	0x3d0(%rsp), %eax
    20d4: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    20d8: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    20e0: 48 8b 4c 01 10               	movq	0x10(%rcx,%rax), %rcx
    20e5: e8 00 00 00 00               	callq	0x20ea <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xbda>
    20ea: 8b 84 24 d0 03 00 00         	movl	0x3d0(%rsp), %eax
    20f1: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    20f5: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    20fd: 48 8b 0c 01                  	movq	(%rcx,%rax), %rcx
    2101: e8 00 00 00 00               	callq	0x2106 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xbf6>
    2106: 90                           	nop
    2107: e9 4d ff ff ff               	jmp	0x2059 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xb49>
    210c: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    2114: e8 00 00 00 00               	callq	0x2119 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xc09>
    2119: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    2121: e8 00 00 00 00               	callq	0x2126 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xc16>
    2126: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    212b: e8 00 00 00 00               	callq	0x2130 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xc20>
    2130: 90                           	nop
    2131: b2 05                        	movb	$0x5, %dl
    2133: 48 8d 8c 24 a8 04 00 00      	leaq	0x4a8(%rsp), %rcx
    213b: e8 00 00 00 00               	callq	0x2140 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xc30>
    2140: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    2148: 48 8b f0                     	movq	%rax, %rsi
    214b: b9 10 00 00 00               	movl	$0x10, %ecx
    2150: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    2152: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    215a: e9 35 04 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    215f: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    2167: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    216f: 48 89 08                     	movq	%rcx, (%rax)
    2172: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    217a: 8b 4c 24 74                  	movl	0x74(%rsp), %ecx
    217e: 89 48 08                     	movl	%ecx, 0x8(%rax)
    2181: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    2189: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    218e: 48 89 48 10                  	movq	%rcx, 0x10(%rax)
    2192: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    219a: 8b 4c 24 54                  	movl	0x54(%rsp), %ecx
    219e: 89 48 18                     	movl	%ecx, 0x18(%rax)
    21a1: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    21a9: 8b 8c 24 28 03 00 00         	movl	0x328(%rsp), %ecx
    21b0: 89 48 38                     	movl	%ecx, 0x38(%rax)
    21b3: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    21bb: c7 40 3c 00 00 00 00         	movl	$0x0, 0x3c(%rax)
    21c2: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    21ca: 48 c7 40 20 00 00 00 00      	movq	$0x0, 0x20(%rax)
    21d2: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    21da: 48 c7 40 28 00 00 00 00      	movq	$0x0, 0x28(%rax)
    21e2: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    21ea: 48 c7 40 30 00 00 00 00      	movq	$0x0, 0x30(%rax)
    21f2: 83 bc 24 28 03 00 00 00      	cmpl	$0x0, 0x328(%rsp)
    21fa: 0f 86 39 03 00 00            	jbe	0x2539 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1029>
    2200: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    2207: 48 c1 e0 03                  	shlq	$0x3, %rax
    220b: 48 8b c8                     	movq	%rax, %rcx
    220e: e8 00 00 00 00               	callq	0x2213 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xd03>
    2213: 48 8b 8c 24 c8 03 00 00      	movq	0x3c8(%rsp), %rcx
    221b: 48 89 41 20                  	movq	%rax, 0x20(%rcx)
    221f: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    2226: 48 c1 e0 02                  	shlq	$0x2, %rax
    222a: 48 8b c8                     	movq	%rax, %rcx
    222d: e8 00 00 00 00               	callq	0x2232 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xd22>
    2232: 48 8b 8c 24 c8 03 00 00      	movq	0x3c8(%rsp), %rcx
    223a: 48 89 41 28                  	movq	%rax, 0x28(%rcx)
    223e: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    2245: 8b c8                        	movl	%eax, %ecx
    2247: e8 00 00 00 00               	callq	0x224c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xd3c>
    224c: 48 8b 8c 24 c8 03 00 00      	movq	0x3c8(%rsp), %rcx
    2254: 48 89 41 30                  	movq	%rax, 0x30(%rcx)
    2258: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    2260: 48 83 78 20 00               	cmpq	$0x0, 0x20(%rax)
    2265: 74 22                        	je	0x2289 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xd79>
    2267: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    226f: 48 83 78 28 00               	cmpq	$0x0, 0x28(%rax)
    2274: 74 13                        	je	0x2289 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xd79>
    2276: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    227e: 48 83 78 30 00               	cmpq	$0x0, 0x30(%rax)
    2283: 0f 85 83 01 00 00            	jne	0x240c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xefc>
    2289: c7 84 24 d4 03 00 00 00 00 00 00     	movl	$0x0, 0x3d4(%rsp)
    2294: eb 10                        	jmp	0x22a6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xd96>
    2296: 8b 84 24 d4 03 00 00         	movl	0x3d4(%rsp), %eax
    229d: ff c0                        	incl	%eax
    229f: 89 84 24 d4 03 00 00         	movl	%eax, 0x3d4(%rsp)
    22a6: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    22ad: 39 84 24 d4 03 00 00         	cmpl	%eax, 0x3d4(%rsp)
    22b4: 0f 83 8f 00 00 00            	jae	0x2349 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xe39>
    22ba: 8b 84 24 d4 03 00 00         	movl	0x3d4(%rsp), %eax
    22c1: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    22c5: 8b 8c 24 d4 03 00 00         	movl	0x3d4(%rsp), %ecx
    22cc: 48 6b c9 30                  	imulq	$0x30, %rcx, %rcx
    22d0: 48 8b 94 24 30 03 00 00      	movq	0x330(%rsp), %rdx
    22d8: 48 8b bc 24 30 03 00 00      	movq	0x330(%rsp), %rdi
    22e0: 48 8b 4c 0f 10               	movq	0x10(%rdi,%rcx), %rcx
    22e5: 48 39 4c 02 20               	cmpq	%rcx, 0x20(%rdx,%rax)
    22ea: 74 1e                        	je	0x230a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xdfa>
    22ec: 8b 84 24 d4 03 00 00         	movl	0x3d4(%rsp), %eax
    22f3: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    22f7: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    22ff: 48 8b 4c 01 20               	movq	0x20(%rcx,%rax), %rcx
    2304: e8 00 00 00 00               	callq	0x2309 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xdf9>
    2309: 90                           	nop
    230a: 8b 84 24 d4 03 00 00         	movl	0x3d4(%rsp), %eax
    2311: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    2315: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    231d: 48 8b 4c 01 10               	movq	0x10(%rcx,%rax), %rcx
    2322: e8 00 00 00 00               	callq	0x2327 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xe17>
    2327: 8b 84 24 d4 03 00 00         	movl	0x3d4(%rsp), %eax
    232e: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    2332: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    233a: 48 8b 0c 01                  	movq	(%rcx,%rax), %rcx
    233e: e8 00 00 00 00               	callq	0x2343 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xe33>
    2343: 90                           	nop
    2344: e9 4d ff ff ff               	jmp	0x2296 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xd86>
    2349: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    2351: 48 83 78 20 00               	cmpq	$0x0, 0x20(%rax)
    2356: 74 12                        	je	0x236a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xe5a>
    2358: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    2360: 48 8b 48 20                  	movq	0x20(%rax), %rcx
    2364: e8 00 00 00 00               	callq	0x2369 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xe59>
    2369: 90                           	nop
    236a: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    2372: 48 83 78 28 00               	cmpq	$0x0, 0x28(%rax)
    2377: 74 12                        	je	0x238b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xe7b>
    2379: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    2381: 48 8b 48 28                  	movq	0x28(%rax), %rcx
    2385: e8 00 00 00 00               	callq	0x238a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xe7a>
    238a: 90                           	nop
    238b: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    2393: 48 83 78 30 00               	cmpq	$0x0, 0x30(%rax)
    2398: 74 12                        	je	0x23ac <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xe9c>
    239a: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    23a2: 48 8b 48 30                  	movq	0x30(%rax), %rcx
    23a6: e8 00 00 00 00               	callq	0x23ab <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xe9b>
    23ab: 90                           	nop
    23ac: 48 8b 8c 24 c8 03 00 00      	movq	0x3c8(%rsp), %rcx
    23b4: e8 00 00 00 00               	callq	0x23b9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xea9>
    23b9: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    23c1: e8 00 00 00 00               	callq	0x23c6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xeb6>
    23c6: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    23cb: e8 00 00 00 00               	callq	0x23d0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xec0>
    23d0: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    23d8: e8 00 00 00 00               	callq	0x23dd <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xecd>
    23dd: 90                           	nop
    23de: b2 05                        	movb	$0x5, %dl
    23e0: 48 8d 8c 24 b8 04 00 00      	leaq	0x4b8(%rsp), %rcx
    23e8: e8 00 00 00 00               	callq	0x23ed <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xedd>
    23ed: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    23f5: 48 8b f0                     	movq	%rax, %rsi
    23f8: b9 10 00 00 00               	movl	$0x10, %ecx
    23fd: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    23ff: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    2407: e9 88 01 00 00               	jmp	0x2594 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1084>
    240c: c7 84 24 d8 03 00 00 00 00 00 00     	movl	$0x0, 0x3d8(%rsp)
    2417: eb 10                        	jmp	0x2429 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xf19>
    2419: 8b 84 24 d8 03 00 00         	movl	0x3d8(%rsp), %eax
    2420: ff c0                        	incl	%eax
    2422: 89 84 24 d8 03 00 00         	movl	%eax, 0x3d8(%rsp)
    2429: 8b 84 24 28 03 00 00         	movl	0x328(%rsp), %eax
    2430: 39 84 24 d8 03 00 00         	cmpl	%eax, 0x3d8(%rsp)
    2437: 0f 83 fc 00 00 00            	jae	0x2539 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1029>
    243d: 8b 84 24 d8 03 00 00         	movl	0x3d8(%rsp), %eax
    2444: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    2448: 8b 8c 24 d8 03 00 00         	movl	0x3d8(%rsp), %ecx
    244f: 48 8b 94 24 c8 03 00 00      	movq	0x3c8(%rsp), %rdx
    2457: 48 8b 52 20                  	movq	0x20(%rdx), %rdx
    245b: 48 8b bc 24 30 03 00 00      	movq	0x330(%rsp), %rdi
    2463: 48 8b 04 07                  	movq	(%rdi,%rax), %rax
    2467: 48 89 04 ca                  	movq	%rax, (%rdx,%rcx,8)
    246b: 8b 84 24 d8 03 00 00         	movl	0x3d8(%rsp), %eax
    2472: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    2476: 8b 8c 24 d8 03 00 00         	movl	0x3d8(%rsp), %ecx
    247d: 48 8b 94 24 c8 03 00 00      	movq	0x3c8(%rsp), %rdx
    2485: 48 8b 52 28                  	movq	0x28(%rdx), %rdx
    2489: 48 8b bc 24 30 03 00 00      	movq	0x330(%rsp), %rdi
    2491: 8b 44 07 08                  	movl	0x8(%rdi,%rax), %eax
    2495: 89 04 8a                     	movl	%eax, (%rdx,%rcx,4)
    2498: 8b 84 24 d8 03 00 00         	movl	0x3d8(%rsp), %eax
    249f: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    24a3: 8b 8c 24 d8 03 00 00         	movl	0x3d8(%rsp), %ecx
    24aa: 48 8b 94 24 c8 03 00 00      	movq	0x3c8(%rsp), %rdx
    24b2: 48 8b 52 30                  	movq	0x30(%rdx), %rdx
    24b6: 48 8b bc 24 30 03 00 00      	movq	0x330(%rsp), %rdi
    24be: 0f b6 44 07 2c               	movzbl	0x2c(%rdi,%rax), %eax
    24c3: 88 04 0a                     	movb	%al, (%rdx,%rcx)
    24c6: 8b 84 24 d8 03 00 00         	movl	0x3d8(%rsp), %eax
    24cd: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    24d1: 8b 8c 24 d8 03 00 00         	movl	0x3d8(%rsp), %ecx
    24d8: 48 6b c9 30                  	imulq	$0x30, %rcx, %rcx
    24dc: 48 8b 94 24 30 03 00 00      	movq	0x330(%rsp), %rdx
    24e4: 48 8b bc 24 30 03 00 00      	movq	0x330(%rsp), %rdi
    24ec: 48 8b 4c 0f 10               	movq	0x10(%rdi,%rcx), %rcx
    24f1: 48 39 4c 02 20               	cmpq	%rcx, 0x20(%rdx,%rax)
    24f6: 74 1e                        	je	0x2516 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1006>
    24f8: 8b 84 24 d8 03 00 00         	movl	0x3d8(%rsp), %eax
    24ff: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    2503: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    250b: 48 8b 4c 01 20               	movq	0x20(%rcx,%rax), %rcx
    2510: e8 00 00 00 00               	callq	0x2515 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1005>
    2515: 90                           	nop
    2516: 8b 84 24 d8 03 00 00         	movl	0x3d8(%rsp), %eax
    251d: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    2521: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    2529: 48 8b 4c 01 10               	movq	0x10(%rcx,%rax), %rcx
    252e: e8 00 00 00 00               	callq	0x2533 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1023>
    2533: 90                           	nop
    2534: e9 e0 fe ff ff               	jmp	0x2419 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0xf09>
    2539: 48 8b 8c 24 30 03 00 00      	movq	0x330(%rsp), %rcx
    2541: e8 00 00 00 00               	callq	0x2546 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1036>
    2546: c6 84 24 e8 03 00 00 01      	movb	$0x1, 0x3e8(%rsp)
    254e: 48 8b 84 24 c8 03 00 00      	movq	0x3c8(%rsp), %rax
    2556: 48 89 84 24 f0 03 00 00      	movq	%rax, 0x3f0(%rsp)
    255e: 0f b6 94 24 e8 03 00 00      	movzbl	0x3e8(%rsp), %edx
    2566: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x256d <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x105d>
    256d: e8 00 00 00 00               	callq	0x2572 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1062>
    2572: 48 8d 84 24 e8 03 00 00      	leaq	0x3e8(%rsp), %rax
    257a: 48 8b bc 24 00 05 00 00      	movq	0x500(%rsp), %rdi
    2582: 48 8b f0                     	movq	%rax, %rsi
    2585: b9 10 00 00 00               	movl	$0x10, %ecx
    258a: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    258c: 48 8b 84 24 00 05 00 00      	movq	0x500(%rsp), %rax
    2594: 48 8b f8                     	movq	%rax, %rdi
    2597: 48 8b cc                     	movq	%rsp, %rcx
    259a: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x25a1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1091>
    25a1: e8 00 00 00 00               	callq	0x25a6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir+0x1096>
    25a6: 48 8b c7                     	movq	%rdi, %rax
    25a9: 48 81 c4 e8 04 00 00         	addq	$0x4e8, %rsp            # imm = 0x4E8
    25b0: 5f                           	popq	%rdi
    25b1: 5e                           	popq	%rsi
    25b2: c3                           	retq
    25b3: cc                           	int3
    25b4: cc                           	int3
    25b5: cc                           	int3
    25b6: cc                           	int3
    25b7: cc                           	int3
    25b8: cc                           	int3
    25b9: cc                           	int3
    25ba: cc                           	int3
    25bb: cc                           	int3
    25bc: cc                           	int3
    25bd: cc                           	int3
    25be: cc                           	int3
    25bf: cc                           	int3

00000000000025c0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir>:
    25c0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    25c5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    25ca: 57                           	pushq	%rdi
    25cb: 48 81 ec 80 00 00 00         	subq	$0x80, %rsp
    25d2: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    25d7: b9 18 00 00 00               	movl	$0x18, %ecx
    25dc: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    25e1: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    25e3: 48 8b 8c 24 90 00 00 00      	movq	0x90(%rsp), %rcx
    25eb: 48 8b 8c 24 90 00 00 00      	movq	0x90(%rsp), %rcx
    25f3: e8 00 00 00 00               	callq	0x25f8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0x38>
    25f8: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    25fd: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    2606: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%rsp)
    260e: 4c 8d 4c 24 54               	leaq	0x54(%rsp), %r9
    2613: 4c 8d 44 24 38               	leaq	0x38(%rsp), %r8
    2618: 48 8b 94 24 98 00 00 00      	movq	0x98(%rsp), %rdx
    2620: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    2625: e8 00 00 00 00               	callq	0x262a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0x6a>
    262a: 85 c0                        	testl	%eax, %eax
    262c: 75 09                        	jne	0x2637 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0x77>
    262e: b1 03                        	movb	$0x3, %cl
    2630: e8 00 00 00 00               	callq	0x2635 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0x75>
    2635: eb 77                        	jmp	0x26ae <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xee>
    2637: 45 33 c0                     	xorl	%r8d, %r8d
    263a: 8b 54 24 54                  	movl	0x54(%rsp), %edx
    263e: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    2643: e8 00 00 00 00               	callq	0x2648 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0x88>
    2648: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    264d: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    2652: e8 00 00 00 00               	callq	0x2657 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0x97>
    2657: 48 83 7c 24 68 00            	cmpq	$0x0, 0x68(%rsp)
    265d: 75 09                        	jne	0x2668 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xa8>
    265f: b1 03                        	movb	$0x3, %cl
    2661: e8 00 00 00 00               	callq	0x2666 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xa6>
    2666: eb 46                        	jmp	0x26ae <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xee>
    2668: 33 d2                        	xorl	%edx, %edx
    266a: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    266f: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x2675 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xb5>
    2675: 89 44 24 70                  	movl	%eax, 0x70(%rsp)
    2679: 83 7c 24 70 00               	cmpl	$0x0, 0x70(%rsp)
    267e: 75 1f                        	jne	0x269f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xdf>
    2680: e8 00 00 00 00               	callq	0x2685 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xc5>
    2685: 88 44 24 74                  	movb	%al, 0x74(%rsp)
    2689: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    268e: e8 00 00 00 00               	callq	0x2693 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xd3>
    2693: 0f b6 4c 24 74               	movzbl	0x74(%rsp), %ecx
    2698: e8 00 00 00 00               	callq	0x269d <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xdd>
    269d: eb 0f                        	jmp	0x26ae <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xee>
    269f: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    26a4: e8 00 00 00 00               	callq	0x26a9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xe9>
    26a9: e8 00 00 00 00               	callq	0x26ae <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xee>
    26ae: 48 8b f8                     	movq	%rax, %rdi
    26b1: 48 8b cc                     	movq	%rsp, %rcx
    26b4: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x26bb <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0xfb>
    26bb: e8 00 00 00 00               	callq	0x26c0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3acreate_x5fdir+0x100>
    26c0: 48 8b c7                     	movq	%rdi, %rax
    26c3: 48 81 c4 80 00 00 00         	addq	$0x80, %rsp
    26ca: 5f                           	popq	%rdi
    26cb: c3                           	retq
    26cc: cc                           	int3
    26cd: cc                           	int3
    26ce: cc                           	int3
    26cf: cc                           	int3
    26d0: cc                           	int3
    26d1: cc                           	int3
    26d2: cc                           	int3
    26d3: cc                           	int3
    26d4: cc                           	int3
    26d5: cc                           	int3
    26d6: cc                           	int3
    26d7: cc                           	int3
    26d8: cc                           	int3
    26d9: cc                           	int3
    26da: cc                           	int3
    26db: cc                           	int3
    26dc: cc                           	int3
    26dd: cc                           	int3
    26de: cc                           	int3
    26df: cc                           	int3

00000000000026e0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir>:
    26e0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    26e5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    26ea: 57                           	pushq	%rdi
    26eb: 48 81 ec 20 01 00 00         	subq	$0x120, %rsp            # imm = 0x120
    26f2: 48 8d 7c 24 40               	leaq	0x40(%rsp), %rdi
    26f7: b9 38 00 00 00               	movl	$0x38, %ecx
    26fc: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    2701: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    2703: 48 8b 8c 24 30 01 00 00      	movq	0x130(%rsp), %rcx
    270b: 48 c7 44 24 30 00 00 00 00   	movq	$0x0, 0x30(%rsp)
    2714: c7 44 24 28 80 00 00 00      	movl	$0x80, 0x28(%rsp)
    271c: c7 44 24 20 04 00 00 00      	movl	$0x4, 0x20(%rsp)
    2724: 45 33 c9                     	xorl	%r9d, %r9d
    2727: 41 b8 01 00 00 00            	movl	$0x1, %r8d
    272d: ba 04 00 00 00               	movl	$0x4, %edx
    2732: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2739 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x59>
    2739: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x273f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5f>
    273f: 48 89 44 24 40               	movq	%rax, 0x40(%rsp)
    2744: 48 83 7c 24 40 ff            	cmpq	$-0x1, 0x40(%rsp)
    274a: 74 3d                        	je	0x2789 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0xa9>
    274c: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%rsp)
    2754: c6 44 24 74 45               	movb	$0x45, 0x74(%rsp)
    2759: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
    2762: 4c 8d 4c 24 54               	leaq	0x54(%rsp), %r9
    2767: 41 b8 01 00 00 00            	movl	$0x1, %r8d
    276d: 48 8d 54 24 74               	leaq	0x74(%rsp), %rdx
    2772: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    2777: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x277d <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x9d>
    277d: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    2782: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x2788 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0xa8>
    2788: 90                           	nop
    2789: 48 8b 8c 24 30 01 00 00      	movq	0x130(%rsp), %rcx
    2791: e8 00 00 00 00               	callq	0x2796 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0xb6>
    2796: 48 89 84 24 88 00 00 00      	movq	%rax, 0x88(%rsp)
    279e: 48 c7 84 24 98 00 00 00 00 00 00 00  	movq	$0x0, 0x98(%rsp)
    27aa: c7 84 24 b4 00 00 00 00 00 00 00     	movl	$0x0, 0xb4(%rsp)
    27b5: 4c 8d 8c 24 b4 00 00 00      	leaq	0xb4(%rsp), %r9
    27bd: 4c 8d 84 24 98 00 00 00      	leaq	0x98(%rsp), %r8
    27c5: 48 8b 94 24 38 01 00 00      	movq	0x138(%rsp), %rdx
    27cd: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    27d5: e8 00 00 00 00               	callq	0x27da <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0xfa>
    27da: 85 c0                        	testl	%eax, %eax
    27dc: 75 0c                        	jne	0x27ea <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x10a>
    27de: b1 03                        	movb	$0x3, %cl
    27e0: e8 00 00 00 00               	callq	0x27e5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x105>
    27e5: e9 c8 04 00 00               	jmp	0x2cb2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5d2>
    27ea: c7 84 24 d4 00 00 00 00 00 00 00     	movl	$0x0, 0xd4(%rsp)
    27f5: 4c 8d 84 24 d4 00 00 00      	leaq	0xd4(%rsp), %r8
    27fd: 8b 94 24 b4 00 00 00         	movl	0xb4(%rsp), %edx
    2804: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
    280c: e8 00 00 00 00               	callq	0x2811 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x131>
    2811: 48 89 84 24 e8 00 00 00      	movq	%rax, 0xe8(%rsp)
    2819: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
    2821: e8 00 00 00 00               	callq	0x2826 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x146>
    2826: 48 83 bc 24 e8 00 00 00 00   	cmpq	$0x0, 0xe8(%rsp)
    282f: 75 0c                        	jne	0x283d <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x15d>
    2831: b1 03                        	movb	$0x3, %cl
    2833: e8 00 00 00 00               	callq	0x2838 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x158>
    2838: e9 75 04 00 00               	jmp	0x2cb2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5d2>
    283d: 83 bc 24 d4 00 00 00 00      	cmpl	$0x0, 0xd4(%rsp)
    2845: 75 19                        	jne	0x2860 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x180>
    2847: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    284f: e8 00 00 00 00               	callq	0x2854 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x174>
    2854: b1 03                        	movb	$0x3, %cl
    2856: e8 00 00 00 00               	callq	0x285b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x17b>
    285b: e9 52 04 00 00               	jmp	0x2cb2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5d2>
    2860: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    2868: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x286e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x18e>
    286e: 89 84 24 f0 00 00 00         	movl	%eax, 0xf0(%rsp)
    2875: 83 bc 24 f0 00 00 00 ff      	cmpl	$-0x1, 0xf0(%rsp)
    287d: 74 3e                        	je	0x28bd <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x1dd>
    287f: 8b 84 24 f0 00 00 00         	movl	0xf0(%rsp), %eax
    2886: 83 e0 10                     	andl	$0x10, %eax
    2889: 85 c0                        	testl	%eax, %eax
    288b: 74 17                        	je	0x28a4 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x1c4>
    288d: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    2895: e8 00 00 00 00               	callq	0x289a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x1ba>
    289a: e8 00 00 00 00               	callq	0x289f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x1bf>
    289f: e9 0e 04 00 00               	jmp	0x2cb2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5d2>
    28a4: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    28ac: e8 00 00 00 00               	callq	0x28b1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x1d1>
    28b1: b1 02                        	movb	$0x2, %cl
    28b3: e8 00 00 00 00               	callq	0x28b8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x1d8>
    28b8: e9 f5 03 00 00               	jmp	0x2cb2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5d2>
    28bd: 8b 84 24 d4 00 00 00         	movl	0xd4(%rsp), %eax
    28c4: ff c0                        	incl	%eax
    28c6: 8b c0                        	movl	%eax, %eax
    28c8: 48 d1 e0                     	shlq	%rax
    28cb: 48 8b c8                     	movq	%rax, %rcx
    28ce: e8 00 00 00 00               	callq	0x28d3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x1f3>
    28d3: 48 89 84 24 f8 00 00 00      	movq	%rax, 0xf8(%rsp)
    28db: 48 83 bc 24 f8 00 00 00 00   	cmpq	$0x0, 0xf8(%rsp)
    28e4: 75 19                        	jne	0x28ff <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x21f>
    28e6: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    28ee: e8 00 00 00 00               	callq	0x28f3 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x213>
    28f3: b1 05                        	movb	$0x5, %cl
    28f5: e8 00 00 00 00               	callq	0x28fa <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x21a>
    28fa: e9 b3 03 00 00               	jmp	0x2cb2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5d2>
    28ff: 8b 84 24 d4 00 00 00         	movl	0xd4(%rsp), %eax
    2906: 48 d1 e0                     	shlq	%rax
    2909: 4c 8b c0                     	movq	%rax, %r8
    290c: 48 8b 94 24 e8 00 00 00      	movq	0xe8(%rsp), %rdx
    2914: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    291c: e8 00 00 00 00               	callq	0x2921 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x241>
    2921: 8b 84 24 d4 00 00 00         	movl	0xd4(%rsp), %eax
    2928: 33 c9                        	xorl	%ecx, %ecx
    292a: 48 8b 94 24 f8 00 00 00      	movq	0xf8(%rsp), %rdx
    2932: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    2936: c7 84 24 00 01 00 00 00 00 00 00     	movl	$0x0, 0x100(%rsp)
    2941: 83 bc 24 d4 00 00 00 03      	cmpl	$0x3, 0xd4(%rsp)
    2949: 72 44                        	jb	0x298f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x2af>
    294b: b8 02 00 00 00               	movl	$0x2, %eax
    2950: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    2954: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    295c: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    2960: 83 f8 3a                     	cmpl	$0x3a, %eax
    2963: 75 2a                        	jne	0x298f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x2af>
    2965: b8 02 00 00 00               	movl	$0x2, %eax
    296a: 48 6b c0 02                  	imulq	$0x2, %rax, %rax
    296e: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    2976: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    297a: 83 f8 5c                     	cmpl	$0x5c, %eax
    297d: 75 10                        	jne	0x298f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x2af>
    297f: c7 84 24 00 01 00 00 03 00 00 00     	movl	$0x3, 0x100(%rsp)
    298a: e9 3e 01 00 00               	jmp	0x2acd <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x3ed>
    298f: 83 bc 24 d4 00 00 00 02      	cmpl	$0x2, 0xd4(%rsp)
    2997: 0f 82 0b 01 00 00            	jb	0x2aa8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x3c8>
    299d: b8 02 00 00 00               	movl	$0x2, %eax
    29a2: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    29a6: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    29ae: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    29b2: 83 f8 5c                     	cmpl	$0x5c, %eax
    29b5: 0f 85 ed 00 00 00            	jne	0x2aa8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x3c8>
    29bb: b8 02 00 00 00               	movl	$0x2, %eax
    29c0: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    29c4: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    29cc: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    29d0: 83 f8 5c                     	cmpl	$0x5c, %eax
    29d3: 0f 85 cf 00 00 00            	jne	0x2aa8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x3c8>
    29d9: c7 84 24 04 01 00 00 02 00 00 00     	movl	$0x2, 0x104(%rsp)
    29e4: 8b 84 24 d4 00 00 00         	movl	0xd4(%rsp), %eax
    29eb: 39 84 24 04 01 00 00         	cmpl	%eax, 0x104(%rsp)
    29f2: 73 2a                        	jae	0x2a1e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x33e>
    29f4: 8b 84 24 04 01 00 00         	movl	0x104(%rsp), %eax
    29fb: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    2a03: 0f b7 04 41                  	movzwl	(%rcx,%rax,2), %eax
    2a07: 83 f8 5c                     	cmpl	$0x5c, %eax
    2a0a: 74 12                        	je	0x2a1e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x33e>
    2a0c: 8b 84 24 04 01 00 00         	movl	0x104(%rsp), %eax
    2a13: ff c0                        	incl	%eax
    2a15: 89 84 24 04 01 00 00         	movl	%eax, 0x104(%rsp)
    2a1c: eb c6                        	jmp	0x29e4 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x304>
    2a1e: 8b 84 24 d4 00 00 00         	movl	0xd4(%rsp), %eax
    2a25: 39 84 24 04 01 00 00         	cmpl	%eax, 0x104(%rsp)
    2a2c: 73 10                        	jae	0x2a3e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x35e>
    2a2e: 8b 84 24 04 01 00 00         	movl	0x104(%rsp), %eax
    2a35: ff c0                        	incl	%eax
    2a37: 89 84 24 04 01 00 00         	movl	%eax, 0x104(%rsp)
    2a3e: 8b 84 24 d4 00 00 00         	movl	0xd4(%rsp), %eax
    2a45: 39 84 24 04 01 00 00         	cmpl	%eax, 0x104(%rsp)
    2a4c: 73 2a                        	jae	0x2a78 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x398>
    2a4e: 8b 84 24 04 01 00 00         	movl	0x104(%rsp), %eax
    2a55: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    2a5d: 0f b7 04 41                  	movzwl	(%rcx,%rax,2), %eax
    2a61: 83 f8 5c                     	cmpl	$0x5c, %eax
    2a64: 74 12                        	je	0x2a78 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x398>
    2a66: 8b 84 24 04 01 00 00         	movl	0x104(%rsp), %eax
    2a6d: ff c0                        	incl	%eax
    2a6f: 89 84 24 04 01 00 00         	movl	%eax, 0x104(%rsp)
    2a76: eb c6                        	jmp	0x2a3e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x35e>
    2a78: 8b 84 24 d4 00 00 00         	movl	0xd4(%rsp), %eax
    2a7f: 39 84 24 04 01 00 00         	cmpl	%eax, 0x104(%rsp)
    2a86: 73 10                        	jae	0x2a98 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x3b8>
    2a88: 8b 84 24 04 01 00 00         	movl	0x104(%rsp), %eax
    2a8f: ff c0                        	incl	%eax
    2a91: 89 84 24 04 01 00 00         	movl	%eax, 0x104(%rsp)
    2a98: 8b 84 24 04 01 00 00         	movl	0x104(%rsp), %eax
    2a9f: 89 84 24 00 01 00 00         	movl	%eax, 0x100(%rsp)
    2aa6: eb 25                        	jmp	0x2acd <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x3ed>
    2aa8: b8 02 00 00 00               	movl	$0x2, %eax
    2aad: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    2ab1: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    2ab9: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    2abd: 83 f8 5c                     	cmpl	$0x5c, %eax
    2ac0: 75 0b                        	jne	0x2acd <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x3ed>
    2ac2: c7 84 24 00 01 00 00 01 00 00 00     	movl	$0x1, 0x100(%rsp)
    2acd: 8b 84 24 00 01 00 00         	movl	0x100(%rsp), %eax
    2ad4: 89 84 24 08 01 00 00         	movl	%eax, 0x108(%rsp)
    2adb: eb 10                        	jmp	0x2aed <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x40d>
    2add: 8b 84 24 08 01 00 00         	movl	0x108(%rsp), %eax
    2ae4: ff c0                        	incl	%eax
    2ae6: 89 84 24 08 01 00 00         	movl	%eax, 0x108(%rsp)
    2aed: 8b 84 24 d4 00 00 00         	movl	0xd4(%rsp), %eax
    2af4: 39 84 24 08 01 00 00         	cmpl	%eax, 0x108(%rsp)
    2afb: 0f 87 92 01 00 00            	ja	0x2c93 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5b3>
    2b01: 8b 84 24 d4 00 00 00         	movl	0xd4(%rsp), %eax
    2b08: 39 84 24 08 01 00 00         	cmpl	%eax, 0x108(%rsp)
    2b0f: 74 1c                        	je	0x2b2d <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x44d>
    2b11: 8b 84 24 08 01 00 00         	movl	0x108(%rsp), %eax
    2b18: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2b20: 0f b7 04 41                  	movzwl	(%rcx,%rax,2), %eax
    2b24: 83 f8 5c                     	cmpl	$0x5c, %eax
    2b27: 0f 85 61 01 00 00            	jne	0x2c8e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5ae>
    2b2d: 8b 84 24 08 01 00 00         	movl	0x108(%rsp), %eax
    2b34: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2b3c: 0f b7 04 41                  	movzwl	(%rcx,%rax,2), %eax
    2b40: 66 89 84 24 0c 01 00 00      	movw	%ax, 0x10c(%rsp)
    2b48: 8b 84 24 08 01 00 00         	movl	0x108(%rsp), %eax
    2b4f: 33 c9                        	xorl	%ecx, %ecx
    2b51: 48 8b 94 24 f8 00 00 00      	movq	0xf8(%rsp), %rdx
    2b59: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    2b5d: b8 02 00 00 00               	movl	$0x2, %eax
    2b62: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    2b66: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2b6e: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    2b72: 85 c0                        	testl	%eax, %eax
    2b74: 0f 84 f9 00 00 00            	je	0x2c73 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x593>
    2b7a: 33 d2                        	xorl	%edx, %edx
    2b7c: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2b84: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x2b8a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x4aa>
    2b8a: 85 c0                        	testl	%eax, %eax
    2b8c: 0f 85 e1 00 00 00            	jne	0x2c73 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x593>
    2b92: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x2b98 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x4b8>
    2b98: 89 84 24 10 01 00 00         	movl	%eax, 0x110(%rsp)
    2b9f: 81 bc 24 10 01 00 00 b7 00 00 00     	cmpl	$0xb7, 0x110(%rsp)
    2baa: 75 70                        	jne	0x2c1c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x53c>
    2bac: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2bb4: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x2bba <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x4da>
    2bba: 89 84 24 14 01 00 00         	movl	%eax, 0x114(%rsp)
    2bc1: 83 bc 24 14 01 00 00 ff      	cmpl	$-0x1, 0x114(%rsp)
    2bc9: 74 0e                        	je	0x2bd9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x4f9>
    2bcb: 8b 84 24 14 01 00 00         	movl	0x114(%rsp), %eax
    2bd2: 83 e0 10                     	andl	$0x10, %eax
    2bd5: 85 c0                        	testl	%eax, %eax
    2bd7: 75 41                        	jne	0x2c1a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x53a>
    2bd9: 8b 84 24 08 01 00 00         	movl	0x108(%rsp), %eax
    2be0: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2be8: 0f b7 94 24 0c 01 00 00      	movzwl	0x10c(%rsp), %edx
    2bf0: 66 89 14 41                  	movw	%dx, (%rcx,%rax,2)
    2bf4: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2bfc: e8 00 00 00 00               	callq	0x2c01 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x521>
    2c01: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    2c09: e8 00 00 00 00               	callq	0x2c0e <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x52e>
    2c0e: b1 02                        	movb	$0x2, %cl
    2c10: e8 00 00 00 00               	callq	0x2c15 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x535>
    2c15: e9 98 00 00 00               	jmp	0x2cb2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5d2>
    2c1a: eb 57                        	jmp	0x2c73 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x593>
    2c1c: 8b 8c 24 10 01 00 00         	movl	0x110(%rsp), %ecx
    2c23: e8 00 00 00 00               	callq	0x2c28 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x548>
    2c28: 88 84 24 18 01 00 00         	movb	%al, 0x118(%rsp)
    2c2f: 8b 84 24 08 01 00 00         	movl	0x108(%rsp), %eax
    2c36: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2c3e: 0f b7 94 24 0c 01 00 00      	movzwl	0x10c(%rsp), %edx
    2c46: 66 89 14 41                  	movw	%dx, (%rcx,%rax,2)
    2c4a: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2c52: e8 00 00 00 00               	callq	0x2c57 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x577>
    2c57: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    2c5f: e8 00 00 00 00               	callq	0x2c64 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x584>
    2c64: 0f b6 8c 24 18 01 00 00      	movzbl	0x118(%rsp), %ecx
    2c6c: e8 00 00 00 00               	callq	0x2c71 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x591>
    2c71: eb 3f                        	jmp	0x2cb2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5d2>
    2c73: 8b 84 24 08 01 00 00         	movl	0x108(%rsp), %eax
    2c7a: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2c82: 0f b7 94 24 0c 01 00 00      	movzwl	0x10c(%rsp), %edx
    2c8a: 66 89 14 41                  	movw	%dx, (%rcx,%rax,2)
    2c8e: e9 4a fe ff ff               	jmp	0x2add <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x3fd>
    2c93: 48 8b 8c 24 f8 00 00 00      	movq	0xf8(%rsp), %rcx
    2c9b: e8 00 00 00 00               	callq	0x2ca0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5c0>
    2ca0: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    2ca8: e8 00 00 00 00               	callq	0x2cad <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5cd>
    2cad: e8 00 00 00 00               	callq	0x2cb2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5d2>
    2cb2: 48 8b f8                     	movq	%rax, %rdi
    2cb5: 48 8b cc                     	movq	%rsp, %rcx
    2cb8: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x2cbf <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5df>
    2cbf: e8 00 00 00 00               	callq	0x2cc4 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir+0x5e4>
    2cc4: 48 8b c7                     	movq	%rdi, %rax
    2cc7: 48 81 c4 20 01 00 00         	addq	$0x120, %rsp            # imm = 0x120
    2cce: 5f                           	popq	%rdi
    2ccf: c3                           	retq
    2cd0: cc                           	int3
    2cd1: cc                           	int3
    2cd2: cc                           	int3
    2cd3: cc                           	int3
    2cd4: cc                           	int3
    2cd5: cc                           	int3
    2cd6: cc                           	int3
    2cd7: cc                           	int3
    2cd8: cc                           	int3
    2cd9: cc                           	int3
    2cda: cc                           	int3
    2cdb: cc                           	int3
    2cdc: cc                           	int3
    2cdd: cc                           	int3
    2cde: cc                           	int3
    2cdf: cc                           	int3

0000000000002ce0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind>:
    2ce0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    2ce5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    2cea: 57                           	pushq	%rdi
    2ceb: 48 81 ec 10 01 00 00         	subq	$0x110, %rsp            # imm = 0x110
    2cf2: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    2cf7: b9 3c 00 00 00               	movl	$0x3c, %ecx
    2cfc: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    2d01: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    2d03: 48 8b 8c 24 20 01 00 00      	movq	0x120(%rsp), %rcx
    2d0b: b2 4b                        	movb	$0x4b, %dl
    2d0d: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2d14 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x34>
    2d14: e8 00 00 00 00               	callq	0x2d19 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x39>
    2d19: 48 8b 8c 24 20 01 00 00      	movq	0x120(%rsp), %rcx
    2d21: e8 00 00 00 00               	callq	0x2d26 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x46>
    2d26: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    2d2b: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    2d34: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%rsp)
    2d3c: 4c 8d 4c 24 54               	leaq	0x54(%rsp), %r9
    2d41: 4c 8d 44 24 38               	leaq	0x38(%rsp), %r8
    2d46: 48 8b 94 24 28 01 00 00      	movq	0x128(%rsp), %rdx
    2d4e: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    2d53: e8 00 00 00 00               	callq	0x2d58 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x78>
    2d58: 85 c0                        	testl	%eax, %eax
    2d5a: 75 38                        	jne	0x2d94 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0xb4>
    2d5c: b1 03                        	movb	$0x3, %cl
    2d5e: e8 00 00 00 00               	callq	0x2d63 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x83>
    2d63: 66 89 44 24 74               	movw	%ax, 0x74(%rsp)
    2d68: 0f b6 54 24 74               	movzbl	0x74(%rsp), %edx
    2d6d: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2d74 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x94>
    2d74: e8 00 00 00 00               	callq	0x2d79 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x99>
    2d79: 0f b6 54 24 75               	movzbl	0x75(%rsp), %edx
    2d7e: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2d85 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0xa5>
    2d85: e8 00 00 00 00               	callq	0x2d8a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0xaa>
    2d8a: 0f b7 44 24 74               	movzwl	0x74(%rsp), %eax
    2d8f: e9 76 01 00 00               	jmp	0x2f0a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x22a>
    2d94: 45 33 c0                     	xorl	%r8d, %r8d
    2d97: 8b 54 24 54                  	movl	0x54(%rsp), %edx
    2d9b: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    2da0: e8 00 00 00 00               	callq	0x2da5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0xc5>
    2da5: 48 89 84 24 88 00 00 00      	movq	%rax, 0x88(%rsp)
    2dad: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    2db2: e8 00 00 00 00               	callq	0x2db7 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0xd7>
    2db7: 48 83 bc 24 88 00 00 00 00   	cmpq	$0x0, 0x88(%rsp)
    2dc0: 75 44                        	jne	0x2e06 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x126>
    2dc2: b1 03                        	movb	$0x3, %cl
    2dc4: e8 00 00 00 00               	callq	0x2dc9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0xe9>
    2dc9: 66 89 84 24 94 00 00 00      	movw	%ax, 0x94(%rsp)
    2dd1: 0f b6 94 24 94 00 00 00      	movzbl	0x94(%rsp), %edx
    2dd9: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2de0 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x100>
    2de0: e8 00 00 00 00               	callq	0x2de5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x105>
    2de5: 0f b6 94 24 95 00 00 00      	movzbl	0x95(%rsp), %edx
    2ded: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2df4 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x114>
    2df4: e8 00 00 00 00               	callq	0x2df9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x119>
    2df9: 0f b7 84 24 94 00 00 00      	movzwl	0x94(%rsp), %eax
    2e01: e9 04 01 00 00               	jmp	0x2f0a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x22a>
    2e06: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    2e0e: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x2e14 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x134>
    2e14: 89 84 24 a4 00 00 00         	movl	%eax, 0xa4(%rsp)
    2e1b: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    2e23: e8 00 00 00 00               	callq	0x2e28 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x148>
    2e28: 83 bc 24 a4 00 00 00 ff      	cmpl	$-0x1, 0xa4(%rsp)
    2e30: 75 4a                        	jne	0x2e7c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x19c>
    2e32: e8 00 00 00 00               	callq	0x2e37 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x157>
    2e37: 0f b6 c8                     	movzbl	%al, %ecx
    2e3a: e8 00 00 00 00               	callq	0x2e3f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x15f>
    2e3f: 66 89 84 24 b4 00 00 00      	movw	%ax, 0xb4(%rsp)
    2e47: 0f b6 94 24 b4 00 00 00      	movzbl	0xb4(%rsp), %edx
    2e4f: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2e56 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x176>
    2e56: e8 00 00 00 00               	callq	0x2e5b <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x17b>
    2e5b: 0f b6 94 24 b5 00 00 00      	movzbl	0xb5(%rsp), %edx
    2e63: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2e6a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x18a>
    2e6a: e8 00 00 00 00               	callq	0x2e6f <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x18f>
    2e6f: 0f b7 84 24 b4 00 00 00      	movzwl	0xb4(%rsp), %eax
    2e77: e9 8e 00 00 00               	jmp	0x2f0a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x22a>
    2e7c: 8b 84 24 a4 00 00 00         	movl	0xa4(%rsp), %eax
    2e83: 83 e0 10                     	andl	$0x10, %eax
    2e86: 85 c0                        	testl	%eax, %eax
    2e88: 74 41                        	je	0x2ecb <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x1eb>
    2e8a: b1 01                        	movb	$0x1, %cl
    2e8c: e8 00 00 00 00               	callq	0x2e91 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x1b1>
    2e91: 66 89 84 24 d4 00 00 00      	movw	%ax, 0xd4(%rsp)
    2e99: 0f b6 94 24 d4 00 00 00      	movzbl	0xd4(%rsp), %edx
    2ea1: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2ea8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x1c8>
    2ea8: e8 00 00 00 00               	callq	0x2ead <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x1cd>
    2ead: 0f b6 94 24 d5 00 00 00      	movzbl	0xd5(%rsp), %edx
    2eb5: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2ebc <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x1dc>
    2ebc: e8 00 00 00 00               	callq	0x2ec1 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x1e1>
    2ec1: 0f b7 84 24 d4 00 00 00      	movzwl	0xd4(%rsp), %eax
    2ec9: eb 3f                        	jmp	0x2f0a <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x22a>
    2ecb: 33 c9                        	xorl	%ecx, %ecx
    2ecd: e8 00 00 00 00               	callq	0x2ed2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x1f2>
    2ed2: 66 89 84 24 f4 00 00 00      	movw	%ax, 0xf4(%rsp)
    2eda: 0f b6 94 24 f4 00 00 00      	movzbl	0xf4(%rsp), %edx
    2ee2: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2ee9 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x209>
    2ee9: e8 00 00 00 00               	callq	0x2eee <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x20e>
    2eee: 0f b6 94 24 f5 00 00 00      	movzbl	0xf5(%rsp), %edx
    2ef6: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x2efd <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x21d>
    2efd: e8 00 00 00 00               	callq	0x2f02 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x222>
    2f02: 0f b7 84 24 f4 00 00 00      	movzwl	0xf4(%rsp), %eax
    2f0a: 48 8b f8                     	movq	%rax, %rdi
    2f0d: 48 8b cc                     	movq	%rsp, %rcx
    2f10: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x2f17 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x237>
    2f17: e8 00 00 00 00               	callq	0x2f1c <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind+0x23c>
    2f1c: 48 8b c7                     	movq	%rdi, %rax
    2f1f: 48 81 c4 10 01 00 00         	addq	$0x110, %rsp            # imm = 0x110
    2f26: 5f                           	popq	%rdi
    2f27: c3                           	retq
    2f28: cc                           	int3
    2f29: cc                           	int3
    2f2a: cc                           	int3
    2f2b: cc                           	int3
    2f2c: cc                           	int3
    2f2d: cc                           	int3
    2f2e: cc                           	int3
    2f2f: cc                           	int3

0000000000002f30 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict>:
    2f30: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    2f35: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    2f3a: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    2f3f: 56                           	pushq	%rsi
    2f40: 57                           	pushq	%rdi
    2f41: 48 81 ec 88 00 00 00         	subq	$0x88, %rsp
    2f48: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    2f4d: b9 1a 00 00 00               	movl	$0x1a, %ecx
    2f52: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    2f57: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    2f59: 48 8b 8c 24 a0 00 00 00      	movq	0xa0(%rsp), %rcx
    2f61: 48 c7 44 24 28 00 00 00 00   	movq	$0x0, 0x28(%rsp)
    2f6a: 48 83 bc 24 a8 00 00 00 00   	cmpq	$0x0, 0xa8(%rsp)
    2f73: 74 13                        	je	0x2f88 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x58>
    2f75: 48 8b 84 24 a8 00 00 00      	movq	0xa8(%rsp), %rax
    2f7d: 48 8b 40 08                  	movq	0x8(%rax), %rax
    2f81: 48 89 44 24 70               	movq	%rax, 0x70(%rsp)
    2f86: eb 09                        	jmp	0x2f91 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x61>
    2f88: 48 c7 44 24 70 00 00 00 00   	movq	$0x0, 0x70(%rsp)
    2f91: 48 8b 44 24 70               	movq	0x70(%rsp), %rax
    2f96: 48 89 44 24 30               	movq	%rax, 0x30(%rsp)
    2f9b: b9 18 00 00 00               	movl	$0x18, %ecx
    2fa0: e8 00 00 00 00               	callq	0x2fa5 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x75>
    2fa5: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    2faa: 48 83 7c 24 48 00            	cmpq	$0x0, 0x48(%rsp)
    2fb0: 75 24                        	jne	0x2fd6 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0xa6>
    2fb2: 48 8d 44 24 28               	leaq	0x28(%rsp), %rax
    2fb7: 48 8b bc 24 a0 00 00 00      	movq	0xa0(%rsp), %rdi
    2fbf: 48 8b f0                     	movq	%rax, %rsi
    2fc2: b9 10 00 00 00               	movl	$0x10, %ecx
    2fc7: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    2fc9: 48 8b 84 24 a0 00 00 00      	movq	0xa0(%rsp), %rax
    2fd1: e9 15 01 00 00               	jmp	0x30eb <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x1bb>
    2fd6: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    2fdb: c7 40 0c 01 00 00 00         	movl	$0x1, 0xc(%rax)
    2fe2: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    2fe7: c7 40 10 00 00 00 00         	movl	$0x0, 0x10(%rax)
    2fee: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    2ff3: 48 c7 00 00 00 00 00         	movq	$0x0, (%rax)
    2ffa: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    2fff: c7 40 08 00 00 00 00         	movl	$0x0, 0x8(%rax)
    3006: 48 83 bc 24 b0 00 00 00 00   	cmpq	$0x0, 0xb0(%rsp)
    300f: 0f 84 ad 00 00 00            	je	0x30c2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x192>
    3015: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    301d: 48 83 38 00                  	cmpq	$0x0, (%rax)
    3021: 0f 84 9b 00 00 00            	je	0x30c2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x192>
    3027: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    302f: 48 8b 50 08                  	movq	0x8(%rax), %rdx
    3033: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    303b: 48 8b 08                     	movq	(%rax), %rcx
    303e: e8 00 00 00 00               	callq	0x3043 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x113>
    3043: 85 c0                        	testl	%eax, %eax
    3045: 74 7b                        	je	0x30c2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x192>
    3047: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    304f: 48 8b 50 08                  	movq	0x8(%rax), %rdx
    3053: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    305b: 48 8b 08                     	movq	(%rax), %rcx
    305e: e8 00 00 00 00               	callq	0x3063 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x133>
    3063: 85 c0                        	testl	%eax, %eax
    3065: 75 5b                        	jne	0x30c2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x192>
    3067: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%rsp)
    306f: 4c 8d 44 24 54               	leaq	0x54(%rsp), %r8
    3074: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    307c: 48 8b 50 08                  	movq	0x8(%rax), %rdx
    3080: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    3088: 48 8b 08                     	movq	(%rax), %rcx
    308b: e8 00 00 00 00               	callq	0x3090 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x160>
    3090: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    3095: 48 83 7c 24 68 00            	cmpq	$0x0, 0x68(%rsp)
    309b: 74 25                        	je	0x30c2 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x192>
    309d: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    30a2: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    30a7: 48 89 08                     	movq	%rcx, (%rax)
    30aa: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    30af: 8b 4c 24 54                  	movl	0x54(%rsp), %ecx
    30b3: 89 48 08                     	movl	%ecx, 0x8(%rax)
    30b6: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    30bb: c7 40 10 01 00 00 00         	movl	$0x1, 0x10(%rax)
    30c2: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    30c7: 48 89 44 24 28               	movq	%rax, 0x28(%rsp)
    30cc: 48 8d 44 24 28               	leaq	0x28(%rsp), %rax
    30d1: 48 8b bc 24 a0 00 00 00      	movq	0xa0(%rsp), %rdi
    30d9: 48 8b f0                     	movq	%rax, %rsi
    30dc: b9 10 00 00 00               	movl	$0x10, %ecx
    30e1: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    30e3: 48 8b 84 24 a0 00 00 00      	movq	0xa0(%rsp), %rax
    30eb: 48 8b f8                     	movq	%rax, %rdi
    30ee: 48 8b cc                     	movq	%rsp, %rcx
    30f1: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x30f8 <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x1c8>
    30f8: e8 00 00 00 00               	callq	0x30fd <cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3arestrict+0x1cd>
    30fd: 48 8b c7                     	movq	%rdi, %rax
    3100: 48 81 c4 88 00 00 00         	addq	$0x88, %rsp
    3107: 5f                           	popq	%rdi
    3108: 5e                           	popq	%rsi
    3109: c3                           	retq
    310a: cc                           	int3
    310b: cc                           	int3
    310c: cc                           	int3
    310d: cc                           	int3
    310e: cc                           	int3
    310f: cc                           	int3

0000000000003110 <File_x3a_x3aRead_x3a_x3aread_x5fall>:
    3110: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3115: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    311a: 56                           	pushq	%rsi
    311b: 57                           	pushq	%rdi
    311c: 48 83 ec 78                  	subq	$0x78, %rsp
    3120: 48 83 bc 24 98 00 00 00 00   	cmpq	$0x0, 0x98(%rsp)
    3129: 75 28                        	jne	0x3153 <File_x3a_x3aRead_x3a_x3aread_x5fall+0x43>
    312b: b2 05                        	movb	$0x5, %dl
    312d: 48 8d 4c 24 28               	leaq	0x28(%rsp), %rcx
    3132: e8 00 00 00 00               	callq	0x3137 <File_x3a_x3aRead_x3a_x3aread_x5fall+0x27>
    3137: 48 8b bc 24 90 00 00 00      	movq	0x90(%rsp), %rdi
    313f: 48 8b f0                     	movq	%rax, %rsi
    3142: b9 20 00 00 00               	movl	$0x20, %ecx
    3147: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    3149: 48 8b 84 24 90 00 00 00      	movq	0x90(%rsp), %rax
    3151: eb 39                        	jmp	0x318c <File_x3a_x3aRead_x3a_x3aread_x5fall+0x7c>
    3153: 48 8b 84 24 98 00 00 00      	movq	0x98(%rsp), %rax
    315b: 48 8b 00                     	movq	(%rax), %rax
    315e: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    3163: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    3168: 48 8d 4c 24 48               	leaq	0x48(%rsp), %rcx
    316d: e8 00 00 00 00               	callq	0x3172 <File_x3a_x3aRead_x3a_x3aread_x5fall+0x62>
    3172: 48 8b bc 24 90 00 00 00      	movq	0x90(%rsp), %rdi
    317a: 48 8b f0                     	movq	%rax, %rsi
    317d: b9 20 00 00 00               	movl	$0x20, %ecx
    3182: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    3184: 48 8b 84 24 90 00 00 00      	movq	0x90(%rsp), %rax
    318c: 48 83 c4 78                  	addq	$0x78, %rsp
    3190: 5f                           	popq	%rdi
    3191: 5e                           	popq	%rsi
    3192: c3                           	retq
    3193: cc                           	int3
    3194: cc                           	int3
    3195: cc                           	int3
    3196: cc                           	int3
    3197: cc                           	int3
    3198: cc                           	int3
    3199: cc                           	int3
    319a: cc                           	int3
    319b: cc                           	int3
    319c: cc                           	int3
    319d: cc                           	int3
    319e: cc                           	int3
    319f: cc                           	int3

00000000000031a0 <File_x3a_x3aRead_x3a_x3aread_x5fall_x5fbytes>:
    31a0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    31a5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    31aa: 56                           	pushq	%rsi
    31ab: 57                           	pushq	%rdi
    31ac: 48 83 ec 78                  	subq	$0x78, %rsp
    31b0: 48 83 bc 24 98 00 00 00 00   	cmpq	$0x0, 0x98(%rsp)
    31b9: 75 28                        	jne	0x31e3 <File_x3a_x3aRead_x3a_x3aread_x5fall_x5fbytes+0x43>
    31bb: b2 05                        	movb	$0x5, %dl
    31bd: 48 8d 4c 24 28               	leaq	0x28(%rsp), %rcx
    31c2: e8 00 00 00 00               	callq	0x31c7 <File_x3a_x3aRead_x3a_x3aread_x5fall_x5fbytes+0x27>
    31c7: 48 8b bc 24 90 00 00 00      	movq	0x90(%rsp), %rdi
    31cf: 48 8b f0                     	movq	%rax, %rsi
    31d2: b9 20 00 00 00               	movl	$0x20, %ecx
    31d7: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    31d9: 48 8b 84 24 90 00 00 00      	movq	0x90(%rsp), %rax
    31e1: eb 39                        	jmp	0x321c <File_x3a_x3aRead_x3a_x3aread_x5fall_x5fbytes+0x7c>
    31e3: 48 8b 84 24 98 00 00 00      	movq	0x98(%rsp), %rax
    31eb: 48 8b 00                     	movq	(%rax), %rax
    31ee: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    31f3: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    31f8: 48 8d 4c 24 48               	leaq	0x48(%rsp), %rcx
    31fd: e8 00 00 00 00               	callq	0x3202 <File_x3a_x3aRead_x3a_x3aread_x5fall_x5fbytes+0x62>
    3202: 48 8b bc 24 90 00 00 00      	movq	0x90(%rsp), %rdi
    320a: 48 8b f0                     	movq	%rax, %rsi
    320d: b9 20 00 00 00               	movl	$0x20, %ecx
    3212: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    3214: 48 8b 84 24 90 00 00 00      	movq	0x90(%rsp), %rax
    321c: 48 83 c4 78                  	addq	$0x78, %rsp
    3220: 5f                           	popq	%rdi
    3221: 5e                           	popq	%rsi
    3222: c3                           	retq
    3223: cc                           	int3
    3224: cc                           	int3
    3225: cc                           	int3
    3226: cc                           	int3
    3227: cc                           	int3
    3228: cc                           	int3
    3229: cc                           	int3
    322a: cc                           	int3
    322b: cc                           	int3
    322c: cc                           	int3
    322d: cc                           	int3
    322e: cc                           	int3
    322f: cc                           	int3

0000000000003230 <File_x3a_x3aRead_x3a_x3aclose>:
    3230: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3235: 57                           	pushq	%rdi
    3236: 48 83 ec 30                  	subq	$0x30, %rsp
    323a: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    323f: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    3244: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    324a: 74 14                        	je	0x3260 <File_x3a_x3aRead_x3a_x3aclose+0x30>
    324c: 48 83 7c 24 20 ff            	cmpq	$-0x1, 0x20(%rsp)
    3252: 74 0c                        	je	0x3260 <File_x3a_x3aRead_x3a_x3aclose+0x30>
    3254: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    3259: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x325f <File_x3a_x3aRead_x3a_x3aclose+0x2f>
    325f: 90                           	nop
    3260: 48 83 c4 30                  	addq	$0x30, %rsp
    3264: 5f                           	popq	%rdi
    3265: c3                           	retq
    3266: cc                           	int3
    3267: cc                           	int3
    3268: cc                           	int3
    3269: cc                           	int3
    326a: cc                           	int3
    326b: cc                           	int3
    326c: cc                           	int3
    326d: cc                           	int3
    326e: cc                           	int3
    326f: cc                           	int3

0000000000003270 <File_x3a_x3aWrite_x3a_x3awrite>:
    3270: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3275: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    327a: 57                           	pushq	%rdi
    327b: 48 83 ec 30                  	subq	$0x30, %rsp
    327f: 48 83 7c 24 40 00            	cmpq	$0x0, 0x40(%rsp)
    3285: 75 09                        	jne	0x3290 <File_x3a_x3aWrite_x3a_x3awrite+0x20>
    3287: b1 05                        	movb	$0x5, %cl
    3289: e8 00 00 00 00               	callq	0x328e <File_x3a_x3aWrite_x3a_x3awrite+0x1e>
    328e: eb 1c                        	jmp	0x32ac <File_x3a_x3aWrite_x3a_x3awrite+0x3c>
    3290: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    3295: 48 8b 00                     	movq	(%rax), %rax
    3298: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    329d: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
    32a2: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    32a7: e8 00 00 00 00               	callq	0x32ac <File_x3a_x3aWrite_x3a_x3awrite+0x3c>
    32ac: 48 83 c4 30                  	addq	$0x30, %rsp
    32b0: 5f                           	popq	%rdi
    32b1: c3                           	retq
    32b2: cc                           	int3
    32b3: cc                           	int3
    32b4: cc                           	int3
    32b5: cc                           	int3
    32b6: cc                           	int3
    32b7: cc                           	int3
    32b8: cc                           	int3
    32b9: cc                           	int3
    32ba: cc                           	int3
    32bb: cc                           	int3
    32bc: cc                           	int3
    32bd: cc                           	int3
    32be: cc                           	int3
    32bf: cc                           	int3

00000000000032c0 <File_x3a_x3aWrite_x3a_x3aflush>:
    32c0: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    32c5: 57                           	pushq	%rdi
    32c6: 48 83 ec 30                  	subq	$0x30, %rsp
    32ca: 48 83 7c 24 40 00            	cmpq	$0x0, 0x40(%rsp)
    32d0: 75 09                        	jne	0x32db <File_x3a_x3aWrite_x3a_x3aflush+0x1b>
    32d2: b1 05                        	movb	$0x5, %cl
    32d4: e8 00 00 00 00               	callq	0x32d9 <File_x3a_x3aWrite_x3a_x3aflush+0x19>
    32d9: eb 49                        	jmp	0x3324 <File_x3a_x3aWrite_x3a_x3aflush+0x64>
    32db: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    32e0: 48 8b 00                     	movq	(%rax), %rax
    32e3: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    32e8: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    32ee: 74 08                        	je	0x32f8 <File_x3a_x3aWrite_x3a_x3aflush+0x38>
    32f0: 48 83 7c 24 20 ff            	cmpq	$-0x1, 0x20(%rsp)
    32f6: 75 09                        	jne	0x3301 <File_x3a_x3aWrite_x3a_x3aflush+0x41>
    32f8: b1 05                        	movb	$0x5, %cl
    32fa: e8 00 00 00 00               	callq	0x32ff <File_x3a_x3aWrite_x3a_x3aflush+0x3f>
    32ff: eb 23                        	jmp	0x3324 <File_x3a_x3aWrite_x3a_x3aflush+0x64>
    3301: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    3306: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x330c <File_x3a_x3aWrite_x3a_x3aflush+0x4c>
    330c: 85 c0                        	testl	%eax, %eax
    330e: 75 0f                        	jne	0x331f <File_x3a_x3aWrite_x3a_x3aflush+0x5f>
    3310: e8 00 00 00 00               	callq	0x3315 <File_x3a_x3aWrite_x3a_x3aflush+0x55>
    3315: 0f b6 c8                     	movzbl	%al, %ecx
    3318: e8 00 00 00 00               	callq	0x331d <File_x3a_x3aWrite_x3a_x3aflush+0x5d>
    331d: eb 05                        	jmp	0x3324 <File_x3a_x3aWrite_x3a_x3aflush+0x64>
    331f: e8 00 00 00 00               	callq	0x3324 <File_x3a_x3aWrite_x3a_x3aflush+0x64>
    3324: 48 83 c4 30                  	addq	$0x30, %rsp
    3328: 5f                           	popq	%rdi
    3329: c3                           	retq
    332a: cc                           	int3
    332b: cc                           	int3
    332c: cc                           	int3
    332d: cc                           	int3
    332e: cc                           	int3
    332f: cc                           	int3

0000000000003330 <File_x3a_x3aWrite_x3a_x3aclose>:
    3330: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3335: 57                           	pushq	%rdi
    3336: 48 83 ec 30                  	subq	$0x30, %rsp
    333a: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    333f: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    3344: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    334a: 74 14                        	je	0x3360 <File_x3a_x3aWrite_x3a_x3aclose+0x30>
    334c: 48 83 7c 24 20 ff            	cmpq	$-0x1, 0x20(%rsp)
    3352: 74 0c                        	je	0x3360 <File_x3a_x3aWrite_x3a_x3aclose+0x30>
    3354: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    3359: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x335f <File_x3a_x3aWrite_x3a_x3aclose+0x2f>
    335f: 90                           	nop
    3360: 48 83 c4 30                  	addq	$0x30, %rsp
    3364: 5f                           	popq	%rdi
    3365: c3                           	retq
    3366: cc                           	int3
    3367: cc                           	int3
    3368: cc                           	int3
    3369: cc                           	int3
    336a: cc                           	int3
    336b: cc                           	int3
    336c: cc                           	int3
    336d: cc                           	int3
    336e: cc                           	int3
    336f: cc                           	int3

0000000000003370 <File_x3a_x3aAppend_x3a_x3awrite>:
    3370: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3375: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    337a: 57                           	pushq	%rdi
    337b: 48 83 ec 30                  	subq	$0x30, %rsp
    337f: 48 83 7c 24 40 00            	cmpq	$0x0, 0x40(%rsp)
    3385: 75 09                        	jne	0x3390 <File_x3a_x3aAppend_x3a_x3awrite+0x20>
    3387: b1 05                        	movb	$0x5, %cl
    3389: e8 00 00 00 00               	callq	0x338e <File_x3a_x3aAppend_x3a_x3awrite+0x1e>
    338e: eb 1c                        	jmp	0x33ac <File_x3a_x3aAppend_x3a_x3awrite+0x3c>
    3390: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    3395: 48 8b 00                     	movq	(%rax), %rax
    3398: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    339d: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
    33a2: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    33a7: e8 00 00 00 00               	callq	0x33ac <File_x3a_x3aAppend_x3a_x3awrite+0x3c>
    33ac: 48 83 c4 30                  	addq	$0x30, %rsp
    33b0: 5f                           	popq	%rdi
    33b1: c3                           	retq
    33b2: cc                           	int3
    33b3: cc                           	int3
    33b4: cc                           	int3
    33b5: cc                           	int3
    33b6: cc                           	int3
    33b7: cc                           	int3
    33b8: cc                           	int3
    33b9: cc                           	int3
    33ba: cc                           	int3
    33bb: cc                           	int3
    33bc: cc                           	int3
    33bd: cc                           	int3
    33be: cc                           	int3
    33bf: cc                           	int3

00000000000033c0 <File_x3a_x3aAppend_x3a_x3aflush>:
    33c0: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    33c5: 57                           	pushq	%rdi
    33c6: 48 83 ec 30                  	subq	$0x30, %rsp
    33ca: 48 83 7c 24 40 00            	cmpq	$0x0, 0x40(%rsp)
    33d0: 75 09                        	jne	0x33db <File_x3a_x3aAppend_x3a_x3aflush+0x1b>
    33d2: b1 05                        	movb	$0x5, %cl
    33d4: e8 00 00 00 00               	callq	0x33d9 <File_x3a_x3aAppend_x3a_x3aflush+0x19>
    33d9: eb 49                        	jmp	0x3424 <File_x3a_x3aAppend_x3a_x3aflush+0x64>
    33db: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    33e0: 48 8b 00                     	movq	(%rax), %rax
    33e3: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    33e8: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    33ee: 74 08                        	je	0x33f8 <File_x3a_x3aAppend_x3a_x3aflush+0x38>
    33f0: 48 83 7c 24 20 ff            	cmpq	$-0x1, 0x20(%rsp)
    33f6: 75 09                        	jne	0x3401 <File_x3a_x3aAppend_x3a_x3aflush+0x41>
    33f8: b1 05                        	movb	$0x5, %cl
    33fa: e8 00 00 00 00               	callq	0x33ff <File_x3a_x3aAppend_x3a_x3aflush+0x3f>
    33ff: eb 23                        	jmp	0x3424 <File_x3a_x3aAppend_x3a_x3aflush+0x64>
    3401: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    3406: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x340c <File_x3a_x3aAppend_x3a_x3aflush+0x4c>
    340c: 85 c0                        	testl	%eax, %eax
    340e: 75 0f                        	jne	0x341f <File_x3a_x3aAppend_x3a_x3aflush+0x5f>
    3410: e8 00 00 00 00               	callq	0x3415 <File_x3a_x3aAppend_x3a_x3aflush+0x55>
    3415: 0f b6 c8                     	movzbl	%al, %ecx
    3418: e8 00 00 00 00               	callq	0x341d <File_x3a_x3aAppend_x3a_x3aflush+0x5d>
    341d: eb 05                        	jmp	0x3424 <File_x3a_x3aAppend_x3a_x3aflush+0x64>
    341f: e8 00 00 00 00               	callq	0x3424 <File_x3a_x3aAppend_x3a_x3aflush+0x64>
    3424: 48 83 c4 30                  	addq	$0x30, %rsp
    3428: 5f                           	popq	%rdi
    3429: c3                           	retq
    342a: cc                           	int3
    342b: cc                           	int3
    342c: cc                           	int3
    342d: cc                           	int3
    342e: cc                           	int3
    342f: cc                           	int3

0000000000003430 <File_x3a_x3aAppend_x3a_x3aclose>:
    3430: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3435: 57                           	pushq	%rdi
    3436: 48 83 ec 30                  	subq	$0x30, %rsp
    343a: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    343f: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    3444: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    344a: 74 14                        	je	0x3460 <File_x3a_x3aAppend_x3a_x3aclose+0x30>
    344c: 48 83 7c 24 20 ff            	cmpq	$-0x1, 0x20(%rsp)
    3452: 74 0c                        	je	0x3460 <File_x3a_x3aAppend_x3a_x3aclose+0x30>
    3454: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    3459: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x345f <File_x3a_x3aAppend_x3a_x3aclose+0x2f>
    345f: 90                           	nop
    3460: 48 83 c4 30                  	addq	$0x30, %rsp
    3464: 5f                           	popq	%rdi
    3465: c3                           	retq
    3466: cc                           	int3
    3467: cc                           	int3
    3468: cc                           	int3
    3469: cc                           	int3
    346a: cc                           	int3
    346b: cc                           	int3
    346c: cc                           	int3
    346d: cc                           	int3
    346e: cc                           	int3
    346f: cc                           	int3

0000000000003470 <DirIter_x3a_x3aOpen_x3a_x3anext>:
    3470: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3475: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    347a: 56                           	pushq	%rsi
    347b: 57                           	pushq	%rdi
    347c: 48 81 ec 68 01 00 00         	subq	$0x168, %rsp            # imm = 0x168
    3483: 48 8d 7c 24 30               	leaq	0x30(%rsp), %rdi
    3488: b9 4e 00 00 00               	movl	$0x4e, %ecx
    348d: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    3492: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    3494: 48 8b 8c 24 80 01 00 00      	movq	0x180(%rsp), %rcx
    349c: 48 83 bc 24 88 01 00 00 00   	cmpq	$0x0, 0x188(%rsp)
    34a5: 75 3f                        	jne	0x34e6 <DirIter_x3a_x3aOpen_x3a_x3anext+0x76>
    34a7: c6 44 24 40 02               	movb	$0x2, 0x40(%rsp)
    34ac: c6 44 24 48 05               	movb	$0x5, 0x48(%rsp)
    34b1: 0f b6 54 24 40               	movzbl	0x40(%rsp), %edx
    34b6: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x34bd <DirIter_x3a_x3aOpen_x3a_x3anext+0x4d>
    34bd: e8 00 00 00 00               	callq	0x34c2 <DirIter_x3a_x3aOpen_x3a_x3anext+0x52>
    34c2: 48 8d 44 24 40               	leaq	0x40(%rsp), %rax
    34c7: 48 8b bc 24 80 01 00 00      	movq	0x180(%rsp), %rdi
    34cf: 48 8b f0                     	movq	%rax, %rsi
    34d2: b9 40 00 00 00               	movl	$0x40, %ecx
    34d7: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    34d9: 48 8b 84 24 80 01 00 00      	movq	0x180(%rsp), %rax
    34e1: e9 87 05 00 00               	jmp	0x3a6d <DirIter_x3a_x3aOpen_x3a_x3anext+0x5fd>
    34e6: 48 8b 84 24 88 01 00 00      	movq	0x188(%rsp), %rax
    34ee: 48 8b 08                     	movq	(%rax), %rcx
    34f1: e8 00 00 00 00               	callq	0x34f6 <DirIter_x3a_x3aOpen_x3a_x3anext+0x86>
    34f6: 48 89 84 24 88 00 00 00      	movq	%rax, 0x88(%rsp)
    34fe: 48 83 bc 24 88 00 00 00 00   	cmpq	$0x0, 0x88(%rsp)
    3507: 75 3f                        	jne	0x3548 <DirIter_x3a_x3aOpen_x3a_x3anext+0xd8>
    3509: c6 44 24 40 02               	movb	$0x2, 0x40(%rsp)
    350e: c6 44 24 48 05               	movb	$0x5, 0x48(%rsp)
    3513: 0f b6 54 24 40               	movzbl	0x40(%rsp), %edx
    3518: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x351f <DirIter_x3a_x3aOpen_x3a_x3anext+0xaf>
    351f: e8 00 00 00 00               	callq	0x3524 <DirIter_x3a_x3aOpen_x3a_x3anext+0xb4>
    3524: 48 8d 44 24 40               	leaq	0x40(%rsp), %rax
    3529: 48 8b bc 24 80 01 00 00      	movq	0x180(%rsp), %rdi
    3531: 48 8b f0                     	movq	%rax, %rsi
    3534: b9 40 00 00 00               	movl	$0x40, %ecx
    3539: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    353b: 48 8b 84 24 80 01 00 00      	movq	0x180(%rsp), %rax
    3543: e9 25 05 00 00               	jmp	0x3a6d <DirIter_x3a_x3aOpen_x3a_x3anext+0x5fd>
    3548: 48 8b 84 24 88 00 00 00      	movq	0x88(%rsp), %rax
    3550: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    3558: 8b 49 38                     	movl	0x38(%rcx), %ecx
    355b: 39 48 3c                     	cmpl	%ecx, 0x3c(%rax)
    355e: 72 3f                        	jb	0x359f <DirIter_x3a_x3aOpen_x3a_x3anext+0x12f>
    3560: c6 44 24 40 00               	movb	$0x0, 0x40(%rsp)
    3565: c6 44 24 48 00               	movb	$0x0, 0x48(%rsp)
    356a: 0f b6 54 24 40               	movzbl	0x40(%rsp), %edx
    356f: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x3576 <DirIter_x3a_x3aOpen_x3a_x3anext+0x106>
    3576: e8 00 00 00 00               	callq	0x357b <DirIter_x3a_x3aOpen_x3a_x3anext+0x10b>
    357b: 48 8d 44 24 40               	leaq	0x40(%rsp), %rax
    3580: 48 8b bc 24 80 01 00 00      	movq	0x180(%rsp), %rdi
    3588: 48 8b f0                     	movq	%rax, %rsi
    358b: b9 40 00 00 00               	movl	$0x40, %ecx
    3590: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    3592: 48 8b 84 24 80 01 00 00      	movq	0x180(%rsp), %rax
    359a: e9 ce 04 00 00               	jmp	0x3a6d <DirIter_x3a_x3aOpen_x3a_x3anext+0x5fd>
    359f: 48 8b 84 24 88 00 00 00      	movq	0x88(%rsp), %rax
    35a7: 8b 40 3c                     	movl	0x3c(%rax), %eax
    35aa: 89 84 24 90 00 00 00         	movl	%eax, 0x90(%rsp)
    35b1: 8b 84 24 90 00 00 00         	movl	0x90(%rsp), %eax
    35b8: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    35c0: 48 8b 49 20                  	movq	0x20(%rcx), %rcx
    35c4: 48 8b 04 c1                  	movq	(%rcx,%rax,8), %rax
    35c8: 48 89 84 24 98 00 00 00      	movq	%rax, 0x98(%rsp)
    35d0: 8b 84 24 90 00 00 00         	movl	0x90(%rsp), %eax
    35d7: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    35df: 48 8b 49 28                  	movq	0x28(%rcx), %rcx
    35e3: 8b 04 81                     	movl	(%rcx,%rax,4), %eax
    35e6: 89 84 24 a0 00 00 00         	movl	%eax, 0xa0(%rsp)
    35ed: c7 84 24 b4 00 00 00 00 00 00 00     	movl	$0x0, 0xb4(%rsp)
    35f8: 4c 8d 84 24 b4 00 00 00      	leaq	0xb4(%rsp), %r8
    3600: 8b 94 24 a0 00 00 00         	movl	0xa0(%rsp), %edx
    3607: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
    360f: e8 00 00 00 00               	callq	0x3614 <DirIter_x3a_x3aOpen_x3a_x3anext+0x1a4>
    3614: 48 89 84 24 c8 00 00 00      	movq	%rax, 0xc8(%rsp)
    361c: 48 83 bc 24 c8 00 00 00 00   	cmpq	$0x0, 0xc8(%rsp)
    3625: 75 49                        	jne	0x3670 <DirIter_x3a_x3aOpen_x3a_x3anext+0x200>
    3627: 83 bc 24 a0 00 00 00 00      	cmpl	$0x0, 0xa0(%rsp)
    362f: 74 3f                        	je	0x3670 <DirIter_x3a_x3aOpen_x3a_x3anext+0x200>
    3631: c6 44 24 40 02               	movb	$0x2, 0x40(%rsp)
    3636: c6 44 24 48 05               	movb	$0x5, 0x48(%rsp)
    363b: 0f b6 54 24 40               	movzbl	0x40(%rsp), %edx
    3640: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x3647 <DirIter_x3a_x3aOpen_x3a_x3anext+0x1d7>
    3647: e8 00 00 00 00               	callq	0x364c <DirIter_x3a_x3aOpen_x3a_x3anext+0x1dc>
    364c: 48 8d 44 24 40               	leaq	0x40(%rsp), %rax
    3651: 48 8b bc 24 80 01 00 00      	movq	0x180(%rsp), %rdi
    3659: 48 8b f0                     	movq	%rax, %rsi
    365c: b9 40 00 00 00               	movl	$0x40, %ecx
    3661: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    3663: 48 8b 84 24 80 01 00 00      	movq	0x180(%rsp), %rax
    366b: e9 fd 03 00 00               	jmp	0x3a6d <DirIter_x3a_x3aOpen_x3a_x3anext+0x5fd>
    3670: c7 84 24 d4 00 00 00 00 00 00 00     	movl	$0x0, 0xd4(%rsp)
    367b: 8b 84 24 b4 00 00 00         	movl	0xb4(%rsp), %eax
    3682: 48 8d 8c 24 d4 00 00 00      	leaq	0xd4(%rsp), %rcx
    368a: 48 89 4c 24 20               	movq	%rcx, 0x20(%rsp)
    368f: 44 8b c8                     	movl	%eax, %r9d
    3692: 4c 8b 84 24 c8 00 00 00      	movq	0xc8(%rsp), %r8
    369a: 48 8b 84 24 88 00 00 00      	movq	0x88(%rsp), %rax
    36a2: 8b 50 18                     	movl	0x18(%rax), %edx
    36a5: 48 8b 84 24 88 00 00 00      	movq	0x88(%rsp), %rax
    36ad: 48 8b 48 10                  	movq	0x10(%rax), %rcx
    36b1: e8 00 00 00 00               	callq	0x36b6 <DirIter_x3a_x3aOpen_x3a_x3anext+0x246>
    36b6: 48 89 84 24 e8 00 00 00      	movq	%rax, 0xe8(%rsp)
    36be: 48 83 bc 24 e8 00 00 00 00   	cmpq	$0x0, 0xe8(%rsp)
    36c7: 75 70                        	jne	0x3739 <DirIter_x3a_x3aOpen_x3a_x3anext+0x2c9>
    36c9: 48 8b 84 24 88 00 00 00      	movq	0x88(%rsp), %rax
    36d1: 83 78 18 00                  	cmpl	$0x0, 0x18(%rax)
    36d5: 75 0a                        	jne	0x36e1 <DirIter_x3a_x3aOpen_x3a_x3anext+0x271>
    36d7: 83 bc 24 b4 00 00 00 00      	cmpl	$0x0, 0xb4(%rsp)
    36df: 74 58                        	je	0x3739 <DirIter_x3a_x3aOpen_x3a_x3anext+0x2c9>
    36e1: 48 83 bc 24 c8 00 00 00 00   	cmpq	$0x0, 0xc8(%rsp)
    36ea: 74 0e                        	je	0x36fa <DirIter_x3a_x3aOpen_x3a_x3anext+0x28a>
    36ec: 48 8b 8c 24 c8 00 00 00      	movq	0xc8(%rsp), %rcx
    36f4: e8 00 00 00 00               	callq	0x36f9 <DirIter_x3a_x3aOpen_x3a_x3anext+0x289>
    36f9: 90                           	nop
    36fa: c6 44 24 40 02               	movb	$0x2, 0x40(%rsp)
    36ff: c6 44 24 48 05               	movb	$0x5, 0x48(%rsp)
    3704: 0f b6 54 24 40               	movzbl	0x40(%rsp), %edx
    3709: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x3710 <DirIter_x3a_x3aOpen_x3a_x3anext+0x2a0>
    3710: e8 00 00 00 00               	callq	0x3715 <DirIter_x3a_x3aOpen_x3a_x3anext+0x2a5>
    3715: 48 8d 44 24 40               	leaq	0x40(%rsp), %rax
    371a: 48 8b bc 24 80 01 00 00      	movq	0x180(%rsp), %rdi
    3722: 48 8b f0                     	movq	%rax, %rsi
    3725: b9 40 00 00 00               	movl	$0x40, %ecx
    372a: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    372c: 48 8b 84 24 80 01 00 00      	movq	0x180(%rsp), %rax
    3734: e9 34 03 00 00               	jmp	0x3a6d <DirIter_x3a_x3aOpen_x3a_x3anext+0x5fd>
    3739: 48 c7 84 24 f0 00 00 00 00 00 00 00  	movq	$0x0, 0xf0(%rsp)
    3745: 8b 84 24 b4 00 00 00         	movl	0xb4(%rsp), %eax
    374c: 48 89 84 24 f8 00 00 00      	movq	%rax, 0xf8(%rsp)
    3754: 48 83 bc 24 f8 00 00 00 00   	cmpq	$0x0, 0xf8(%rsp)
    375d: 75 0e                        	jne	0x376d <DirIter_x3a_x3aOpen_x3a_x3anext+0x2fd>
    375f: 48 c7 84 24 f0 00 00 00 00 00 00 00  	movq	$0x0, 0xf0(%rsp)
    376b: eb 1a                        	jmp	0x3787 <DirIter_x3a_x3aOpen_x3a_x3anext+0x317>
    376d: 45 33 c0                     	xorl	%r8d, %r8d
    3770: 48 8b 94 24 f8 00 00 00      	movq	0xf8(%rsp), %rdx
    3778: 33 c9                        	xorl	%ecx, %ecx
    377a: e8 00 00 00 00               	callq	0x377f <DirIter_x3a_x3aOpen_x3a_x3anext+0x30f>
    377f: 48 89 84 24 f0 00 00 00      	movq	%rax, 0xf0(%rsp)
    3787: 48 83 bc 24 f8 00 00 00 00   	cmpq	$0x0, 0xf8(%rsp)
    3790: 74 7c                        	je	0x380e <DirIter_x3a_x3aOpen_x3a_x3anext+0x39e>
    3792: 48 83 bc 24 f0 00 00 00 00   	cmpq	$0x0, 0xf0(%rsp)
    379b: 75 71                        	jne	0x380e <DirIter_x3a_x3aOpen_x3a_x3anext+0x39e>
    379d: 48 83 bc 24 c8 00 00 00 00   	cmpq	$0x0, 0xc8(%rsp)
    37a6: 74 0e                        	je	0x37b6 <DirIter_x3a_x3aOpen_x3a_x3anext+0x346>
    37a8: 48 8b 8c 24 c8 00 00 00      	movq	0xc8(%rsp), %rcx
    37b0: e8 00 00 00 00               	callq	0x37b5 <DirIter_x3a_x3aOpen_x3a_x3anext+0x345>
    37b5: 90                           	nop
    37b6: 48 83 bc 24 e8 00 00 00 00   	cmpq	$0x0, 0xe8(%rsp)
    37bf: 74 0e                        	je	0x37cf <DirIter_x3a_x3aOpen_x3a_x3anext+0x35f>
    37c1: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    37c9: e8 00 00 00 00               	callq	0x37ce <DirIter_x3a_x3aOpen_x3a_x3anext+0x35e>
    37ce: 90                           	nop
    37cf: c6 44 24 40 02               	movb	$0x2, 0x40(%rsp)
    37d4: c6 44 24 48 05               	movb	$0x5, 0x48(%rsp)
    37d9: 0f b6 54 24 40               	movzbl	0x40(%rsp), %edx
    37de: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x37e5 <DirIter_x3a_x3aOpen_x3a_x3anext+0x375>
    37e5: e8 00 00 00 00               	callq	0x37ea <DirIter_x3a_x3aOpen_x3a_x3anext+0x37a>
    37ea: 48 8d 44 24 40               	leaq	0x40(%rsp), %rax
    37ef: 48 8b bc 24 80 01 00 00      	movq	0x180(%rsp), %rdi
    37f7: 48 8b f0                     	movq	%rax, %rsi
    37fa: b9 40 00 00 00               	movl	$0x40, %ecx
    37ff: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    3801: 48 8b 84 24 80 01 00 00      	movq	0x180(%rsp), %rax
    3809: e9 5f 02 00 00               	jmp	0x3a6d <DirIter_x3a_x3aOpen_x3a_x3anext+0x5fd>
    380e: 48 c7 84 24 00 01 00 00 00 00 00 00  	movq	$0x0, 0x100(%rsp)
    381a: 8b 84 24 d4 00 00 00         	movl	0xd4(%rsp), %eax
    3821: 48 89 84 24 08 01 00 00      	movq	%rax, 0x108(%rsp)
    3829: 48 83 bc 24 08 01 00 00 00   	cmpq	$0x0, 0x108(%rsp)
    3832: 75 0e                        	jne	0x3842 <DirIter_x3a_x3aOpen_x3a_x3anext+0x3d2>
    3834: 48 c7 84 24 00 01 00 00 00 00 00 00  	movq	$0x0, 0x100(%rsp)
    3840: eb 1a                        	jmp	0x385c <DirIter_x3a_x3aOpen_x3a_x3anext+0x3ec>
    3842: 45 33 c0                     	xorl	%r8d, %r8d
    3845: 48 8b 94 24 08 01 00 00      	movq	0x108(%rsp), %rdx
    384d: 33 c9                        	xorl	%ecx, %ecx
    384f: e8 00 00 00 00               	callq	0x3854 <DirIter_x3a_x3aOpen_x3a_x3anext+0x3e4>
    3854: 48 89 84 24 00 01 00 00      	movq	%rax, 0x100(%rsp)
    385c: 48 83 bc 24 08 01 00 00 00   	cmpq	$0x0, 0x108(%rsp)
    3865: 0f 84 99 00 00 00            	je	0x3904 <DirIter_x3a_x3aOpen_x3a_x3anext+0x494>
    386b: 48 83 bc 24 00 01 00 00 00   	cmpq	$0x0, 0x100(%rsp)
    3874: 0f 85 8a 00 00 00            	jne	0x3904 <DirIter_x3a_x3aOpen_x3a_x3anext+0x494>
    387a: 48 83 bc 24 f0 00 00 00 00   	cmpq	$0x0, 0xf0(%rsp)
    3883: 74 0e                        	je	0x3893 <DirIter_x3a_x3aOpen_x3a_x3anext+0x423>
    3885: 48 8b 8c 24 f0 00 00 00      	movq	0xf0(%rsp), %rcx
    388d: e8 00 00 00 00               	callq	0x3892 <DirIter_x3a_x3aOpen_x3a_x3anext+0x422>
    3892: 90                           	nop
    3893: 48 83 bc 24 c8 00 00 00 00   	cmpq	$0x0, 0xc8(%rsp)
    389c: 74 0e                        	je	0x38ac <DirIter_x3a_x3aOpen_x3a_x3anext+0x43c>
    389e: 48 8b 8c 24 c8 00 00 00      	movq	0xc8(%rsp), %rcx
    38a6: e8 00 00 00 00               	callq	0x38ab <DirIter_x3a_x3aOpen_x3a_x3anext+0x43b>
    38ab: 90                           	nop
    38ac: 48 83 bc 24 e8 00 00 00 00   	cmpq	$0x0, 0xe8(%rsp)
    38b5: 74 0e                        	je	0x38c5 <DirIter_x3a_x3aOpen_x3a_x3anext+0x455>
    38b7: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    38bf: e8 00 00 00 00               	callq	0x38c4 <DirIter_x3a_x3aOpen_x3a_x3anext+0x454>
    38c4: 90                           	nop
    38c5: c6 44 24 40 02               	movb	$0x2, 0x40(%rsp)
    38ca: c6 44 24 48 05               	movb	$0x5, 0x48(%rsp)
    38cf: 0f b6 54 24 40               	movzbl	0x40(%rsp), %edx
    38d4: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x38db <DirIter_x3a_x3aOpen_x3a_x3anext+0x46b>
    38db: e8 00 00 00 00               	callq	0x38e0 <DirIter_x3a_x3aOpen_x3a_x3anext+0x470>
    38e0: 48 8d 44 24 40               	leaq	0x40(%rsp), %rax
    38e5: 48 8b bc 24 80 01 00 00      	movq	0x180(%rsp), %rdi
    38ed: 48 8b f0                     	movq	%rax, %rsi
    38f0: b9 40 00 00 00               	movl	$0x40, %ecx
    38f5: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    38f7: 48 8b 84 24 80 01 00 00      	movq	0x180(%rsp), %rax
    38ff: e9 69 01 00 00               	jmp	0x3a6d <DirIter_x3a_x3aOpen_x3a_x3anext+0x5fd>
    3904: 48 83 bc 24 f8 00 00 00 00   	cmpq	$0x0, 0xf8(%rsp)
    390d: 74 1e                        	je	0x392d <DirIter_x3a_x3aOpen_x3a_x3anext+0x4bd>
    390f: 4c 8b 84 24 f8 00 00 00      	movq	0xf8(%rsp), %r8
    3917: 48 8b 94 24 c8 00 00 00      	movq	0xc8(%rsp), %rdx
    391f: 48 8b 8c 24 f0 00 00 00      	movq	0xf0(%rsp), %rcx
    3927: e8 00 00 00 00               	callq	0x392c <DirIter_x3a_x3aOpen_x3a_x3anext+0x4bc>
    392c: 90                           	nop
    392d: 48 83 bc 24 08 01 00 00 00   	cmpq	$0x0, 0x108(%rsp)
    3936: 74 1e                        	je	0x3956 <DirIter_x3a_x3aOpen_x3a_x3anext+0x4e6>
    3938: 4c 8b 84 24 08 01 00 00      	movq	0x108(%rsp), %r8
    3940: 48 8b 94 24 e8 00 00 00      	movq	0xe8(%rsp), %rdx
    3948: 48 8b 8c 24 00 01 00 00      	movq	0x100(%rsp), %rcx
    3950: e8 00 00 00 00               	callq	0x3955 <DirIter_x3a_x3aOpen_x3a_x3anext+0x4e5>
    3955: 90                           	nop
    3956: 48 83 bc 24 c8 00 00 00 00   	cmpq	$0x0, 0xc8(%rsp)
    395f: 74 0e                        	je	0x396f <DirIter_x3a_x3aOpen_x3a_x3anext+0x4ff>
    3961: 48 8b 8c 24 c8 00 00 00      	movq	0xc8(%rsp), %rcx
    3969: e8 00 00 00 00               	callq	0x396e <DirIter_x3a_x3aOpen_x3a_x3anext+0x4fe>
    396e: 90                           	nop
    396f: 48 83 bc 24 e8 00 00 00 00   	cmpq	$0x0, 0xe8(%rsp)
    3978: 74 0e                        	je	0x3988 <DirIter_x3a_x3aOpen_x3a_x3anext+0x518>
    397a: 48 8b 8c 24 e8 00 00 00      	movq	0xe8(%rsp), %rcx
    3982: e8 00 00 00 00               	callq	0x3987 <DirIter_x3a_x3aOpen_x3a_x3anext+0x517>
    3987: 90                           	nop
    3988: 48 8b 84 24 f0 00 00 00      	movq	0xf0(%rsp), %rax
    3990: 48 89 84 24 18 01 00 00      	movq	%rax, 0x118(%rsp)
    3998: 48 8b 84 24 f8 00 00 00      	movq	0xf8(%rsp), %rax
    39a0: 48 89 84 24 20 01 00 00      	movq	%rax, 0x120(%rsp)
    39a8: 48 8b 84 24 f8 00 00 00      	movq	0xf8(%rsp), %rax
    39b0: 48 89 84 24 28 01 00 00      	movq	%rax, 0x128(%rsp)
    39b8: 48 8b 84 24 00 01 00 00      	movq	0x100(%rsp), %rax
    39c0: 48 89 84 24 30 01 00 00      	movq	%rax, 0x130(%rsp)
    39c8: 48 8b 84 24 08 01 00 00      	movq	0x108(%rsp), %rax
    39d0: 48 89 84 24 38 01 00 00      	movq	%rax, 0x138(%rsp)
    39d8: 48 8b 84 24 08 01 00 00      	movq	0x108(%rsp), %rax
    39e0: 48 89 84 24 40 01 00 00      	movq	%rax, 0x140(%rsp)
    39e8: 8b 84 24 90 00 00 00         	movl	0x90(%rsp), %eax
    39ef: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    39f7: 48 8b 49 30                  	movq	0x30(%rcx), %rcx
    39fb: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    39ff: 88 84 24 48 01 00 00         	movb	%al, 0x148(%rsp)
    3a06: 48 8b 84 24 88 00 00 00      	movq	0x88(%rsp), %rax
    3a0e: 8b 40 3c                     	movl	0x3c(%rax), %eax
    3a11: ff c0                        	incl	%eax
    3a13: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    3a1b: 89 41 3c                     	movl	%eax, 0x3c(%rcx)
    3a1e: c6 44 24 40 01               	movb	$0x1, 0x40(%rsp)
    3a23: 48 8d 44 24 48               	leaq	0x48(%rsp), %rax
    3a28: 48 8d 8c 24 18 01 00 00      	leaq	0x118(%rsp), %rcx
    3a30: 48 8b f8                     	movq	%rax, %rdi
    3a33: 48 8b f1                     	movq	%rcx, %rsi
    3a36: b9 38 00 00 00               	movl	$0x38, %ecx
    3a3b: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    3a3d: 0f b6 54 24 40               	movzbl	0x40(%rsp), %edx
    3a42: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x3a49 <DirIter_x3a_x3aOpen_x3a_x3anext+0x5d9>
    3a49: e8 00 00 00 00               	callq	0x3a4e <DirIter_x3a_x3aOpen_x3a_x3anext+0x5de>
    3a4e: 48 8d 44 24 40               	leaq	0x40(%rsp), %rax
    3a53: 48 8b bc 24 80 01 00 00      	movq	0x180(%rsp), %rdi
    3a5b: 48 8b f0                     	movq	%rax, %rsi
    3a5e: b9 40 00 00 00               	movl	$0x40, %ecx
    3a63: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    3a65: 48 8b 84 24 80 01 00 00      	movq	0x180(%rsp), %rax
    3a6d: 48 8b f8                     	movq	%rax, %rdi
    3a70: 48 8b cc                     	movq	%rsp, %rcx
    3a73: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x3a7a <DirIter_x3a_x3aOpen_x3a_x3anext+0x60a>
    3a7a: e8 00 00 00 00               	callq	0x3a7f <DirIter_x3a_x3aOpen_x3a_x3anext+0x60f>
    3a7f: 48 8b c7                     	movq	%rdi, %rax
    3a82: 48 81 c4 68 01 00 00         	addq	$0x168, %rsp            # imm = 0x168
    3a89: 5f                           	popq	%rdi
    3a8a: 5e                           	popq	%rsi
    3a8b: c3                           	retq
    3a8c: cc                           	int3
    3a8d: cc                           	int3
    3a8e: cc                           	int3
    3a8f: cc                           	int3
    3a90: cc                           	int3
    3a91: cc                           	int3
    3a92: cc                           	int3
    3a93: cc                           	int3
    3a94: cc                           	int3
    3a95: cc                           	int3
    3a96: cc                           	int3
    3a97: cc                           	int3
    3a98: cc                           	int3
    3a99: cc                           	int3
    3a9a: cc                           	int3
    3a9b: cc                           	int3
    3a9c: cc                           	int3
    3a9d: cc                           	int3
    3a9e: cc                           	int3
    3a9f: cc                           	int3

0000000000003aa0 <DirIter_x3a_x3aOpen_x3a_x3aclose>:
    3aa0: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3aa5: 57                           	pushq	%rdi
    3aa6: 48 83 ec 30                  	subq	$0x30, %rsp
    3aaa: b2 43                        	movb	$0x43, %dl
    3aac: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x3ab3 <DirIter_x3a_x3aOpen_x3a_x3aclose+0x13>
    3ab3: e8 00 00 00 00               	callq	0x3ab8 <DirIter_x3a_x3aOpen_x3a_x3aclose+0x18>
    3ab8: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    3abd: e8 00 00 00 00               	callq	0x3ac2 <DirIter_x3a_x3aOpen_x3a_x3aclose+0x22>
    3ac2: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    3ac7: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    3acd: 75 05                        	jne	0x3ad4 <DirIter_x3a_x3aOpen_x3a_x3aclose+0x34>
    3acf: e9 cb 00 00 00               	jmp	0x3b9f <DirIter_x3a_x3aOpen_x3a_x3aclose+0xff>
    3ad4: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3ad9: 48 83 78 20 00               	cmpq	$0x0, 0x20(%rax)
    3ade: 74 4a                        	je	0x3b2a <DirIter_x3a_x3aOpen_x3a_x3aclose+0x8a>
    3ae0: c7 44 24 28 00 00 00 00      	movl	$0x0, 0x28(%rsp)
    3ae8: eb 0a                        	jmp	0x3af4 <DirIter_x3a_x3aOpen_x3a_x3aclose+0x54>
    3aea: 8b 44 24 28                  	movl	0x28(%rsp), %eax
    3aee: ff c0                        	incl	%eax
    3af0: 89 44 24 28                  	movl	%eax, 0x28(%rsp)
    3af4: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3af9: 8b 40 38                     	movl	0x38(%rax), %eax
    3afc: 39 44 24 28                  	cmpl	%eax, 0x28(%rsp)
    3b00: 73 19                        	jae	0x3b1b <DirIter_x3a_x3aOpen_x3a_x3aclose+0x7b>
    3b02: 8b 44 24 28                  	movl	0x28(%rsp), %eax
    3b06: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    3b0b: 48 8b 49 20                  	movq	0x20(%rcx), %rcx
    3b0f: 48 8b 0c c1                  	movq	(%rcx,%rax,8), %rcx
    3b13: e8 00 00 00 00               	callq	0x3b18 <DirIter_x3a_x3aOpen_x3a_x3aclose+0x78>
    3b18: 90                           	nop
    3b19: eb cf                        	jmp	0x3aea <DirIter_x3a_x3aOpen_x3a_x3aclose+0x4a>
    3b1b: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3b20: 48 8b 48 20                  	movq	0x20(%rax), %rcx
    3b24: e8 00 00 00 00               	callq	0x3b29 <DirIter_x3a_x3aOpen_x3a_x3aclose+0x89>
    3b29: 90                           	nop
    3b2a: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3b2f: 48 83 78 28 00               	cmpq	$0x0, 0x28(%rax)
    3b34: 74 0f                        	je	0x3b45 <DirIter_x3a_x3aOpen_x3a_x3aclose+0xa5>
    3b36: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3b3b: 48 8b 48 28                  	movq	0x28(%rax), %rcx
    3b3f: e8 00 00 00 00               	callq	0x3b44 <DirIter_x3a_x3aOpen_x3a_x3aclose+0xa4>
    3b44: 90                           	nop
    3b45: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3b4a: 48 83 78 30 00               	cmpq	$0x0, 0x30(%rax)
    3b4f: 74 0f                        	je	0x3b60 <DirIter_x3a_x3aOpen_x3a_x3aclose+0xc0>
    3b51: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3b56: 48 8b 48 30                  	movq	0x30(%rax), %rcx
    3b5a: e8 00 00 00 00               	callq	0x3b5f <DirIter_x3a_x3aOpen_x3a_x3aclose+0xbf>
    3b5f: 90                           	nop
    3b60: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3b65: 48 83 38 00                  	cmpq	$0x0, (%rax)
    3b69: 74 0e                        	je	0x3b79 <DirIter_x3a_x3aOpen_x3a_x3aclose+0xd9>
    3b6b: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3b70: 48 8b 08                     	movq	(%rax), %rcx
    3b73: e8 00 00 00 00               	callq	0x3b78 <DirIter_x3a_x3aOpen_x3a_x3aclose+0xd8>
    3b78: 90                           	nop
    3b79: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3b7e: 48 83 78 10 00               	cmpq	$0x0, 0x10(%rax)
    3b83: 74 0f                        	je	0x3b94 <DirIter_x3a_x3aOpen_x3a_x3aclose+0xf4>
    3b85: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3b8a: 48 8b 48 10                  	movq	0x10(%rax), %rcx
    3b8e: e8 00 00 00 00               	callq	0x3b93 <DirIter_x3a_x3aOpen_x3a_x3aclose+0xf3>
    3b93: 90                           	nop
    3b94: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    3b99: e8 00 00 00 00               	callq	0x3b9e <DirIter_x3a_x3aOpen_x3a_x3aclose+0xfe>
    3b9e: 90                           	nop
    3b9f: 48 83 c4 30                  	addq	$0x30, %rsp
    3ba3: 5f                           	popq	%rdi
    3ba4: c3                           	retq
    3ba5: cc                           	int3
    3ba6: cc                           	int3
    3ba7: cc                           	int3
    3ba8: cc                           	int3
    3ba9: cc                           	int3
    3baa: cc                           	int3
    3bab: cc                           	int3
    3bac: cc                           	int3
    3bad: cc                           	int3
    3bae: cc                           	int3
    3baf: cc                           	int3

0000000000003bb0 <c0_min_u64>:
    3bb0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3bb5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3bba: 57                           	pushq	%rdi
    3bbb: 48 83 ec 10                  	subq	$0x10, %rsp
    3bbf: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
    3bc4: 48 39 44 24 20               	cmpq	%rax, 0x20(%rsp)
    3bc9: 73 0b                        	jae	0x3bd6 <c0_min_u64+0x26>
    3bcb: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3bd0: 48 89 04 24                  	movq	%rax, (%rsp)
    3bd4: eb 09                        	jmp	0x3bdf <c0_min_u64+0x2f>
    3bd6: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
    3bdb: 48 89 04 24                  	movq	%rax, (%rsp)
    3bdf: 48 8b 04 24                  	movq	(%rsp), %rax
    3be3: 48 83 c4 10                  	addq	$0x10, %rsp
    3be7: 5f                           	popq	%rdi
    3be8: c3                           	retq
    3be9: cc                           	int3
    3bea: cc                           	int3
    3beb: cc                           	int3
    3bec: cc                           	int3
    3bed: cc                           	int3
    3bee: cc                           	int3
    3bef: cc                           	int3

0000000000003bf0 <c0_process_heap>:
    3bf0: 40 57                        	pushq	%rdi
    3bf2: 48 83 ec 20                  	subq	$0x20, %rsp
    3bf6: 48 83 3d 00 00 00 00 00      	cmpq	$0x0, (%rip)            # 0x3bfe <c0_process_heap+0xe>
    3bfe: 75 0d                        	jne	0x3c0d <c0_process_heap+0x1d>
    3c00: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x3c06 <c0_process_heap+0x16>
    3c06: 48 89 05 00 00 00 00         	movq	%rax, (%rip)            # 0x3c0d <c0_process_heap+0x1d>
    3c0d: 48 8b 05 00 00 00 00         	movq	(%rip), %rax            # 0x3c14 <c0_process_heap+0x24>
    3c14: 48 83 c4 20                  	addq	$0x20, %rsp
    3c18: 5f                           	popq	%rdi
    3c19: c3                           	retq
    3c1a: cc                           	int3
    3c1b: cc                           	int3
    3c1c: cc                           	int3
    3c1d: cc                           	int3
    3c1e: cc                           	int3
    3c1f: cc                           	int3

0000000000003c20 <c0_heap_alloc_raw>:
    3c20: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3c25: 57                           	pushq	%rdi
    3c26: 48 83 ec 20                  	subq	$0x20, %rsp
    3c2a: e8 00 00 00 00               	callq	0x3c2f <c0_heap_alloc_raw+0xf>
    3c2f: 4c 8b 44 24 30               	movq	0x30(%rsp), %r8
    3c34: 33 d2                        	xorl	%edx, %edx
    3c36: 48 8b c8                     	movq	%rax, %rcx
    3c39: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x3c3f <c0_heap_alloc_raw+0x1f>
    3c3f: 48 83 c4 20                  	addq	$0x20, %rsp
    3c43: 5f                           	popq	%rdi
    3c44: c3                           	retq
    3c45: cc                           	int3
    3c46: cc                           	int3
    3c47: cc                           	int3
    3c48: cc                           	int3
    3c49: cc                           	int3
    3c4a: cc                           	int3
    3c4b: cc                           	int3
    3c4c: cc                           	int3
    3c4d: cc                           	int3
    3c4e: cc                           	int3
    3c4f: cc                           	int3

0000000000003c50 <c0_heap_free_raw>:
    3c50: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3c55: 57                           	pushq	%rdi
    3c56: 48 83 ec 20                  	subq	$0x20, %rsp
    3c5a: 48 83 7c 24 30 00            	cmpq	$0x0, 0x30(%rsp)
    3c60: 74 16                        	je	0x3c78 <c0_heap_free_raw+0x28>
    3c62: e8 00 00 00 00               	callq	0x3c67 <c0_heap_free_raw+0x17>
    3c67: 4c 8b 44 24 30               	movq	0x30(%rsp), %r8
    3c6c: 33 d2                        	xorl	%edx, %edx
    3c6e: 48 8b c8                     	movq	%rax, %rcx
    3c71: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x3c77 <c0_heap_free_raw+0x27>
    3c77: 90                           	nop
    3c78: 48 83 c4 20                  	addq	$0x20, %rsp
    3c7c: 5f                           	popq	%rdi
    3c7d: c3                           	retq
    3c7e: cc                           	int3
    3c7f: cc                           	int3
    3c80: cc                           	int3
    3c81: cc                           	int3
    3c82: cc                           	int3
    3c83: cc                           	int3
    3c84: cc                           	int3
    3c85: cc                           	int3
    3c86: cc                           	int3
    3c87: cc                           	int3
    3c88: cc                           	int3
    3c89: cc                           	int3
    3c8a: cc                           	int3
    3c8b: cc                           	int3
    3c8c: cc                           	int3
    3c8d: cc                           	int3
    3c8e: cc                           	int3
    3c8f: cc                           	int3

0000000000003c90 <c0_memcpy>:
    3c90: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    3c95: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3c9a: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3c9f: 57                           	pushq	%rdi
    3ca0: 48 83 ec 20                  	subq	$0x20, %rsp
    3ca4: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
    3ca9: 48 89 04 24                  	movq	%rax, (%rsp)
    3cad: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    3cb2: 48 89 44 24 08               	movq	%rax, 0x8(%rsp)
    3cb7: 48 c7 44 24 10 00 00 00 00   	movq	$0x0, 0x10(%rsp)
    3cc0: eb 0d                        	jmp	0x3ccf <c0_memcpy+0x3f>
    3cc2: 48 8b 44 24 10               	movq	0x10(%rsp), %rax
    3cc7: 48 ff c0                     	incq	%rax
    3cca: 48 89 44 24 10               	movq	%rax, 0x10(%rsp)
    3ccf: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    3cd4: 48 39 44 24 10               	cmpq	%rax, 0x10(%rsp)
    3cd9: 73 26                        	jae	0x3d01 <c0_memcpy+0x71>
    3cdb: 48 8b 44 24 10               	movq	0x10(%rsp), %rax
    3ce0: 48 8b 0c 24                  	movq	(%rsp), %rcx
    3ce4: 48 03 c8                     	addq	%rax, %rcx
    3ce7: 48 8b c1                     	movq	%rcx, %rax
    3cea: 48 8b 4c 24 10               	movq	0x10(%rsp), %rcx
    3cef: 48 8b 54 24 08               	movq	0x8(%rsp), %rdx
    3cf4: 48 03 d1                     	addq	%rcx, %rdx
    3cf7: 48 8b ca                     	movq	%rdx, %rcx
    3cfa: 0f b6 09                     	movzbl	(%rcx), %ecx
    3cfd: 88 08                        	movb	%cl, (%rax)
    3cff: eb c1                        	jmp	0x3cc2 <c0_memcpy+0x32>
    3d01: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
    3d06: 48 83 c4 20                  	addq	$0x20, %rsp
    3d0a: 5f                           	popq	%rdi
    3d0b: c3                           	retq
    3d0c: cc                           	int3
    3d0d: cc                           	int3
    3d0e: cc                           	int3
    3d0f: cc                           	int3
    3d10: cc                           	int3
    3d11: cc                           	int3
    3d12: cc                           	int3
    3d13: cc                           	int3
    3d14: cc                           	int3
    3d15: cc                           	int3
    3d16: cc                           	int3
    3d17: cc                           	int3
    3d18: cc                           	int3
    3d19: cc                           	int3
    3d1a: cc                           	int3
    3d1b: cc                           	int3
    3d1c: cc                           	int3
    3d1d: cc                           	int3
    3d1e: cc                           	int3
    3d1f: cc                           	int3

0000000000003d20 <c0_wcslen>:
    3d20: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3d25: 57                           	pushq	%rdi
    3d26: 48 83 ec 10                  	subq	$0x10, %rsp
    3d2a: 48 c7 04 24 00 00 00 00      	movq	$0x0, (%rsp)
    3d32: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    3d38: 75 04                        	jne	0x3d3e <c0_wcslen+0x1e>
    3d3a: 33 c0                        	xorl	%eax, %eax
    3d3c: eb 22                        	jmp	0x3d60 <c0_wcslen+0x40>
    3d3e: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3d43: 48 8b 0c 24                  	movq	(%rsp), %rcx
    3d47: 0f b7 04 48                  	movzwl	(%rax,%rcx,2), %eax
    3d4b: 85 c0                        	testl	%eax, %eax
    3d4d: 74 0d                        	je	0x3d5c <c0_wcslen+0x3c>
    3d4f: 48 8b 04 24                  	movq	(%rsp), %rax
    3d53: 48 ff c0                     	incq	%rax
    3d56: 48 89 04 24                  	movq	%rax, (%rsp)
    3d5a: eb e2                        	jmp	0x3d3e <c0_wcslen+0x1e>
    3d5c: 48 8b 04 24                  	movq	(%rsp), %rax
    3d60: 48 83 c4 10                  	addq	$0x10, %rsp
    3d64: 5f                           	popq	%rdi
    3d65: c3                           	retq
    3d66: cc                           	int3
    3d67: cc                           	int3
    3d68: cc                           	int3
    3d69: cc                           	int3
    3d6a: cc                           	int3
    3d6b: cc                           	int3
    3d6c: cc                           	int3
    3d6d: cc                           	int3
    3d6e: cc                           	int3
    3d6f: cc                           	int3

0000000000003d70 <c0_utf8_has_null>:
    3d70: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3d75: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3d7a: 57                           	pushq	%rdi
    3d7b: 48 83 ec 10                  	subq	$0x10, %rsp
    3d7f: 48 c7 04 24 00 00 00 00      	movq	$0x0, (%rsp)
    3d87: eb 0b                        	jmp	0x3d94 <c0_utf8_has_null+0x24>
    3d89: 48 8b 04 24                  	movq	(%rsp), %rax
    3d8d: 48 ff c0                     	incq	%rax
    3d90: 48 89 04 24                  	movq	%rax, (%rsp)
    3d94: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
    3d99: 48 39 04 24                  	cmpq	%rax, (%rsp)
    3d9d: 73 1f                        	jae	0x3dbe <c0_utf8_has_null+0x4e>
    3d9f: 48 8b 04 24                  	movq	(%rsp), %rax
    3da3: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    3da8: 48 03 c8                     	addq	%rax, %rcx
    3dab: 48 8b c1                     	movq	%rcx, %rax
    3dae: 0f b6 00                     	movzbl	(%rax), %eax
    3db1: 85 c0                        	testl	%eax, %eax
    3db3: 75 07                        	jne	0x3dbc <c0_utf8_has_null+0x4c>
    3db5: b8 01 00 00 00               	movl	$0x1, %eax
    3dba: eb 04                        	jmp	0x3dc0 <c0_utf8_has_null+0x50>
    3dbc: eb cb                        	jmp	0x3d89 <c0_utf8_has_null+0x19>
    3dbe: 33 c0                        	xorl	%eax, %eax
    3dc0: 48 83 c4 10                  	addq	$0x10, %rsp
    3dc4: 5f                           	popq	%rdi
    3dc5: c3                           	retq
    3dc6: cc                           	int3
    3dc7: cc                           	int3
    3dc8: cc                           	int3
    3dc9: cc                           	int3
    3dca: cc                           	int3
    3dcb: cc                           	int3
    3dcc: cc                           	int3
    3dcd: cc                           	int3
    3dce: cc                           	int3
    3dcf: cc                           	int3

0000000000003dd0 <c0_heap_can_alloc>:
    3dd0: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    3dd5: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3dda: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3ddf: 57                           	pushq	%rdi
    3de0: 48 83 ec 10                  	subq	$0x10, %rsp
    3de4: 48 83 7c 24 30 00            	cmpq	$0x0, 0x30(%rsp)
    3dea: 74 0b                        	je	0x3df7 <c0_heap_can_alloc+0x27>
    3dec: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
    3df1: c7 00 00 00 00 00            	movl	$0x0, (%rax)
    3df7: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    3dfd: 74 08                        	je	0x3e07 <c0_heap_can_alloc+0x37>
    3dff: 48 83 7c 24 28 00            	cmpq	$0x0, 0x28(%rsp)
    3e05: 75 07                        	jne	0x3e0e <c0_heap_can_alloc+0x3e>
    3e07: b8 01 00 00 00               	movl	$0x1, %eax
    3e0c: eb 60                        	jmp	0x3e6e <c0_heap_can_alloc+0x9e>
    3e0e: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3e13: 48 89 04 24                  	movq	%rax, (%rsp)
    3e17: eb 0b                        	jmp	0x3e24 <c0_heap_can_alloc+0x54>
    3e19: 48 8b 04 24                  	movq	(%rsp), %rax
    3e1d: 48 8b 00                     	movq	(%rax), %rax
    3e20: 48 89 04 24                  	movq	%rax, (%rsp)
    3e24: 48 83 3c 24 00               	cmpq	$0x0, (%rsp)
    3e29: 74 3e                        	je	0x3e69 <c0_heap_can_alloc+0x99>
    3e2b: 48 8b 04 24                  	movq	(%rsp), %rax
    3e2f: 48 83 78 08 00               	cmpq	$0x0, 0x8(%rax)
    3e34: 74 31                        	je	0x3e67 <c0_heap_can_alloc+0x97>
    3e36: 48 8b 04 24                  	movq	(%rsp), %rax
    3e3a: 48 8b 0c 24                  	movq	(%rsp), %rcx
    3e3e: 48 8b 49 10                  	movq	0x10(%rcx), %rcx
    3e42: 48 8b 40 08                  	movq	0x8(%rax), %rax
    3e46: 48 2b c1                     	subq	%rcx, %rax
    3e49: 48 39 44 24 28               	cmpq	%rax, 0x28(%rsp)
    3e4e: 76 17                        	jbe	0x3e67 <c0_heap_can_alloc+0x97>
    3e50: 48 83 7c 24 30 00            	cmpq	$0x0, 0x30(%rsp)
    3e56: 74 0b                        	je	0x3e63 <c0_heap_can_alloc+0x93>
    3e58: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
    3e5d: c7 00 01 00 00 00            	movl	$0x1, (%rax)
    3e63: 33 c0                        	xorl	%eax, %eax
    3e65: eb 07                        	jmp	0x3e6e <c0_heap_can_alloc+0x9e>
    3e67: eb b0                        	jmp	0x3e19 <c0_heap_can_alloc+0x49>
    3e69: b8 01 00 00 00               	movl	$0x1, %eax
    3e6e: 48 83 c4 10                  	addq	$0x10, %rsp
    3e72: 5f                           	popq	%rdi
    3e73: c3                           	retq
    3e74: cc                           	int3
    3e75: cc                           	int3
    3e76: cc                           	int3
    3e77: cc                           	int3
    3e78: cc                           	int3
    3e79: cc                           	int3
    3e7a: cc                           	int3
    3e7b: cc                           	int3
    3e7c: cc                           	int3
    3e7d: cc                           	int3
    3e7e: cc                           	int3
    3e7f: cc                           	int3

0000000000003e80 <c0_heap_add_used>:
    3e80: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3e85: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3e8a: 57                           	pushq	%rdi
    3e8b: 48 83 ec 10                  	subq	$0x10, %rsp
    3e8f: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    3e95: 74 08                        	je	0x3e9f <c0_heap_add_used+0x1f>
    3e97: 48 83 7c 24 28 00            	cmpq	$0x0, 0x28(%rsp)
    3e9d: 75 02                        	jne	0x3ea1 <c0_heap_add_used+0x21>
    3e9f: eb 34                        	jmp	0x3ed5 <c0_heap_add_used+0x55>
    3ea1: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3ea6: 48 89 04 24                  	movq	%rax, (%rsp)
    3eaa: eb 0b                        	jmp	0x3eb7 <c0_heap_add_used+0x37>
    3eac: 48 8b 04 24                  	movq	(%rsp), %rax
    3eb0: 48 8b 00                     	movq	(%rax), %rax
    3eb3: 48 89 04 24                  	movq	%rax, (%rsp)
    3eb7: 48 83 3c 24 00               	cmpq	$0x0, (%rsp)
    3ebc: 74 17                        	je	0x3ed5 <c0_heap_add_used+0x55>
    3ebe: 48 8b 04 24                  	movq	(%rsp), %rax
    3ec2: 48 8b 40 10                  	movq	0x10(%rax), %rax
    3ec6: 48 03 44 24 28               	addq	0x28(%rsp), %rax
    3ecb: 48 8b 0c 24                  	movq	(%rsp), %rcx
    3ecf: 48 89 41 10                  	movq	%rax, 0x10(%rcx)
    3ed3: eb d7                        	jmp	0x3eac <c0_heap_add_used+0x2c>
    3ed5: 48 83 c4 10                  	addq	$0x10, %rsp
    3ed9: 5f                           	popq	%rdi
    3eda: c3                           	retq
    3edb: cc                           	int3
    3edc: cc                           	int3
    3edd: cc                           	int3
    3ede: cc                           	int3
    3edf: cc                           	int3
    3ee0: cc                           	int3
    3ee1: cc                           	int3
    3ee2: cc                           	int3
    3ee3: cc                           	int3
    3ee4: cc                           	int3
    3ee5: cc                           	int3
    3ee6: cc                           	int3
    3ee7: cc                           	int3
    3ee8: cc                           	int3
    3ee9: cc                           	int3
    3eea: cc                           	int3
    3eeb: cc                           	int3
    3eec: cc                           	int3
    3eed: cc                           	int3
    3eee: cc                           	int3
    3eef: cc                           	int3

0000000000003ef0 <c0_heap_sub_used>:
    3ef0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3ef5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3efa: 57                           	pushq	%rdi
    3efb: 48 83 ec 10                  	subq	$0x10, %rsp
    3eff: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    3f05: 74 08                        	je	0x3f0f <c0_heap_sub_used+0x1f>
    3f07: 48 83 7c 24 28 00            	cmpq	$0x0, 0x28(%rsp)
    3f0d: 75 02                        	jne	0x3f11 <c0_heap_sub_used+0x21>
    3f0f: eb 54                        	jmp	0x3f65 <c0_heap_sub_used+0x75>
    3f11: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3f16: 48 89 04 24                  	movq	%rax, (%rsp)
    3f1a: eb 0b                        	jmp	0x3f27 <c0_heap_sub_used+0x37>
    3f1c: 48 8b 04 24                  	movq	(%rsp), %rax
    3f20: 48 8b 00                     	movq	(%rax), %rax
    3f23: 48 89 04 24                  	movq	%rax, (%rsp)
    3f27: 48 83 3c 24 00               	cmpq	$0x0, (%rsp)
    3f2c: 74 37                        	je	0x3f65 <c0_heap_sub_used+0x75>
    3f2e: 48 8b 04 24                  	movq	(%rsp), %rax
    3f32: 48 8b 4c 24 28               	movq	0x28(%rsp), %rcx
    3f37: 48 39 48 10                  	cmpq	%rcx, 0x10(%rax)
    3f3b: 72 1a                        	jb	0x3f57 <c0_heap_sub_used+0x67>
    3f3d: 48 8b 04 24                  	movq	(%rsp), %rax
    3f41: 48 8b 4c 24 28               	movq	0x28(%rsp), %rcx
    3f46: 48 8b 40 10                  	movq	0x10(%rax), %rax
    3f4a: 48 2b c1                     	subq	%rcx, %rax
    3f4d: 48 8b 0c 24                  	movq	(%rsp), %rcx
    3f51: 48 89 41 10                  	movq	%rax, 0x10(%rcx)
    3f55: eb 0c                        	jmp	0x3f63 <c0_heap_sub_used+0x73>
    3f57: 48 8b 04 24                  	movq	(%rsp), %rax
    3f5b: 48 c7 40 10 00 00 00 00      	movq	$0x0, 0x10(%rax)
    3f63: eb b7                        	jmp	0x3f1c <c0_heap_sub_used+0x2c>
    3f65: 48 83 c4 10                  	addq	$0x10, %rsp
    3f69: 5f                           	popq	%rdi
    3f6a: c3                           	retq
    3f6b: cc                           	int3
    3f6c: cc                           	int3
    3f6d: cc                           	int3
    3f6e: cc                           	int3
    3f6f: cc                           	int3
    3f70: cc                           	int3
    3f71: cc                           	int3
    3f72: cc                           	int3
    3f73: cc                           	int3
    3f74: cc                           	int3
    3f75: cc                           	int3
    3f76: cc                           	int3
    3f77: cc                           	int3
    3f78: cc                           	int3
    3f79: cc                           	int3
    3f7a: cc                           	int3
    3f7b: cc                           	int3
    3f7c: cc                           	int3
    3f7d: cc                           	int3
    3f7e: cc                           	int3
    3f7f: cc                           	int3

0000000000003f80 <c0_header_from_data>:
    3f80: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3f85: 57                           	pushq	%rdi
    3f86: 48 83 ec 10                  	subq	$0x10, %rsp
    3f8a: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    3f90: 74 0f                        	je	0x3fa1 <c0_header_from_data+0x21>
    3f92: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    3f97: 48 83 e8 10                  	subq	$0x10, %rax
    3f9b: 48 89 04 24                  	movq	%rax, (%rsp)
    3f9f: eb 08                        	jmp	0x3fa9 <c0_header_from_data+0x29>
    3fa1: 48 c7 04 24 00 00 00 00      	movq	$0x0, (%rsp)
    3fa9: 48 8b 04 24                  	movq	(%rsp), %rax
    3fad: 48 83 c4 10                  	addq	$0x10, %rsp
    3fb1: 5f                           	popq	%rdi
    3fb2: c3                           	retq
    3fb3: cc                           	int3
    3fb4: cc                           	int3
    3fb5: cc                           	int3
    3fb6: cc                           	int3
    3fb7: cc                           	int3
    3fb8: cc                           	int3
    3fb9: cc                           	int3
    3fba: cc                           	int3
    3fbb: cc                           	int3
    3fbc: cc                           	int3
    3fbd: cc                           	int3
    3fbe: cc                           	int3
    3fbf: cc                           	int3

0000000000003fc0 <c0_alloc_managed_bytes>:
    3fc0: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    3fc5: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    3fca: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    3fcf: 57                           	pushq	%rdi
    3fd0: 48 83 ec 30                  	subq	$0x30, %rsp
    3fd4: 48 83 7c 24 48 00            	cmpq	$0x0, 0x48(%rsp)
    3fda: 75 07                        	jne	0x3fe3 <c0_alloc_managed_bytes+0x23>
    3fdc: 33 c0                        	xorl	%eax, %eax
    3fde: e9 90 00 00 00               	jmp	0x4073 <c0_alloc_managed_bytes+0xb3>
    3fe3: 4c 8b 44 24 50               	movq	0x50(%rsp), %r8
    3fe8: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
    3fed: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    3ff2: e8 00 00 00 00               	callq	0x3ff7 <c0_alloc_managed_bytes+0x37>
    3ff7: 85 c0                        	testl	%eax, %eax
    3ff9: 75 04                        	jne	0x3fff <c0_alloc_managed_bytes+0x3f>
    3ffb: 33 c0                        	xorl	%eax, %eax
    3ffd: eb 74                        	jmp	0x4073 <c0_alloc_managed_bytes+0xb3>
    3fff: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    4004: 48 83 c0 10                  	addq	$0x10, %rax
    4008: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    400d: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    4012: 48 39 44 24 20               	cmpq	%rax, 0x20(%rsp)
    4017: 72 08                        	jb	0x4021 <c0_alloc_managed_bytes+0x61>
    4019: 48 83 7c 24 20 ff            	cmpq	$-0x1, 0x20(%rsp)
    401f: 76 04                        	jbe	0x4025 <c0_alloc_managed_bytes+0x65>
    4021: 33 c0                        	xorl	%eax, %eax
    4023: eb 4e                        	jmp	0x4073 <c0_alloc_managed_bytes+0xb3>
    4025: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    402a: e8 00 00 00 00               	callq	0x402f <c0_alloc_managed_bytes+0x6f>
    402f: 48 89 44 24 28               	movq	%rax, 0x28(%rsp)
    4034: 48 83 7c 24 28 00            	cmpq	$0x0, 0x28(%rsp)
    403a: 75 04                        	jne	0x4040 <c0_alloc_managed_bytes+0x80>
    403c: 33 c0                        	xorl	%eax, %eax
    403e: eb 33                        	jmp	0x4073 <c0_alloc_managed_bytes+0xb3>
    4040: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
    4045: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    404a: 48 89 08                     	movq	%rcx, (%rax)
    404d: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
    4052: 48 8b 4c 24 48               	movq	0x48(%rsp), %rcx
    4057: 48 89 48 08                  	movq	%rcx, 0x8(%rax)
    405b: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
    4060: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4065: e8 00 00 00 00               	callq	0x406a <c0_alloc_managed_bytes+0xaa>
    406a: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
    406f: 48 83 c0 10                  	addq	$0x10, %rax
    4073: 48 83 c4 30                  	addq	$0x30, %rsp
    4077: 5f                           	popq	%rdi
    4078: c3                           	retq
    4079: cc                           	int3
    407a: cc                           	int3
    407b: cc                           	int3
    407c: cc                           	int3
    407d: cc                           	int3
    407e: cc                           	int3
    407f: cc                           	int3

0000000000004080 <c0_free_managed_bytes>:
    4080: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    4085: 57                           	pushq	%rdi
    4086: 48 83 ec 30                  	subq	$0x30, %rsp
    408a: 48 83 7c 24 40 00            	cmpq	$0x0, 0x40(%rsp)
    4090: 75 02                        	jne	0x4094 <c0_free_managed_bytes+0x14>
    4092: eb 44                        	jmp	0x40d8 <c0_free_managed_bytes+0x58>
    4094: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4099: e8 00 00 00 00               	callq	0x409e <c0_free_managed_bytes+0x1e>
    409e: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    40a3: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    40a9: 74 22                        	je	0x40cd <c0_free_managed_bytes+0x4d>
    40ab: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    40b0: 48 83 38 00                  	cmpq	$0x0, (%rax)
    40b4: 74 17                        	je	0x40cd <c0_free_managed_bytes+0x4d>
    40b6: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    40bb: 48 8b 50 08                  	movq	0x8(%rax), %rdx
    40bf: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    40c4: 48 8b 08                     	movq	(%rax), %rcx
    40c7: e8 00 00 00 00               	callq	0x40cc <c0_free_managed_bytes+0x4c>
    40cc: 90                           	nop
    40cd: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    40d2: e8 00 00 00 00               	callq	0x40d7 <c0_free_managed_bytes+0x57>
    40d7: 90                           	nop
    40d8: 48 83 c4 30                  	addq	$0x30, %rsp
    40dc: 5f                           	popq	%rdi
    40dd: c3                           	retq
    40de: cc                           	int3
    40df: cc                           	int3
    40e0: cc                           	int3
    40e1: cc                           	int3
    40e2: cc                           	int3
    40e3: cc                           	int3
    40e4: cc                           	int3
    40e5: cc                           	int3
    40e6: cc                           	int3
    40e7: cc                           	int3
    40e8: cc                           	int3
    40e9: cc                           	int3
    40ea: cc                           	int3
    40eb: cc                           	int3
    40ec: cc                           	int3
    40ed: cc                           	int3
    40ee: cc                           	int3
    40ef: cc                           	int3

00000000000040f0 <c0_utf8_valid>:
    40f0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    40f5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    40fa: 57                           	pushq	%rdi
    40fb: 48 83 ec 10                  	subq	$0x10, %rsp
    40ff: 48 c7 04 24 00 00 00 00      	movq	$0x0, (%rsp)
    4107: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
    410c: 48 39 04 24                  	cmpq	%rax, (%rsp)
    4110: 0f 83 0e 05 00 00            	jae	0x4624 <c0_utf8_valid+0x534>
    4116: 48 8b 04 24                  	movq	(%rsp), %rax
    411a: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    411f: 48 03 c8                     	addq	%rax, %rcx
    4122: 48 8b c1                     	movq	%rcx, %rax
    4125: 0f b6 00                     	movzbl	(%rax), %eax
    4128: 88 44 24 08                  	movb	%al, 0x8(%rsp)
    412c: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    4131: 3d 80 00 00 00               	cmpl	$0x80, %eax
    4136: 7d 0d                        	jge	0x4145 <c0_utf8_valid+0x55>
    4138: 48 8b 04 24                  	movq	(%rsp), %rax
    413c: 48 ff c0                     	incq	%rax
    413f: 48 89 04 24                  	movq	%rax, (%rsp)
    4143: eb c2                        	jmp	0x4107 <c0_utf8_valid+0x17>
    4145: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    414a: 3d c2 00 00 00               	cmpl	$0xc2, %eax
    414f: 7c 58                        	jl	0x41a9 <c0_utf8_valid+0xb9>
    4151: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    4156: 3d df 00 00 00               	cmpl	$0xdf, %eax
    415b: 7f 4c                        	jg	0x41a9 <c0_utf8_valid+0xb9>
    415d: 48 8b 04 24                  	movq	(%rsp), %rax
    4161: 48 ff c0                     	incq	%rax
    4164: 48 3b 44 24 28               	cmpq	0x28(%rsp), %rax
    4169: 72 07                        	jb	0x4172 <c0_utf8_valid+0x82>
    416b: 33 c0                        	xorl	%eax, %eax
    416d: e9 b7 04 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4172: 48 8b 04 24                  	movq	(%rsp), %rax
    4176: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    417b: 48 03 c8                     	addq	%rax, %rcx
    417e: 48 8b c1                     	movq	%rcx, %rax
    4181: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    4185: 25 c0 00 00 00               	andl	$0xc0, %eax
    418a: 3d 80 00 00 00               	cmpl	$0x80, %eax
    418f: 74 07                        	je	0x4198 <c0_utf8_valid+0xa8>
    4191: 33 c0                        	xorl	%eax, %eax
    4193: e9 91 04 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4198: 48 8b 04 24                  	movq	(%rsp), %rax
    419c: 48 83 c0 02                  	addq	$0x2, %rax
    41a0: 48 89 04 24                  	movq	%rax, (%rsp)
    41a4: e9 5e ff ff ff               	jmp	0x4107 <c0_utf8_valid+0x17>
    41a9: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    41ae: 3d e0 00 00 00               	cmpl	$0xe0, %eax
    41b3: 0f 85 88 00 00 00            	jne	0x4241 <c0_utf8_valid+0x151>
    41b9: 48 8b 04 24                  	movq	(%rsp), %rax
    41bd: 48 83 c0 02                  	addq	$0x2, %rax
    41c1: 48 3b 44 24 28               	cmpq	0x28(%rsp), %rax
    41c6: 72 07                        	jb	0x41cf <c0_utf8_valid+0xdf>
    41c8: 33 c0                        	xorl	%eax, %eax
    41ca: e9 5a 04 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    41cf: 48 8b 04 24                  	movq	(%rsp), %rax
    41d3: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    41d8: 48 03 c8                     	addq	%rax, %rcx
    41db: 48 8b c1                     	movq	%rcx, %rax
    41de: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    41e2: 3d a0 00 00 00               	cmpl	$0xa0, %eax
    41e7: 7c 1a                        	jl	0x4203 <c0_utf8_valid+0x113>
    41e9: 48 8b 04 24                  	movq	(%rsp), %rax
    41ed: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    41f2: 48 03 c8                     	addq	%rax, %rcx
    41f5: 48 8b c1                     	movq	%rcx, %rax
    41f8: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    41fc: 3d bf 00 00 00               	cmpl	$0xbf, %eax
    4201: 7e 07                        	jle	0x420a <c0_utf8_valid+0x11a>
    4203: 33 c0                        	xorl	%eax, %eax
    4205: e9 1f 04 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    420a: 48 8b 04 24                  	movq	(%rsp), %rax
    420e: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4213: 48 03 c8                     	addq	%rax, %rcx
    4216: 48 8b c1                     	movq	%rcx, %rax
    4219: 0f b6 40 02                  	movzbl	0x2(%rax), %eax
    421d: 25 c0 00 00 00               	andl	$0xc0, %eax
    4222: 3d 80 00 00 00               	cmpl	$0x80, %eax
    4227: 74 07                        	je	0x4230 <c0_utf8_valid+0x140>
    4229: 33 c0                        	xorl	%eax, %eax
    422b: e9 f9 03 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4230: 48 8b 04 24                  	movq	(%rsp), %rax
    4234: 48 83 c0 03                  	addq	$0x3, %rax
    4238: 48 89 04 24                  	movq	%rax, (%rsp)
    423c: e9 c6 fe ff ff               	jmp	0x4107 <c0_utf8_valid+0x17>
    4241: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    4246: 3d e1 00 00 00               	cmpl	$0xe1, %eax
    424b: 7c 7f                        	jl	0x42cc <c0_utf8_valid+0x1dc>
    424d: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    4252: 3d ec 00 00 00               	cmpl	$0xec, %eax
    4257: 7f 73                        	jg	0x42cc <c0_utf8_valid+0x1dc>
    4259: 48 8b 04 24                  	movq	(%rsp), %rax
    425d: 48 83 c0 02                  	addq	$0x2, %rax
    4261: 48 3b 44 24 28               	cmpq	0x28(%rsp), %rax
    4266: 72 07                        	jb	0x426f <c0_utf8_valid+0x17f>
    4268: 33 c0                        	xorl	%eax, %eax
    426a: e9 ba 03 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    426f: 48 8b 04 24                  	movq	(%rsp), %rax
    4273: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4278: 48 03 c8                     	addq	%rax, %rcx
    427b: 48 8b c1                     	movq	%rcx, %rax
    427e: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    4282: 25 c0 00 00 00               	andl	$0xc0, %eax
    4287: 3d 80 00 00 00               	cmpl	$0x80, %eax
    428c: 74 07                        	je	0x4295 <c0_utf8_valid+0x1a5>
    428e: 33 c0                        	xorl	%eax, %eax
    4290: e9 94 03 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4295: 48 8b 04 24                  	movq	(%rsp), %rax
    4299: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    429e: 48 03 c8                     	addq	%rax, %rcx
    42a1: 48 8b c1                     	movq	%rcx, %rax
    42a4: 0f b6 40 02                  	movzbl	0x2(%rax), %eax
    42a8: 25 c0 00 00 00               	andl	$0xc0, %eax
    42ad: 3d 80 00 00 00               	cmpl	$0x80, %eax
    42b2: 74 07                        	je	0x42bb <c0_utf8_valid+0x1cb>
    42b4: 33 c0                        	xorl	%eax, %eax
    42b6: e9 6e 03 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    42bb: 48 8b 04 24                  	movq	(%rsp), %rax
    42bf: 48 83 c0 03                  	addq	$0x3, %rax
    42c3: 48 89 04 24                  	movq	%rax, (%rsp)
    42c7: e9 3b fe ff ff               	jmp	0x4107 <c0_utf8_valid+0x17>
    42cc: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    42d1: 3d ed 00 00 00               	cmpl	$0xed, %eax
    42d6: 0f 85 88 00 00 00            	jne	0x4364 <c0_utf8_valid+0x274>
    42dc: 48 8b 04 24                  	movq	(%rsp), %rax
    42e0: 48 83 c0 02                  	addq	$0x2, %rax
    42e4: 48 3b 44 24 28               	cmpq	0x28(%rsp), %rax
    42e9: 72 07                        	jb	0x42f2 <c0_utf8_valid+0x202>
    42eb: 33 c0                        	xorl	%eax, %eax
    42ed: e9 37 03 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    42f2: 48 8b 04 24                  	movq	(%rsp), %rax
    42f6: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    42fb: 48 03 c8                     	addq	%rax, %rcx
    42fe: 48 8b c1                     	movq	%rcx, %rax
    4301: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    4305: 3d 80 00 00 00               	cmpl	$0x80, %eax
    430a: 7c 1a                        	jl	0x4326 <c0_utf8_valid+0x236>
    430c: 48 8b 04 24                  	movq	(%rsp), %rax
    4310: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4315: 48 03 c8                     	addq	%rax, %rcx
    4318: 48 8b c1                     	movq	%rcx, %rax
    431b: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    431f: 3d 9f 00 00 00               	cmpl	$0x9f, %eax
    4324: 7e 07                        	jle	0x432d <c0_utf8_valid+0x23d>
    4326: 33 c0                        	xorl	%eax, %eax
    4328: e9 fc 02 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    432d: 48 8b 04 24                  	movq	(%rsp), %rax
    4331: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4336: 48 03 c8                     	addq	%rax, %rcx
    4339: 48 8b c1                     	movq	%rcx, %rax
    433c: 0f b6 40 02                  	movzbl	0x2(%rax), %eax
    4340: 25 c0 00 00 00               	andl	$0xc0, %eax
    4345: 3d 80 00 00 00               	cmpl	$0x80, %eax
    434a: 74 07                        	je	0x4353 <c0_utf8_valid+0x263>
    434c: 33 c0                        	xorl	%eax, %eax
    434e: e9 d6 02 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4353: 48 8b 04 24                  	movq	(%rsp), %rax
    4357: 48 83 c0 03                  	addq	$0x3, %rax
    435b: 48 89 04 24                  	movq	%rax, (%rsp)
    435f: e9 a3 fd ff ff               	jmp	0x4107 <c0_utf8_valid+0x17>
    4364: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    4369: 3d ee 00 00 00               	cmpl	$0xee, %eax
    436e: 7c 7f                        	jl	0x43ef <c0_utf8_valid+0x2ff>
    4370: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    4375: 3d ef 00 00 00               	cmpl	$0xef, %eax
    437a: 7f 73                        	jg	0x43ef <c0_utf8_valid+0x2ff>
    437c: 48 8b 04 24                  	movq	(%rsp), %rax
    4380: 48 83 c0 02                  	addq	$0x2, %rax
    4384: 48 3b 44 24 28               	cmpq	0x28(%rsp), %rax
    4389: 72 07                        	jb	0x4392 <c0_utf8_valid+0x2a2>
    438b: 33 c0                        	xorl	%eax, %eax
    438d: e9 97 02 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4392: 48 8b 04 24                  	movq	(%rsp), %rax
    4396: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    439b: 48 03 c8                     	addq	%rax, %rcx
    439e: 48 8b c1                     	movq	%rcx, %rax
    43a1: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    43a5: 25 c0 00 00 00               	andl	$0xc0, %eax
    43aa: 3d 80 00 00 00               	cmpl	$0x80, %eax
    43af: 74 07                        	je	0x43b8 <c0_utf8_valid+0x2c8>
    43b1: 33 c0                        	xorl	%eax, %eax
    43b3: e9 71 02 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    43b8: 48 8b 04 24                  	movq	(%rsp), %rax
    43bc: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    43c1: 48 03 c8                     	addq	%rax, %rcx
    43c4: 48 8b c1                     	movq	%rcx, %rax
    43c7: 0f b6 40 02                  	movzbl	0x2(%rax), %eax
    43cb: 25 c0 00 00 00               	andl	$0xc0, %eax
    43d0: 3d 80 00 00 00               	cmpl	$0x80, %eax
    43d5: 74 07                        	je	0x43de <c0_utf8_valid+0x2ee>
    43d7: 33 c0                        	xorl	%eax, %eax
    43d9: e9 4b 02 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    43de: 48 8b 04 24                  	movq	(%rsp), %rax
    43e2: 48 83 c0 03                  	addq	$0x3, %rax
    43e6: 48 89 04 24                  	movq	%rax, (%rsp)
    43ea: e9 18 fd ff ff               	jmp	0x4107 <c0_utf8_valid+0x17>
    43ef: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    43f4: 3d f0 00 00 00               	cmpl	$0xf0, %eax
    43f9: 0f 85 ae 00 00 00            	jne	0x44ad <c0_utf8_valid+0x3bd>
    43ff: 48 8b 04 24                  	movq	(%rsp), %rax
    4403: 48 83 c0 03                  	addq	$0x3, %rax
    4407: 48 3b 44 24 28               	cmpq	0x28(%rsp), %rax
    440c: 72 07                        	jb	0x4415 <c0_utf8_valid+0x325>
    440e: 33 c0                        	xorl	%eax, %eax
    4410: e9 14 02 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4415: 48 8b 04 24                  	movq	(%rsp), %rax
    4419: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    441e: 48 03 c8                     	addq	%rax, %rcx
    4421: 48 8b c1                     	movq	%rcx, %rax
    4424: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    4428: 3d 90 00 00 00               	cmpl	$0x90, %eax
    442d: 7c 1a                        	jl	0x4449 <c0_utf8_valid+0x359>
    442f: 48 8b 04 24                  	movq	(%rsp), %rax
    4433: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4438: 48 03 c8                     	addq	%rax, %rcx
    443b: 48 8b c1                     	movq	%rcx, %rax
    443e: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    4442: 3d bf 00 00 00               	cmpl	$0xbf, %eax
    4447: 7e 07                        	jle	0x4450 <c0_utf8_valid+0x360>
    4449: 33 c0                        	xorl	%eax, %eax
    444b: e9 d9 01 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4450: 48 8b 04 24                  	movq	(%rsp), %rax
    4454: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4459: 48 03 c8                     	addq	%rax, %rcx
    445c: 48 8b c1                     	movq	%rcx, %rax
    445f: 0f b6 40 02                  	movzbl	0x2(%rax), %eax
    4463: 25 c0 00 00 00               	andl	$0xc0, %eax
    4468: 3d 80 00 00 00               	cmpl	$0x80, %eax
    446d: 74 07                        	je	0x4476 <c0_utf8_valid+0x386>
    446f: 33 c0                        	xorl	%eax, %eax
    4471: e9 b3 01 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4476: 48 8b 04 24                  	movq	(%rsp), %rax
    447a: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    447f: 48 03 c8                     	addq	%rax, %rcx
    4482: 48 8b c1                     	movq	%rcx, %rax
    4485: 0f b6 40 03                  	movzbl	0x3(%rax), %eax
    4489: 25 c0 00 00 00               	andl	$0xc0, %eax
    448e: 3d 80 00 00 00               	cmpl	$0x80, %eax
    4493: 74 07                        	je	0x449c <c0_utf8_valid+0x3ac>
    4495: 33 c0                        	xorl	%eax, %eax
    4497: e9 8d 01 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    449c: 48 8b 04 24                  	movq	(%rsp), %rax
    44a0: 48 83 c0 04                  	addq	$0x4, %rax
    44a4: 48 89 04 24                  	movq	%rax, (%rsp)
    44a8: e9 5a fc ff ff               	jmp	0x4107 <c0_utf8_valid+0x17>
    44ad: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    44b2: 3d f1 00 00 00               	cmpl	$0xf1, %eax
    44b7: 0f 8c a9 00 00 00            	jl	0x4566 <c0_utf8_valid+0x476>
    44bd: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    44c2: 3d f3 00 00 00               	cmpl	$0xf3, %eax
    44c7: 0f 8f 99 00 00 00            	jg	0x4566 <c0_utf8_valid+0x476>
    44cd: 48 8b 04 24                  	movq	(%rsp), %rax
    44d1: 48 83 c0 03                  	addq	$0x3, %rax
    44d5: 48 3b 44 24 28               	cmpq	0x28(%rsp), %rax
    44da: 72 07                        	jb	0x44e3 <c0_utf8_valid+0x3f3>
    44dc: 33 c0                        	xorl	%eax, %eax
    44de: e9 46 01 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    44e3: 48 8b 04 24                  	movq	(%rsp), %rax
    44e7: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    44ec: 48 03 c8                     	addq	%rax, %rcx
    44ef: 48 8b c1                     	movq	%rcx, %rax
    44f2: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    44f6: 25 c0 00 00 00               	andl	$0xc0, %eax
    44fb: 3d 80 00 00 00               	cmpl	$0x80, %eax
    4500: 74 07                        	je	0x4509 <c0_utf8_valid+0x419>
    4502: 33 c0                        	xorl	%eax, %eax
    4504: e9 20 01 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4509: 48 8b 04 24                  	movq	(%rsp), %rax
    450d: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4512: 48 03 c8                     	addq	%rax, %rcx
    4515: 48 8b c1                     	movq	%rcx, %rax
    4518: 0f b6 40 02                  	movzbl	0x2(%rax), %eax
    451c: 25 c0 00 00 00               	andl	$0xc0, %eax
    4521: 3d 80 00 00 00               	cmpl	$0x80, %eax
    4526: 74 07                        	je	0x452f <c0_utf8_valid+0x43f>
    4528: 33 c0                        	xorl	%eax, %eax
    452a: e9 fa 00 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    452f: 48 8b 04 24                  	movq	(%rsp), %rax
    4533: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4538: 48 03 c8                     	addq	%rax, %rcx
    453b: 48 8b c1                     	movq	%rcx, %rax
    453e: 0f b6 40 03                  	movzbl	0x3(%rax), %eax
    4542: 25 c0 00 00 00               	andl	$0xc0, %eax
    4547: 3d 80 00 00 00               	cmpl	$0x80, %eax
    454c: 74 07                        	je	0x4555 <c0_utf8_valid+0x465>
    454e: 33 c0                        	xorl	%eax, %eax
    4550: e9 d4 00 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    4555: 48 8b 04 24                  	movq	(%rsp), %rax
    4559: 48 83 c0 04                  	addq	$0x4, %rax
    455d: 48 89 04 24                  	movq	%rax, (%rsp)
    4561: e9 a1 fb ff ff               	jmp	0x4107 <c0_utf8_valid+0x17>
    4566: 0f b6 44 24 08               	movzbl	0x8(%rsp), %eax
    456b: 3d f4 00 00 00               	cmpl	$0xf4, %eax
    4570: 0f 85 a5 00 00 00            	jne	0x461b <c0_utf8_valid+0x52b>
    4576: 48 8b 04 24                  	movq	(%rsp), %rax
    457a: 48 83 c0 03                  	addq	$0x3, %rax
    457e: 48 3b 44 24 28               	cmpq	0x28(%rsp), %rax
    4583: 72 07                        	jb	0x458c <c0_utf8_valid+0x49c>
    4585: 33 c0                        	xorl	%eax, %eax
    4587: e9 9d 00 00 00               	jmp	0x4629 <c0_utf8_valid+0x539>
    458c: 48 8b 04 24                  	movq	(%rsp), %rax
    4590: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4595: 48 03 c8                     	addq	%rax, %rcx
    4598: 48 8b c1                     	movq	%rcx, %rax
    459b: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    459f: 3d 80 00 00 00               	cmpl	$0x80, %eax
    45a4: 7c 1a                        	jl	0x45c0 <c0_utf8_valid+0x4d0>
    45a6: 48 8b 04 24                  	movq	(%rsp), %rax
    45aa: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    45af: 48 03 c8                     	addq	%rax, %rcx
    45b2: 48 8b c1                     	movq	%rcx, %rax
    45b5: 0f b6 40 01                  	movzbl	0x1(%rax), %eax
    45b9: 3d 8f 00 00 00               	cmpl	$0x8f, %eax
    45be: 7e 04                        	jle	0x45c4 <c0_utf8_valid+0x4d4>
    45c0: 33 c0                        	xorl	%eax, %eax
    45c2: eb 65                        	jmp	0x4629 <c0_utf8_valid+0x539>
    45c4: 48 8b 04 24                  	movq	(%rsp), %rax
    45c8: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    45cd: 48 03 c8                     	addq	%rax, %rcx
    45d0: 48 8b c1                     	movq	%rcx, %rax
    45d3: 0f b6 40 02                  	movzbl	0x2(%rax), %eax
    45d7: 25 c0 00 00 00               	andl	$0xc0, %eax
    45dc: 3d 80 00 00 00               	cmpl	$0x80, %eax
    45e1: 74 04                        	je	0x45e7 <c0_utf8_valid+0x4f7>
    45e3: 33 c0                        	xorl	%eax, %eax
    45e5: eb 42                        	jmp	0x4629 <c0_utf8_valid+0x539>
    45e7: 48 8b 04 24                  	movq	(%rsp), %rax
    45eb: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    45f0: 48 03 c8                     	addq	%rax, %rcx
    45f3: 48 8b c1                     	movq	%rcx, %rax
    45f6: 0f b6 40 03                  	movzbl	0x3(%rax), %eax
    45fa: 25 c0 00 00 00               	andl	$0xc0, %eax
    45ff: 3d 80 00 00 00               	cmpl	$0x80, %eax
    4604: 74 04                        	je	0x460a <c0_utf8_valid+0x51a>
    4606: 33 c0                        	xorl	%eax, %eax
    4608: eb 1f                        	jmp	0x4629 <c0_utf8_valid+0x539>
    460a: 48 8b 04 24                  	movq	(%rsp), %rax
    460e: 48 83 c0 04                  	addq	$0x4, %rax
    4612: 48 89 04 24                  	movq	%rax, (%rsp)
    4616: e9 ec fa ff ff               	jmp	0x4107 <c0_utf8_valid+0x17>
    461b: 33 c0                        	xorl	%eax, %eax
    461d: eb 0a                        	jmp	0x4629 <c0_utf8_valid+0x539>
    461f: e9 e3 fa ff ff               	jmp	0x4107 <c0_utf8_valid+0x17>
    4624: b8 01 00 00 00               	movl	$0x1, %eax
    4629: 48 83 c4 10                  	addq	$0x10, %rsp
    462d: 5f                           	popq	%rdi
    462e: c3                           	retq
    462f: cc                           	int3
    4630: cc                           	int3
    4631: cc                           	int3
    4632: cc                           	int3
    4633: cc                           	int3
    4634: cc                           	int3
    4635: cc                           	int3
    4636: cc                           	int3
    4637: cc                           	int3
    4638: cc                           	int3
    4639: cc                           	int3
    463a: cc                           	int3
    463b: cc                           	int3
    463c: cc                           	int3
    463d: cc                           	int3
    463e: cc                           	int3
    463f: cc                           	int3

0000000000004640 <c0_utf8_to_wide>:
    4640: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    4645: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    464a: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    464f: 57                           	pushq	%rdi
    4650: 48 83 ec 50                  	subq	$0x50, %rsp
    4654: 48 83 7c 24 70 00            	cmpq	$0x0, 0x70(%rsp)
    465a: 74 0b                        	je	0x4667 <c0_utf8_to_wide+0x27>
    465c: 48 8b 44 24 70               	movq	0x70(%rsp), %rax
    4661: c7 00 00 00 00 00            	movl	$0x0, (%rax)
    4667: 48 83 7c 24 60 00            	cmpq	$0x0, 0x60(%rsp)
    466d: 75 0f                        	jne	0x467e <c0_utf8_to_wide+0x3e>
    466f: 48 83 7c 24 68 00            	cmpq	$0x0, 0x68(%rsp)
    4675: 74 07                        	je	0x467e <c0_utf8_to_wide+0x3e>
    4677: 33 c0                        	xorl	%eax, %eax
    4679: e9 13 01 00 00               	jmp	0x4791 <c0_utf8_to_wide+0x151>
    467e: 48 83 7c 24 68 00            	cmpq	$0x0, 0x68(%rsp)
    4684: 75 07                        	jne	0x468d <c0_utf8_to_wide+0x4d>
    4686: 33 c0                        	xorl	%eax, %eax
    4688: e9 04 01 00 00               	jmp	0x4791 <c0_utf8_to_wide+0x151>
    468d: 48 81 7c 24 68 ff ff ff 7f   	cmpq	$0x7fffffff, 0x68(%rsp) # imm = 0x7FFFFFFF
    4696: 76 07                        	jbe	0x469f <c0_utf8_to_wide+0x5f>
    4698: 33 c0                        	xorl	%eax, %eax
    469a: e9 f2 00 00 00               	jmp	0x4791 <c0_utf8_to_wide+0x151>
    469f: 48 8b 54 24 68               	movq	0x68(%rsp), %rdx
    46a4: 48 8b 4c 24 60               	movq	0x60(%rsp), %rcx
    46a9: e8 00 00 00 00               	callq	0x46ae <c0_utf8_to_wide+0x6e>
    46ae: 85 c0                        	testl	%eax, %eax
    46b0: 74 07                        	je	0x46b9 <c0_utf8_to_wide+0x79>
    46b2: 33 c0                        	xorl	%eax, %eax
    46b4: e9 d8 00 00 00               	jmp	0x4791 <c0_utf8_to_wide+0x151>
    46b9: c7 44 24 28 00 00 00 00      	movl	$0x0, 0x28(%rsp)
    46c1: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
    46ca: 44 8b 4c 24 68               	movl	0x68(%rsp), %r9d
    46cf: 4c 8b 44 24 60               	movq	0x60(%rsp), %r8
    46d4: ba 08 00 00 00               	movl	$0x8, %edx
    46d9: b9 e9 fd 00 00               	movl	$0xfde9, %ecx           # imm = 0xFDE9
    46de: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x46e4 <c0_utf8_to_wide+0xa4>
    46e4: 89 44 24 30                  	movl	%eax, 0x30(%rsp)
    46e8: 83 7c 24 30 00               	cmpl	$0x0, 0x30(%rsp)
    46ed: 7f 07                        	jg	0x46f6 <c0_utf8_to_wide+0xb6>
    46ef: 33 c0                        	xorl	%eax, %eax
    46f1: e9 9b 00 00 00               	jmp	0x4791 <c0_utf8_to_wide+0x151>
    46f6: 8b 44 24 30                  	movl	0x30(%rsp), %eax
    46fa: ff c0                        	incl	%eax
    46fc: 48 98                        	cltq
    46fe: 48 d1 e0                     	shlq	%rax
    4701: 48 89 44 24 38               	movq	%rax, 0x38(%rsp)
    4706: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    470b: e8 00 00 00 00               	callq	0x4710 <c0_utf8_to_wide+0xd0>
    4710: 48 89 44 24 40               	movq	%rax, 0x40(%rsp)
    4715: 48 83 7c 24 40 00            	cmpq	$0x0, 0x40(%rsp)
    471b: 75 04                        	jne	0x4721 <c0_utf8_to_wide+0xe1>
    471d: 33 c0                        	xorl	%eax, %eax
    471f: eb 70                        	jmp	0x4791 <c0_utf8_to_wide+0x151>
    4721: 8b 44 24 30                  	movl	0x30(%rsp), %eax
    4725: 89 44 24 28                  	movl	%eax, 0x28(%rsp)
    4729: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    472e: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    4733: 44 8b 4c 24 68               	movl	0x68(%rsp), %r9d
    4738: 4c 8b 44 24 60               	movq	0x60(%rsp), %r8
    473d: ba 08 00 00 00               	movl	$0x8, %edx
    4742: b9 e9 fd 00 00               	movl	$0xfde9, %ecx           # imm = 0xFDE9
    4747: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x474d <c0_utf8_to_wide+0x10d>
    474d: 89 44 24 48                  	movl	%eax, 0x48(%rsp)
    4751: 8b 44 24 30                  	movl	0x30(%rsp), %eax
    4755: 39 44 24 48                  	cmpl	%eax, 0x48(%rsp)
    4759: 74 0e                        	je	0x4769 <c0_utf8_to_wide+0x129>
    475b: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4760: e8 00 00 00 00               	callq	0x4765 <c0_utf8_to_wide+0x125>
    4765: 33 c0                        	xorl	%eax, %eax
    4767: eb 28                        	jmp	0x4791 <c0_utf8_to_wide+0x151>
    4769: 48 63 44 24 30               	movslq	0x30(%rsp), %rax
    476e: 33 c9                        	xorl	%ecx, %ecx
    4770: 48 8b 54 24 40               	movq	0x40(%rsp), %rdx
    4775: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    4779: 48 83 7c 24 70 00            	cmpq	$0x0, 0x70(%rsp)
    477f: 74 0b                        	je	0x478c <c0_utf8_to_wide+0x14c>
    4781: 48 8b 44 24 70               	movq	0x70(%rsp), %rax
    4786: 8b 4c 24 30                  	movl	0x30(%rsp), %ecx
    478a: 89 08                        	movl	%ecx, (%rax)
    478c: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    4791: 48 83 c4 50                  	addq	$0x50, %rsp
    4795: 5f                           	popq	%rdi
    4796: c3                           	retq
    4797: cc                           	int3
    4798: cc                           	int3
    4799: cc                           	int3
    479a: cc                           	int3
    479b: cc                           	int3
    479c: cc                           	int3
    479d: cc                           	int3
    479e: cc                           	int3
    479f: cc                           	int3

00000000000047a0 <c0_wide_to_utf8>:
    47a0: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    47a5: 89 54 24 10                  	movl	%edx, 0x10(%rsp)
    47a9: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    47ae: 57                           	pushq	%rdi
    47af: 48 83 ec 60                  	subq	$0x60, %rsp
    47b3: 48 83 bc 24 80 00 00 00 00   	cmpq	$0x0, 0x80(%rsp)
    47bc: 74 0e                        	je	0x47cc <c0_wide_to_utf8+0x2c>
    47be: 48 8b 84 24 80 00 00 00      	movq	0x80(%rsp), %rax
    47c6: c7 00 00 00 00 00            	movl	$0x0, (%rax)
    47cc: 48 83 7c 24 70 00            	cmpq	$0x0, 0x70(%rsp)
    47d2: 74 07                        	je	0x47db <c0_wide_to_utf8+0x3b>
    47d4: 83 7c 24 78 00               	cmpl	$0x0, 0x78(%rsp)
    47d9: 75 07                        	jne	0x47e2 <c0_wide_to_utf8+0x42>
    47db: 33 c0                        	xorl	%eax, %eax
    47dd: e9 f0 00 00 00               	jmp	0x48d2 <c0_wide_to_utf8+0x132>
    47e2: 81 7c 24 78 ff ff ff 7f      	cmpl	$0x7fffffff, 0x78(%rsp) # imm = 0x7FFFFFFF
    47ea: 76 07                        	jbe	0x47f3 <c0_wide_to_utf8+0x53>
    47ec: 33 c0                        	xorl	%eax, %eax
    47ee: e9 df 00 00 00               	jmp	0x48d2 <c0_wide_to_utf8+0x132>
    47f3: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    47fc: 48 c7 44 24 30 00 00 00 00   	movq	$0x0, 0x30(%rsp)
    4805: c7 44 24 28 00 00 00 00      	movl	$0x0, 0x28(%rsp)
    480d: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
    4816: 44 8b 4c 24 78               	movl	0x78(%rsp), %r9d
    481b: 4c 8b 44 24 70               	movq	0x70(%rsp), %r8
    4820: 33 d2                        	xorl	%edx, %edx
    4822: b9 e9 fd 00 00               	movl	$0xfde9, %ecx           # imm = 0xFDE9
    4827: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x482d <c0_wide_to_utf8+0x8d>
    482d: 89 44 24 40                  	movl	%eax, 0x40(%rsp)
    4831: 83 7c 24 40 00               	cmpl	$0x0, 0x40(%rsp)
    4836: 7f 07                        	jg	0x483f <c0_wide_to_utf8+0x9f>
    4838: 33 c0                        	xorl	%eax, %eax
    483a: e9 93 00 00 00               	jmp	0x48d2 <c0_wide_to_utf8+0x132>
    483f: 48 63 44 24 40               	movslq	0x40(%rsp), %rax
    4844: 48 8b c8                     	movq	%rax, %rcx
    4847: e8 00 00 00 00               	callq	0x484c <c0_wide_to_utf8+0xac>
    484c: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    4851: 48 83 7c 24 48 00            	cmpq	$0x0, 0x48(%rsp)
    4857: 75 04                        	jne	0x485d <c0_wide_to_utf8+0xbd>
    4859: 33 c0                        	xorl	%eax, %eax
    485b: eb 75                        	jmp	0x48d2 <c0_wide_to_utf8+0x132>
    485d: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    4866: 48 c7 44 24 30 00 00 00 00   	movq	$0x0, 0x30(%rsp)
    486f: 8b 44 24 40                  	movl	0x40(%rsp), %eax
    4873: 89 44 24 28                  	movl	%eax, 0x28(%rsp)
    4877: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    487c: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    4881: 44 8b 4c 24 78               	movl	0x78(%rsp), %r9d
    4886: 4c 8b 44 24 70               	movq	0x70(%rsp), %r8
    488b: 33 d2                        	xorl	%edx, %edx
    488d: b9 e9 fd 00 00               	movl	$0xfde9, %ecx           # imm = 0xFDE9
    4892: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x4898 <c0_wide_to_utf8+0xf8>
    4898: 89 44 24 50                  	movl	%eax, 0x50(%rsp)
    489c: 8b 44 24 40                  	movl	0x40(%rsp), %eax
    48a0: 39 44 24 50                  	cmpl	%eax, 0x50(%rsp)
    48a4: 74 0e                        	je	0x48b4 <c0_wide_to_utf8+0x114>
    48a6: 48 8b 4c 24 48               	movq	0x48(%rsp), %rcx
    48ab: e8 00 00 00 00               	callq	0x48b0 <c0_wide_to_utf8+0x110>
    48b0: 33 c0                        	xorl	%eax, %eax
    48b2: eb 1e                        	jmp	0x48d2 <c0_wide_to_utf8+0x132>
    48b4: 48 83 bc 24 80 00 00 00 00   	cmpq	$0x0, 0x80(%rsp)
    48bd: 74 0e                        	je	0x48cd <c0_wide_to_utf8+0x12d>
    48bf: 48 8b 84 24 80 00 00 00      	movq	0x80(%rsp), %rax
    48c7: 8b 4c 24 40                  	movl	0x40(%rsp), %ecx
    48cb: 89 08                        	movl	%ecx, (%rax)
    48cd: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    48d2: 48 83 c4 60                  	addq	$0x60, %rsp
    48d6: 5f                           	popq	%rdi
    48d7: c3                           	retq
    48d8: cc                           	int3
    48d9: cc                           	int3
    48da: cc                           	int3
    48db: cc                           	int3
    48dc: cc                           	int3
    48dd: cc                           	int3
    48de: cc                           	int3
    48df: cc                           	int3

00000000000048e0 <c0_dbg_write_byte>:
    48e0: 88 54 24 10                  	movb	%dl, 0x10(%rsp)
    48e4: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    48e9: 57                           	pushq	%rdi
    48ea: 48 83 ec 70                  	subq	$0x70, %rsp
    48ee: 48 8d 7c 24 40               	leaq	0x40(%rsp), %rdi
    48f3: b9 0c 00 00 00               	movl	$0xc, %ecx
    48f8: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    48fd: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    48ff: 48 8b 8c 24 80 00 00 00      	movq	0x80(%rsp), %rcx
    4907: 48 c7 44 24 30 00 00 00 00   	movq	$0x0, 0x30(%rsp)
    4910: c7 44 24 28 80 00 00 00      	movl	$0x80, 0x28(%rsp)
    4918: c7 44 24 20 04 00 00 00      	movl	$0x4, 0x20(%rsp)
    4920: 45 33 c9                     	xorl	%r9d, %r9d
    4923: 41 b8 01 00 00 00            	movl	$0x1, %r8d
    4929: ba 04 00 00 00               	movl	$0x4, %edx
    492e: 48 8b 8c 24 80 00 00 00      	movq	0x80(%rsp), %rcx
    4936: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x493c <c0_dbg_write_byte+0x5c>
    493c: 48 89 44 24 40               	movq	%rax, 0x40(%rsp)
    4941: 48 83 7c 24 40 ff            	cmpq	$-0x1, 0x40(%rsp)
    4947: 74 3b                        	je	0x4984 <c0_dbg_write_byte+0xa4>
    4949: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%rsp)
    4951: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
    495a: 4c 8d 4c 24 54               	leaq	0x54(%rsp), %r9
    495f: 41 b8 01 00 00 00            	movl	$0x1, %r8d
    4965: 48 8d 94 24 88 00 00 00      	leaq	0x88(%rsp), %rdx
    496d: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4972: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x4978 <c0_dbg_write_byte+0x98>
    4978: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    497d: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x4983 <c0_dbg_write_byte+0xa3>
    4983: 90                           	nop
    4984: 48 8b cc                     	movq	%rsp, %rcx
    4987: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x498e <c0_dbg_write_byte+0xae>
    498e: e8 00 00 00 00               	callq	0x4993 <c0_dbg_write_byte+0xb3>
    4993: 48 83 c4 70                  	addq	$0x70, %rsp
    4997: 5f                           	popq	%rdi
    4998: c3                           	retq
    4999: cc                           	int3
    499a: cc                           	int3
    499b: cc                           	int3
    499c: cc                           	int3
    499d: cc                           	int3
    499e: cc                           	int3
    499f: cc                           	int3

00000000000049a0 <c0_is_sep>:
    49a0: 88 4c 24 08                  	movb	%cl, 0x8(%rsp)
    49a4: 57                           	pushq	%rdi
    49a5: 48 83 ec 10                  	subq	$0x10, %rsp
    49a9: 0f b6 44 24 20               	movzbl	0x20(%rsp), %eax
    49ae: 83 f8 2f                     	cmpl	$0x2f, %eax
    49b1: 74 13                        	je	0x49c6 <c0_is_sep+0x26>
    49b3: 0f b6 44 24 20               	movzbl	0x20(%rsp), %eax
    49b8: 83 f8 5c                     	cmpl	$0x5c, %eax
    49bb: 74 09                        	je	0x49c6 <c0_is_sep+0x26>
    49bd: c7 04 24 00 00 00 00         	movl	$0x0, (%rsp)
    49c4: eb 07                        	jmp	0x49cd <c0_is_sep+0x2d>
    49c6: c7 04 24 01 00 00 00         	movl	$0x1, (%rsp)
    49cd: 8b 04 24                     	movl	(%rsp), %eax
    49d0: 48 83 c4 10                  	addq	$0x10, %rsp
    49d4: 5f                           	popq	%rdi
    49d5: c3                           	retq
    49d6: cc                           	int3
    49d7: cc                           	int3
    49d8: cc                           	int3
    49d9: cc                           	int3
    49da: cc                           	int3
    49db: cc                           	int3
    49dc: cc                           	int3
    49dd: cc                           	int3
    49de: cc                           	int3
    49df: cc                           	int3

00000000000049e0 <c0_is_ascii_letter>:
    49e0: 88 4c 24 08                  	movb	%cl, 0x8(%rsp)
    49e4: 57                           	pushq	%rdi
    49e5: 48 83 ec 10                  	subq	$0x10, %rsp
    49e9: 0f b6 44 24 20               	movzbl	0x20(%rsp), %eax
    49ee: 83 f8 41                     	cmpl	$0x41, %eax
    49f1: 7c 0a                        	jl	0x49fd <c0_is_ascii_letter+0x1d>
    49f3: 0f b6 44 24 20               	movzbl	0x20(%rsp), %eax
    49f8: 83 f8 5a                     	cmpl	$0x5a, %eax
    49fb: 7e 1d                        	jle	0x4a1a <c0_is_ascii_letter+0x3a>
    49fd: 0f b6 44 24 20               	movzbl	0x20(%rsp), %eax
    4a02: 83 f8 61                     	cmpl	$0x61, %eax
    4a05: 7c 0a                        	jl	0x4a11 <c0_is_ascii_letter+0x31>
    4a07: 0f b6 44 24 20               	movzbl	0x20(%rsp), %eax
    4a0c: 83 f8 7a                     	cmpl	$0x7a, %eax
    4a0f: 7e 09                        	jle	0x4a1a <c0_is_ascii_letter+0x3a>
    4a11: c7 04 24 00 00 00 00         	movl	$0x0, (%rsp)
    4a18: eb 07                        	jmp	0x4a21 <c0_is_ascii_letter+0x41>
    4a1a: c7 04 24 01 00 00 00         	movl	$0x1, (%rsp)
    4a21: 8b 04 24                     	movl	(%rsp), %eax
    4a24: 48 83 c4 10                  	addq	$0x10, %rsp
    4a28: 5f                           	popq	%rdi
    4a29: c3                           	retq
    4a2a: cc                           	int3
    4a2b: cc                           	int3
    4a2c: cc                           	int3
    4a2d: cc                           	int3
    4a2e: cc                           	int3
    4a2f: cc                           	int3

0000000000004a30 <c0_path_is_drive_rooted>:
    4a30: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    4a35: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    4a3a: 57                           	pushq	%rdi
    4a3b: 48 83 ec 20                  	subq	$0x20, %rsp
    4a3f: 48 83 7c 24 30 00            	cmpq	$0x0, 0x30(%rsp)
    4a45: 74 08                        	je	0x4a4f <c0_path_is_drive_rooted+0x1f>
    4a47: 48 83 7c 24 38 03            	cmpq	$0x3, 0x38(%rsp)
    4a4d: 73 04                        	jae	0x4a53 <c0_path_is_drive_rooted+0x23>
    4a4f: 33 c0                        	xorl	%eax, %eax
    4a51: eb 51                        	jmp	0x4aa4 <c0_path_is_drive_rooted+0x74>
    4a53: b8 01 00 00 00               	movl	$0x1, %eax
    4a58: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    4a5c: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
    4a61: 0f b6 0c 01                  	movzbl	(%rcx,%rax), %ecx
    4a65: e8 00 00 00 00               	callq	0x4a6a <c0_path_is_drive_rooted+0x3a>
    4a6a: 85 c0                        	testl	%eax, %eax
    4a6c: 75 04                        	jne	0x4a72 <c0_path_is_drive_rooted+0x42>
    4a6e: 33 c0                        	xorl	%eax, %eax
    4a70: eb 32                        	jmp	0x4aa4 <c0_path_is_drive_rooted+0x74>
    4a72: b8 01 00 00 00               	movl	$0x1, %eax
    4a77: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    4a7b: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
    4a80: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    4a84: 83 f8 3a                     	cmpl	$0x3a, %eax
    4a87: 74 04                        	je	0x4a8d <c0_path_is_drive_rooted+0x5d>
    4a89: 33 c0                        	xorl	%eax, %eax
    4a8b: eb 17                        	jmp	0x4aa4 <c0_path_is_drive_rooted+0x74>
    4a8d: b8 01 00 00 00               	movl	$0x1, %eax
    4a92: 48 6b c0 02                  	imulq	$0x2, %rax, %rax
    4a96: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
    4a9b: 0f b6 0c 01                  	movzbl	(%rcx,%rax), %ecx
    4a9f: e8 00 00 00 00               	callq	0x4aa4 <c0_path_is_drive_rooted+0x74>
    4aa4: 48 83 c4 20                  	addq	$0x20, %rsp
    4aa8: 5f                           	popq	%rdi
    4aa9: c3                           	retq
    4aaa: cc                           	int3
    4aab: cc                           	int3
    4aac: cc                           	int3
    4aad: cc                           	int3
    4aae: cc                           	int3
    4aaf: cc                           	int3

0000000000004ab0 <c0_path_is_unc>:
    4ab0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    4ab5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    4aba: 57                           	pushq	%rdi
    4abb: 48 83 ec 10                  	subq	$0x10, %rsp
    4abf: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    4ac5: 74 08                        	je	0x4acf <c0_path_is_unc+0x1f>
    4ac7: 48 83 7c 24 28 02            	cmpq	$0x2, 0x28(%rsp)
    4acd: 73 04                        	jae	0x4ad3 <c0_path_is_unc+0x23>
    4acf: 33 c0                        	xorl	%eax, %eax
    4ad1: eb 6f                        	jmp	0x4b42 <c0_path_is_unc+0x92>
    4ad3: b8 01 00 00 00               	movl	$0x1, %eax
    4ad8: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    4adc: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4ae1: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    4ae5: 83 f8 2f                     	cmpl	$0x2f, %eax
    4ae8: 75 17                        	jne	0x4b01 <c0_path_is_unc+0x51>
    4aea: b8 01 00 00 00               	movl	$0x1, %eax
    4aef: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    4af3: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4af8: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    4afc: 83 f8 2f                     	cmpl	$0x2f, %eax
    4aff: 74 37                        	je	0x4b38 <c0_path_is_unc+0x88>
    4b01: b8 01 00 00 00               	movl	$0x1, %eax
    4b06: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    4b0a: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4b0f: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    4b13: 83 f8 5c                     	cmpl	$0x5c, %eax
    4b16: 75 17                        	jne	0x4b2f <c0_path_is_unc+0x7f>
    4b18: b8 01 00 00 00               	movl	$0x1, %eax
    4b1d: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    4b21: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    4b26: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    4b2a: 83 f8 5c                     	cmpl	$0x5c, %eax
    4b2d: 74 09                        	je	0x4b38 <c0_path_is_unc+0x88>
    4b2f: c7 04 24 00 00 00 00         	movl	$0x0, (%rsp)
    4b36: eb 07                        	jmp	0x4b3f <c0_path_is_unc+0x8f>
    4b38: c7 04 24 01 00 00 00         	movl	$0x1, (%rsp)
    4b3f: 8b 04 24                     	movl	(%rsp), %eax
    4b42: 48 83 c4 10                  	addq	$0x10, %rsp
    4b46: 5f                           	popq	%rdi
    4b47: c3                           	retq
    4b48: cc                           	int3
    4b49: cc                           	int3
    4b4a: cc                           	int3
    4b4b: cc                           	int3
    4b4c: cc                           	int3
    4b4d: cc                           	int3
    4b4e: cc                           	int3
    4b4f: cc                           	int3

0000000000004b50 <c0_path_is_root_relative>:
    4b50: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    4b55: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    4b5a: 57                           	pushq	%rdi
    4b5b: 48 83 ec 30                  	subq	$0x30, %rsp
    4b5f: 48 83 7c 24 40 00            	cmpq	$0x0, 0x40(%rsp)
    4b65: 74 08                        	je	0x4b6f <c0_path_is_root_relative+0x1f>
    4b67: 48 83 7c 24 48 00            	cmpq	$0x0, 0x48(%rsp)
    4b6d: 75 04                        	jne	0x4b73 <c0_path_is_root_relative+0x23>
    4b6f: 33 c0                        	xorl	%eax, %eax
    4b71: eb 72                        	jmp	0x4be5 <c0_path_is_root_relative+0x95>
    4b73: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
    4b78: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4b7d: e8 00 00 00 00               	callq	0x4b82 <c0_path_is_root_relative+0x32>
    4b82: 85 c0                        	testl	%eax, %eax
    4b84: 74 04                        	je	0x4b8a <c0_path_is_root_relative+0x3a>
    4b86: 33 c0                        	xorl	%eax, %eax
    4b88: eb 5b                        	jmp	0x4be5 <c0_path_is_root_relative+0x95>
    4b8a: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
    4b8f: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4b94: e8 00 00 00 00               	callq	0x4b99 <c0_path_is_root_relative+0x49>
    4b99: 85 c0                        	testl	%eax, %eax
    4b9b: 74 04                        	je	0x4ba1 <c0_path_is_root_relative+0x51>
    4b9d: 33 c0                        	xorl	%eax, %eax
    4b9f: eb 44                        	jmp	0x4be5 <c0_path_is_root_relative+0x95>
    4ba1: b8 01 00 00 00               	movl	$0x1, %eax
    4ba6: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    4baa: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4baf: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    4bb3: 83 f8 2f                     	cmpl	$0x2f, %eax
    4bb6: 74 21                        	je	0x4bd9 <c0_path_is_root_relative+0x89>
    4bb8: b8 01 00 00 00               	movl	$0x1, %eax
    4bbd: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    4bc1: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4bc6: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    4bca: 83 f8 5c                     	cmpl	$0x5c, %eax
    4bcd: 74 0a                        	je	0x4bd9 <c0_path_is_root_relative+0x89>
    4bcf: c7 44 24 20 00 00 00 00      	movl	$0x0, 0x20(%rsp)
    4bd7: eb 08                        	jmp	0x4be1 <c0_path_is_root_relative+0x91>
    4bd9: c7 44 24 20 01 00 00 00      	movl	$0x1, 0x20(%rsp)
    4be1: 8b 44 24 20                  	movl	0x20(%rsp), %eax
    4be5: 48 83 c4 30                  	addq	$0x30, %rsp
    4be9: 5f                           	popq	%rdi
    4bea: c3                           	retq
    4beb: cc                           	int3
    4bec: cc                           	int3
    4bed: cc                           	int3
    4bee: cc                           	int3
    4bef: cc                           	int3
    4bf0: cc                           	int3
    4bf1: cc                           	int3
    4bf2: cc                           	int3
    4bf3: cc                           	int3
    4bf4: cc                           	int3
    4bf5: cc                           	int3
    4bf6: cc                           	int3
    4bf7: cc                           	int3
    4bf8: cc                           	int3
    4bf9: cc                           	int3
    4bfa: cc                           	int3
    4bfb: cc                           	int3
    4bfc: cc                           	int3
    4bfd: cc                           	int3
    4bfe: cc                           	int3
    4bff: cc                           	int3

0000000000004c00 <c0_path_is_abs>:
    4c00: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    4c05: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    4c0a: 57                           	pushq	%rdi
    4c0b: 48 83 ec 30                  	subq	$0x30, %rsp
    4c0f: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
    4c14: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4c19: e8 00 00 00 00               	callq	0x4c1e <c0_path_is_abs+0x1e>
    4c1e: 85 c0                        	testl	%eax, %eax
    4c20: 75 30                        	jne	0x4c52 <c0_path_is_abs+0x52>
    4c22: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
    4c27: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4c2c: e8 00 00 00 00               	callq	0x4c31 <c0_path_is_abs+0x31>
    4c31: 85 c0                        	testl	%eax, %eax
    4c33: 75 1d                        	jne	0x4c52 <c0_path_is_abs+0x52>
    4c35: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
    4c3a: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    4c3f: e8 00 00 00 00               	callq	0x4c44 <c0_path_is_abs+0x44>
    4c44: 85 c0                        	testl	%eax, %eax
    4c46: 75 0a                        	jne	0x4c52 <c0_path_is_abs+0x52>
    4c48: c7 44 24 20 00 00 00 00      	movl	$0x0, 0x20(%rsp)
    4c50: eb 08                        	jmp	0x4c5a <c0_path_is_abs+0x5a>
    4c52: c7 44 24 20 01 00 00 00      	movl	$0x1, 0x20(%rsp)
    4c5a: 8b 44 24 20                  	movl	0x20(%rsp), %eax
    4c5e: 48 83 c4 30                  	addq	$0x30, %rsp
    4c62: 5f                           	popq	%rdi
    4c63: c3                           	retq
    4c64: cc                           	int3
    4c65: cc                           	int3
    4c66: cc                           	int3
    4c67: cc                           	int3
    4c68: cc                           	int3
    4c69: cc                           	int3
    4c6a: cc                           	int3
    4c6b: cc                           	int3
    4c6c: cc                           	int3
    4c6d: cc                           	int3
    4c6e: cc                           	int3
    4c6f: cc                           	int3

0000000000004c70 <c0_path_canon>:
    4c70: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    4c75: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    4c7a: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    4c7f: 57                           	pushq	%rdi
    4c80: 48 81 ec b0 00 00 00         	subq	$0xb0, %rsp
    4c87: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    4c8c: b9 24 00 00 00               	movl	$0x24, %ecx
    4c91: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    4c96: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    4c98: 48 8b 8c 24 c0 00 00 00      	movq	0xc0(%rsp), %rcx
    4ca0: 48 83 bc 24 d0 00 00 00 00   	cmpq	$0x0, 0xd0(%rsp)
    4ca9: 74 0e                        	je	0x4cb9 <c0_path_canon+0x49>
    4cab: 48 8b 84 24 d0 00 00 00      	movq	0xd0(%rsp), %rax
    4cb3: c7 00 00 00 00 00            	movl	$0x0, (%rax)
    4cb9: 48 83 bc 24 c0 00 00 00 00   	cmpq	$0x0, 0xc0(%rsp)
    4cc2: 75 12                        	jne	0x4cd6 <c0_path_canon+0x66>
    4cc4: 48 83 bc 24 c8 00 00 00 00   	cmpq	$0x0, 0xc8(%rsp)
    4ccd: 74 07                        	je	0x4cd6 <c0_path_canon+0x66>
    4ccf: 33 c0                        	xorl	%eax, %eax
    4cd1: e9 58 06 00 00               	jmp	0x532e <c0_path_canon+0x6be>
    4cd6: b8 ff ff ff ff               	movl	$0xffffffff, %eax       # imm = 0xFFFFFFFF
    4cdb: 48 39 84 24 c8 00 00 00      	cmpq	%rax, 0xc8(%rsp)
    4ce3: 76 07                        	jbe	0x4cec <c0_path_canon+0x7c>
    4ce5: 33 c0                        	xorl	%eax, %eax
    4ce7: e9 42 06 00 00               	jmp	0x532e <c0_path_canon+0x6be>
    4cec: c7 44 24 34 00 00 00 00      	movl	$0x0, 0x34(%rsp)
    4cf4: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    4cfd: c7 44 24 40 00 00 00 00      	movl	$0x0, 0x40(%rsp)
    4d05: 48 8b 94 24 c8 00 00 00      	movq	0xc8(%rsp), %rdx
    4d0d: 48 8b 8c 24 c0 00 00 00      	movq	0xc0(%rsp), %rcx
    4d15: e8 00 00 00 00               	callq	0x4d1a <c0_path_canon+0xaa>
    4d1a: 85 c0                        	testl	%eax, %eax
    4d1c: 74 4e                        	je	0x4d6c <c0_path_canon+0xfc>
    4d1e: b8 01 00 00 00               	movl	$0x1, %eax
    4d23: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    4d27: b9 01 00 00 00               	movl	$0x1, %ecx
    4d2c: 48 6b c9 00                  	imulq	$0x0, %rcx, %rcx
    4d30: 48 8b 94 24 c0 00 00 00      	movq	0xc0(%rsp), %rdx
    4d38: 0f b6 04 02                  	movzbl	(%rdx,%rax), %eax
    4d3c: 88 44 0c 24                  	movb	%al, 0x24(%rsp,%rcx)
    4d40: b8 01 00 00 00               	movl	$0x1, %eax
    4d45: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    4d49: c6 44 04 24 3a               	movb	$0x3a, 0x24(%rsp,%rax)
    4d4e: c7 44 24 34 02 00 00 00      	movl	$0x2, 0x34(%rsp)
    4d56: 48 c7 44 24 38 03 00 00 00   	movq	$0x3, 0x38(%rsp)
    4d5f: c7 44 24 40 01 00 00 00      	movl	$0x1, 0x40(%rsp)
    4d67: e9 80 00 00 00               	jmp	0x4dec <c0_path_canon+0x17c>
    4d6c: 48 8b 94 24 c8 00 00 00      	movq	0xc8(%rsp), %rdx
    4d74: 48 8b 8c 24 c0 00 00 00      	movq	0xc0(%rsp), %rcx
    4d7c: e8 00 00 00 00               	callq	0x4d81 <c0_path_canon+0x111>
    4d81: 85 c0                        	testl	%eax, %eax
    4d83: 74 2f                        	je	0x4db4 <c0_path_canon+0x144>
    4d85: b8 01 00 00 00               	movl	$0x1, %eax
    4d8a: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    4d8e: c6 44 04 24 2f               	movb	$0x2f, 0x24(%rsp,%rax)
    4d93: b8 01 00 00 00               	movl	$0x1, %eax
    4d98: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    4d9c: c6 44 04 24 2f               	movb	$0x2f, 0x24(%rsp,%rax)
    4da1: c7 44 24 34 02 00 00 00      	movl	$0x2, 0x34(%rsp)
    4da9: 48 c7 44 24 38 02 00 00 00   	movq	$0x2, 0x38(%rsp)
    4db2: eb 38                        	jmp	0x4dec <c0_path_canon+0x17c>
    4db4: 48 8b 94 24 c8 00 00 00      	movq	0xc8(%rsp), %rdx
    4dbc: 48 8b 8c 24 c0 00 00 00      	movq	0xc0(%rsp), %rcx
    4dc4: e8 00 00 00 00               	callq	0x4dc9 <c0_path_canon+0x159>
    4dc9: 85 c0                        	testl	%eax, %eax
    4dcb: 74 1f                        	je	0x4dec <c0_path_canon+0x17c>
    4dcd: b8 01 00 00 00               	movl	$0x1, %eax
    4dd2: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    4dd6: c6 44 04 24 2f               	movb	$0x2f, 0x24(%rsp,%rax)
    4ddb: c7 44 24 34 01 00 00 00      	movl	$0x1, 0x34(%rsp)
    4de3: 48 c7 44 24 38 01 00 00 00   	movq	$0x1, 0x38(%rsp)
    4dec: c7 44 24 44 00 00 00 00      	movl	$0x0, 0x44(%rsp)
    4df4: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    4df9: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    4dfe: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    4e03: 48 89 44 24 50               	movq	%rax, 0x50(%rsp)
    4e08: eb 0d                        	jmp	0x4e17 <c0_path_canon+0x1a7>
    4e0a: 48 8b 44 24 50               	movq	0x50(%rsp), %rax
    4e0f: 48 ff c0                     	incq	%rax
    4e12: 48 89 44 24 50               	movq	%rax, 0x50(%rsp)
    4e17: 48 8b 84 24 c8 00 00 00      	movq	0xc8(%rsp), %rax
    4e1f: 48 39 44 24 50               	cmpq	%rax, 0x50(%rsp)
    4e24: 77 53                        	ja	0x4e79 <c0_path_canon+0x209>
    4e26: 48 8b 84 24 c8 00 00 00      	movq	0xc8(%rsp), %rax
    4e2e: 48 39 44 24 50               	cmpq	%rax, 0x50(%rsp)
    4e33: 74 1f                        	je	0x4e54 <c0_path_canon+0x1e4>
    4e35: 48 8b 44 24 50               	movq	0x50(%rsp), %rax
    4e3a: 48 8b 8c 24 c0 00 00 00      	movq	0xc0(%rsp), %rcx
    4e42: 48 03 c8                     	addq	%rax, %rcx
    4e45: 48 8b c1                     	movq	%rcx, %rax
    4e48: 0f b6 08                     	movzbl	(%rax), %ecx
    4e4b: e8 00 00 00 00               	callq	0x4e50 <c0_path_canon+0x1e0>
    4e50: 85 c0                        	testl	%eax, %eax
    4e52: 74 23                        	je	0x4e77 <c0_path_canon+0x207>
    4e54: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    4e59: 48 39 44 24 50               	cmpq	%rax, 0x50(%rsp)
    4e5e: 76 0a                        	jbe	0x4e6a <c0_path_canon+0x1fa>
    4e60: 8b 44 24 44                  	movl	0x44(%rsp), %eax
    4e64: ff c0                        	incl	%eax
    4e66: 89 44 24 44                  	movl	%eax, 0x44(%rsp)
    4e6a: 48 8b 44 24 50               	movq	0x50(%rsp), %rax
    4e6f: 48 ff c0                     	incq	%rax
    4e72: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    4e77: eb 91                        	jmp	0x4e0a <c0_path_canon+0x19a>
    4e79: 48 c7 44 24 58 00 00 00 00   	movq	$0x0, 0x58(%rsp)
    4e82: 83 7c 24 44 00               	cmpl	$0x0, 0x44(%rsp)
    4e87: 76 24                        	jbe	0x4ead <c0_path_canon+0x23d>
    4e89: 8b 44 24 44                  	movl	0x44(%rsp), %eax
    4e8d: 48 6b c0 10                  	imulq	$0x10, %rax, %rax
    4e91: 48 8b c8                     	movq	%rax, %rcx
    4e94: e8 00 00 00 00               	callq	0x4e99 <c0_path_canon+0x229>
    4e99: 48 89 44 24 58               	movq	%rax, 0x58(%rsp)
    4e9e: 48 83 7c 24 58 00            	cmpq	$0x0, 0x58(%rsp)
    4ea4: 75 07                        	jne	0x4ead <c0_path_canon+0x23d>
    4ea6: 33 c0                        	xorl	%eax, %eax
    4ea8: e9 81 04 00 00               	jmp	0x532e <c0_path_canon+0x6be>
    4ead: c7 44 24 60 00 00 00 00      	movl	$0x0, 0x60(%rsp)
    4eb5: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    4eba: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    4ebf: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    4ec4: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    4ec9: eb 0d                        	jmp	0x4ed8 <c0_path_canon+0x268>
    4ecb: 48 8b 44 24 68               	movq	0x68(%rsp), %rax
    4ed0: 48 ff c0                     	incq	%rax
    4ed3: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    4ed8: 48 8b 84 24 c8 00 00 00      	movq	0xc8(%rsp), %rax
    4ee0: 48 39 44 24 68               	cmpq	%rax, 0x68(%rsp)
    4ee5: 0f 87 31 01 00 00            	ja	0x501c <c0_path_canon+0x3ac>
    4eeb: 48 8b 84 24 c8 00 00 00      	movq	0xc8(%rsp), %rax
    4ef3: 48 39 44 24 68               	cmpq	%rax, 0x68(%rsp)
    4ef8: 74 23                        	je	0x4f1d <c0_path_canon+0x2ad>
    4efa: 48 8b 44 24 68               	movq	0x68(%rsp), %rax
    4eff: 48 8b 8c 24 c0 00 00 00      	movq	0xc0(%rsp), %rcx
    4f07: 48 03 c8                     	addq	%rax, %rcx
    4f0a: 48 8b c1                     	movq	%rcx, %rax
    4f0d: 0f b6 08                     	movzbl	(%rax), %ecx
    4f10: e8 00 00 00 00               	callq	0x4f15 <c0_path_canon+0x2a5>
    4f15: 85 c0                        	testl	%eax, %eax
    4f17: 0f 84 fa 00 00 00            	je	0x5017 <c0_path_canon+0x3a7>
    4f1d: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    4f22: 48 39 44 24 68               	cmpq	%rax, 0x68(%rsp)
    4f27: 0f 86 dd 00 00 00            	jbe	0x500a <c0_path_canon+0x39a>
    4f2d: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    4f32: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    4f37: 48 2b c8                     	subq	%rax, %rcx
    4f3a: 48 8b c1                     	movq	%rcx, %rax
    4f3d: 89 44 24 70                  	movl	%eax, 0x70(%rsp)
    4f41: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    4f46: 48 8b 8c 24 c0 00 00 00      	movq	0xc0(%rsp), %rcx
    4f4e: 48 03 c8                     	addq	%rax, %rcx
    4f51: 48 8b c1                     	movq	%rcx, %rax
    4f54: 48 89 44 24 78               	movq	%rax, 0x78(%rsp)
    4f59: 83 7c 24 70 01               	cmpl	$0x1, 0x70(%rsp)
    4f5e: 75 1c                        	jne	0x4f7c <c0_path_canon+0x30c>
    4f60: b8 01 00 00 00               	movl	$0x1, %eax
    4f65: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    4f69: 48 8b 4c 24 78               	movq	0x78(%rsp), %rcx
    4f6e: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    4f72: 83 f8 2e                     	cmpl	$0x2e, %eax
    4f75: 75 05                        	jne	0x4f7c <c0_path_canon+0x30c>
    4f77: e9 8e 00 00 00               	jmp	0x500a <c0_path_canon+0x39a>
    4f7c: 83 7c 24 70 02               	cmpl	$0x2, 0x70(%rsp)
    4f81: 75 4a                        	jne	0x4fcd <c0_path_canon+0x35d>
    4f83: b8 01 00 00 00               	movl	$0x1, %eax
    4f88: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    4f8c: 48 8b 4c 24 78               	movq	0x78(%rsp), %rcx
    4f91: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    4f95: 83 f8 2e                     	cmpl	$0x2e, %eax
    4f98: 75 33                        	jne	0x4fcd <c0_path_canon+0x35d>
    4f9a: b8 01 00 00 00               	movl	$0x1, %eax
    4f9f: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    4fa3: 48 8b 4c 24 78               	movq	0x78(%rsp), %rcx
    4fa8: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    4fac: 83 f8 2e                     	cmpl	$0x2e, %eax
    4faf: 75 1c                        	jne	0x4fcd <c0_path_canon+0x35d>
    4fb1: 48 83 7c 24 58 00            	cmpq	$0x0, 0x58(%rsp)
    4fb7: 74 0b                        	je	0x4fc4 <c0_path_canon+0x354>
    4fb9: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    4fbe: e8 00 00 00 00               	callq	0x4fc3 <c0_path_canon+0x353>
    4fc3: 90                           	nop
    4fc4: 33 c0                        	xorl	%eax, %eax
    4fc6: e9 63 03 00 00               	jmp	0x532e <c0_path_canon+0x6be>
    4fcb: eb 3d                        	jmp	0x500a <c0_path_canon+0x39a>
    4fcd: 48 83 7c 24 58 00            	cmpq	$0x0, 0x58(%rsp)
    4fd3: 74 2b                        	je	0x5000 <c0_path_canon+0x390>
    4fd5: 8b 44 24 60                  	movl	0x60(%rsp), %eax
    4fd9: 48 6b c0 10                  	imulq	$0x10, %rax, %rax
    4fdd: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    4fe2: 48 8b 54 24 78               	movq	0x78(%rsp), %rdx
    4fe7: 48 89 14 01                  	movq	%rdx, (%rcx,%rax)
    4feb: 8b 44 24 60                  	movl	0x60(%rsp), %eax
    4fef: 48 6b c0 10                  	imulq	$0x10, %rax, %rax
    4ff3: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    4ff8: 8b 54 24 70                  	movl	0x70(%rsp), %edx
    4ffc: 89 54 01 08                  	movl	%edx, 0x8(%rcx,%rax)
    5000: 8b 44 24 60                  	movl	0x60(%rsp), %eax
    5004: ff c0                        	incl	%eax
    5006: 89 44 24 60                  	movl	%eax, 0x60(%rsp)
    500a: 48 8b 44 24 68               	movq	0x68(%rsp), %rax
    500f: 48 ff c0                     	incq	%rax
    5012: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    5017: e9 af fe ff ff               	jmp	0x4ecb <c0_path_canon+0x25b>
    501c: 8b 44 24 34                  	movl	0x34(%rsp), %eax
    5020: 48 89 84 24 80 00 00 00      	movq	%rax, 0x80(%rsp)
    5028: 83 7c 24 60 00               	cmpl	$0x0, 0x60(%rsp)
    502d: 0f 86 9c 00 00 00            	jbe	0x50cf <c0_path_canon+0x45f>
    5033: 83 7c 24 34 00               	cmpl	$0x0, 0x34(%rsp)
    5038: 76 1a                        	jbe	0x5054 <c0_path_canon+0x3e4>
    503a: 83 7c 24 40 00               	cmpl	$0x0, 0x40(%rsp)
    503f: 74 13                        	je	0x5054 <c0_path_canon+0x3e4>
    5041: 48 8b 84 24 80 00 00 00      	movq	0x80(%rsp), %rax
    5049: 48 ff c0                     	incq	%rax
    504c: 48 89 84 24 80 00 00 00      	movq	%rax, 0x80(%rsp)
    5054: c7 84 24 88 00 00 00 00 00 00 00     	movl	$0x0, 0x88(%rsp)
    505f: eb 10                        	jmp	0x5071 <c0_path_canon+0x401>
    5061: 8b 84 24 88 00 00 00         	movl	0x88(%rsp), %eax
    5068: ff c0                        	incl	%eax
    506a: 89 84 24 88 00 00 00         	movl	%eax, 0x88(%rsp)
    5071: 8b 44 24 60                  	movl	0x60(%rsp), %eax
    5075: 39 84 24 88 00 00 00         	cmpl	%eax, 0x88(%rsp)
    507c: 73 2c                        	jae	0x50aa <c0_path_canon+0x43a>
    507e: 8b 84 24 88 00 00 00         	movl	0x88(%rsp), %eax
    5085: 48 6b c0 10                  	imulq	$0x10, %rax, %rax
    5089: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    508e: 8b 44 01 08                  	movl	0x8(%rcx,%rax), %eax
    5092: 48 8b 8c 24 80 00 00 00      	movq	0x80(%rsp), %rcx
    509a: 48 03 c8                     	addq	%rax, %rcx
    509d: 48 8b c1                     	movq	%rcx, %rax
    50a0: 48 89 84 24 80 00 00 00      	movq	%rax, 0x80(%rsp)
    50a8: eb b7                        	jmp	0x5061 <c0_path_canon+0x3f1>
    50aa: 83 7c 24 60 01               	cmpl	$0x1, 0x60(%rsp)
    50af: 76 1e                        	jbe	0x50cf <c0_path_canon+0x45f>
    50b1: 8b 44 24 60                  	movl	0x60(%rsp), %eax
    50b5: ff c8                        	decl	%eax
    50b7: 8b c0                        	movl	%eax, %eax
    50b9: 48 8b 8c 24 80 00 00 00      	movq	0x80(%rsp), %rcx
    50c1: 48 03 c8                     	addq	%rax, %rcx
    50c4: 48 8b c1                     	movq	%rcx, %rax
    50c7: 48 89 84 24 80 00 00 00      	movq	%rax, 0x80(%rsp)
    50cf: b8 ff ff ff ff               	movl	$0xffffffff, %eax       # imm = 0xFFFFFFFF
    50d4: 48 39 84 24 80 00 00 00      	cmpq	%rax, 0x80(%rsp)
    50dc: 76 1a                        	jbe	0x50f8 <c0_path_canon+0x488>
    50de: 48 83 7c 24 58 00            	cmpq	$0x0, 0x58(%rsp)
    50e4: 74 0b                        	je	0x50f1 <c0_path_canon+0x481>
    50e6: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    50eb: e8 00 00 00 00               	callq	0x50f0 <c0_path_canon+0x480>
    50f0: 90                           	nop
    50f1: 33 c0                        	xorl	%eax, %eax
    50f3: e9 36 02 00 00               	jmp	0x532e <c0_path_canon+0x6be>
    50f8: 8b 84 24 80 00 00 00         	movl	0x80(%rsp), %eax
    50ff: 89 84 24 8c 00 00 00         	movl	%eax, 0x8c(%rsp)
    5106: 8b 84 24 8c 00 00 00         	movl	0x8c(%rsp), %eax
    510d: 48 ff c0                     	incq	%rax
    5110: 48 89 84 24 90 00 00 00      	movq	%rax, 0x90(%rsp)
    5118: 48 83 bc 24 90 00 00 00 ff   	cmpq	$-0x1, 0x90(%rsp)
    5121: 76 1a                        	jbe	0x513d <c0_path_canon+0x4cd>
    5123: 48 83 7c 24 58 00            	cmpq	$0x0, 0x58(%rsp)
    5129: 74 0b                        	je	0x5136 <c0_path_canon+0x4c6>
    512b: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    5130: e8 00 00 00 00               	callq	0x5135 <c0_path_canon+0x4c5>
    5135: 90                           	nop
    5136: 33 c0                        	xorl	%eax, %eax
    5138: e9 f1 01 00 00               	jmp	0x532e <c0_path_canon+0x6be>
    513d: 48 8b 8c 24 90 00 00 00      	movq	0x90(%rsp), %rcx
    5145: e8 00 00 00 00               	callq	0x514a <c0_path_canon+0x4da>
    514a: 48 89 84 24 98 00 00 00      	movq	%rax, 0x98(%rsp)
    5152: 48 83 bc 24 98 00 00 00 00   	cmpq	$0x0, 0x98(%rsp)
    515b: 75 1a                        	jne	0x5177 <c0_path_canon+0x507>
    515d: 48 83 7c 24 58 00            	cmpq	$0x0, 0x58(%rsp)
    5163: 74 0b                        	je	0x5170 <c0_path_canon+0x500>
    5165: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    516a: e8 00 00 00 00               	callq	0x516f <c0_path_canon+0x4ff>
    516f: 90                           	nop
    5170: 33 c0                        	xorl	%eax, %eax
    5172: e9 b7 01 00 00               	jmp	0x532e <c0_path_canon+0x6be>
    5177: c7 84 24 a0 00 00 00 00 00 00 00     	movl	$0x0, 0xa0(%rsp)
    5182: 83 7c 24 34 00               	cmpl	$0x0, 0x34(%rsp)
    5187: 76 3c                        	jbe	0x51c5 <c0_path_canon+0x555>
    5189: 8b 44 24 34                  	movl	0x34(%rsp), %eax
    518d: 8b 8c 24 a0 00 00 00         	movl	0xa0(%rsp), %ecx
    5194: 48 8b 94 24 98 00 00 00      	movq	0x98(%rsp), %rdx
    519c: 48 03 d1                     	addq	%rcx, %rdx
    519f: 48 8b ca                     	movq	%rdx, %rcx
    51a2: 44 8b c0                     	movl	%eax, %r8d
    51a5: 48 8d 54 24 24               	leaq	0x24(%rsp), %rdx
    51aa: e8 00 00 00 00               	callq	0x51af <c0_path_canon+0x53f>
    51af: 8b 44 24 34                  	movl	0x34(%rsp), %eax
    51b3: 8b 8c 24 a0 00 00 00         	movl	0xa0(%rsp), %ecx
    51ba: 03 c8                        	addl	%eax, %ecx
    51bc: 8b c1                        	movl	%ecx, %eax
    51be: 89 84 24 a0 00 00 00         	movl	%eax, 0xa0(%rsp)
    51c5: 83 7c 24 60 00               	cmpl	$0x0, 0x60(%rsp)
    51ca: 0f 86 14 01 00 00            	jbe	0x52e4 <c0_path_canon+0x674>
    51d0: 83 7c 24 34 00               	cmpl	$0x0, 0x34(%rsp)
    51d5: 76 2a                        	jbe	0x5201 <c0_path_canon+0x591>
    51d7: 83 7c 24 40 00               	cmpl	$0x0, 0x40(%rsp)
    51dc: 74 23                        	je	0x5201 <c0_path_canon+0x591>
    51de: 8b 84 24 a0 00 00 00         	movl	0xa0(%rsp), %eax
    51e5: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
    51ed: c6 04 01 2f                  	movb	$0x2f, (%rcx,%rax)
    51f1: 8b 84 24 a0 00 00 00         	movl	0xa0(%rsp), %eax
    51f8: ff c0                        	incl	%eax
    51fa: 89 84 24 a0 00 00 00         	movl	%eax, 0xa0(%rsp)
    5201: c7 84 24 a4 00 00 00 00 00 00 00     	movl	$0x0, 0xa4(%rsp)
    520c: eb 10                        	jmp	0x521e <c0_path_canon+0x5ae>
    520e: 8b 84 24 a4 00 00 00         	movl	0xa4(%rsp), %eax
    5215: ff c0                        	incl	%eax
    5217: 89 84 24 a4 00 00 00         	movl	%eax, 0xa4(%rsp)
    521e: 8b 44 24 60                  	movl	0x60(%rsp), %eax
    5222: 39 84 24 a4 00 00 00         	cmpl	%eax, 0xa4(%rsp)
    5229: 0f 83 b5 00 00 00            	jae	0x52e4 <c0_path_canon+0x674>
    522f: 8b 84 24 a4 00 00 00         	movl	0xa4(%rsp), %eax
    5236: 48 6b c0 10                  	imulq	$0x10, %rax, %rax
    523a: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    523f: 8b 44 01 08                  	movl	0x8(%rcx,%rax), %eax
    5243: 8b 8c 24 a4 00 00 00         	movl	0xa4(%rsp), %ecx
    524a: 48 6b c9 10                  	imulq	$0x10, %rcx, %rcx
    524e: 8b 94 24 a0 00 00 00         	movl	0xa0(%rsp), %edx
    5255: 4c 8b 84 24 98 00 00 00      	movq	0x98(%rsp), %r8
    525d: 4c 03 c2                     	addq	%rdx, %r8
    5260: 49 8b d0                     	movq	%r8, %rdx
    5263: 48 89 94 24 a8 00 00 00      	movq	%rdx, 0xa8(%rsp)
    526b: 44 8b c0                     	movl	%eax, %r8d
    526e: 48 8b 44 24 58               	movq	0x58(%rsp), %rax
    5273: 48 8b 14 08                  	movq	(%rax,%rcx), %rdx
    5277: 48 8b 84 24 a8 00 00 00      	movq	0xa8(%rsp), %rax
    527f: 48 8b c8                     	movq	%rax, %rcx
    5282: e8 00 00 00 00               	callq	0x5287 <c0_path_canon+0x617>
    5287: 8b 84 24 a4 00 00 00         	movl	0xa4(%rsp), %eax
    528e: 48 6b c0 10                  	imulq	$0x10, %rax, %rax
    5292: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    5297: 8b 44 01 08                  	movl	0x8(%rcx,%rax), %eax
    529b: 8b 8c 24 a0 00 00 00         	movl	0xa0(%rsp), %ecx
    52a2: 03 c8                        	addl	%eax, %ecx
    52a4: 8b c1                        	movl	%ecx, %eax
    52a6: 89 84 24 a0 00 00 00         	movl	%eax, 0xa0(%rsp)
    52ad: 8b 84 24 a4 00 00 00         	movl	0xa4(%rsp), %eax
    52b4: ff c0                        	incl	%eax
    52b6: 3b 44 24 60                  	cmpl	0x60(%rsp), %eax
    52ba: 73 23                        	jae	0x52df <c0_path_canon+0x66f>
    52bc: 8b 84 24 a0 00 00 00         	movl	0xa0(%rsp), %eax
    52c3: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
    52cb: c6 04 01 2f                  	movb	$0x2f, (%rcx,%rax)
    52cf: 8b 84 24 a0 00 00 00         	movl	0xa0(%rsp), %eax
    52d6: ff c0                        	incl	%eax
    52d8: 89 84 24 a0 00 00 00         	movl	%eax, 0xa0(%rsp)
    52df: e9 2a ff ff ff               	jmp	0x520e <c0_path_canon+0x59e>
    52e4: 8b 84 24 a0 00 00 00         	movl	0xa0(%rsp), %eax
    52eb: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
    52f3: c6 04 01 00                  	movb	$0x0, (%rcx,%rax)
    52f7: 48 83 7c 24 58 00            	cmpq	$0x0, 0x58(%rsp)
    52fd: 74 0b                        	je	0x530a <c0_path_canon+0x69a>
    52ff: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    5304: e8 00 00 00 00               	callq	0x5309 <c0_path_canon+0x699>
    5309: 90                           	nop
    530a: 48 83 bc 24 d0 00 00 00 00   	cmpq	$0x0, 0xd0(%rsp)
    5313: 74 11                        	je	0x5326 <c0_path_canon+0x6b6>
    5315: 48 8b 84 24 d0 00 00 00      	movq	0xd0(%rsp), %rax
    531d: 8b 8c 24 8c 00 00 00         	movl	0x8c(%rsp), %ecx
    5324: 89 08                        	movl	%ecx, (%rax)
    5326: 48 8b 84 24 98 00 00 00      	movq	0x98(%rsp), %rax
    532e: 48 8b f8                     	movq	%rax, %rdi
    5331: 48 8b cc                     	movq	%rsp, %rcx
    5334: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x533b <c0_path_canon+0x6cb>
    533b: e8 00 00 00 00               	callq	0x5340 <c0_path_canon+0x6d0>
    5340: 48 8b c7                     	movq	%rdi, %rax
    5343: 48 81 c4 b0 00 00 00         	addq	$0xb0, %rsp
    534a: 5f                           	popq	%rdi
    534b: c3                           	retq
    534c: cc                           	int3
    534d: cc                           	int3
    534e: cc                           	int3
    534f: cc                           	int3
    5350: cc                           	int3
    5351: cc                           	int3
    5352: cc                           	int3
    5353: cc                           	int3
    5354: cc                           	int3
    5355: cc                           	int3
    5356: cc                           	int3
    5357: cc                           	int3
    5358: cc                           	int3
    5359: cc                           	int3
    535a: cc                           	int3
    535b: cc                           	int3
    535c: cc                           	int3
    535d: cc                           	int3
    535e: cc                           	int3
    535f: cc                           	int3

0000000000005360 <c0_path_join>:
    5360: 4c 89 4c 24 20               	movq	%r9, 0x20(%rsp)
    5365: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    536a: 89 54 24 10                  	movl	%edx, 0x10(%rsp)
    536e: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    5373: 57                           	pushq	%rdi
    5374: 48 83 ec 70                  	subq	$0x70, %rsp
    5378: 48 83 bc 24 a0 00 00 00 00   	cmpq	$0x0, 0xa0(%rsp)
    5381: 74 0e                        	je	0x5391 <c0_path_join+0x31>
    5383: 48 8b 84 24 a0 00 00 00      	movq	0xa0(%rsp), %rax
    538b: c7 00 00 00 00 00            	movl	$0x0, (%rax)
    5391: 48 83 bc 24 80 00 00 00 00   	cmpq	$0x0, 0x80(%rsp)
    539a: 75 11                        	jne	0x53ad <c0_path_join+0x4d>
    539c: 83 bc 24 88 00 00 00 00      	cmpl	$0x0, 0x88(%rsp)
    53a4: 74 07                        	je	0x53ad <c0_path_join+0x4d>
    53a6: 33 c0                        	xorl	%eax, %eax
    53a8: e9 c3 02 00 00               	jmp	0x5670 <c0_path_join+0x310>
    53ad: 48 83 bc 24 90 00 00 00 00   	cmpq	$0x0, 0x90(%rsp)
    53b6: 75 12                        	jne	0x53ca <c0_path_join+0x6a>
    53b8: 48 83 bc 24 98 00 00 00 00   	cmpq	$0x0, 0x98(%rsp)
    53c1: 74 07                        	je	0x53ca <c0_path_join+0x6a>
    53c3: 33 c0                        	xorl	%eax, %eax
    53c5: e9 a6 02 00 00               	jmp	0x5670 <c0_path_join+0x310>
    53ca: b8 ff ff ff ff               	movl	$0xffffffff, %eax       # imm = 0xFFFFFFFF
    53cf: 48 39 84 24 98 00 00 00      	cmpq	%rax, 0x98(%rsp)
    53d7: 76 07                        	jbe	0x53e0 <c0_path_join+0x80>
    53d9: 33 c0                        	xorl	%eax, %eax
    53db: e9 90 02 00 00               	jmp	0x5670 <c0_path_join+0x310>
    53e0: 83 bc 24 88 00 00 00 00      	cmpl	$0x0, 0x88(%rsp)
    53e8: 0f 85 95 00 00 00            	jne	0x5483 <c0_path_join+0x123>
    53ee: 8b 84 24 98 00 00 00         	movl	0x98(%rsp), %eax
    53f5: 89 44 24 20                  	movl	%eax, 0x20(%rsp)
    53f9: 8b 44 24 20                  	movl	0x20(%rsp), %eax
    53fd: 48 ff c0                     	incq	%rax
    5400: 48 89 44 24 28               	movq	%rax, 0x28(%rsp)
    5405: 48 83 7c 24 28 ff            	cmpq	$-0x1, 0x28(%rsp)
    540b: 76 07                        	jbe	0x5414 <c0_path_join+0xb4>
    540d: 33 c0                        	xorl	%eax, %eax
    540f: e9 5c 02 00 00               	jmp	0x5670 <c0_path_join+0x310>
    5414: 48 8b 4c 24 28               	movq	0x28(%rsp), %rcx
    5419: e8 00 00 00 00               	callq	0x541e <c0_path_join+0xbe>
    541e: 48 89 44 24 30               	movq	%rax, 0x30(%rsp)
    5423: 48 83 7c 24 30 00            	cmpq	$0x0, 0x30(%rsp)
    5429: 75 07                        	jne	0x5432 <c0_path_join+0xd2>
    542b: 33 c0                        	xorl	%eax, %eax
    542d: e9 3e 02 00 00               	jmp	0x5670 <c0_path_join+0x310>
    5432: 83 7c 24 20 00               	cmpl	$0x0, 0x20(%rsp)
    5437: 76 1a                        	jbe	0x5453 <c0_path_join+0xf3>
    5439: 8b 44 24 20                  	movl	0x20(%rsp), %eax
    543d: 44 8b c0                     	movl	%eax, %r8d
    5440: 48 8b 94 24 90 00 00 00      	movq	0x90(%rsp), %rdx
    5448: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
    544d: e8 00 00 00 00               	callq	0x5452 <c0_path_join+0xf2>
    5452: 90                           	nop
    5453: 8b 44 24 20                  	movl	0x20(%rsp), %eax
    5457: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
    545c: c6 04 01 00                  	movb	$0x0, (%rcx,%rax)
    5460: 48 83 bc 24 a0 00 00 00 00   	cmpq	$0x0, 0xa0(%rsp)
    5469: 74 0e                        	je	0x5479 <c0_path_join+0x119>
    546b: 48 8b 84 24 a0 00 00 00      	movq	0xa0(%rsp), %rax
    5473: 8b 4c 24 20                  	movl	0x20(%rsp), %ecx
    5477: 89 08                        	movl	%ecx, (%rax)
    5479: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
    547e: e9 ed 01 00 00               	jmp	0x5670 <c0_path_join+0x310>
    5483: 48 83 bc 24 98 00 00 00 00   	cmpq	$0x0, 0x98(%rsp)
    548c: 0f 85 8e 00 00 00            	jne	0x5520 <c0_path_join+0x1c0>
    5492: 8b 84 24 88 00 00 00         	movl	0x88(%rsp), %eax
    5499: 48 ff c0                     	incq	%rax
    549c: 48 89 44 24 38               	movq	%rax, 0x38(%rsp)
    54a1: 48 83 7c 24 38 ff            	cmpq	$-0x1, 0x38(%rsp)
    54a7: 76 07                        	jbe	0x54b0 <c0_path_join+0x150>
    54a9: 33 c0                        	xorl	%eax, %eax
    54ab: e9 c0 01 00 00               	jmp	0x5670 <c0_path_join+0x310>
    54b0: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    54b5: e8 00 00 00 00               	callq	0x54ba <c0_path_join+0x15a>
    54ba: 48 89 44 24 40               	movq	%rax, 0x40(%rsp)
    54bf: 48 83 7c 24 40 00            	cmpq	$0x0, 0x40(%rsp)
    54c5: 75 07                        	jne	0x54ce <c0_path_join+0x16e>
    54c7: 33 c0                        	xorl	%eax, %eax
    54c9: e9 a2 01 00 00               	jmp	0x5670 <c0_path_join+0x310>
    54ce: 8b 84 24 88 00 00 00         	movl	0x88(%rsp), %eax
    54d5: 44 8b c0                     	movl	%eax, %r8d
    54d8: 48 8b 94 24 80 00 00 00      	movq	0x80(%rsp), %rdx
    54e0: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    54e5: e8 00 00 00 00               	callq	0x54ea <c0_path_join+0x18a>
    54ea: 8b 84 24 88 00 00 00         	movl	0x88(%rsp), %eax
    54f1: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    54f6: c6 04 01 00                  	movb	$0x0, (%rcx,%rax)
    54fa: 48 83 bc 24 a0 00 00 00 00   	cmpq	$0x0, 0xa0(%rsp)
    5503: 74 11                        	je	0x5516 <c0_path_join+0x1b6>
    5505: 48 8b 84 24 a0 00 00 00      	movq	0xa0(%rsp), %rax
    550d: 8b 8c 24 88 00 00 00         	movl	0x88(%rsp), %ecx
    5514: 89 08                        	movl	%ecx, (%rax)
    5516: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    551b: e9 50 01 00 00               	jmp	0x5670 <c0_path_join+0x310>
    5520: 8b 84 24 88 00 00 00         	movl	0x88(%rsp), %eax
    5527: ff c8                        	decl	%eax
    5529: 8b c0                        	movl	%eax, %eax
    552b: 48 8b 8c 24 80 00 00 00      	movq	0x80(%rsp), %rcx
    5533: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    5537: 83 f8 2f                     	cmpl	$0x2f, %eax
    553a: 75 0a                        	jne	0x5546 <c0_path_join+0x1e6>
    553c: c7 44 24 6c 00 00 00 00      	movl	$0x0, 0x6c(%rsp)
    5544: eb 08                        	jmp	0x554e <c0_path_join+0x1ee>
    5546: c7 44 24 6c 01 00 00 00      	movl	$0x1, 0x6c(%rsp)
    554e: 8b 44 24 6c                  	movl	0x6c(%rsp), %eax
    5552: 89 44 24 48                  	movl	%eax, 0x48(%rsp)
    5556: 8b 84 24 88 00 00 00         	movl	0x88(%rsp), %eax
    555d: 8b 4c 24 48                  	movl	0x48(%rsp), %ecx
    5561: 48 8b 94 24 98 00 00 00      	movq	0x98(%rsp), %rdx
    5569: 48 03 d0                     	addq	%rax, %rdx
    556c: 48 8b c2                     	movq	%rdx, %rax
    556f: 48 03 c8                     	addq	%rax, %rcx
    5572: 48 8b c1                     	movq	%rcx, %rax
    5575: 48 89 44 24 50               	movq	%rax, 0x50(%rsp)
    557a: b8 ff ff ff ff               	movl	$0xffffffff, %eax       # imm = 0xFFFFFFFF
    557f: 48 39 44 24 50               	cmpq	%rax, 0x50(%rsp)
    5584: 76 07                        	jbe	0x558d <c0_path_join+0x22d>
    5586: 33 c0                        	xorl	%eax, %eax
    5588: e9 e3 00 00 00               	jmp	0x5670 <c0_path_join+0x310>
    558d: 48 8b 44 24 50               	movq	0x50(%rsp), %rax
    5592: 48 ff c0                     	incq	%rax
    5595: 48 89 44 24 58               	movq	%rax, 0x58(%rsp)
    559a: 48 83 7c 24 58 ff            	cmpq	$-0x1, 0x58(%rsp)
    55a0: 76 07                        	jbe	0x55a9 <c0_path_join+0x249>
    55a2: 33 c0                        	xorl	%eax, %eax
    55a4: e9 c7 00 00 00               	jmp	0x5670 <c0_path_join+0x310>
    55a9: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    55ae: e8 00 00 00 00               	callq	0x55b3 <c0_path_join+0x253>
    55b3: 48 89 44 24 60               	movq	%rax, 0x60(%rsp)
    55b8: 48 83 7c 24 60 00            	cmpq	$0x0, 0x60(%rsp)
    55be: 75 07                        	jne	0x55c7 <c0_path_join+0x267>
    55c0: 33 c0                        	xorl	%eax, %eax
    55c2: e9 a9 00 00 00               	jmp	0x5670 <c0_path_join+0x310>
    55c7: 8b 84 24 88 00 00 00         	movl	0x88(%rsp), %eax
    55ce: 44 8b c0                     	movl	%eax, %r8d
    55d1: 48 8b 94 24 80 00 00 00      	movq	0x80(%rsp), %rdx
    55d9: 48 8b 4c 24 60               	movq	0x60(%rsp), %rcx
    55de: e8 00 00 00 00               	callq	0x55e3 <c0_path_join+0x283>
    55e3: 8b 84 24 88 00 00 00         	movl	0x88(%rsp), %eax
    55ea: 89 44 24 68                  	movl	%eax, 0x68(%rsp)
    55ee: 83 7c 24 48 00               	cmpl	$0x0, 0x48(%rsp)
    55f3: 74 17                        	je	0x560c <c0_path_join+0x2ac>
    55f5: 8b 44 24 68                  	movl	0x68(%rsp), %eax
    55f9: 48 8b 4c 24 60               	movq	0x60(%rsp), %rcx
    55fe: c6 04 01 2f                  	movb	$0x2f, (%rcx,%rax)
    5602: 8b 44 24 68                  	movl	0x68(%rsp), %eax
    5606: ff c0                        	incl	%eax
    5608: 89 44 24 68                  	movl	%eax, 0x68(%rsp)
    560c: 8b 84 24 98 00 00 00         	movl	0x98(%rsp), %eax
    5613: 8b 4c 24 68                  	movl	0x68(%rsp), %ecx
    5617: 48 8b 54 24 60               	movq	0x60(%rsp), %rdx
    561c: 48 03 d1                     	addq	%rcx, %rdx
    561f: 48 8b ca                     	movq	%rdx, %rcx
    5622: 44 8b c0                     	movl	%eax, %r8d
    5625: 48 8b 94 24 90 00 00 00      	movq	0x90(%rsp), %rdx
    562d: e8 00 00 00 00               	callq	0x5632 <c0_path_join+0x2d2>
    5632: 8b 84 24 98 00 00 00         	movl	0x98(%rsp), %eax
    5639: 8b 4c 24 68                  	movl	0x68(%rsp), %ecx
    563d: 03 c8                        	addl	%eax, %ecx
    563f: 8b c1                        	movl	%ecx, %eax
    5641: 89 44 24 68                  	movl	%eax, 0x68(%rsp)
    5645: 8b 44 24 68                  	movl	0x68(%rsp), %eax
    5649: 48 8b 4c 24 60               	movq	0x60(%rsp), %rcx
    564e: c6 04 01 00                  	movb	$0x0, (%rcx,%rax)
    5652: 48 83 bc 24 a0 00 00 00 00   	cmpq	$0x0, 0xa0(%rsp)
    565b: 74 0e                        	je	0x566b <c0_path_join+0x30b>
    565d: 48 8b 84 24 a0 00 00 00      	movq	0xa0(%rsp), %rax
    5665: 8b 4c 24 50                  	movl	0x50(%rsp), %ecx
    5669: 89 08                        	movl	%ecx, (%rax)
    566b: 48 8b 44 24 60               	movq	0x60(%rsp), %rax
    5670: 48 83 c4 70                  	addq	$0x70, %rsp
    5674: 5f                           	popq	%rdi
    5675: c3                           	retq
    5676: cc                           	int3
    5677: cc                           	int3
    5678: cc                           	int3
    5679: cc                           	int3
    567a: cc                           	int3
    567b: cc                           	int3
    567c: cc                           	int3
    567d: cc                           	int3
    567e: cc                           	int3
    567f: cc                           	int3

0000000000005680 <c0_path_prefix>:
    5680: 44 89 4c 24 20               	movl	%r9d, 0x20(%rsp)
    5685: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    568a: 89 54 24 10                  	movl	%edx, 0x10(%rsp)
    568e: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    5693: 57                           	pushq	%rdi
    5694: 48 83 ec 10                  	subq	$0x10, %rsp
    5698: 83 7c 24 38 00               	cmpl	$0x0, 0x38(%rsp)
    569d: 75 0a                        	jne	0x56a9 <c0_path_prefix+0x29>
    569f: b8 01 00 00 00               	movl	$0x1, %eax
    56a4: e9 ba 00 00 00               	jmp	0x5763 <c0_path_prefix+0xe3>
    56a9: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    56af: 74 08                        	je	0x56b9 <c0_path_prefix+0x39>
    56b1: 48 83 7c 24 30 00            	cmpq	$0x0, 0x30(%rsp)
    56b7: 75 07                        	jne	0x56c0 <c0_path_prefix+0x40>
    56b9: 33 c0                        	xorl	%eax, %eax
    56bb: e9 a3 00 00 00               	jmp	0x5763 <c0_path_prefix+0xe3>
    56c0: 8b 44 24 38                  	movl	0x38(%rsp), %eax
    56c4: 39 44 24 28                  	cmpl	%eax, 0x28(%rsp)
    56c8: 73 07                        	jae	0x56d1 <c0_path_prefix+0x51>
    56ca: 33 c0                        	xorl	%eax, %eax
    56cc: e9 92 00 00 00               	jmp	0x5763 <c0_path_prefix+0xe3>
    56d1: c7 04 24 00 00 00 00         	movl	$0x0, (%rsp)
    56d8: eb 08                        	jmp	0x56e2 <c0_path_prefix+0x62>
    56da: 8b 04 24                     	movl	(%rsp), %eax
    56dd: ff c0                        	incl	%eax
    56df: 89 04 24                     	movl	%eax, (%rsp)
    56e2: 8b 44 24 38                  	movl	0x38(%rsp), %eax
    56e6: 39 04 24                     	cmpl	%eax, (%rsp)
    56e9: 73 22                        	jae	0x570d <c0_path_prefix+0x8d>
    56eb: 8b 04 24                     	movl	(%rsp), %eax
    56ee: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    56f3: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    56f7: 8b 0c 24                     	movl	(%rsp), %ecx
    56fa: 48 8b 54 24 30               	movq	0x30(%rsp), %rdx
    56ff: 0f b6 0c 0a                  	movzbl	(%rdx,%rcx), %ecx
    5703: 3b c1                        	cmpl	%ecx, %eax
    5705: 74 04                        	je	0x570b <c0_path_prefix+0x8b>
    5707: 33 c0                        	xorl	%eax, %eax
    5709: eb 58                        	jmp	0x5763 <c0_path_prefix+0xe3>
    570b: eb cd                        	jmp	0x56da <c0_path_prefix+0x5a>
    570d: 8b 44 24 38                  	movl	0x38(%rsp), %eax
    5711: 39 44 24 28                  	cmpl	%eax, 0x28(%rsp)
    5715: 75 07                        	jne	0x571e <c0_path_prefix+0x9e>
    5717: b8 01 00 00 00               	movl	$0x1, %eax
    571c: eb 45                        	jmp	0x5763 <c0_path_prefix+0xe3>
    571e: 8b 44 24 38                  	movl	0x38(%rsp), %eax
    5722: ff c8                        	decl	%eax
    5724: 8b c0                        	movl	%eax, %eax
    5726: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
    572b: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    572f: 83 f8 2f                     	cmpl	$0x2f, %eax
    5732: 75 07                        	jne	0x573b <c0_path_prefix+0xbb>
    5734: b8 01 00 00 00               	movl	$0x1, %eax
    5739: eb 28                        	jmp	0x5763 <c0_path_prefix+0xe3>
    573b: 8b 44 24 38                  	movl	0x38(%rsp), %eax
    573f: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    5744: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    5748: 83 f8 2f                     	cmpl	$0x2f, %eax
    574b: 75 0a                        	jne	0x5757 <c0_path_prefix+0xd7>
    574d: c7 44 24 04 01 00 00 00      	movl	$0x1, 0x4(%rsp)
    5755: eb 08                        	jmp	0x575f <c0_path_prefix+0xdf>
    5757: c7 44 24 04 00 00 00 00      	movl	$0x0, 0x4(%rsp)
    575f: 8b 44 24 04                  	movl	0x4(%rsp), %eax
    5763: 48 83 c4 10                  	addq	$0x10, %rsp
    5767: 5f                           	popq	%rdi
    5768: c3                           	retq
    5769: cc                           	int3
    576a: cc                           	int3
    576b: cc                           	int3
    576c: cc                           	int3
    576d: cc                           	int3
    576e: cc                           	int3
    576f: cc                           	int3

0000000000005770 <c0_path_utf8_to_wide>:
    5770: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    5775: 89 54 24 10                  	movl	%edx, 0x10(%rsp)
    5779: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    577e: 57                           	pushq	%rdi
    577f: 48 83 ec 50                  	subq	$0x50, %rsp
    5783: 48 83 7c 24 70 00            	cmpq	$0x0, 0x70(%rsp)
    5789: 74 0b                        	je	0x5796 <c0_path_utf8_to_wide+0x26>
    578b: 48 8b 44 24 70               	movq	0x70(%rsp), %rax
    5790: c7 00 00 00 00 00            	movl	$0x0, (%rax)
    5796: 83 7c 24 68 00               	cmpl	$0x0, 0x68(%rsp)
    579b: 75 4f                        	jne	0x57ec <c0_path_utf8_to_wide+0x7c>
    579d: b9 02 00 00 00               	movl	$0x2, %ecx
    57a2: e8 00 00 00 00               	callq	0x57a7 <c0_path_utf8_to_wide+0x37>
    57a7: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    57ac: 48 83 7c 24 20 00            	cmpq	$0x0, 0x20(%rsp)
    57b2: 75 07                        	jne	0x57bb <c0_path_utf8_to_wide+0x4b>
    57b4: 33 c0                        	xorl	%eax, %eax
    57b6: e9 b5 01 00 00               	jmp	0x5970 <c0_path_utf8_to_wide+0x200>
    57bb: b8 02 00 00 00               	movl	$0x2, %eax
    57c0: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    57c4: 33 c9                        	xorl	%ecx, %ecx
    57c6: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    57cb: 66 89 0c 02                  	movw	%cx, (%rdx,%rax)
    57cf: 48 83 7c 24 70 00            	cmpq	$0x0, 0x70(%rsp)
    57d5: 74 0b                        	je	0x57e2 <c0_path_utf8_to_wide+0x72>
    57d7: 48 8b 44 24 70               	movq	0x70(%rsp), %rax
    57dc: c7 00 00 00 00 00            	movl	$0x0, (%rax)
    57e2: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    57e7: e9 84 01 00 00               	jmp	0x5970 <c0_path_utf8_to_wide+0x200>
    57ec: 8b 44 24 68                  	movl	0x68(%rsp), %eax
    57f0: 4c 8b 44 24 70               	movq	0x70(%rsp), %r8
    57f5: 8b d0                        	movl	%eax, %edx
    57f7: 48 8b 4c 24 60               	movq	0x60(%rsp), %rcx
    57fc: e8 00 00 00 00               	callq	0x5801 <c0_path_utf8_to_wide+0x91>
    5801: 48 89 44 24 28               	movq	%rax, 0x28(%rsp)
    5806: 48 83 7c 24 28 00            	cmpq	$0x0, 0x28(%rsp)
    580c: 75 07                        	jne	0x5815 <c0_path_utf8_to_wide+0xa5>
    580e: 33 c0                        	xorl	%eax, %eax
    5810: e9 5b 01 00 00               	jmp	0x5970 <c0_path_utf8_to_wide+0x200>
    5815: 48 83 7c 24 70 00            	cmpq	$0x0, 0x70(%rsp)
    581b: 74 0d                        	je	0x582a <c0_path_utf8_to_wide+0xba>
    581d: 48 8b 44 24 70               	movq	0x70(%rsp), %rax
    5822: 8b 00                        	movl	(%rax), %eax
    5824: 89 44 24 40                  	movl	%eax, 0x40(%rsp)
    5828: eb 0e                        	jmp	0x5838 <c0_path_utf8_to_wide+0xc8>
    582a: 48 8b 4c 24 28               	movq	0x28(%rsp), %rcx
    582f: e8 00 00 00 00               	callq	0x5834 <c0_path_utf8_to_wide+0xc4>
    5834: 89 44 24 40                  	movl	%eax, 0x40(%rsp)
    5838: 8b 44 24 40                  	movl	0x40(%rsp), %eax
    583c: 89 44 24 30                  	movl	%eax, 0x30(%rsp)
    5840: c7 44 24 34 00 00 00 00      	movl	$0x0, 0x34(%rsp)
    5848: eb 0a                        	jmp	0x5854 <c0_path_utf8_to_wide+0xe4>
    584a: 8b 44 24 34                  	movl	0x34(%rsp), %eax
    584e: ff c0                        	incl	%eax
    5850: 89 44 24 34                  	movl	%eax, 0x34(%rsp)
    5854: 8b 44 24 30                  	movl	0x30(%rsp), %eax
    5858: 39 44 24 34                  	cmpl	%eax, 0x34(%rsp)
    585c: 73 26                        	jae	0x5884 <c0_path_utf8_to_wide+0x114>
    585e: 8b 44 24 34                  	movl	0x34(%rsp), %eax
    5862: 48 8b 4c 24 28               	movq	0x28(%rsp), %rcx
    5867: 0f b7 04 41                  	movzwl	(%rcx,%rax,2), %eax
    586b: 83 f8 2f                     	cmpl	$0x2f, %eax
    586e: 75 12                        	jne	0x5882 <c0_path_utf8_to_wide+0x112>
    5870: 8b 44 24 34                  	movl	0x34(%rsp), %eax
    5874: b9 5c 00 00 00               	movl	$0x5c, %ecx
    5879: 48 8b 54 24 28               	movq	0x28(%rsp), %rdx
    587e: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    5882: eb c6                        	jmp	0x584a <c0_path_utf8_to_wide+0xda>
    5884: 83 7c 24 30 02               	cmpl	$0x2, 0x30(%rsp)
    5889: 0f 85 dc 00 00 00            	jne	0x596b <c0_path_utf8_to_wide+0x1fb>
    588f: b8 02 00 00 00               	movl	$0x2, %eax
    5894: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    5898: 48 8b 4c 24 28               	movq	0x28(%rsp), %rcx
    589d: 0f b7 04 01                  	movzwl	(%rcx,%rax), %eax
    58a1: 83 f8 3a                     	cmpl	$0x3a, %eax
    58a4: 0f 85 c1 00 00 00            	jne	0x596b <c0_path_utf8_to_wide+0x1fb>
    58aa: b9 08 00 00 00               	movl	$0x8, %ecx
    58af: e8 00 00 00 00               	callq	0x58b4 <c0_path_utf8_to_wide+0x144>
    58b4: 48 89 44 24 38               	movq	%rax, 0x38(%rsp)
    58b9: 48 83 7c 24 38 00            	cmpq	$0x0, 0x38(%rsp)
    58bf: 75 11                        	jne	0x58d2 <c0_path_utf8_to_wide+0x162>
    58c1: 48 8b 4c 24 28               	movq	0x28(%rsp), %rcx
    58c6: e8 00 00 00 00               	callq	0x58cb <c0_path_utf8_to_wide+0x15b>
    58cb: 33 c0                        	xorl	%eax, %eax
    58cd: e9 9e 00 00 00               	jmp	0x5970 <c0_path_utf8_to_wide+0x200>
    58d2: b8 02 00 00 00               	movl	$0x2, %eax
    58d7: 48 6b c0 00                  	imulq	$0x0, %rax, %rax
    58db: b9 02 00 00 00               	movl	$0x2, %ecx
    58e0: 48 6b c9 00                  	imulq	$0x0, %rcx, %rcx
    58e4: 48 8b 54 24 38               	movq	0x38(%rsp), %rdx
    58e9: 4c 8b 44 24 28               	movq	0x28(%rsp), %r8
    58ee: 41 0f b7 04 00               	movzwl	(%r8,%rax), %eax
    58f3: 66 89 04 0a                  	movw	%ax, (%rdx,%rcx)
    58f7: b8 02 00 00 00               	movl	$0x2, %eax
    58fc: 48 6b c0 01                  	imulq	$0x1, %rax, %rax
    5900: b9 02 00 00 00               	movl	$0x2, %ecx
    5905: 48 6b c9 01                  	imulq	$0x1, %rcx, %rcx
    5909: 48 8b 54 24 38               	movq	0x38(%rsp), %rdx
    590e: 4c 8b 44 24 28               	movq	0x28(%rsp), %r8
    5913: 41 0f b7 04 00               	movzwl	(%r8,%rax), %eax
    5918: 66 89 04 0a                  	movw	%ax, (%rdx,%rcx)
    591c: b8 02 00 00 00               	movl	$0x2, %eax
    5921: 48 6b c0 02                  	imulq	$0x2, %rax, %rax
    5925: b9 5c 00 00 00               	movl	$0x5c, %ecx
    592a: 48 8b 54 24 38               	movq	0x38(%rsp), %rdx
    592f: 66 89 0c 02                  	movw	%cx, (%rdx,%rax)
    5933: b8 02 00 00 00               	movl	$0x2, %eax
    5938: 48 6b c0 03                  	imulq	$0x3, %rax, %rax
    593c: 33 c9                        	xorl	%ecx, %ecx
    593e: 48 8b 54 24 38               	movq	0x38(%rsp), %rdx
    5943: 66 89 0c 02                  	movw	%cx, (%rdx,%rax)
    5947: 48 8b 4c 24 28               	movq	0x28(%rsp), %rcx
    594c: e8 00 00 00 00               	callq	0x5951 <c0_path_utf8_to_wide+0x1e1>
    5951: 48 83 7c 24 70 00            	cmpq	$0x0, 0x70(%rsp)
    5957: 74 0b                        	je	0x5964 <c0_path_utf8_to_wide+0x1f4>
    5959: 48 8b 44 24 70               	movq	0x70(%rsp), %rax
    595e: c7 00 03 00 00 00            	movl	$0x3, (%rax)
    5964: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    5969: eb 05                        	jmp	0x5970 <c0_path_utf8_to_wide+0x200>
    596b: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
    5970: 48 83 c4 50                  	addq	$0x50, %rsp
    5974: 5f                           	popq	%rdi
    5975: c3                           	retq
    5976: cc                           	int3
    5977: cc                           	int3
    5978: cc                           	int3
    5979: cc                           	int3
    597a: cc                           	int3
    597b: cc                           	int3
    597c: cc                           	int3
    597d: cc                           	int3
    597e: cc                           	int3
    597f: cc                           	int3

0000000000005980 <c0_map_win_error>:
    5980: 89 4c 24 08                  	movl	%ecx, 0x8(%rsp)
    5984: 57                           	pushq	%rdi
    5985: 48 83 ec 10                  	subq	$0x10, %rsp
    5989: 8b 44 24 20                  	movl	0x20(%rsp), %eax
    598d: 89 04 24                     	movl	%eax, (%rsp)
    5990: 81 3c 24 0b 01 00 00         	cmpl	$0x10b, (%rsp)          # imm = 0x10B
    5997: 77 39                        	ja	0x59d2 <c0_map_win_error+0x52>
    5999: 81 3c 24 0b 01 00 00         	cmpl	$0x10b, (%rsp)          # imm = 0x10B
    59a0: 74 47                        	je	0x59e9 <$LN15>
    59a2: 8b 04 24                     	movl	(%rsp), %eax
    59a5: 83 e8 02                     	subl	$0x2, %eax
    59a8: 89 04 24                     	movl	%eax, (%rsp)
    59ab: 81 3c 24 e5 00 00 00         	cmpl	$0xe5, (%rsp)
    59b2: 77 3d                        	ja	0x59f1 <$LN20>
    59b4: 8b 04 24                     	movl	(%rsp), %eax
    59b7: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x59be <c0_map_win_error+0x3e>
    59be: 0f b6 84 01 00 00 00 00      	movzbl	(%rcx,%rax), %eax
    59c6: 8b 84 81 00 00 00 00         	movl	(%rcx,%rax,4), %eax
    59cd: 48 03 c1                     	addq	%rcx, %rax
    59d0: ff e0                        	jmpq	*%rax
    59d2: 81 3c 24 22 05 00 00         	cmpl	$0x522, (%rsp)          # imm = 0x522
    59d9: 74 06                        	je	0x59e1 <$LN7>
    59db: eb 14                        	jmp	0x59f1 <$LN20>

00000000000059dd <$LN6>:
    59dd: 32 c0                        	xorb	%al, %al
    59df: eb 12                        	jmp	0x59f3 <$LN20+0x2>

00000000000059e1 <$LN7>:
    59e1: b0 01                        	movb	$0x1, %al
    59e3: eb 0e                        	jmp	0x59f3 <$LN20+0x2>

00000000000059e5 <$LN9>:
    59e5: b0 02                        	movb	$0x2, %al
    59e7: eb 0a                        	jmp	0x59f3 <$LN20+0x2>

00000000000059e9 <$LN15>:
    59e9: b0 03                        	movb	$0x3, %al
    59eb: eb 06                        	jmp	0x59f3 <$LN20+0x2>

00000000000059ed <$LN19>:
    59ed: b0 04                        	movb	$0x4, %al
    59ef: eb 02                        	jmp	0x59f3 <$LN20+0x2>

00000000000059f1 <$LN20>:
    59f1: b0 05                        	movb	$0x5, %al
    59f3: 48 83 c4 10                  	addq	$0x10, %rsp
    59f7: 5f                           	popq	%rdi
    59f8: c3                           	retq
    59f9: 0f 1f 00                     	nopl	(%rax)

00000000000059fc <$LN24>:
		...

0000000000005a38 <$LN23>:
    5a38: 00 01                        	addb	%al, (%rcx)
    5a3a: 0e                           	<unknown>
    5a3b: 02 0e                        	addb	(%rsi), %cl
    5a3d: 0e                           	<unknown>
    5a3e: 0e                           	<unknown>
    5a3f: 0e                           	<unknown>
    5a40: 0e                           	<unknown>
    5a41: 0e                           	<unknown>
    5a42: 0e                           	<unknown>
    5a43: 0e                           	<unknown>
    5a44: 0e                           	<unknown>
    5a45: 03 0e                        	addl	(%rsi), %ecx
    5a47: 0e                           	<unknown>
    5a48: 0e                           	<unknown>
    5a49: 0e                           	<unknown>
    5a4a: 0e                           	<unknown>
    5a4b: 0e                           	<unknown>
    5a4c: 0e                           	<unknown>
    5a4d: 0e                           	<unknown>
    5a4e: 0e                           	<unknown>
    5a4f: 0e                           	<unknown>
    5a50: 0e                           	<unknown>
    5a51: 0e                           	<unknown>
    5a52: 0e                           	<unknown>
    5a53: 0e                           	<unknown>
    5a54: 0e                           	<unknown>
    5a55: 0e                           	<unknown>
    5a56: 04 05                        	addb	$0x5, %al
    5a58: 0e                           	<unknown>
    5a59: 0e                           	<unknown>
    5a5a: 0e                           	<unknown>
    5a5b: 0e                           	<unknown>
    5a5c: 0e                           	<unknown>
    5a5d: 0e                           	<unknown>
    5a5e: 0e                           	<unknown>
    5a5f: 0e                           	<unknown>
    5a60: 0e                           	<unknown>
    5a61: 0e                           	<unknown>
    5a62: 0e                           	<unknown>
    5a63: 0e                           	<unknown>
    5a64: 0e                           	<unknown>
    5a65: 0e                           	<unknown>
    5a66: 0e                           	<unknown>
    5a67: 0e                           	<unknown>
    5a68: 0e                           	<unknown>
    5a69: 0e                           	<unknown>
    5a6a: 0e                           	<unknown>
    5a6b: 0e                           	<unknown>
    5a6c: 0e                           	<unknown>
    5a6d: 0e                           	<unknown>
    5a6e: 0e                           	<unknown>
    5a6f: 0e                           	<unknown>
    5a70: 0e                           	<unknown>
    5a71: 0e                           	<unknown>
    5a72: 0e                           	<unknown>
    5a73: 0e                           	<unknown>
    5a74: 0e                           	<unknown>
    5a75: 0e                           	<unknown>
    5a76: 0e                           	<unknown>
    5a77: 0e                           	<unknown>
    5a78: 0e                           	<unknown>
    5a79: 0e                           	<unknown>
    5a7a: 0e                           	<unknown>
    5a7b: 0e                           	<unknown>
    5a7c: 0e                           	<unknown>
    5a7d: 0e                           	<unknown>
    5a7e: 0e                           	<unknown>
    5a7f: 0e                           	<unknown>
    5a80: 0e                           	<unknown>
    5a81: 0e                           	<unknown>
    5a82: 0e                           	<unknown>
    5a83: 0e                           	<unknown>
    5a84: 0e                           	<unknown>
    5a85: 0e                           	<unknown>
    5a86: 06                           	<unknown>
    5a87: 0e                           	<unknown>
    5a88: 0e                           	<unknown>
    5a89: 0e                           	<unknown>
    5a8a: 0e                           	<unknown>
    5a8b: 0e                           	<unknown>
    5a8c: 0e                           	<unknown>
    5a8d: 07                           	<unknown>
    5a8e: 0e                           	<unknown>
    5a8f: 0e                           	<unknown>
    5a90: 0e                           	<unknown>
    5a91: 0e                           	<unknown>
    5a92: 0e                           	<unknown>
    5a93: 0e                           	<unknown>
    5a94: 0e                           	<unknown>
    5a95: 0e                           	<unknown>
    5a96: 0e                           	<unknown>
    5a97: 0e                           	<unknown>
    5a98: 0e                           	<unknown>
    5a99: 0e                           	<unknown>
    5a9a: 0e                           	<unknown>
    5a9b: 0e                           	<unknown>
    5a9c: 0e                           	<unknown>
    5a9d: 0e                           	<unknown>
    5a9e: 0e                           	<unknown>
    5a9f: 0e                           	<unknown>
    5aa0: 0e                           	<unknown>
    5aa1: 0e                           	<unknown>
    5aa2: 0e                           	<unknown>
    5aa3: 0e                           	<unknown>
    5aa4: 0e                           	<unknown>
    5aa5: 0e                           	<unknown>
    5aa6: 0e                           	<unknown>
    5aa7: 0e                           	<unknown>
    5aa8: 0e                           	<unknown>
    5aa9: 0e                           	<unknown>
    5aaa: 0e                           	<unknown>
    5aab: 0e                           	<unknown>
    5aac: 0e                           	<unknown>
    5aad: 0e                           	<unknown>
    5aae: 0e                           	<unknown>
    5aaf: 0e                           	<unknown>
    5ab0: 0e                           	<unknown>
    5ab1: 08 0e                        	orb	%cl, (%rsi)
    5ab3: 0e                           	<unknown>
    5ab4: 0e                           	<unknown>
    5ab5: 0e                           	<unknown>
    5ab6: 0e                           	<unknown>
    5ab7: 0e                           	<unknown>
    5ab8: 0e                           	<unknown>
    5ab9: 0e                           	<unknown>
    5aba: 0e                           	<unknown>
    5abb: 0e                           	<unknown>
    5abc: 0e                           	<unknown>
    5abd: 0e                           	<unknown>
    5abe: 0e                           	<unknown>
    5abf: 0e                           	<unknown>
    5ac0: 0e                           	<unknown>
    5ac1: 0e                           	<unknown>
    5ac2: 0e                           	<unknown>
    5ac3: 0e                           	<unknown>
    5ac4: 0e                           	<unknown>
    5ac5: 0e                           	<unknown>
    5ac6: 0e                           	<unknown>
    5ac7: 0e                           	<unknown>
    5ac8: 0e                           	<unknown>
    5ac9: 0e                           	<unknown>
    5aca: 0e                           	<unknown>
    5acb: 0e                           	<unknown>
    5acc: 0e                           	<unknown>
    5acd: 0e                           	<unknown>
    5ace: 0e                           	<unknown>
    5acf: 0e                           	<unknown>
    5ad0: 0e                           	<unknown>
    5ad1: 0e                           	<unknown>
    5ad2: 0e                           	<unknown>
    5ad3: 0e                           	<unknown>
    5ad4: 0e                           	<unknown>
    5ad5: 0e                           	<unknown>
    5ad6: 0e                           	<unknown>
    5ad7: 09 0e                        	orl	%ecx, (%rsi)
    5ad9: 0e                           	<unknown>
    5ada: 0e                           	<unknown>
    5adb: 0e                           	<unknown>
    5adc: 0e                           	<unknown>
    5add: 0e                           	<unknown>
    5ade: 0e                           	<unknown>
    5adf: 0e                           	<unknown>
    5ae0: 0a 0e                        	orb	(%rsi), %cl
    5ae2: 0e                           	<unknown>
    5ae3: 0e                           	<unknown>
    5ae4: 0e                           	<unknown>
    5ae5: 0e                           	<unknown>
    5ae6: 0e                           	<unknown>
    5ae7: 0e                           	<unknown>
    5ae8: 0e                           	<unknown>
    5ae9: 0e                           	<unknown>
    5aea: 0e                           	<unknown>
    5aeb: 0e                           	<unknown>
    5aec: 0e                           	<unknown>
    5aed: 0b 0e                        	orl	(%rsi), %ecx
    5aef: 0e                           	<unknown>
    5af0: 0e                           	<unknown>
    5af1: 0e                           	<unknown>
    5af2: 0e                           	<unknown>
    5af3: 0e                           	<unknown>
    5af4: 0e                           	<unknown>
    5af5: 0e                           	<unknown>
    5af6: 0e                           	<unknown>
    5af7: 0e                           	<unknown>
    5af8: 0e                           	<unknown>
    5af9: 0e                           	<unknown>
    5afa: 0e                           	<unknown>
    5afb: 0e                           	<unknown>
    5afc: 0e                           	<unknown>
    5afd: 0e                           	<unknown>
    5afe: 0e                           	<unknown>
    5aff: 0e                           	<unknown>
    5b00: 0e                           	<unknown>
    5b01: 0e                           	<unknown>
    5b02: 0e                           	<unknown>
    5b03: 0e                           	<unknown>
    5b04: 0c 0e                        	orb	$0xe, %al
    5b06: 0e                           	<unknown>
    5b07: 0e                           	<unknown>
    5b08: 0e                           	<unknown>
    5b09: 0e                           	<unknown>
    5b0a: 0e                           	<unknown>
    5b0b: 0e                           	<unknown>
    5b0c: 0e                           	<unknown>
    5b0d: 0e                           	<unknown>
    5b0e: 0e                           	<unknown>
    5b0f: 0e                           	<unknown>
    5b10: 0e                           	<unknown>
    5b11: 0e                           	<unknown>
    5b12: 0e                           	<unknown>
    5b13: 0e                           	<unknown>
    5b14: 0e                           	<unknown>
    5b15: 0e                           	<unknown>
    5b16: 0e                           	<unknown>
    5b17: 0e                           	<unknown>
    5b18: 0e                           	<unknown>
    5b19: 0e                           	<unknown>
    5b1a: 0e                           	<unknown>
    5b1b: 0e                           	<unknown>
    5b1c: 0e                           	<unknown>
    5b1d: 0d cc cc cc cc               	orl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    5b22: cc                           	int3
    5b23: cc                           	int3
    5b24: cc                           	int3
    5b25: cc                           	int3
    5b26: cc                           	int3
    5b27: cc                           	int3
    5b28: cc                           	int3
    5b29: cc                           	int3
    5b2a: cc                           	int3
    5b2b: cc                           	int3
    5b2c: cc                           	int3
    5b2d: cc                           	int3
    5b2e: cc                           	int3
    5b2f: cc                           	int3

0000000000005b30 <c0_last_io_error>:
    5b30: 40 57                        	pushq	%rdi
    5b32: 48 83 ec 30                  	subq	$0x30, %rsp
    5b36: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x5b3c <c0_last_io_error+0xc>
    5b3c: 89 44 24 20                  	movl	%eax, 0x20(%rsp)
    5b40: 83 7c 24 20 00               	cmpl	$0x0, 0x20(%rsp)
    5b45: 75 04                        	jne	0x5b4b <c0_last_io_error+0x1b>
    5b47: b0 05                        	movb	$0x5, %al
    5b49: eb 09                        	jmp	0x5b54 <c0_last_io_error+0x24>
    5b4b: 8b 4c 24 20                  	movl	0x20(%rsp), %ecx
    5b4f: e8 00 00 00 00               	callq	0x5b54 <c0_last_io_error+0x24>
    5b54: 48 83 c4 30                  	addq	$0x30, %rsp
    5b58: 5f                           	popq	%rdi
    5b59: c3                           	retq
    5b5a: cc                           	int3
    5b5b: cc                           	int3
    5b5c: cc                           	int3
    5b5d: cc                           	int3
    5b5e: cc                           	int3
    5b5f: cc                           	int3

0000000000005b60 <c0_fs_state>:
    5b60: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    5b65: 57                           	pushq	%rdi
    5b66: 48 83 7c 24 10 00            	cmpq	$0x0, 0x10(%rsp)
    5b6c: 75 04                        	jne	0x5b72 <c0_fs_state+0x12>
    5b6e: 33 c0                        	xorl	%eax, %eax
    5b70: eb 08                        	jmp	0x5b7a <c0_fs_state+0x1a>
    5b72: 48 8b 44 24 10               	movq	0x10(%rsp), %rax
    5b77: 48 8b 00                     	movq	(%rax), %rax
    5b7a: 5f                           	popq	%rdi
    5b7b: c3                           	retq
    5b7c: cc                           	int3
    5b7d: cc                           	int3
    5b7e: cc                           	int3
    5b7f: cc                           	int3
    5b80: cc                           	int3
    5b81: cc                           	int3
    5b82: cc                           	int3
    5b83: cc                           	int3
    5b84: cc                           	int3
    5b85: cc                           	int3
    5b86: cc                           	int3
    5b87: cc                           	int3
    5b88: cc                           	int3
    5b89: cc                           	int3
    5b8a: cc                           	int3
    5b8b: cc                           	int3
    5b8c: cc                           	int3
    5b8d: cc                           	int3
    5b8e: cc                           	int3
    5b8f: cc                           	int3

0000000000005b90 <c0_fs_resolve_path>:
    5b90: 4c 89 4c 24 20               	movq	%r9, 0x20(%rsp)
    5b95: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    5b9a: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    5b9f: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    5ba4: 57                           	pushq	%rdi
    5ba5: 48 81 ec a0 00 00 00         	subq	$0xa0, %rsp
    5bac: 48 8d 7c 24 30               	leaq	0x30(%rsp), %rdi
    5bb1: b9 1c 00 00 00               	movl	$0x1c, %ecx
    5bb6: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    5bbb: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    5bbd: 48 8b 8c 24 b0 00 00 00      	movq	0xb0(%rsp), %rcx
    5bc5: 48 83 bc 24 c0 00 00 00 00   	cmpq	$0x0, 0xc0(%rsp)
    5bce: 74 0f                        	je	0x5bdf <c0_fs_resolve_path+0x4f>
    5bd0: 48 8b 84 24 c0 00 00 00      	movq	0xc0(%rsp), %rax
    5bd8: 48 c7 00 00 00 00 00         	movq	$0x0, (%rax)
    5bdf: 48 83 bc 24 c8 00 00 00 00   	cmpq	$0x0, 0xc8(%rsp)
    5be8: 74 0e                        	je	0x5bf8 <c0_fs_resolve_path+0x68>
    5bea: 48 8b 84 24 c8 00 00 00      	movq	0xc8(%rsp), %rax
    5bf2: c7 00 00 00 00 00            	movl	$0x0, (%rax)
    5bf8: 48 83 bc 24 b8 00 00 00 00   	cmpq	$0x0, 0xb8(%rsp)
    5c01: 75 07                        	jne	0x5c0a <c0_fs_resolve_path+0x7a>
    5c03: 33 c0                        	xorl	%eax, %eax
    5c05: e9 d2 02 00 00               	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5c0a: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5c12: 48 83 38 00                  	cmpq	$0x0, (%rax)
    5c16: 75 16                        	jne	0x5c2e <c0_fs_resolve_path+0x9e>
    5c18: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5c20: 48 83 78 08 00               	cmpq	$0x0, 0x8(%rax)
    5c25: 74 07                        	je	0x5c2e <c0_fs_resolve_path+0x9e>
    5c27: 33 c0                        	xorl	%eax, %eax
    5c29: e9 ae 02 00 00               	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5c2e: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5c36: 48 8b 50 08                  	movq	0x8(%rax), %rdx
    5c3a: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5c42: 48 8b 08                     	movq	(%rax), %rcx
    5c45: e8 00 00 00 00               	callq	0x5c4a <c0_fs_resolve_path+0xba>
    5c4a: 85 c0                        	testl	%eax, %eax
    5c4c: 75 07                        	jne	0x5c55 <c0_fs_resolve_path+0xc5>
    5c4e: 33 c0                        	xorl	%eax, %eax
    5c50: e9 87 02 00 00               	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5c55: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5c5d: 48 8b 50 08                  	movq	0x8(%rax), %rdx
    5c61: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5c69: 48 8b 08                     	movq	(%rax), %rcx
    5c6c: e8 00 00 00 00               	callq	0x5c71 <c0_fs_resolve_path+0xe1>
    5c71: 85 c0                        	testl	%eax, %eax
    5c73: 74 07                        	je	0x5c7c <c0_fs_resolve_path+0xec>
    5c75: 33 c0                        	xorl	%eax, %eax
    5c77: e9 60 02 00 00               	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5c7c: 48 83 bc 24 b0 00 00 00 00   	cmpq	$0x0, 0xb0(%rsp)
    5c85: 0f 84 d5 01 00 00            	je	0x5e60 <c0_fs_resolve_path+0x2d0>
    5c8b: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    5c93: 83 78 0c 00                  	cmpl	$0x0, 0xc(%rax)
    5c97: 0f 84 c3 01 00 00            	je	0x5e60 <c0_fs_resolve_path+0x2d0>
    5c9d: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    5ca5: 83 78 10 00                  	cmpl	$0x0, 0x10(%rax)
    5ca9: 75 07                        	jne	0x5cb2 <c0_fs_resolve_path+0x122>
    5cab: 33 c0                        	xorl	%eax, %eax
    5cad: e9 2a 02 00 00               	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5cb2: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5cba: 48 8b 50 08                  	movq	0x8(%rax), %rdx
    5cbe: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5cc6: 48 8b 08                     	movq	(%rax), %rcx
    5cc9: e8 00 00 00 00               	callq	0x5cce <c0_fs_resolve_path+0x13e>
    5cce: 85 c0                        	testl	%eax, %eax
    5cd0: 74 07                        	je	0x5cd9 <c0_fs_resolve_path+0x149>
    5cd2: 33 c0                        	xorl	%eax, %eax
    5cd4: e9 03 02 00 00               	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5cd9: c7 44 24 34 00 00 00 00      	movl	$0x0, 0x34(%rsp)
    5ce1: 48 8d 44 24 34               	leaq	0x34(%rsp), %rax
    5ce6: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    5ceb: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5cf3: 4c 8b 48 08                  	movq	0x8(%rax), %r9
    5cf7: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5cff: 4c 8b 00                     	movq	(%rax), %r8
    5d02: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    5d0a: 8b 50 08                     	movl	0x8(%rax), %edx
    5d0d: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    5d15: 48 8b 08                     	movq	(%rax), %rcx
    5d18: e8 00 00 00 00               	callq	0x5d1d <c0_fs_resolve_path+0x18d>
    5d1d: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    5d22: 48 83 7c 24 48 00            	cmpq	$0x0, 0x48(%rsp)
    5d28: 75 24                        	jne	0x5d4e <c0_fs_resolve_path+0x1be>
    5d2a: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    5d32: 83 78 08 00                  	cmpl	$0x0, 0x8(%rax)
    5d36: 75 0f                        	jne	0x5d47 <c0_fs_resolve_path+0x1b7>
    5d38: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5d40: 48 83 78 08 00               	cmpq	$0x0, 0x8(%rax)
    5d45: 74 07                        	je	0x5d4e <c0_fs_resolve_path+0x1be>
    5d47: 33 c0                        	xorl	%eax, %eax
    5d49: e9 8e 01 00 00               	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5d4e: c7 44 24 54 00 00 00 00      	movl	$0x0, 0x54(%rsp)
    5d56: 48 83 7c 24 48 00            	cmpq	$0x0, 0x48(%rsp)
    5d5c: 74 0e                        	je	0x5d6c <c0_fs_resolve_path+0x1dc>
    5d5e: 8b 44 24 34                  	movl	0x34(%rsp), %eax
    5d62: 48 89 84 24 90 00 00 00      	movq	%rax, 0x90(%rsp)
    5d6a: eb 14                        	jmp	0x5d80 <c0_fs_resolve_path+0x1f0>
    5d6c: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5d74: 48 8b 40 08                  	movq	0x8(%rax), %rax
    5d78: 48 89 84 24 90 00 00 00      	movq	%rax, 0x90(%rsp)
    5d80: 48 83 7c 24 48 00            	cmpq	$0x0, 0x48(%rsp)
    5d86: 74 0f                        	je	0x5d97 <c0_fs_resolve_path+0x207>
    5d88: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    5d8d: 48 89 84 24 98 00 00 00      	movq	%rax, 0x98(%rsp)
    5d95: eb 13                        	jmp	0x5daa <c0_fs_resolve_path+0x21a>
    5d97: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5d9f: 48 8b 00                     	movq	(%rax), %rax
    5da2: 48 89 84 24 98 00 00 00      	movq	%rax, 0x98(%rsp)
    5daa: 4c 8d 44 24 54               	leaq	0x54(%rsp), %r8
    5daf: 48 8b 94 24 90 00 00 00      	movq	0x90(%rsp), %rdx
    5db7: 48 8b 8c 24 98 00 00 00      	movq	0x98(%rsp), %rcx
    5dbf: e8 00 00 00 00               	callq	0x5dc4 <c0_fs_resolve_path+0x234>
    5dc4: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    5dc9: 48 83 7c 24 48 00            	cmpq	$0x0, 0x48(%rsp)
    5dcf: 74 0b                        	je	0x5ddc <c0_fs_resolve_path+0x24c>
    5dd1: 48 8b 4c 24 48               	movq	0x48(%rsp), %rcx
    5dd6: e8 00 00 00 00               	callq	0x5ddb <c0_fs_resolve_path+0x24b>
    5ddb: 90                           	nop
    5ddc: 48 83 7c 24 68 00            	cmpq	$0x0, 0x68(%rsp)
    5de2: 75 07                        	jne	0x5deb <c0_fs_resolve_path+0x25b>
    5de4: 33 c0                        	xorl	%eax, %eax
    5de6: e9 f1 00 00 00               	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5deb: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    5df3: 44 8b 48 08                  	movl	0x8(%rax), %r9d
    5df7: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    5dff: 4c 8b 00                     	movq	(%rax), %r8
    5e02: 8b 54 24 54                  	movl	0x54(%rsp), %edx
    5e06: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    5e0b: e8 00 00 00 00               	callq	0x5e10 <c0_fs_resolve_path+0x280>
    5e10: 85 c0                        	testl	%eax, %eax
    5e12: 75 11                        	jne	0x5e25 <c0_fs_resolve_path+0x295>
    5e14: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    5e19: e8 00 00 00 00               	callq	0x5e1e <c0_fs_resolve_path+0x28e>
    5e1e: 33 c0                        	xorl	%eax, %eax
    5e20: e9 b7 00 00 00               	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5e25: 48 83 bc 24 c0 00 00 00 00   	cmpq	$0x0, 0xc0(%rsp)
    5e2e: 74 10                        	je	0x5e40 <c0_fs_resolve_path+0x2b0>
    5e30: 48 8b 84 24 c0 00 00 00      	movq	0xc0(%rsp), %rax
    5e38: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    5e3d: 48 89 08                     	movq	%rcx, (%rax)
    5e40: 48 83 bc 24 c8 00 00 00 00   	cmpq	$0x0, 0xc8(%rsp)
    5e49: 74 0e                        	je	0x5e59 <c0_fs_resolve_path+0x2c9>
    5e4b: 48 8b 84 24 c8 00 00 00      	movq	0xc8(%rsp), %rax
    5e53: 8b 4c 24 54                  	movl	0x54(%rsp), %ecx
    5e57: 89 08                        	movl	%ecx, (%rax)
    5e59: b8 01 00 00 00               	movl	$0x1, %eax
    5e5e: eb 7c                        	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5e60: c7 44 24 74 00 00 00 00      	movl	$0x0, 0x74(%rsp)
    5e68: 4c 8d 44 24 74               	leaq	0x74(%rsp), %r8
    5e6d: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5e75: 48 8b 50 08                  	movq	0x8(%rax), %rdx
    5e79: 48 8b 84 24 b8 00 00 00      	movq	0xb8(%rsp), %rax
    5e81: 48 8b 08                     	movq	(%rax), %rcx
    5e84: e8 00 00 00 00               	callq	0x5e89 <c0_fs_resolve_path+0x2f9>
    5e89: 48 89 84 24 88 00 00 00      	movq	%rax, 0x88(%rsp)
    5e91: 48 83 bc 24 88 00 00 00 00   	cmpq	$0x0, 0x88(%rsp)
    5e9a: 75 04                        	jne	0x5ea0 <c0_fs_resolve_path+0x310>
    5e9c: 33 c0                        	xorl	%eax, %eax
    5e9e: eb 3c                        	jmp	0x5edc <c0_fs_resolve_path+0x34c>
    5ea0: 48 83 bc 24 c0 00 00 00 00   	cmpq	$0x0, 0xc0(%rsp)
    5ea9: 74 13                        	je	0x5ebe <c0_fs_resolve_path+0x32e>
    5eab: 48 8b 84 24 c0 00 00 00      	movq	0xc0(%rsp), %rax
    5eb3: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    5ebb: 48 89 08                     	movq	%rcx, (%rax)
    5ebe: 48 83 bc 24 c8 00 00 00 00   	cmpq	$0x0, 0xc8(%rsp)
    5ec7: 74 0e                        	je	0x5ed7 <c0_fs_resolve_path+0x347>
    5ec9: 48 8b 84 24 c8 00 00 00      	movq	0xc8(%rsp), %rax
    5ed1: 8b 4c 24 74                  	movl	0x74(%rsp), %ecx
    5ed5: 89 08                        	movl	%ecx, (%rax)
    5ed7: b8 01 00 00 00               	movl	$0x1, %eax
    5edc: 48 8b f8                     	movq	%rax, %rdi
    5edf: 48 8b cc                     	movq	%rsp, %rcx
    5ee2: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x5ee9 <c0_fs_resolve_path+0x359>
    5ee9: e8 00 00 00 00               	callq	0x5eee <c0_fs_resolve_path+0x35e>
    5eee: 48 8b c7                     	movq	%rdi, %rax
    5ef1: 48 81 c4 a0 00 00 00         	addq	$0xa0, %rsp
    5ef8: 5f                           	popq	%rdi
    5ef9: c3                           	retq
    5efa: cc                           	int3
    5efb: cc                           	int3
    5efc: cc                           	int3
    5efd: cc                           	int3
    5efe: cc                           	int3
    5eff: cc                           	int3

0000000000005f00 <c0_file_err>:
    5f00: 88 54 24 10                  	movb	%dl, 0x10(%rsp)
    5f04: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    5f09: 56                           	pushq	%rsi
    5f0a: 57                           	pushq	%rdi
    5f0b: 48 83 ec 58                  	subq	$0x58, %rsp
    5f0f: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    5f14: b9 0e 00 00 00               	movl	$0xe, %ecx
    5f19: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    5f1e: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    5f20: 48 8b 4c 24 70               	movq	0x70(%rsp), %rcx
    5f25: c6 44 24 28 00               	movb	$0x0, 0x28(%rsp)
    5f2a: 0f b6 44 24 78               	movzbl	0x78(%rsp), %eax
    5f2f: 88 44 24 30                  	movb	%al, 0x30(%rsp)
    5f33: 48 8d 44 24 28               	leaq	0x28(%rsp), %rax
    5f38: 48 8b 7c 24 70               	movq	0x70(%rsp), %rdi
    5f3d: 48 8b f0                     	movq	%rax, %rsi
    5f40: b9 10 00 00 00               	movl	$0x10, %ecx
    5f45: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    5f47: 48 8b 44 24 70               	movq	0x70(%rsp), %rax
    5f4c: 48 8b f8                     	movq	%rax, %rdi
    5f4f: 48 8b cc                     	movq	%rsp, %rcx
    5f52: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x5f59 <c0_file_err+0x59>
    5f59: e8 00 00 00 00               	callq	0x5f5e <c0_file_err+0x5e>
    5f5e: 48 8b c7                     	movq	%rdi, %rax
    5f61: 48 83 c4 58                  	addq	$0x58, %rsp
    5f65: 5f                           	popq	%rdi
    5f66: 5e                           	popq	%rsi
    5f67: c3                           	retq
    5f68: cc                           	int3
    5f69: cc                           	int3
    5f6a: cc                           	int3
    5f6b: cc                           	int3
    5f6c: cc                           	int3
    5f6d: cc                           	int3
    5f6e: cc                           	int3
    5f6f: cc                           	int3

0000000000005f70 <c0_dir_err>:
    5f70: 88 54 24 10                  	movb	%dl, 0x10(%rsp)
    5f74: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    5f79: 56                           	pushq	%rsi
    5f7a: 57                           	pushq	%rdi
    5f7b: 48 83 ec 58                  	subq	$0x58, %rsp
    5f7f: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    5f84: b9 0e 00 00 00               	movl	$0xe, %ecx
    5f89: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    5f8e: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    5f90: 48 8b 4c 24 70               	movq	0x70(%rsp), %rcx
    5f95: c6 44 24 28 00               	movb	$0x0, 0x28(%rsp)
    5f9a: 0f b6 44 24 78               	movzbl	0x78(%rsp), %eax
    5f9f: 88 44 24 30                  	movb	%al, 0x30(%rsp)
    5fa3: 0f b6 54 24 28               	movzbl	0x28(%rsp), %edx
    5fa8: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x5faf <c0_dir_err+0x3f>
    5faf: e8 00 00 00 00               	callq	0x5fb4 <c0_dir_err+0x44>
    5fb4: 48 8d 44 24 28               	leaq	0x28(%rsp), %rax
    5fb9: 48 8b 7c 24 70               	movq	0x70(%rsp), %rdi
    5fbe: 48 8b f0                     	movq	%rax, %rsi
    5fc1: b9 10 00 00 00               	movl	$0x10, %ecx
    5fc6: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    5fc8: 48 8b 44 24 70               	movq	0x70(%rsp), %rax
    5fcd: 48 8b f8                     	movq	%rax, %rdi
    5fd0: 48 8b cc                     	movq	%rsp, %rcx
    5fd3: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x5fda <c0_dir_err+0x6a>
    5fda: e8 00 00 00 00               	callq	0x5fdf <c0_dir_err+0x6f>
    5fdf: 48 8b c7                     	movq	%rdi, %rax
    5fe2: 48 83 c4 58                  	addq	$0x58, %rsp
    5fe6: 5f                           	popq	%rdi
    5fe7: 5e                           	popq	%rsi
    5fe8: c3                           	retq
    5fe9: cc                           	int3
    5fea: cc                           	int3
    5feb: cc                           	int3
    5fec: cc                           	int3
    5fed: cc                           	int3
    5fee: cc                           	int3
    5fef: cc                           	int3

0000000000005ff0 <c0_unit_err>:
    5ff0: 88 4c 24 08                  	movb	%cl, 0x8(%rsp)
    5ff4: 57                           	pushq	%rdi
    5ff5: 48 83 ec 40                  	subq	$0x40, %rsp
    5ff9: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    5ffe: b9 08 00 00 00               	movl	$0x8, %ecx
    6003: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    6008: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    600a: 0f b6 4c 24 50               	movzbl	0x50(%rsp), %ecx
    600f: c6 44 24 24 01               	movb	$0x1, 0x24(%rsp)
    6014: 0f b6 44 24 50               	movzbl	0x50(%rsp), %eax
    6019: 88 44 24 25                  	movb	%al, 0x25(%rsp)
    601d: 0f b7 44 24 24               	movzwl	0x24(%rsp), %eax
    6022: 8b f8                        	movl	%eax, %edi
    6024: 48 8b cc                     	movq	%rsp, %rcx
    6027: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x602e <c0_unit_err+0x3e>
    602e: e8 00 00 00 00               	callq	0x6033 <c0_unit_err+0x43>
    6033: 8b c7                        	movl	%edi, %eax
    6035: 48 83 c4 40                  	addq	$0x40, %rsp
    6039: 5f                           	popq	%rdi
    603a: c3                           	retq
    603b: cc                           	int3
    603c: cc                           	int3
    603d: cc                           	int3
    603e: cc                           	int3
    603f: cc                           	int3
    6040: cc                           	int3
    6041: cc                           	int3
    6042: cc                           	int3
    6043: cc                           	int3
    6044: cc                           	int3
    6045: cc                           	int3
    6046: cc                           	int3
    6047: cc                           	int3
    6048: cc                           	int3
    6049: cc                           	int3
    604a: cc                           	int3
    604b: cc                           	int3
    604c: cc                           	int3
    604d: cc                           	int3
    604e: cc                           	int3
    604f: cc                           	int3

0000000000006050 <c0_unit_ok>:
    6050: 40 57                        	pushq	%rdi
    6052: 48 83 ec 40                  	subq	$0x40, %rsp
    6056: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    605b: b9 08 00 00 00               	movl	$0x8, %ecx
    6060: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    6065: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    6067: c6 44 24 24 00               	movb	$0x0, 0x24(%rsp)
    606c: c6 44 24 25 00               	movb	$0x0, 0x25(%rsp)
    6071: 0f b7 44 24 24               	movzwl	0x24(%rsp), %eax
    6076: 8b f8                        	movl	%eax, %edi
    6078: 48 8b cc                     	movq	%rsp, %rcx
    607b: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x6082 <c0_unit_ok+0x32>
    6082: e8 00 00 00 00               	callq	0x6087 <c0_unit_ok+0x37>
    6087: 8b c7                        	movl	%edi, %eax
    6089: 48 83 c4 40                  	addq	$0x40, %rsp
    608d: 5f                           	popq	%rdi
    608e: c3                           	retq
    608f: cc                           	int3
    6090: cc                           	int3
    6091: cc                           	int3
    6092: cc                           	int3
    6093: cc                           	int3
    6094: cc                           	int3
    6095: cc                           	int3
    6096: cc                           	int3
    6097: cc                           	int3
    6098: cc                           	int3
    6099: cc                           	int3
    609a: cc                           	int3
    609b: cc                           	int3
    609c: cc                           	int3
    609d: cc                           	int3
    609e: cc                           	int3
    609f: cc                           	int3

00000000000060a0 <c0_kind_err>:
    60a0: 88 4c 24 08                  	movb	%cl, 0x8(%rsp)
    60a4: 57                           	pushq	%rdi
    60a5: 48 83 ec 40                  	subq	$0x40, %rsp
    60a9: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    60ae: b9 08 00 00 00               	movl	$0x8, %ecx
    60b3: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    60b8: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    60ba: 0f b6 4c 24 50               	movzbl	0x50(%rsp), %ecx
    60bf: c6 44 24 24 01               	movb	$0x1, 0x24(%rsp)
    60c4: 0f b6 44 24 50               	movzbl	0x50(%rsp), %eax
    60c9: 88 44 24 25                  	movb	%al, 0x25(%rsp)
    60cd: 0f b7 44 24 24               	movzwl	0x24(%rsp), %eax
    60d2: 8b f8                        	movl	%eax, %edi
    60d4: 48 8b cc                     	movq	%rsp, %rcx
    60d7: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x60de <c0_kind_err+0x3e>
    60de: e8 00 00 00 00               	callq	0x60e3 <c0_kind_err+0x43>
    60e3: 8b c7                        	movl	%edi, %eax
    60e5: 48 83 c4 40                  	addq	$0x40, %rsp
    60e9: 5f                           	popq	%rdi
    60ea: c3                           	retq
    60eb: cc                           	int3
    60ec: cc                           	int3
    60ed: cc                           	int3
    60ee: cc                           	int3
    60ef: cc                           	int3
    60f0: cc                           	int3
    60f1: cc                           	int3
    60f2: cc                           	int3
    60f3: cc                           	int3
    60f4: cc                           	int3
    60f5: cc                           	int3
    60f6: cc                           	int3
    60f7: cc                           	int3
    60f8: cc                           	int3
    60f9: cc                           	int3
    60fa: cc                           	int3
    60fb: cc                           	int3
    60fc: cc                           	int3
    60fd: cc                           	int3
    60fe: cc                           	int3
    60ff: cc                           	int3

0000000000006100 <c0_kind_ok>:
    6100: 88 4c 24 08                  	movb	%cl, 0x8(%rsp)
    6104: 57                           	pushq	%rdi
    6105: 48 83 ec 40                  	subq	$0x40, %rsp
    6109: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    610e: b9 08 00 00 00               	movl	$0x8, %ecx
    6113: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    6118: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    611a: 0f b6 4c 24 50               	movzbl	0x50(%rsp), %ecx
    611f: c6 44 24 24 00               	movb	$0x0, 0x24(%rsp)
    6124: 0f b6 44 24 50               	movzbl	0x50(%rsp), %eax
    6129: 88 44 24 25                  	movb	%al, 0x25(%rsp)
    612d: 0f b7 44 24 24               	movzwl	0x24(%rsp), %eax
    6132: 8b f8                        	movl	%eax, %edi
    6134: 48 8b cc                     	movq	%rsp, %rcx
    6137: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x613e <c0_kind_ok+0x3e>
    613e: e8 00 00 00 00               	callq	0x6143 <c0_kind_ok+0x43>
    6143: 8b c7                        	movl	%edi, %eax
    6145: 48 83 c4 40                  	addq	$0x40, %rsp
    6149: 5f                           	popq	%rdi
    614a: c3                           	retq
    614b: cc                           	int3
    614c: cc                           	int3
    614d: cc                           	int3
    614e: cc                           	int3
    614f: cc                           	int3
    6150: cc                           	int3
    6151: cc                           	int3
    6152: cc                           	int3
    6153: cc                           	int3
    6154: cc                           	int3
    6155: cc                           	int3
    6156: cc                           	int3
    6157: cc                           	int3
    6158: cc                           	int3
    6159: cc                           	int3
    615a: cc                           	int3
    615b: cc                           	int3
    615c: cc                           	int3
    615d: cc                           	int3
    615e: cc                           	int3
    615f: cc                           	int3

0000000000006160 <c0_string_io_err>:
    6160: 88 54 24 10                  	movb	%dl, 0x10(%rsp)
    6164: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    6169: 56                           	pushq	%rsi
    616a: 57                           	pushq	%rdi
    616b: 48 83 ec 68                  	subq	$0x68, %rsp
    616f: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    6174: b9 12 00 00 00               	movl	$0x12, %ecx
    6179: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    617e: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    6180: 48 8b 8c 24 80 00 00 00      	movq	0x80(%rsp), %rcx
    6188: c6 44 24 28 00               	movb	$0x0, 0x28(%rsp)
    618d: 0f b6 84 24 88 00 00 00      	movzbl	0x88(%rsp), %eax
    6195: 88 44 24 30                  	movb	%al, 0x30(%rsp)
    6199: 48 8d 44 24 28               	leaq	0x28(%rsp), %rax
    619e: 48 8b bc 24 80 00 00 00      	movq	0x80(%rsp), %rdi
    61a6: 48 8b f0                     	movq	%rax, %rsi
    61a9: b9 20 00 00 00               	movl	$0x20, %ecx
    61ae: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    61b0: 48 8b 84 24 80 00 00 00      	movq	0x80(%rsp), %rax
    61b8: 48 8b f8                     	movq	%rax, %rdi
    61bb: 48 8b cc                     	movq	%rsp, %rcx
    61be: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x61c5 <c0_string_io_err+0x65>
    61c5: e8 00 00 00 00               	callq	0x61ca <c0_string_io_err+0x6a>
    61ca: 48 8b c7                     	movq	%rdi, %rax
    61cd: 48 83 c4 68                  	addq	$0x68, %rsp
    61d1: 5f                           	popq	%rdi
    61d2: 5e                           	popq	%rsi
    61d3: c3                           	retq
    61d4: cc                           	int3
    61d5: cc                           	int3
    61d6: cc                           	int3
    61d7: cc                           	int3
    61d8: cc                           	int3
    61d9: cc                           	int3
    61da: cc                           	int3
    61db: cc                           	int3
    61dc: cc                           	int3
    61dd: cc                           	int3
    61de: cc                           	int3
    61df: cc                           	int3

00000000000061e0 <c0_bytes_io_err>:
    61e0: 88 54 24 10                  	movb	%dl, 0x10(%rsp)
    61e4: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    61e9: 56                           	pushq	%rsi
    61ea: 57                           	pushq	%rdi
    61eb: 48 83 ec 68                  	subq	$0x68, %rsp
    61ef: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    61f4: b9 12 00 00 00               	movl	$0x12, %ecx
    61f9: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    61fe: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    6200: 48 8b 8c 24 80 00 00 00      	movq	0x80(%rsp), %rcx
    6208: c6 44 24 28 00               	movb	$0x0, 0x28(%rsp)
    620d: 0f b6 84 24 88 00 00 00      	movzbl	0x88(%rsp), %eax
    6215: 88 44 24 30                  	movb	%al, 0x30(%rsp)
    6219: 48 8d 44 24 28               	leaq	0x28(%rsp), %rax
    621e: 48 8b bc 24 80 00 00 00      	movq	0x80(%rsp), %rdi
    6226: 48 8b f0                     	movq	%rax, %rsi
    6229: b9 20 00 00 00               	movl	$0x20, %ecx
    622e: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    6230: 48 8b 84 24 80 00 00 00      	movq	0x80(%rsp), %rax
    6238: 48 8b f8                     	movq	%rax, %rdi
    623b: 48 8b cc                     	movq	%rsp, %rcx
    623e: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x6245 <c0_bytes_io_err+0x65>
    6245: e8 00 00 00 00               	callq	0x624a <c0_bytes_io_err+0x6a>
    624a: 48 8b c7                     	movq	%rdi, %rax
    624d: 48 83 c4 68                  	addq	$0x68, %rsp
    6251: 5f                           	popq	%rdi
    6252: 5e                           	popq	%rsi
    6253: c3                           	retq
    6254: cc                           	int3
    6255: cc                           	int3
    6256: cc                           	int3
    6257: cc                           	int3
    6258: cc                           	int3
    6259: cc                           	int3
    625a: cc                           	int3
    625b: cc                           	int3
    625c: cc                           	int3
    625d: cc                           	int3
    625e: cc                           	int3
    625f: cc                           	int3

0000000000006260 <c0_read_all_bytes_handle>:
    6260: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    6265: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    626a: 56                           	pushq	%rsi
    626b: 57                           	pushq	%rdi
    626c: 48 81 ec e8 01 00 00         	subq	$0x1e8, %rsp            # imm = 0x1E8
    6273: 48 8d 7c 24 30               	leaq	0x30(%rsp), %rdi
    6278: b9 6e 00 00 00               	movl	$0x6e, %ecx
    627d: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    6282: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    6284: 48 8b 8c 24 00 02 00 00      	movq	0x200(%rsp), %rcx
    628c: 48 83 bc 24 08 02 00 00 00   	cmpq	$0x0, 0x208(%rsp)
    6295: 74 0b                        	je	0x62a2 <c0_read_all_bytes_handle+0x42>
    6297: 48 83 bc 24 08 02 00 00 ff   	cmpq	$-0x1, 0x208(%rsp)
    62a0: 75 2e                        	jne	0x62d0 <c0_read_all_bytes_handle+0x70>
    62a2: b2 05                        	movb	$0x5, %dl
    62a4: 48 8d 8c 24 f8 00 00 00      	leaq	0xf8(%rsp), %rcx
    62ac: e8 00 00 00 00               	callq	0x62b1 <c0_read_all_bytes_handle+0x51>
    62b1: 48 8b bc 24 00 02 00 00      	movq	0x200(%rsp), %rdi
    62b9: 48 8b f0                     	movq	%rax, %rsi
    62bc: b9 20 00 00 00               	movl	$0x20, %ecx
    62c1: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    62c3: 48 8b 84 24 00 02 00 00      	movq	0x200(%rsp), %rax
    62cb: e9 fe 02 00 00               	jmp	0x65ce <c0_read_all_bytes_handle+0x36e>
    62d0: 48 8d 54 24 38               	leaq	0x38(%rsp), %rdx
    62d5: 48 8b 8c 24 08 02 00 00      	movq	0x208(%rsp), %rcx
    62dd: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x62e3 <c0_read_all_bytes_handle+0x83>
    62e3: 85 c0                        	testl	%eax, %eax
    62e5: 75 34                        	jne	0x631b <c0_read_all_bytes_handle+0xbb>
    62e7: e8 00 00 00 00               	callq	0x62ec <c0_read_all_bytes_handle+0x8c>
    62ec: 0f b6 d0                     	movzbl	%al, %edx
    62ef: 48 8d 8c 24 18 01 00 00      	leaq	0x118(%rsp), %rcx
    62f7: e8 00 00 00 00               	callq	0x62fc <c0_read_all_bytes_handle+0x9c>
    62fc: 48 8b bc 24 00 02 00 00      	movq	0x200(%rsp), %rdi
    6304: 48 8b f0                     	movq	%rax, %rsi
    6307: b9 20 00 00 00               	movl	$0x20, %ecx
    630c: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    630e: 48 8b 84 24 00 02 00 00      	movq	0x200(%rsp), %rax
    6316: e9 b3 02 00 00               	jmp	0x65ce <c0_read_all_bytes_handle+0x36e>
    631b: 48 83 7c 24 38 00            	cmpq	$0x0, 0x38(%rsp)
    6321: 7d 2e                        	jge	0x6351 <c0_read_all_bytes_handle+0xf1>
    6323: b2 05                        	movb	$0x5, %dl
    6325: 48 8d 8c 24 38 01 00 00      	leaq	0x138(%rsp), %rcx
    632d: e8 00 00 00 00               	callq	0x6332 <c0_read_all_bytes_handle+0xd2>
    6332: 48 8b bc 24 00 02 00 00      	movq	0x200(%rsp), %rdi
    633a: 48 8b f0                     	movq	%rax, %rsi
    633d: b9 20 00 00 00               	movl	$0x20, %ecx
    6342: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    6344: 48 8b 84 24 00 02 00 00      	movq	0x200(%rsp), %rax
    634c: e9 7d 02 00 00               	jmp	0x65ce <c0_read_all_bytes_handle+0x36e>
    6351: 48 83 7c 24 38 ff            	cmpq	$-0x1, 0x38(%rsp)
    6357: 76 2e                        	jbe	0x6387 <c0_read_all_bytes_handle+0x127>
    6359: b2 05                        	movb	$0x5, %dl
    635b: 48 8d 8c 24 58 01 00 00      	leaq	0x158(%rsp), %rcx
    6363: e8 00 00 00 00               	callq	0x6368 <c0_read_all_bytes_handle+0x108>
    6368: 48 8b bc 24 00 02 00 00      	movq	0x200(%rsp), %rdi
    6370: 48 8b f0                     	movq	%rax, %rsi
    6373: b9 20 00 00 00               	movl	$0x20, %ecx
    6378: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    637a: 48 8b 84 24 00 02 00 00      	movq	0x200(%rsp), %rax
    6382: e9 47 02 00 00               	jmp	0x65ce <c0_read_all_bytes_handle+0x36e>
    6387: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    638c: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    6391: 48 83 7c 24 48 00            	cmpq	$0x0, 0x48(%rsp)
    6397: 75 44                        	jne	0x63dd <c0_read_all_bytes_handle+0x17d>
    6399: c6 44 24 58 01               	movb	$0x1, 0x58(%rsp)
    639e: 48 c7 44 24 60 00 00 00 00   	movq	$0x0, 0x60(%rsp)
    63a7: 48 c7 44 24 68 00 00 00 00   	movq	$0x0, 0x68(%rsp)
    63b0: 48 c7 44 24 70 00 00 00 00   	movq	$0x0, 0x70(%rsp)
    63b9: 48 8d 44 24 58               	leaq	0x58(%rsp), %rax
    63be: 48 8b bc 24 00 02 00 00      	movq	0x200(%rsp), %rdi
    63c6: 48 8b f0                     	movq	%rax, %rsi
    63c9: b9 20 00 00 00               	movl	$0x20, %ecx
    63ce: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    63d0: 48 8b 84 24 00 02 00 00      	movq	0x200(%rsp), %rax
    63d8: e9 f1 01 00 00               	jmp	0x65ce <c0_read_all_bytes_handle+0x36e>
    63dd: 45 33 c0                     	xorl	%r8d, %r8d
    63e0: 48 8b 54 24 48               	movq	0x48(%rsp), %rdx
    63e5: 33 c9                        	xorl	%ecx, %ecx
    63e7: e8 00 00 00 00               	callq	0x63ec <c0_read_all_bytes_handle+0x18c>
    63ec: 48 89 84 24 88 00 00 00      	movq	%rax, 0x88(%rsp)
    63f4: 48 83 bc 24 88 00 00 00 00   	cmpq	$0x0, 0x88(%rsp)
    63fd: 75 2e                        	jne	0x642d <c0_read_all_bytes_handle+0x1cd>
    63ff: b2 05                        	movb	$0x5, %dl
    6401: 48 8d 8c 24 78 01 00 00      	leaq	0x178(%rsp), %rcx
    6409: e8 00 00 00 00               	callq	0x640e <c0_read_all_bytes_handle+0x1ae>
    640e: 48 8b bc 24 00 02 00 00      	movq	0x200(%rsp), %rdi
    6416: 48 8b f0                     	movq	%rax, %rsi
    6419: b9 20 00 00 00               	movl	$0x20, %ecx
    641e: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    6420: 48 8b 84 24 00 02 00 00      	movq	0x200(%rsp), %rax
    6428: e9 a1 01 00 00               	jmp	0x65ce <c0_read_all_bytes_handle+0x36e>
    642d: 48 c7 84 24 90 00 00 00 00 00 00 00  	movq	$0x0, 0x90(%rsp)
    6439: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    643e: 48 39 84 24 90 00 00 00      	cmpq	%rax, 0x90(%rsp)
    6446: 0f 83 e6 00 00 00            	jae	0x6532 <c0_read_all_bytes_handle+0x2d2>
    644c: c7 84 24 a4 00 00 00 00 00 00 00     	movl	$0x0, 0xa4(%rsp)
    6457: 48 8b 84 24 90 00 00 00      	movq	0x90(%rsp), %rax
    645f: 48 8b 4c 24 48               	movq	0x48(%rsp), %rcx
    6464: 48 2b c8                     	subq	%rax, %rcx
    6467: 48 8b c1                     	movq	%rcx, %rax
    646a: ba ff ff ff 7f               	movl	$0x7fffffff, %edx       # imm = 0x7FFFFFFF
    646f: 48 8b c8                     	movq	%rax, %rcx
    6472: e8 00 00 00 00               	callq	0x6477 <c0_read_all_bytes_handle+0x217>
    6477: 89 84 24 b4 00 00 00         	movl	%eax, 0xb4(%rsp)
    647e: 48 8b 84 24 90 00 00 00      	movq	0x90(%rsp), %rax
    6486: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    648e: 48 03 c8                     	addq	%rax, %rcx
    6491: 48 8b c1                     	movq	%rcx, %rax
    6494: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
    649d: 4c 8d 8c 24 a4 00 00 00      	leaq	0xa4(%rsp), %r9
    64a5: 44 8b 84 24 b4 00 00 00      	movl	0xb4(%rsp), %r8d
    64ad: 48 8b d0                     	movq	%rax, %rdx
    64b0: 48 8b 8c 24 08 02 00 00      	movq	0x208(%rsp), %rcx
    64b8: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x64be <c0_read_all_bytes_handle+0x25e>
    64be: 85 c0                        	testl	%eax, %eax
    64c0: 75 42                        	jne	0x6504 <c0_read_all_bytes_handle+0x2a4>
    64c2: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    64ca: e8 00 00 00 00               	callq	0x64cf <c0_read_all_bytes_handle+0x26f>
    64cf: 90                           	nop
    64d0: e8 00 00 00 00               	callq	0x64d5 <c0_read_all_bytes_handle+0x275>
    64d5: 0f b6 d0                     	movzbl	%al, %edx
    64d8: 48 8d 8c 24 98 01 00 00      	leaq	0x198(%rsp), %rcx
    64e0: e8 00 00 00 00               	callq	0x64e5 <c0_read_all_bytes_handle+0x285>
    64e5: 48 8b bc 24 00 02 00 00      	movq	0x200(%rsp), %rdi
    64ed: 48 8b f0                     	movq	%rax, %rsi
    64f0: b9 20 00 00 00               	movl	$0x20, %ecx
    64f5: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    64f7: 48 8b 84 24 00 02 00 00      	movq	0x200(%rsp), %rax
    64ff: e9 ca 00 00 00               	jmp	0x65ce <c0_read_all_bytes_handle+0x36e>
    6504: 83 bc 24 a4 00 00 00 00      	cmpl	$0x0, 0xa4(%rsp)
    650c: 75 02                        	jne	0x6510 <c0_read_all_bytes_handle+0x2b0>
    650e: eb 22                        	jmp	0x6532 <c0_read_all_bytes_handle+0x2d2>
    6510: 8b 84 24 a4 00 00 00         	movl	0xa4(%rsp), %eax
    6517: 48 8b 8c 24 90 00 00 00      	movq	0x90(%rsp), %rcx
    651f: 48 03 c8                     	addq	%rax, %rcx
    6522: 48 8b c1                     	movq	%rcx, %rax
    6525: 48 89 84 24 90 00 00 00      	movq	%rax, 0x90(%rsp)
    652d: e9 07 ff ff ff               	jmp	0x6439 <c0_read_all_bytes_handle+0x1d9>
    6532: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    6537: 48 39 84 24 90 00 00 00      	cmpq	%rax, 0x90(%rsp)
    653f: 74 39                        	je	0x657a <c0_read_all_bytes_handle+0x31a>
    6541: 48 8b 8c 24 88 00 00 00      	movq	0x88(%rsp), %rcx
    6549: e8 00 00 00 00               	callq	0x654e <c0_read_all_bytes_handle+0x2ee>
    654e: 90                           	nop
    654f: b2 05                        	movb	$0x5, %dl
    6551: 48 8d 8c 24 b8 01 00 00      	leaq	0x1b8(%rsp), %rcx
    6559: e8 00 00 00 00               	callq	0x655e <c0_read_all_bytes_handle+0x2fe>
    655e: 48 8b bc 24 00 02 00 00      	movq	0x200(%rsp), %rdi
    6566: 48 8b f0                     	movq	%rax, %rsi
    6569: b9 20 00 00 00               	movl	$0x20, %ecx
    656e: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    6570: 48 8b 84 24 00 02 00 00      	movq	0x200(%rsp), %rax
    6578: eb 54                        	jmp	0x65ce <c0_read_all_bytes_handle+0x36e>
    657a: c6 84 24 c8 00 00 00 01      	movb	$0x1, 0xc8(%rsp)
    6582: 48 8b 84 24 88 00 00 00      	movq	0x88(%rsp), %rax
    658a: 48 89 84 24 d0 00 00 00      	movq	%rax, 0xd0(%rsp)
    6592: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    6597: 48 89 84 24 d8 00 00 00      	movq	%rax, 0xd8(%rsp)
    659f: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    65a4: 48 89 84 24 e0 00 00 00      	movq	%rax, 0xe0(%rsp)
    65ac: 48 8d 84 24 c8 00 00 00      	leaq	0xc8(%rsp), %rax
    65b4: 48 8b bc 24 00 02 00 00      	movq	0x200(%rsp), %rdi
    65bc: 48 8b f0                     	movq	%rax, %rsi
    65bf: b9 20 00 00 00               	movl	$0x20, %ecx
    65c4: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    65c6: 48 8b 84 24 00 02 00 00      	movq	0x200(%rsp), %rax
    65ce: 48 8b f8                     	movq	%rax, %rdi
    65d1: 48 8b cc                     	movq	%rsp, %rcx
    65d4: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x65db <c0_read_all_bytes_handle+0x37b>
    65db: e8 00 00 00 00               	callq	0x65e0 <c0_read_all_bytes_handle+0x380>
    65e0: 48 8b c7                     	movq	%rdi, %rax
    65e3: 48 81 c4 e8 01 00 00         	addq	$0x1e8, %rsp            # imm = 0x1E8
    65ea: 5f                           	popq	%rdi
    65eb: 5e                           	popq	%rsi
    65ec: c3                           	retq
    65ed: cc                           	int3
    65ee: cc                           	int3
    65ef: cc                           	int3
    65f0: cc                           	int3
    65f1: cc                           	int3
    65f2: cc                           	int3
    65f3: cc                           	int3
    65f4: cc                           	int3
    65f5: cc                           	int3
    65f6: cc                           	int3
    65f7: cc                           	int3
    65f8: cc                           	int3
    65f9: cc                           	int3
    65fa: cc                           	int3
    65fb: cc                           	int3
    65fc: cc                           	int3
    65fd: cc                           	int3
    65fe: cc                           	int3
    65ff: cc                           	int3

0000000000006600 <c0_read_all_string_handle>:
    6600: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    6605: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    660a: 56                           	pushq	%rsi
    660b: 57                           	pushq	%rdi
    660c: 48 81 ec 08 01 00 00         	subq	$0x108, %rsp            # imm = 0x108
    6613: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    6618: b9 3a 00 00 00               	movl	$0x3a, %ecx
    661d: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    6622: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    6624: 48 8b 8c 24 20 01 00 00      	movq	0x120(%rsp), %rcx
    662c: 48 8b 94 24 28 01 00 00      	movq	0x128(%rsp), %rdx
    6634: 48 8d 8c 24 98 00 00 00      	leaq	0x98(%rsp), %rcx
    663c: e8 00 00 00 00               	callq	0x6641 <c0_read_all_string_handle+0x41>
    6641: 48 8d 4c 24 28               	leaq	0x28(%rsp), %rcx
    6646: 48 8b f9                     	movq	%rcx, %rdi
    6649: 48 8b f0                     	movq	%rax, %rsi
    664c: b9 20 00 00 00               	movl	$0x20, %ecx
    6651: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    6653: 0f b6 44 24 28               	movzbl	0x28(%rsp), %eax
    6658: 85 c0                        	testl	%eax, %eax
    665a: 75 31                        	jne	0x668d <c0_read_all_string_handle+0x8d>
    665c: 0f b6 54 24 30               	movzbl	0x30(%rsp), %edx
    6661: 48 8d 8c 24 b8 00 00 00      	leaq	0xb8(%rsp), %rcx
    6669: e8 00 00 00 00               	callq	0x666e <c0_read_all_string_handle+0x6e>
    666e: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
    6676: 48 8b f0                     	movq	%rax, %rsi
    6679: b9 20 00 00 00               	movl	$0x20, %ecx
    667e: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    6680: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
    6688: e9 8e 00 00 00               	jmp	0x671b <c0_read_all_string_handle+0x11b>
    668d: 48 8b 54 24 38               	movq	0x38(%rsp), %rdx
    6692: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
    6697: e8 00 00 00 00               	callq	0x669c <c0_read_all_string_handle+0x9c>
    669c: 85 c0                        	testl	%eax, %eax
    669e: 75 36                        	jne	0x66d6 <c0_read_all_string_handle+0xd6>
    66a0: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
    66a5: e8 00 00 00 00               	callq	0x66aa <c0_read_all_string_handle+0xaa>
    66aa: 90                           	nop
    66ab: b2 05                        	movb	$0x5, %dl
    66ad: 48 8d 8c 24 d8 00 00 00      	leaq	0xd8(%rsp), %rcx
    66b5: e8 00 00 00 00               	callq	0x66ba <c0_read_all_string_handle+0xba>
    66ba: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
    66c2: 48 8b f0                     	movq	%rax, %rsi
    66c5: b9 20 00 00 00               	movl	$0x20, %ecx
    66ca: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    66cc: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
    66d4: eb 45                        	jmp	0x671b <c0_read_all_string_handle+0x11b>
    66d6: c6 44 24 68 01               	movb	$0x1, 0x68(%rsp)
    66db: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
    66e0: 48 89 44 24 70               	movq	%rax, 0x70(%rsp)
    66e5: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    66ea: 48 89 44 24 78               	movq	%rax, 0x78(%rsp)
    66ef: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    66f4: 48 89 84 24 80 00 00 00      	movq	%rax, 0x80(%rsp)
    66fc: 48 8d 44 24 68               	leaq	0x68(%rsp), %rax
    6701: 48 8b bc 24 20 01 00 00      	movq	0x120(%rsp), %rdi
    6709: 48 8b f0                     	movq	%rax, %rsi
    670c: b9 20 00 00 00               	movl	$0x20, %ecx
    6711: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    6713: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
    671b: 48 8b f8                     	movq	%rax, %rdi
    671e: 48 8b cc                     	movq	%rsp, %rcx
    6721: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x6728 <c0_read_all_string_handle+0x128>
    6728: e8 00 00 00 00               	callq	0x672d <c0_read_all_string_handle+0x12d>
    672d: 48 8b c7                     	movq	%rdi, %rax
    6730: 48 81 c4 08 01 00 00         	addq	$0x108, %rsp            # imm = 0x108
    6737: 5f                           	popq	%rdi
    6738: 5e                           	popq	%rsi
    6739: c3                           	retq
    673a: cc                           	int3
    673b: cc                           	int3
    673c: cc                           	int3
    673d: cc                           	int3
    673e: cc                           	int3
    673f: cc                           	int3

0000000000006740 <c0_entry_key_utf8>:
    6740: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    6745: 89 54 24 10                  	movl	%edx, 0x10(%rsp)
    6749: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    674e: 57                           	pushq	%rdi
    674f: 48 81 ec a0 00 00 00         	subq	$0xa0, %rsp
    6756: 48 8d 7c 24 30               	leaq	0x30(%rsp), %rdi
    675b: b9 1c 00 00 00               	movl	$0x1c, %ecx
    6760: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    6765: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    6767: 48 8b 8c 24 b0 00 00 00      	movq	0xb0(%rsp), %rcx
    676f: 48 83 bc 24 c0 00 00 00 00   	cmpq	$0x0, 0xc0(%rsp)
    6778: 74 0e                        	je	0x6788 <c0_entry_key_utf8+0x48>
    677a: 48 8b 84 24 c0 00 00 00      	movq	0xc0(%rsp), %rax
    6782: c7 00 00 00 00 00            	movl	$0x0, (%rax)
    6788: 48 83 bc 24 b0 00 00 00 00   	cmpq	$0x0, 0xb0(%rsp)
    6791: 74 0a                        	je	0x679d <c0_entry_key_utf8+0x5d>
    6793: 83 bc 24 b8 00 00 00 00      	cmpl	$0x0, 0xb8(%rsp)
    679b: 75 07                        	jne	0x67a4 <c0_entry_key_utf8+0x64>
    679d: 33 c0                        	xorl	%eax, %eax
    679f: e9 68 02 00 00               	jmp	0x6a0c <c0_entry_key_utf8+0x2cc>
    67a4: c7 44 24 20 00 00 00 00      	movl	$0x0, 0x20(%rsp)
    67ac: 45 33 c9                     	xorl	%r9d, %r9d
    67af: 44 8b 84 24 b8 00 00 00      	movl	0xb8(%rsp), %r8d
    67b7: 48 8b 94 24 b0 00 00 00      	movq	0xb0(%rsp), %rdx
    67bf: b9 20 00 00 00               	movl	$0x20, %ecx
    67c4: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x67ca <c0_entry_key_utf8+0x8a>
    67ca: 89 44 24 30                  	movl	%eax, 0x30(%rsp)
    67ce: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    67d7: 8b 84 24 b8 00 00 00         	movl	0xb8(%rsp), %eax
    67de: 89 44 24 40                  	movl	%eax, 0x40(%rsp)
    67e2: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    67ea: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    67ef: 83 7c 24 30 00               	cmpl	$0x0, 0x30(%rsp)
    67f4: 0f 8e a6 00 00 00            	jle	0x68a0 <c0_entry_key_utf8+0x160>
    67fa: 8b 44 24 30                  	movl	0x30(%rsp), %eax
    67fe: ff c0                        	incl	%eax
    6800: 48 98                        	cltq
    6802: 48 d1 e0                     	shlq	%rax
    6805: 48 8b c8                     	movq	%rax, %rcx
    6808: e8 00 00 00 00               	callq	0x680d <c0_entry_key_utf8+0xcd>
    680d: 48 89 44 24 38               	movq	%rax, 0x38(%rsp)
    6812: 48 83 7c 24 38 00            	cmpq	$0x0, 0x38(%rsp)
    6818: 0f 84 82 00 00 00            	je	0x68a0 <c0_entry_key_utf8+0x160>
    681e: 8b 44 24 30                  	movl	0x30(%rsp), %eax
    6822: 89 44 24 20                  	movl	%eax, 0x20(%rsp)
    6826: 4c 8b 4c 24 38               	movq	0x38(%rsp), %r9
    682b: 44 8b 84 24 b8 00 00 00      	movl	0xb8(%rsp), %r8d
    6833: 48 8b 94 24 b0 00 00 00      	movq	0xb0(%rsp), %rdx
    683b: b9 20 00 00 00               	movl	$0x20, %ecx
    6840: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x6846 <c0_entry_key_utf8+0x106>
    6846: 89 44 24 50                  	movl	%eax, 0x50(%rsp)
    684a: 83 7c 24 50 00               	cmpl	$0x0, 0x50(%rsp)
    684f: 7e 24                        	jle	0x6875 <c0_entry_key_utf8+0x135>
    6851: 48 63 44 24 50               	movslq	0x50(%rsp), %rax
    6856: 33 c9                        	xorl	%ecx, %ecx
    6858: 48 8b 54 24 38               	movq	0x38(%rsp), %rdx
    685d: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    6861: 8b 44 24 50                  	movl	0x50(%rsp), %eax
    6865: 89 44 24 40                  	movl	%eax, 0x40(%rsp)
    6869: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    686e: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    6873: eb 2b                        	jmp	0x68a0 <c0_entry_key_utf8+0x160>
    6875: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    687a: e8 00 00 00 00               	callq	0x687f <c0_entry_key_utf8+0x13f>
    687f: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    6888: 48 8b 84 24 b0 00 00 00      	movq	0xb0(%rsp), %rax
    6890: 48 89 44 24 48               	movq	%rax, 0x48(%rsp)
    6895: 8b 84 24 b8 00 00 00         	movl	0xb8(%rsp), %eax
    689c: 89 44 24 40                  	movl	%eax, 0x40(%rsp)
    68a0: c7 44 24 28 00 00 00 00      	movl	$0x0, 0x28(%rsp)
    68a8: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
    68b1: 44 8b 4c 24 40               	movl	0x40(%rsp), %r9d
    68b6: 4c 8b 44 24 48               	movq	0x48(%rsp), %r8
    68bb: ba 00 01 00 00               	movl	$0x100, %edx            # imm = 0x100
    68c0: b9 7f 00 00 00               	movl	$0x7f, %ecx
    68c5: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x68cb <c0_entry_key_utf8+0x18b>
    68cb: 89 44 24 54                  	movl	%eax, 0x54(%rsp)
    68cf: 48 c7 44 24 58 00 00 00 00   	movq	$0x0, 0x58(%rsp)
    68d8: 8b 44 24 40                  	movl	0x40(%rsp), %eax
    68dc: 89 44 24 60                  	movl	%eax, 0x60(%rsp)
    68e0: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    68e5: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    68ea: 83 7c 24 54 00               	cmpl	$0x0, 0x54(%rsp)
    68ef: 0f 8e a4 00 00 00            	jle	0x6999 <c0_entry_key_utf8+0x259>
    68f5: 8b 44 24 54                  	movl	0x54(%rsp), %eax
    68f9: ff c0                        	incl	%eax
    68fb: 48 98                        	cltq
    68fd: 48 d1 e0                     	shlq	%rax
    6900: 48 8b c8                     	movq	%rax, %rcx
    6903: e8 00 00 00 00               	callq	0x6908 <c0_entry_key_utf8+0x1c8>
    6908: 48 89 44 24 58               	movq	%rax, 0x58(%rsp)
    690d: 48 83 7c 24 58 00            	cmpq	$0x0, 0x58(%rsp)
    6913: 0f 84 80 00 00 00            	je	0x6999 <c0_entry_key_utf8+0x259>
    6919: 8b 44 24 54                  	movl	0x54(%rsp), %eax
    691d: 89 44 24 28                  	movl	%eax, 0x28(%rsp)
    6921: 48 8b 44 24 58               	movq	0x58(%rsp), %rax
    6926: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    692b: 44 8b 4c 24 40               	movl	0x40(%rsp), %r9d
    6930: 4c 8b 44 24 48               	movq	0x48(%rsp), %r8
    6935: ba 00 01 00 00               	movl	$0x100, %edx            # imm = 0x100
    693a: b9 7f 00 00 00               	movl	$0x7f, %ecx
    693f: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x6945 <c0_entry_key_utf8+0x205>
    6945: 89 44 24 70                  	movl	%eax, 0x70(%rsp)
    6949: 83 7c 24 70 00               	cmpl	$0x0, 0x70(%rsp)
    694e: 7e 24                        	jle	0x6974 <c0_entry_key_utf8+0x234>
    6950: 48 63 44 24 70               	movslq	0x70(%rsp), %rax
    6955: 33 c9                        	xorl	%ecx, %ecx
    6957: 48 8b 54 24 58               	movq	0x58(%rsp), %rdx
    695c: 66 89 0c 42                  	movw	%cx, (%rdx,%rax,2)
    6960: 8b 44 24 70                  	movl	0x70(%rsp), %eax
    6964: 89 44 24 60                  	movl	%eax, 0x60(%rsp)
    6968: 48 8b 44 24 58               	movq	0x58(%rsp), %rax
    696d: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    6972: eb 25                        	jmp	0x6999 <c0_entry_key_utf8+0x259>
    6974: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    6979: e8 00 00 00 00               	callq	0x697e <c0_entry_key_utf8+0x23e>
    697e: 48 c7 44 24 58 00 00 00 00   	movq	$0x0, 0x58(%rsp)
    6987: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    698c: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    6991: 8b 44 24 40                  	movl	0x40(%rsp), %eax
    6995: 89 44 24 60                  	movl	%eax, 0x60(%rsp)
    6999: c7 84 24 84 00 00 00 00 00 00 00     	movl	$0x0, 0x84(%rsp)
    69a4: 4c 8d 84 24 84 00 00 00      	leaq	0x84(%rsp), %r8
    69ac: 8b 54 24 60                  	movl	0x60(%rsp), %edx
    69b0: 48 8b 4c 24 68               	movq	0x68(%rsp), %rcx
    69b5: e8 00 00 00 00               	callq	0x69ba <c0_entry_key_utf8+0x27a>
    69ba: 48 89 84 24 98 00 00 00      	movq	%rax, 0x98(%rsp)
    69c2: 48 83 7c 24 38 00            	cmpq	$0x0, 0x38(%rsp)
    69c8: 74 0b                        	je	0x69d5 <c0_entry_key_utf8+0x295>
    69ca: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    69cf: e8 00 00 00 00               	callq	0x69d4 <c0_entry_key_utf8+0x294>
    69d4: 90                           	nop
    69d5: 48 83 7c 24 58 00            	cmpq	$0x0, 0x58(%rsp)
    69db: 74 0b                        	je	0x69e8 <c0_entry_key_utf8+0x2a8>
    69dd: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
    69e2: e8 00 00 00 00               	callq	0x69e7 <c0_entry_key_utf8+0x2a7>
    69e7: 90                           	nop
    69e8: 48 83 bc 24 c0 00 00 00 00   	cmpq	$0x0, 0xc0(%rsp)
    69f1: 74 11                        	je	0x6a04 <c0_entry_key_utf8+0x2c4>
    69f3: 48 8b 84 24 c0 00 00 00      	movq	0xc0(%rsp), %rax
    69fb: 8b 8c 24 84 00 00 00         	movl	0x84(%rsp), %ecx
    6a02: 89 08                        	movl	%ecx, (%rax)
    6a04: 48 8b 84 24 98 00 00 00      	movq	0x98(%rsp), %rax
    6a0c: 48 8b f8                     	movq	%rax, %rdi
    6a0f: 48 8b cc                     	movq	%rsp, %rcx
    6a12: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x6a19 <c0_entry_key_utf8+0x2d9>
    6a19: e8 00 00 00 00               	callq	0x6a1e <c0_entry_key_utf8+0x2de>
    6a1e: 48 8b c7                     	movq	%rdi, %rax
    6a21: 48 81 c4 a0 00 00 00         	addq	$0xa0, %rsp
    6a28: 5f                           	popq	%rdi
    6a29: c3                           	retq
    6a2a: cc                           	int3
    6a2b: cc                           	int3
    6a2c: cc                           	int3
    6a2d: cc                           	int3
    6a2e: cc                           	int3
    6a2f: cc                           	int3

0000000000006a30 <c0_lex_bytes>:
    6a30: 44 89 4c 24 20               	movl	%r9d, 0x20(%rsp)
    6a35: 4c 89 44 24 18               	movq	%r8, 0x18(%rsp)
    6a3a: 89 54 24 10                  	movl	%edx, 0x10(%rsp)
    6a3e: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    6a43: 57                           	pushq	%rdi
    6a44: 48 83 ec 10                  	subq	$0x10, %rsp
    6a48: 8b 44 24 38                  	movl	0x38(%rsp), %eax
    6a4c: 39 44 24 28                  	cmpl	%eax, 0x28(%rsp)
    6a50: 73 0a                        	jae	0x6a5c <c0_lex_bytes+0x2c>
    6a52: 8b 44 24 28                  	movl	0x28(%rsp), %eax
    6a56: 89 44 24 08                  	movl	%eax, 0x8(%rsp)
    6a5a: eb 08                        	jmp	0x6a64 <c0_lex_bytes+0x34>
    6a5c: 8b 44 24 38                  	movl	0x38(%rsp), %eax
    6a60: 89 44 24 08                  	movl	%eax, 0x8(%rsp)
    6a64: 8b 44 24 08                  	movl	0x8(%rsp), %eax
    6a68: 89 04 24                     	movl	%eax, (%rsp)
    6a6b: c7 44 24 04 00 00 00 00      	movl	$0x0, 0x4(%rsp)
    6a73: eb 0a                        	jmp	0x6a7f <c0_lex_bytes+0x4f>
    6a75: 8b 44 24 04                  	movl	0x4(%rsp), %eax
    6a79: ff c0                        	incl	%eax
    6a7b: 89 44 24 04                  	movl	%eax, 0x4(%rsp)
    6a7f: 8b 04 24                     	movl	(%rsp), %eax
    6a82: 39 44 24 04                  	cmpl	%eax, 0x4(%rsp)
    6a86: 73 56                        	jae	0x6ade <c0_lex_bytes+0xae>
    6a88: 8b 44 24 04                  	movl	0x4(%rsp), %eax
    6a8c: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    6a91: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    6a95: 8b 4c 24 04                  	movl	0x4(%rsp), %ecx
    6a99: 48 8b 54 24 30               	movq	0x30(%rsp), %rdx
    6a9e: 0f b6 0c 0a                  	movzbl	(%rdx,%rcx), %ecx
    6aa2: 3b c1                        	cmpl	%ecx, %eax
    6aa4: 74 36                        	je	0x6adc <c0_lex_bytes+0xac>
    6aa6: 8b 44 24 04                  	movl	0x4(%rsp), %eax
    6aaa: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    6aaf: 0f b6 04 01                  	movzbl	(%rcx,%rax), %eax
    6ab3: 8b 4c 24 04                  	movl	0x4(%rsp), %ecx
    6ab7: 48 8b 54 24 30               	movq	0x30(%rsp), %rdx
    6abc: 0f b6 0c 0a                  	movzbl	(%rdx,%rcx), %ecx
    6ac0: 3b c1                        	cmpl	%ecx, %eax
    6ac2: 7d 0a                        	jge	0x6ace <c0_lex_bytes+0x9e>
    6ac4: c7 44 24 0c ff ff ff ff      	movl	$0xffffffff, 0xc(%rsp)  # imm = 0xFFFFFFFF
    6acc: eb 08                        	jmp	0x6ad6 <c0_lex_bytes+0xa6>
    6ace: c7 44 24 0c 01 00 00 00      	movl	$0x1, 0xc(%rsp)
    6ad6: 8b 44 24 0c                  	movl	0xc(%rsp), %eax
    6ada: eb 26                        	jmp	0x6b02 <c0_lex_bytes+0xd2>
    6adc: eb 97                        	jmp	0x6a75 <c0_lex_bytes+0x45>
    6ade: 8b 44 24 38                  	movl	0x38(%rsp), %eax
    6ae2: 39 44 24 28                  	cmpl	%eax, 0x28(%rsp)
    6ae6: 73 07                        	jae	0x6aef <c0_lex_bytes+0xbf>
    6ae8: b8 ff ff ff ff               	movl	$0xffffffff, %eax       # imm = 0xFFFFFFFF
    6aed: eb 13                        	jmp	0x6b02 <c0_lex_bytes+0xd2>
    6aef: 8b 44 24 38                  	movl	0x38(%rsp), %eax
    6af3: 39 44 24 28                  	cmpl	%eax, 0x28(%rsp)
    6af7: 76 07                        	jbe	0x6b00 <c0_lex_bytes+0xd0>
    6af9: b8 01 00 00 00               	movl	$0x1, %eax
    6afe: eb 02                        	jmp	0x6b02 <c0_lex_bytes+0xd2>
    6b00: 33 c0                        	xorl	%eax, %eax
    6b02: 48 83 c4 10                  	addq	$0x10, %rsp
    6b06: 5f                           	popq	%rdi
    6b07: c3                           	retq
    6b08: cc                           	int3
    6b09: cc                           	int3
    6b0a: cc                           	int3
    6b0b: cc                           	int3
    6b0c: cc                           	int3
    6b0d: cc                           	int3
    6b0e: cc                           	int3
    6b0f: cc                           	int3

0000000000006b10 <c0_entry_cmp>:
    6b10: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    6b15: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    6b1a: 57                           	pushq	%rdi
    6b1b: 48 83 ec 30                  	subq	$0x30, %rsp
    6b1f: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    6b24: 44 8b 48 28                  	movl	0x28(%rax), %r9d
    6b28: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    6b2d: 4c 8b 40 20                  	movq	0x20(%rax), %r8
    6b31: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    6b36: 8b 50 28                     	movl	0x28(%rax), %edx
    6b39: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    6b3e: 48 8b 48 20                  	movq	0x20(%rax), %rcx
    6b42: e8 00 00 00 00               	callq	0x6b47 <c0_entry_cmp+0x37>
    6b47: 89 44 24 20                  	movl	%eax, 0x20(%rsp)
    6b4b: 83 7c 24 20 00               	cmpl	$0x0, 0x20(%rsp)
    6b50: 74 06                        	je	0x6b58 <c0_entry_cmp+0x48>
    6b52: 8b 44 24 20                  	movl	0x20(%rsp), %eax
    6b56: eb 28                        	jmp	0x6b80 <c0_entry_cmp+0x70>
    6b58: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    6b5d: 44 8b 48 18                  	movl	0x18(%rax), %r9d
    6b61: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    6b66: 4c 8b 40 10                  	movq	0x10(%rax), %r8
    6b6a: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    6b6f: 8b 50 18                     	movl	0x18(%rax), %edx
    6b72: 48 8b 44 24 40               	movq	0x40(%rsp), %rax
    6b77: 48 8b 48 10                  	movq	0x10(%rax), %rcx
    6b7b: e8 00 00 00 00               	callq	0x6b80 <c0_entry_cmp+0x70>
    6b80: 48 83 c4 30                  	addq	$0x30, %rsp
    6b84: 5f                           	popq	%rdi
    6b85: c3                           	retq
    6b86: cc                           	int3
    6b87: cc                           	int3
    6b88: cc                           	int3
    6b89: cc                           	int3
    6b8a: cc                           	int3
    6b8b: cc                           	int3
    6b8c: cc                           	int3
    6b8d: cc                           	int3
    6b8e: cc                           	int3
    6b8f: cc                           	int3

0000000000006b90 <c0_sort_entries>:
    6b90: 89 54 24 10                  	movl	%edx, 0x10(%rsp)
    6b94: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    6b99: 56                           	pushq	%rsi
    6b9a: 57                           	pushq	%rdi
    6b9b: 48 81 ec 88 00 00 00         	subq	$0x88, %rsp
    6ba2: 48 8d 7c 24 20               	leaq	0x20(%rsp), %rdi
    6ba7: b9 1a 00 00 00               	movl	$0x1a, %ecx
    6bac: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    6bb1: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    6bb3: 48 8b 8c 24 a0 00 00 00      	movq	0xa0(%rsp), %rcx
    6bbb: c7 44 24 20 01 00 00 00      	movl	$0x1, 0x20(%rsp)
    6bc3: eb 0a                        	jmp	0x6bcf <c0_sort_entries+0x3f>
    6bc5: 8b 44 24 20                  	movl	0x20(%rsp), %eax
    6bc9: ff c0                        	incl	%eax
    6bcb: 89 44 24 20                  	movl	%eax, 0x20(%rsp)
    6bcf: 8b 84 24 a8 00 00 00         	movl	0xa8(%rsp), %eax
    6bd6: 39 44 24 20                  	cmpl	%eax, 0x20(%rsp)
    6bda: 0f 83 ce 00 00 00            	jae	0x6cae <c0_sort_entries+0x11e>
    6be0: 8b 44 24 20                  	movl	0x20(%rsp), %eax
    6be4: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    6be8: 48 8d 4c 24 38               	leaq	0x38(%rsp), %rcx
    6bed: 48 8b 94 24 a0 00 00 00      	movq	0xa0(%rsp), %rdx
    6bf5: 48 8b f9                     	movq	%rcx, %rdi
    6bf8: 48 8d 34 02                  	leaq	(%rdx,%rax), %rsi
    6bfc: b9 30 00 00 00               	movl	$0x30, %ecx
    6c01: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    6c03: 8b 44 24 20                  	movl	0x20(%rsp), %eax
    6c07: 89 44 24 74                  	movl	%eax, 0x74(%rsp)
    6c0b: 83 7c 24 74 00               	cmpl	$0x0, 0x74(%rsp)
    6c10: 76 74                        	jbe	0x6c86 <c0_sort_entries+0xf6>
    6c12: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    6c16: ff c8                        	decl	%eax
    6c18: 8b c0                        	movl	%eax, %eax
    6c1a: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    6c1e: 48 8b 8c 24 a0 00 00 00      	movq	0xa0(%rsp), %rcx
    6c26: 48 03 c8                     	addq	%rax, %rcx
    6c29: 48 8b c1                     	movq	%rcx, %rax
    6c2c: 48 8b d0                     	movq	%rax, %rdx
    6c2f: 48 8d 4c 24 38               	leaq	0x38(%rsp), %rcx
    6c34: e8 00 00 00 00               	callq	0x6c39 <c0_sort_entries+0xa9>
    6c39: 85 c0                        	testl	%eax, %eax
    6c3b: 7d 49                        	jge	0x6c86 <c0_sort_entries+0xf6>
    6c3d: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    6c41: ff c8                        	decl	%eax
    6c43: 8b c0                        	movl	%eax, %eax
    6c45: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    6c49: 8b 4c 24 74                  	movl	0x74(%rsp), %ecx
    6c4d: 48 6b c9 30                  	imulq	$0x30, %rcx, %rcx
    6c51: 48 8b 94 24 a0 00 00 00      	movq	0xa0(%rsp), %rdx
    6c59: 48 8b bc 24 a0 00 00 00      	movq	0xa0(%rsp), %rdi
    6c61: 48 89 7c 24 78               	movq	%rdi, 0x78(%rsp)
    6c66: 48 8d 3c 0a                  	leaq	(%rdx,%rcx), %rdi
    6c6a: 48 8b 4c 24 78               	movq	0x78(%rsp), %rcx
    6c6f: 48 8d 34 01                  	leaq	(%rcx,%rax), %rsi
    6c73: b9 30 00 00 00               	movl	$0x30, %ecx
    6c78: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    6c7a: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    6c7e: ff c8                        	decl	%eax
    6c80: 89 44 24 74                  	movl	%eax, 0x74(%rsp)
    6c84: eb 85                        	jmp	0x6c0b <c0_sort_entries+0x7b>
    6c86: 8b 44 24 74                  	movl	0x74(%rsp), %eax
    6c8a: 48 6b c0 30                  	imulq	$0x30, %rax, %rax
    6c8e: 48 8b 8c 24 a0 00 00 00      	movq	0xa0(%rsp), %rcx
    6c96: 48 8d 54 24 38               	leaq	0x38(%rsp), %rdx
    6c9b: 48 8d 3c 01                  	leaq	(%rcx,%rax), %rdi
    6c9f: 48 8b f2                     	movq	%rdx, %rsi
    6ca2: b9 30 00 00 00               	movl	$0x30, %ecx
    6ca7: f3 a4                        	rep		movsb	(%rsi), %es:(%rdi)
    6ca9: e9 17 ff ff ff               	jmp	0x6bc5 <c0_sort_entries+0x35>
    6cae: 48 8b cc                     	movq	%rsp, %rcx
    6cb1: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x6cb8 <c0_sort_entries+0x128>
    6cb8: e8 00 00 00 00               	callq	0x6cbd <c0_sort_entries+0x12d>
    6cbd: 48 81 c4 88 00 00 00         	addq	$0x88, %rsp
    6cc4: 5f                           	popq	%rdi
    6cc5: 5e                           	popq	%rsi
    6cc6: c3                           	retq
    6cc7: cc                           	int3
    6cc8: cc                           	int3
    6cc9: cc                           	int3
    6cca: cc                           	int3
    6ccb: cc                           	int3
    6ccc: cc                           	int3
    6ccd: cc                           	int3
    6cce: cc                           	int3
    6ccf: cc                           	int3

0000000000006cd0 <c0_dir_state>:
    6cd0: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    6cd5: 57                           	pushq	%rdi
    6cd6: 48 83 7c 24 10 00            	cmpq	$0x0, 0x10(%rsp)
    6cdc: 75 04                        	jne	0x6ce2 <c0_dir_state+0x12>
    6cde: 33 c0                        	xorl	%eax, %eax
    6ce0: eb 05                        	jmp	0x6ce7 <c0_dir_state+0x17>
    6ce2: 48 8b 44 24 10               	movq	0x10(%rsp), %rax
    6ce7: 5f                           	popq	%rdi
    6ce8: c3                           	retq
    6ce9: cc                           	int3
    6cea: cc                           	int3
    6ceb: cc                           	int3
    6cec: cc                           	int3
    6ced: cc                           	int3
    6cee: cc                           	int3
    6cef: cc                           	int3

0000000000006cf0 <c0_file_write_handle>:
    6cf0: 48 89 54 24 10               	movq	%rdx, 0x10(%rsp)
    6cf5: 48 89 4c 24 08               	movq	%rcx, 0x8(%rsp)
    6cfa: 57                           	pushq	%rdi
    6cfb: 48 83 ec 60                  	subq	$0x60, %rsp
    6cff: 48 8d 7c 24 30               	leaq	0x30(%rsp), %rdi
    6d04: b9 0c 00 00 00               	movl	$0xc, %ecx
    6d09: b8 cc cc cc cc               	movl	$0xcccccccc, %eax       # imm = 0xCCCCCCCC
    6d0e: f3 ab                        	rep		stosl	%eax, %es:(%rdi)
    6d10: 48 8b 4c 24 70               	movq	0x70(%rsp), %rcx
    6d15: 48 83 7c 24 70 00            	cmpq	$0x0, 0x70(%rsp)
    6d1b: 74 08                        	je	0x6d25 <c0_file_write_handle+0x35>
    6d1d: 48 83 7c 24 70 ff            	cmpq	$-0x1, 0x70(%rsp)
    6d23: 75 0c                        	jne	0x6d31 <c0_file_write_handle+0x41>
    6d25: b1 05                        	movb	$0x5, %cl
    6d27: e8 00 00 00 00               	callq	0x6d2c <c0_file_write_handle+0x3c>
    6d2c: e9 ea 00 00 00               	jmp	0x6e1b <c0_file_write_handle+0x12b>
    6d31: 48 83 7c 24 78 00            	cmpq	$0x0, 0x78(%rsp)
    6d37: 74 10                        	je	0x6d49 <c0_file_write_handle+0x59>
    6d39: 48 8b 44 24 78               	movq	0x78(%rsp), %rax
    6d3e: 48 8b 40 08                  	movq	0x8(%rax), %rax
    6d42: 48 89 44 24 58               	movq	%rax, 0x58(%rsp)
    6d47: eb 09                        	jmp	0x6d52 <c0_file_write_handle+0x62>
    6d49: 48 c7 44 24 58 00 00 00 00   	movq	$0x0, 0x58(%rsp)
    6d52: 48 8b 44 24 58               	movq	0x58(%rsp), %rax
    6d57: 48 89 44 24 30               	movq	%rax, 0x30(%rsp)
    6d5c: 48 c7 44 24 38 00 00 00 00   	movq	$0x0, 0x38(%rsp)
    6d65: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
    6d6a: 48 39 44 24 38               	cmpq	%rax, 0x38(%rsp)
    6d6f: 0f 83 8c 00 00 00            	jae	0x6e01 <c0_file_write_handle+0x111>
    6d75: c7 44 24 44 00 00 00 00      	movl	$0x0, 0x44(%rsp)
    6d7d: 48 8b 44 24 38               	movq	0x38(%rsp), %rax
    6d82: 48 8b 4c 24 30               	movq	0x30(%rsp), %rcx
    6d87: 48 2b c8                     	subq	%rax, %rcx
    6d8a: 48 8b c1                     	movq	%rcx, %rax
    6d8d: ba ff ff ff 7f               	movl	$0x7fffffff, %edx       # imm = 0x7FFFFFFF
    6d92: 48 8b c8                     	movq	%rax, %rcx
    6d95: e8 00 00 00 00               	callq	0x6d9a <c0_file_write_handle+0xaa>
    6d9a: 89 44 24 54                  	movl	%eax, 0x54(%rsp)
    6d9e: 48 8b 44 24 78               	movq	0x78(%rsp), %rax
    6da3: 48 8b 00                     	movq	(%rax), %rax
    6da6: 48 03 44 24 38               	addq	0x38(%rsp), %rax
    6dab: 48 c7 44 24 20 00 00 00 00   	movq	$0x0, 0x20(%rsp)
    6db4: 4c 8d 4c 24 44               	leaq	0x44(%rsp), %r9
    6db9: 44 8b 44 24 54               	movl	0x54(%rsp), %r8d
    6dbe: 48 8b d0                     	movq	%rax, %rdx
    6dc1: 48 8b 4c 24 70               	movq	0x70(%rsp), %rcx
    6dc6: ff 15 00 00 00 00            	callq	*(%rip)                 # 0x6dcc <c0_file_write_handle+0xdc>
    6dcc: 85 c0                        	testl	%eax, %eax
    6dce: 75 0f                        	jne	0x6ddf <c0_file_write_handle+0xef>
    6dd0: e8 00 00 00 00               	callq	0x6dd5 <c0_file_write_handle+0xe5>
    6dd5: 0f b6 c8                     	movzbl	%al, %ecx
    6dd8: e8 00 00 00 00               	callq	0x6ddd <c0_file_write_handle+0xed>
    6ddd: eb 3c                        	jmp	0x6e1b <c0_file_write_handle+0x12b>
    6ddf: 83 7c 24 44 00               	cmpl	$0x0, 0x44(%rsp)
    6de4: 75 02                        	jne	0x6de8 <c0_file_write_handle+0xf8>
    6de6: eb 19                        	jmp	0x6e01 <c0_file_write_handle+0x111>
    6de8: 8b 44 24 44                  	movl	0x44(%rsp), %eax
    6dec: 48 8b 4c 24 38               	movq	0x38(%rsp), %rcx
    6df1: 48 03 c8                     	addq	%rax, %rcx
    6df4: 48 8b c1                     	movq	%rcx, %rax
    6df7: 48 89 44 24 38               	movq	%rax, 0x38(%rsp)
    6dfc: e9 64 ff ff ff               	jmp	0x6d65 <c0_file_write_handle+0x75>
    6e01: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
    6e06: 48 39 44 24 38               	cmpq	%rax, 0x38(%rsp)
    6e0b: 74 09                        	je	0x6e16 <c0_file_write_handle+0x126>
    6e0d: b1 05                        	movb	$0x5, %cl
    6e0f: e8 00 00 00 00               	callq	0x6e14 <c0_file_write_handle+0x124>
    6e14: eb 05                        	jmp	0x6e1b <c0_file_write_handle+0x12b>
    6e16: e8 00 00 00 00               	callq	0x6e1b <c0_file_write_handle+0x12b>
    6e1b: 48 8b f8                     	movq	%rax, %rdi
    6e1e: 48 8b cc                     	movq	%rsp, %rcx
    6e21: 48 8d 15 00 00 00 00         	leaq	(%rip), %rdx            # 0x6e28 <c0_file_write_handle+0x138>
    6e28: e8 00 00 00 00               	callq	0x6e2d <c0_file_write_handle+0x13d>
    6e2d: 48 8b c7                     	movq	%rdi, %rax
    6e30: 48 83 c4 60                  	addq	$0x60, %rsp
    6e34: 5f                           	popq	%rdi
    6e35: c3                           	retq


.\cursive-bootstrap\tmp_debug_fs_ops2\build\obj\fs_x5fops.obj:	file format coff-x86-64

Disassembly of section .text:

0000000000000000 <fs_x5fops_x3a_x3amain>:
       0: 56                           	pushq	%rsi
       1: 57                           	pushq	%rdi
       2: 55                           	pushq	%rbp
       3: 53                           	pushq	%rbx
       4: 48 81 ec c8 04 00 00         	subq	$0x4c8, %rsp            # imm = 0x4C8
       b: 48 89 ce                     	movq	%rcx, %rsi
       e: 48 89 54 24 20               	movq	%rdx, 0x20(%rsp)
      13: c6 02 00                     	movb	$0x0, (%rdx)
      16: c7 42 04 00 00 00 00         	movl	$0x0, 0x4(%rdx)
      1d: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
      22: e8 00 00 00 00               	callq	0x27 <fs_x5fops_x3a_x3amain+0x27>
		0000000000000023:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3afs_x5fops
      27: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
      2c: 80 38 00                     	cmpb	$0x0, (%rax)
      2f: 0f 85 4c 03 00 00            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
      35: 48 8d 05 00 00 00 00         	leaq	(%rip), %rax            # 0x3c <fs_x5fops_x3a_x3amain+0x3c>
		0000000000000038:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f2E3C6336DA5A2A44
      3c: 48 89 84 24 28 02 00 00      	movq	%rax, 0x228(%rsp)
      44: 48 c7 84 24 30 02 00 00 07 00 00 00  	movq	$0x7, 0x230(%rsp)
      50: 48 8d 05 00 00 00 00         	leaq	(%rip), %rax            # 0x57 <fs_x5fops_x3a_x3amain+0x57>
		0000000000000053:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f7BBDDA29D79BBFC6
      57: 48 89 84 24 88 02 00 00      	movq	%rax, 0x288(%rsp)
      5f: 48 c7 84 24 90 02 00 00 0d 00 00 00  	movq	$0xd, 0x290(%rsp)
      6b: 48 8b 84 24 28 02 00 00      	movq	0x228(%rsp), %rax
      73: 48 89 84 24 20 03 00 00      	movq	%rax, 0x320(%rsp)
      7b: 48 8b 84 24 30 02 00 00      	movq	0x230(%rsp), %rax
      83: 48 89 84 24 28 03 00 00      	movq	%rax, 0x328(%rsp)
      8b: 48 8d 94 24 20 03 00 00      	leaq	0x320(%rsp), %rdx
      93: 48 89 f1                     	movq	%rsi, %rcx
      96: e8 00 00 00 00               	callq	0x9b <fs_x5fops_x3a_x3amain+0x9b>
		0000000000000097:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aensure_x5fdir
      9b: 89 c7                        	movl	%eax, %edi
      9d: 89 d3                        	movl	%edx, %ebx
      9f: 88 44 24 28                  	movb	%al, 0x28(%rsp)
      a3: 88 54 24 29                  	movb	%dl, 0x29(%rsp)
      a7: 84 c0                        	testb	%al, %al
      a9: 74 75                        	je	0x120 <fs_x5fops_x3a_x3amain+0x120>
      ab: 80 7c 24 28 01               	cmpb	$0x1, 0x28(%rsp)
      b0: 0f 85 b4 12 00 00            	jne	0x136a <fs_x5fops_x3a_x3amain+0x136a>
      b6: 40 88 bc 24 a0 01 00 00      	movb	%dil, 0x1a0(%rsp)
      be: 88 9c 24 a1 01 00 00         	movb	%bl, 0x1a1(%rsp)
      c5: 88 5c 24 30                  	movb	%bl, 0x30(%rsp)
      c9: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
      ce: c6 00 00                     	movb	$0x0, (%rax)
      d1: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
      d8: 0f b6 44 24 30               	movzbl	0x30(%rsp), %eax
      dd: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
      e2: 88 44 24 38                  	movb	%al, 0x38(%rsp)
      e6: 48 8d 4c 24 38               	leaq	0x38(%rsp), %rcx
      eb: e8 00 00 00 00               	callq	0xf0 <fs_x5fops_x3a_x3amain+0xf0>
		00000000000000ec:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError
      f0: 40 b5 01                     	movb	$0x1, %bpl
      f3: 40 84 ed                     	testb	%bpl, %bpl
      f6: 75 0d                        	jne	0x105 <fs_x5fops_x3a_x3amain+0x105>
      f8: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
      fd: 8b 48 04                     	movl	0x4(%rax), %ecx
     100: e8 00 00 00 00               	callq	0x105 <fs_x5fops_x3a_x3amain+0x105>
		0000000000000101:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     105: 40 84 ed                     	testb	%bpl, %bpl
     108: 75 25                        	jne	0x12f <fs_x5fops_x3a_x3amain+0x12f>
     10a: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     10f: c6 00 01                     	movb	$0x1, (%rax)
     112: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     117: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     11e: eb 0f                        	jmp	0x12f <fs_x5fops_x3a_x3amain+0x12f>
     120: 40 88 bc 24 98 01 00 00      	movb	%dil, 0x198(%rsp)
     128: 88 9c 24 99 01 00 00         	movb	%bl, 0x199(%rsp)
     12f: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     134: 80 38 00                     	cmpb	$0x0, (%rax)
     137: 0f 85 44 02 00 00            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     13d: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     142: c6 00 00                     	movb	$0x0, (%rax)
     145: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     14c: 88 5c 24 41                  	movb	%bl, 0x41(%rsp)
     150: 40 88 7c 24 40               	movb	%dil, 0x40(%rsp)
     155: 40 84 ff                     	testb	%dil, %dil
     158: 74 2d                        	je	0x187 <fs_x5fops_x3a_x3amain+0x187>
     15a: 80 7c 24 40 01               	cmpb	$0x1, 0x40(%rsp)
     15f: 0f 85 15 12 00 00            	jne	0x137a <fs_x5fops_x3a_x3amain+0x137a>
     165: 40 88 bc 24 a8 01 00 00      	movb	%dil, 0x1a8(%rsp)
     16d: 88 9c 24 a9 01 00 00         	movb	%bl, 0x1a9(%rsp)
     174: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     179: 88 5c 24 48                  	movb	%bl, 0x48(%rsp)
     17d: 48 8d 4c 24 48               	leaq	0x48(%rsp), %rcx
     182: e8 00 00 00 00               	callq	0x187 <fs_x5fops_x3a_x3amain+0x187>
		0000000000000183:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError
     187: 31 c0                        	xorl	%eax, %eax
     189: 84 c0                        	testb	%al, %al
     18b: 74 0d                        	je	0x19a <fs_x5fops_x3a_x3amain+0x19a>
     18d: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     192: 8b 48 04                     	movl	0x4(%rax), %ecx
     195: e8 00 00 00 00               	callq	0x19a <fs_x5fops_x3a_x3amain+0x19a>
		0000000000000196:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     19a: b0 01                        	movb	$0x1, %al
     19c: 84 c0                        	testb	%al, %al
     19e: 75 14                        	jne	0x1b4 <fs_x5fops_x3a_x3amain+0x1b4>
     1a0: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     1a5: c6 00 01                     	movb	$0x1, (%rax)
     1a8: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     1ad: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     1b4: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     1b9: 80 38 00                     	cmpb	$0x0, (%rax)
     1bc: 0f 85 bf 01 00 00            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     1c2: 48 8d 05 00 00 00 00         	leaq	(%rip), %rax            # 0x1c9 <fs_x5fops_x3a_x3amain+0x1c9>
		00000000000001c5:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f08BA5F07B55EC3DA
     1c9: 48 89 84 24 30 03 00 00      	movq	%rax, 0x330(%rsp)
     1d1: 48 c7 84 24 38 03 00 00 02 00 00 00  	movq	$0x2, 0x338(%rsp)
     1dd: 48 8d 8c 24 30 03 00 00      	leaq	0x330(%rsp), %rcx
     1e5: e8 00 00 00 00               	callq	0x1ea <fs_x5fops_x3a_x3amain+0x1ea>
		00000000000001e6:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3abytes_x3a_x3aview_x5fstring
     1ea: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
     1ef: 80 39 00                     	cmpb	$0x0, (%rcx)
     1f2: 0f 85 89 01 00 00            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     1f8: 48 89 84 24 98 02 00 00      	movq	%rax, 0x298(%rsp)
     200: 48 89 94 24 a0 02 00 00      	movq	%rdx, 0x2a0(%rsp)
     208: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     20d: 80 38 00                     	cmpb	$0x0, (%rax)
     210: 0f 85 6b 01 00 00            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     216: 0f 10 84 24 88 02 00 00      	movups	0x288(%rsp), %xmm0
     21e: 48 8b 84 24 98 02 00 00      	movq	0x298(%rsp), %rax
     226: 48 8b 8c 24 a0 02 00 00      	movq	0x2a0(%rsp), %rcx
     22e: 0f 11 84 24 a0 03 00 00      	movups	%xmm0, 0x3a0(%rsp)
     236: 48 89 84 24 40 03 00 00      	movq	%rax, 0x340(%rsp)
     23e: 48 89 8c 24 48 03 00 00      	movq	%rcx, 0x348(%rsp)
     246: 48 8d 94 24 a0 03 00 00      	leaq	0x3a0(%rsp), %rdx
     24e: 4c 8d 84 24 40 03 00 00      	leaq	0x340(%rsp), %r8
     256: 48 89 f1                     	movq	%rsi, %rcx
     259: e8 00 00 00 00               	callq	0x25e <fs_x5fops_x3a_x3amain+0x25e>
		000000000000025a:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5ffile
     25e: 89 c7                        	movl	%eax, %edi
     260: 89 d3                        	movl	%edx, %ebx
     262: 88 44 24 50                  	movb	%al, 0x50(%rsp)
     266: 88 54 24 51                  	movb	%dl, 0x51(%rsp)
     26a: 84 c0                        	testb	%al, %al
     26c: 74 75                        	je	0x2e3 <fs_x5fops_x3a_x3amain+0x2e3>
     26e: 80 7c 24 50 01               	cmpb	$0x1, 0x50(%rsp)
     273: 0f 85 11 11 00 00            	jne	0x138a <fs_x5fops_x3a_x3amain+0x138a>
     279: 40 88 bc 24 b8 01 00 00      	movb	%dil, 0x1b8(%rsp)
     281: 88 9c 24 b9 01 00 00         	movb	%bl, 0x1b9(%rsp)
     288: 88 5c 24 58                  	movb	%bl, 0x58(%rsp)
     28c: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     291: c6 00 00                     	movb	$0x0, (%rax)
     294: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     29b: 0f b6 44 24 58               	movzbl	0x58(%rsp), %eax
     2a0: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     2a5: 88 44 24 60                  	movb	%al, 0x60(%rsp)
     2a9: 48 8d 4c 24 60               	leaq	0x60(%rsp), %rcx
     2ae: e8 00 00 00 00               	callq	0x2b3 <fs_x5fops_x3a_x3amain+0x2b3>
		00000000000002af:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError
     2b3: 40 b5 01                     	movb	$0x1, %bpl
     2b6: 40 84 ed                     	testb	%bpl, %bpl
     2b9: 75 0d                        	jne	0x2c8 <fs_x5fops_x3a_x3amain+0x2c8>
     2bb: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     2c0: 8b 48 04                     	movl	0x4(%rax), %ecx
     2c3: e8 00 00 00 00               	callq	0x2c8 <fs_x5fops_x3a_x3amain+0x2c8>
		00000000000002c4:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     2c8: 40 84 ed                     	testb	%bpl, %bpl
     2cb: 75 25                        	jne	0x2f2 <fs_x5fops_x3a_x3amain+0x2f2>
     2cd: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     2d2: c6 00 01                     	movb	$0x1, (%rax)
     2d5: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     2da: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     2e1: eb 0f                        	jmp	0x2f2 <fs_x5fops_x3a_x3amain+0x2f2>
     2e3: 40 88 bc 24 b0 01 00 00      	movb	%dil, 0x1b0(%rsp)
     2eb: 88 9c 24 b1 01 00 00         	movb	%bl, 0x1b1(%rsp)
     2f2: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     2f7: 80 38 00                     	cmpb	$0x0, (%rax)
     2fa: 0f 85 81 00 00 00            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     300: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     305: c6 00 00                     	movb	$0x0, (%rax)
     308: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     30f: 88 5c 24 69                  	movb	%bl, 0x69(%rsp)
     313: 40 88 7c 24 68               	movb	%dil, 0x68(%rsp)
     318: 40 84 ff                     	testb	%dil, %dil
     31b: 74 2d                        	je	0x34a <fs_x5fops_x3a_x3amain+0x34a>
     31d: 80 7c 24 68 01               	cmpb	$0x1, 0x68(%rsp)
     322: 0f 85 72 10 00 00            	jne	0x139a <fs_x5fops_x3a_x3amain+0x139a>
     328: 40 88 bc 24 c0 01 00 00      	movb	%dil, 0x1c0(%rsp)
     330: 88 9c 24 c1 01 00 00         	movb	%bl, 0x1c1(%rsp)
     337: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     33c: 88 5c 24 70                  	movb	%bl, 0x70(%rsp)
     340: 48 8d 4c 24 70               	leaq	0x70(%rsp), %rcx
     345: e8 00 00 00 00               	callq	0x34a <fs_x5fops_x3a_x3amain+0x34a>
		0000000000000346:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError
     34a: 31 c0                        	xorl	%eax, %eax
     34c: 84 c0                        	testb	%al, %al
     34e: 74 0d                        	je	0x35d <fs_x5fops_x3a_x3amain+0x35d>
     350: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     355: 8b 48 04                     	movl	0x4(%rax), %ecx
     358: e8 00 00 00 00               	callq	0x35d <fs_x5fops_x3a_x3amain+0x35d>
		0000000000000359:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     35d: b0 01                        	movb	$0x1, %al
     35f: 84 c0                        	testb	%al, %al
     361: 75 14                        	jne	0x377 <fs_x5fops_x3a_x3amain+0x377>
     363: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     368: c6 00 01                     	movb	$0x1, (%rax)
     36b: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     370: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     377: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     37c: 80 38 00                     	cmpb	$0x0, (%rax)
     37f: 74 0e                        	je	0x38f <fs_x5fops_x3a_x3amain+0x38f>
     381: 31 c0                        	xorl	%eax, %eax
     383: 48 81 c4 c8 04 00 00         	addq	$0x4c8, %rsp            # imm = 0x4C8
     38a: 5b                           	popq	%rbx
     38b: 5d                           	popq	%rbp
     38c: 5f                           	popq	%rdi
     38d: 5e                           	popq	%rsi
     38e: c3                           	retq
     38f: 0f 10 84 24 88 02 00 00      	movups	0x288(%rsp), %xmm0
     397: 0f 11 84 24 b0 03 00 00      	movups	%xmm0, 0x3b0(%rsp)
     39f: 48 8d 94 24 b0 03 00 00      	leaq	0x3b0(%rsp), %rdx
     3a7: 48 89 f1                     	movq	%rsi, %rcx
     3aa: e8 00 00 00 00               	callq	0x3af <fs_x5fops_x3a_x3amain+0x3af>
		00000000000003ab:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aexists
     3af: 88 84 24 fc 00 00 00         	movb	%al, 0xfc(%rsp)
     3b6: 0f 10 84 24 28 02 00 00      	movups	0x228(%rsp), %xmm0
     3be: 0f 11 84 24 c0 03 00 00      	movups	%xmm0, 0x3c0(%rsp)
     3c6: 48 8d 94 24 c0 03 00 00      	leaq	0x3c0(%rsp), %rdx
     3ce: 48 89 f1                     	movq	%rsi, %rcx
     3d1: e8 00 00 00 00               	callq	0x3d6 <fs_x5fops_x3a_x3amain+0x3d6>
		00000000000003d2:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3akind
     3d6: 89 c7                        	movl	%eax, %edi
     3d8: 89 d3                        	movl	%edx, %ebx
     3da: 88 44 24 78                  	movb	%al, 0x78(%rsp)
     3de: 88 54 24 79                  	movb	%dl, 0x79(%rsp)
     3e2: 84 c0                        	testb	%al, %al
     3e4: 0f 84 94 00 00 00            	je	0x47e <fs_x5fops_x3a_x3amain+0x47e>
     3ea: 80 7c 24 78 01               	cmpb	$0x1, 0x78(%rsp)
     3ef: 0f 85 b5 0f 00 00            	jne	0x13aa <fs_x5fops_x3a_x3amain+0x13aa>
     3f5: 40 88 bc 24 d8 01 00 00      	movb	%dil, 0x1d8(%rsp)
     3fd: 88 9c 24 d9 01 00 00         	movb	%bl, 0x1d9(%rsp)
     404: 88 9c 24 90 00 00 00         	movb	%bl, 0x90(%rsp)
     40b: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     410: c6 00 00                     	movb	$0x0, (%rax)
     413: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     41a: 0f b6 84 24 90 00 00 00      	movzbl	0x90(%rsp), %eax
     422: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     427: 88 84 24 98 00 00 00         	movb	%al, 0x98(%rsp)
     42e: 48 8d 8c 24 98 00 00 00      	leaq	0x98(%rsp), %rcx
     436: e8 00 00 00 00               	callq	0x43b <fs_x5fops_x3a_x3amain+0x43b>
		0000000000000437:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError
     43b: 40 b5 01                     	movb	$0x1, %bpl
     43e: 40 84 ed                     	testb	%bpl, %bpl
     441: 75 0d                        	jne	0x450 <fs_x5fops_x3a_x3amain+0x450>
     443: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     448: 8b 48 04                     	movl	0x4(%rax), %ecx
     44b: e8 00 00 00 00               	callq	0x450 <fs_x5fops_x3a_x3amain+0x450>
		000000000000044c:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     450: 40 84 ed                     	testb	%bpl, %bpl
     453: 75 14                        	jne	0x469 <fs_x5fops_x3a_x3amain+0x469>
     455: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     45a: c6 00 01                     	movb	$0x1, (%rax)
     45d: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     462: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     469: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     46e: 80 38 00                     	cmpb	$0x0, (%rax)
     471: 0f 85 0a ff ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     477: 31 ed                        	xorl	%ebp, %ebp
     479: e9 92 00 00 00               	jmp	0x510 <fs_x5fops_x3a_x3amain+0x510>
     47e: 40 88 bc 24 c8 01 00 00      	movb	%dil, 0x1c8(%rsp)
     486: 88 9c 24 c9 01 00 00         	movb	%bl, 0x1c9(%rsp)
     48d: 88 9c 24 80 00 00 00         	movb	%bl, 0x80(%rsp)
     494: 88 9c 24 d0 01 00 00         	movb	%bl, 0x1d0(%rsp)
     49b: 40 b5 01                     	movb	$0x1, %bpl
     49e: 80 fb 01                     	cmpb	$0x1, %bl
     4a1: 74 02                        	je	0x4a5 <fs_x5fops_x3a_x3amain+0x4a5>
     4a3: 31 ed                        	xorl	%ebp, %ebp
     4a5: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     4aa: c6 00 00                     	movb	$0x0, (%rax)
     4ad: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     4b4: 0f b6 84 24 80 00 00 00      	movzbl	0x80(%rsp), %eax
     4bc: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     4c1: 88 84 24 88 00 00 00         	movb	%al, 0x88(%rsp)
     4c8: 48 8d 8c 24 88 00 00 00      	leaq	0x88(%rsp), %rcx
     4d0: e8 00 00 00 00               	callq	0x4d5 <fs_x5fops_x3a_x3amain+0x4d5>
		00000000000004d1:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aFileKind
     4d5: 31 c0                        	xorl	%eax, %eax
     4d7: 84 c0                        	testb	%al, %al
     4d9: 74 0d                        	je	0x4e8 <fs_x5fops_x3a_x3amain+0x4e8>
     4db: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     4e0: 8b 48 04                     	movl	0x4(%rax), %ecx
     4e3: e8 00 00 00 00               	callq	0x4e8 <fs_x5fops_x3a_x3amain+0x4e8>
		00000000000004e4:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     4e8: b0 01                        	movb	$0x1, %al
     4ea: 84 c0                        	testb	%al, %al
     4ec: 75 14                        	jne	0x502 <fs_x5fops_x3a_x3amain+0x502>
     4ee: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     4f3: c6 00 01                     	movb	$0x1, (%rax)
     4f6: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     4fb: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     502: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     507: 80 38 00                     	cmpb	$0x0, (%rax)
     50a: 0f 85 71 fe ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     510: 40 88 ac 24 fd 00 00 00      	movb	%bpl, 0xfd(%rsp)
     518: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     51d: c6 00 00                     	movb	$0x0, (%rax)
     520: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     527: 88 9c 24 a1 00 00 00         	movb	%bl, 0xa1(%rsp)
     52e: 40 88 bc 24 a0 00 00 00      	movb	%dil, 0xa0(%rsp)
     536: 40 84 ff                     	testb	%dil, %dil
     539: 74 38                        	je	0x573 <fs_x5fops_x3a_x3amain+0x573>
     53b: 80 bc 24 a0 00 00 00 01      	cmpb	$0x1, 0xa0(%rsp)
     543: 0f 85 71 0e 00 00            	jne	0x13ba <fs_x5fops_x3a_x3amain+0x13ba>
     549: 40 88 bc 24 e8 01 00 00      	movb	%dil, 0x1e8(%rsp)
     551: 88 9c 24 e9 01 00 00         	movb	%bl, 0x1e9(%rsp)
     558: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     55d: 88 9c 24 b0 00 00 00         	movb	%bl, 0xb0(%rsp)
     564: 48 8d 8c 24 b0 00 00 00      	leaq	0xb0(%rsp), %rcx
     56c: e8 00 00 00 00               	callq	0x571 <fs_x5fops_x3a_x3amain+0x571>
		000000000000056d:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError
     571: eb 28                        	jmp	0x59b <fs_x5fops_x3a_x3amain+0x59b>
     573: 40 88 bc 24 e0 01 00 00      	movb	%dil, 0x1e0(%rsp)
     57b: 88 9c 24 e1 01 00 00         	movb	%bl, 0x1e1(%rsp)
     582: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     587: 88 9c 24 a8 00 00 00         	movb	%bl, 0xa8(%rsp)
     58e: 48 8d 8c 24 a8 00 00 00      	leaq	0xa8(%rsp), %rcx
     596: e8 00 00 00 00               	callq	0x59b <fs_x5fops_x3a_x3amain+0x59b>
		0000000000000597:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aFileKind
     59b: 31 c0                        	xorl	%eax, %eax
     59d: 84 c0                        	testb	%al, %al
     59f: 74 0d                        	je	0x5ae <fs_x5fops_x3a_x3amain+0x5ae>
     5a1: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     5a6: 8b 48 04                     	movl	0x4(%rax), %ecx
     5a9: e8 00 00 00 00               	callq	0x5ae <fs_x5fops_x3a_x3amain+0x5ae>
		00000000000005aa:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     5ae: b0 01                        	movb	$0x1, %al
     5b0: 84 c0                        	testb	%al, %al
     5b2: 75 14                        	jne	0x5c8 <fs_x5fops_x3a_x3amain+0x5c8>
     5b4: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     5b9: c6 00 01                     	movb	$0x1, (%rax)
     5bc: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     5c1: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     5c8: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     5cd: 80 38 00                     	cmpb	$0x0, (%rax)
     5d0: 0f 85 ab fd ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     5d6: 0f 10 84 24 28 02 00 00      	movups	0x228(%rsp), %xmm0
     5de: 0f 11 84 24 d0 03 00 00      	movups	%xmm0, 0x3d0(%rsp)
     5e6: 48 8d 8c 24 18 02 00 00      	leaq	0x218(%rsp), %rcx
     5ee: 4c 8d 84 24 d0 03 00 00      	leaq	0x3d0(%rsp), %r8
     5f6: 48 89 f2                     	movq	%rsi, %rdx
     5f9: e8 00 00 00 00               	callq	0x5fe <fs_x5fops_x3a_x3amain+0x5fe>
		00000000000005fa:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3aopen_x5fdir
     5fe: 0f b7 84 24 25 02 00 00      	movzwl	0x225(%rsp), %eax
     606: 8b 8c 24 21 02 00 00         	movl	0x221(%rsp), %ecx
     60d: 48 8b 94 24 19 02 00 00      	movq	0x219(%rsp), %rdx
     615: 44 0f b6 84 24 27 02 00 00   	movzbl	0x227(%rsp), %r8d
     61e: 44 0f b6 8c 24 18 02 00 00   	movzbl	0x218(%rsp), %r9d
     627: 0f 57 c0                     	xorps	%xmm0, %xmm0
     62a: 0f 11 84 24 48 01 00 00      	movups	%xmm0, 0x148(%rsp)
     632: 44 88 8c 24 48 01 00 00      	movb	%r9b, 0x148(%rsp)
     63a: 44 88 84 24 57 01 00 00      	movb	%r8b, 0x157(%rsp)
     642: 48 89 94 24 49 01 00 00      	movq	%rdx, 0x149(%rsp)
     64a: 89 8c 24 51 01 00 00         	movl	%ecx, 0x151(%rsp)
     651: 66 89 84 24 55 01 00 00      	movw	%ax, 0x155(%rsp)
     659: 0f b6 84 24 48 01 00 00      	movzbl	0x148(%rsp), %eax
     661: 88 84 24 08 02 00 00         	movb	%al, 0x208(%rsp)
     668: 48 8b 84 24 49 01 00 00      	movq	0x149(%rsp), %rax
     670: 48 89 84 24 09 02 00 00      	movq	%rax, 0x209(%rsp)
     678: 8b 84 24 51 01 00 00         	movl	0x151(%rsp), %eax
     67f: 89 84 24 11 02 00 00         	movl	%eax, 0x211(%rsp)
     686: 0f b7 84 24 55 01 00 00      	movzwl	0x155(%rsp), %eax
     68e: 66 89 84 24 15 02 00 00      	movw	%ax, 0x215(%rsp)
     696: 0f b6 84 24 57 01 00 00      	movzbl	0x157(%rsp), %eax
     69e: 88 84 24 17 02 00 00         	movb	%al, 0x217(%rsp)
     6a5: 80 bc 24 08 02 00 00 01      	cmpb	$0x1, 0x208(%rsp)
     6ad: 0f 85 e4 00 00 00            	jne	0x797 <fs_x5fops_x3a_x3amain+0x797>
     6b3: 0f b6 84 24 48 01 00 00      	movzbl	0x148(%rsp), %eax
     6bb: 88 84 24 38 02 00 00         	movb	%al, 0x238(%rsp)
     6c2: 48 8b 84 24 49 01 00 00      	movq	0x149(%rsp), %rax
     6ca: 48 89 84 24 39 02 00 00      	movq	%rax, 0x239(%rsp)
     6d2: 8b 84 24 51 01 00 00         	movl	0x151(%rsp), %eax
     6d9: 89 84 24 41 02 00 00         	movl	%eax, 0x241(%rsp)
     6e0: 0f b7 84 24 55 01 00 00      	movzwl	0x155(%rsp), %eax
     6e8: 66 89 84 24 45 02 00 00      	movw	%ax, 0x245(%rsp)
     6f0: 0f b6 84 24 57 01 00 00      	movzbl	0x157(%rsp), %eax
     6f8: 88 84 24 47 02 00 00         	movb	%al, 0x247(%rsp)
     6ff: 48 8b 84 24 40 02 00 00      	movq	0x240(%rsp), %rax
     707: 48 89 84 24 40 01 00 00      	movq	%rax, 0x140(%rsp)
     70f: 48 8d 8c 24 58 01 00 00      	leaq	0x158(%rsp), %rcx
     717: 48 8d 94 24 40 01 00 00      	leaq	0x140(%rsp), %rdx
     71f: e8 00 00 00 00               	callq	0x724 <fs_x5fops_x3a_x3amain+0x724>
		0000000000000720:  IMAGE_REL_AMD64_REL32	DirIter_x3a_x3aOpen_x3a_x3anext
     724: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     729: 80 38 00                     	cmpb	$0x0, (%rax)
     72c: 0f 84 2d 01 00 00            	je	0x85f <fs_x5fops_x3a_x3amain+0x85f>
     732: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     737: 8b 70 04                     	movl	0x4(%rax), %esi
     73a: c6 00 00                     	movb	$0x0, (%rax)
     73d: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     744: 48 8b 84 24 40 01 00 00      	movq	0x140(%rsp), %rax
     74c: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     751: 48 89 84 24 a8 02 00 00      	movq	%rax, 0x2a8(%rsp)
     759: 48 8d 8c 24 a8 02 00 00      	leaq	0x2a8(%rsp), %rcx
     761: e8 00 00 00 00               	callq	0x766 <fs_x5fops_x3a_x3amain+0x766>
		0000000000000762:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aDirIter_x3a_x3aOpen
     766: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     76b: 0f b6 18                     	movzbl	(%rax), %ebx
     76e: 84 db                        	testb	%bl, %bl
     770: 74 08                        	je	0x77a <fs_x5fops_x3a_x3amain+0x77a>
     772: 8b 48 04                     	movl	0x4(%rax), %ecx
     775: e8 00 00 00 00               	callq	0x77a <fs_x5fops_x3a_x3amain+0x77a>
		0000000000000776:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     77a: 84 db                        	testb	%bl, %bl
     77c: 0f 85 ff fb ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     782: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     787: c6 00 01                     	movb	$0x1, (%rax)
     78a: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     78f: 89 70 04                     	movl	%esi, 0x4(%rax)
     792: e9 ea fb ff ff               	jmp	0x381 <fs_x5fops_x3a_x3amain+0x381>
     797: 80 bc 24 08 02 00 00 00      	cmpb	$0x0, 0x208(%rsp)
     79f: 0f 85 28 0c 00 00            	jne	0x13cd <fs_x5fops_x3a_x3amain+0x13cd>
     7a5: 0f 10 84 24 48 01 00 00      	movups	0x148(%rsp), %xmm0
     7ad: 0f 11 84 24 18 04 00 00      	movups	%xmm0, 0x418(%rsp)
     7b5: 0f b6 84 24 20 04 00 00      	movzbl	0x420(%rsp), %eax
     7bd: 88 84 24 c8 00 00 00         	movb	%al, 0xc8(%rsp)
     7c4: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     7c9: c6 00 00                     	movb	$0x0, (%rax)
     7cc: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     7d3: 0f b6 84 24 c8 00 00 00      	movzbl	0xc8(%rsp), %eax
     7db: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     7e0: 88 84 24 d0 00 00 00         	movb	%al, 0xd0(%rsp)
     7e7: 48 8d 8c 24 d0 00 00 00      	leaq	0xd0(%rsp), %rcx
     7ef: e8 00 00 00 00               	callq	0x7f4 <fs_x5fops_x3a_x3amain+0x7f4>
		00000000000007f0:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError
     7f4: b3 01                        	movb	$0x1, %bl
     7f6: 84 db                        	testb	%bl, %bl
     7f8: 75 0d                        	jne	0x807 <fs_x5fops_x3a_x3amain+0x807>
     7fa: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     7ff: 8b 48 04                     	movl	0x4(%rax), %ecx
     802: e8 00 00 00 00               	callq	0x807 <fs_x5fops_x3a_x3amain+0x807>
		0000000000000803:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     807: 84 db                        	testb	%bl, %bl
     809: 75 14                        	jne	0x81f <fs_x5fops_x3a_x3amain+0x81f>
     80b: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     810: c6 00 01                     	movb	$0x1, (%rax)
     813: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     818: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     81f: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     824: 80 38 00                     	cmpb	$0x0, (%rax)
     827: 0f 85 54 fb ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     82d: 31 c0                        	xorl	%eax, %eax
     82f: 88 84 24 ff 00 00 00         	movb	%al, 0xff(%rsp)
     836: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     83b: 80 38 00                     	cmpb	$0x0, (%rax)
     83e: 0f 85 3d fb ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
     844: 80 bc 24 fc 00 00 00 00      	cmpb	$0x0, 0xfc(%rsp)
     84c: 0f 84 95 08 00 00            	je	0x10e7 <fs_x5fops_x3a_x3amain+0x10e7>
     852: 0f b6 84 24 fd 00 00 00      	movzbl	0xfd(%rsp), %eax
     85a: e9 8a 08 00 00               	jmp	0x10e9 <fs_x5fops_x3a_x3amain+0x10e9>
     85f: 0f b6 84 24 58 01 00 00      	movzbl	0x158(%rsp), %eax
     867: 0f 57 c0                     	xorps	%xmm0, %xmm0
     86a: 0f 11 84 24 00 01 00 00      	movups	%xmm0, 0x100(%rsp)
     872: 88 84 24 00 01 00 00         	movb	%al, 0x100(%rsp)
     879: 0f b6 84 24 59 01 00 00      	movzbl	0x159(%rsp), %eax
     881: 88 84 24 01 01 00 00         	movb	%al, 0x101(%rsp)
     888: 0f b6 84 24 5a 01 00 00      	movzbl	0x15a(%rsp), %eax
     890: 88 84 24 02 01 00 00         	movb	%al, 0x102(%rsp)
     897: 0f b6 84 24 5b 01 00 00      	movzbl	0x15b(%rsp), %eax
     89f: 88 84 24 03 01 00 00         	movb	%al, 0x103(%rsp)
     8a6: 0f b6 84 24 5c 01 00 00      	movzbl	0x15c(%rsp), %eax
     8ae: 88 84 24 04 01 00 00         	movb	%al, 0x104(%rsp)
     8b5: 0f b6 84 24 5d 01 00 00      	movzbl	0x15d(%rsp), %eax
     8bd: 88 84 24 05 01 00 00         	movb	%al, 0x105(%rsp)
     8c4: 0f b6 84 24 5e 01 00 00      	movzbl	0x15e(%rsp), %eax
     8cc: 88 84 24 06 01 00 00         	movb	%al, 0x106(%rsp)
     8d3: 0f b6 84 24 5f 01 00 00      	movzbl	0x15f(%rsp), %eax
     8db: 88 84 24 07 01 00 00         	movb	%al, 0x107(%rsp)
     8e2: 0f b6 84 24 60 01 00 00      	movzbl	0x160(%rsp), %eax
     8ea: 88 84 24 08 01 00 00         	movb	%al, 0x108(%rsp)
     8f1: 0f b6 84 24 61 01 00 00      	movzbl	0x161(%rsp), %eax
     8f9: 88 84 24 09 01 00 00         	movb	%al, 0x109(%rsp)
     900: 0f b6 84 24 62 01 00 00      	movzbl	0x162(%rsp), %eax
     908: 88 84 24 0a 01 00 00         	movb	%al, 0x10a(%rsp)
     90f: 0f b6 84 24 63 01 00 00      	movzbl	0x163(%rsp), %eax
     917: 88 84 24 0b 01 00 00         	movb	%al, 0x10b(%rsp)
     91e: 0f b6 84 24 64 01 00 00      	movzbl	0x164(%rsp), %eax
     926: 88 84 24 0c 01 00 00         	movb	%al, 0x10c(%rsp)
     92d: 0f b6 84 24 65 01 00 00      	movzbl	0x165(%rsp), %eax
     935: 88 84 24 0d 01 00 00         	movb	%al, 0x10d(%rsp)
     93c: 0f b6 84 24 66 01 00 00      	movzbl	0x166(%rsp), %eax
     944: 88 84 24 0e 01 00 00         	movb	%al, 0x10e(%rsp)
     94b: 0f b6 84 24 67 01 00 00      	movzbl	0x167(%rsp), %eax
     953: 88 84 24 0f 01 00 00         	movb	%al, 0x10f(%rsp)
     95a: 0f b6 84 24 68 01 00 00      	movzbl	0x168(%rsp), %eax
     962: 0f 11 84 24 10 01 00 00      	movups	%xmm0, 0x110(%rsp)
     96a: 88 84 24 10 01 00 00         	movb	%al, 0x110(%rsp)
     971: 0f b6 84 24 69 01 00 00      	movzbl	0x169(%rsp), %eax
     979: 88 84 24 11 01 00 00         	movb	%al, 0x111(%rsp)
     980: 0f b6 84 24 6a 01 00 00      	movzbl	0x16a(%rsp), %eax
     988: 88 84 24 12 01 00 00         	movb	%al, 0x112(%rsp)
     98f: 0f b6 84 24 6b 01 00 00      	movzbl	0x16b(%rsp), %eax
     997: 88 84 24 13 01 00 00         	movb	%al, 0x113(%rsp)
     99e: 0f b6 84 24 6c 01 00 00      	movzbl	0x16c(%rsp), %eax
     9a6: 88 84 24 14 01 00 00         	movb	%al, 0x114(%rsp)
     9ad: 0f b6 84 24 6d 01 00 00      	movzbl	0x16d(%rsp), %eax
     9b5: 88 84 24 15 01 00 00         	movb	%al, 0x115(%rsp)
     9bc: 0f b6 84 24 6e 01 00 00      	movzbl	0x16e(%rsp), %eax
     9c4: 88 84 24 16 01 00 00         	movb	%al, 0x116(%rsp)
     9cb: 0f b6 84 24 6f 01 00 00      	movzbl	0x16f(%rsp), %eax
     9d3: 88 84 24 17 01 00 00         	movb	%al, 0x117(%rsp)
     9da: 0f b6 84 24 70 01 00 00      	movzbl	0x170(%rsp), %eax
     9e2: 88 84 24 18 01 00 00         	movb	%al, 0x118(%rsp)
     9e9: 0f b6 84 24 71 01 00 00      	movzbl	0x171(%rsp), %eax
     9f1: 88 84 24 19 01 00 00         	movb	%al, 0x119(%rsp)
     9f8: 0f b6 84 24 72 01 00 00      	movzbl	0x172(%rsp), %eax
     a00: 88 84 24 1a 01 00 00         	movb	%al, 0x11a(%rsp)
     a07: 0f b6 84 24 73 01 00 00      	movzbl	0x173(%rsp), %eax
     a0f: 88 84 24 1b 01 00 00         	movb	%al, 0x11b(%rsp)
     a16: 0f b6 84 24 74 01 00 00      	movzbl	0x174(%rsp), %eax
     a1e: 88 84 24 1c 01 00 00         	movb	%al, 0x11c(%rsp)
     a25: 0f b6 84 24 75 01 00 00      	movzbl	0x175(%rsp), %eax
     a2d: 88 84 24 1d 01 00 00         	movb	%al, 0x11d(%rsp)
     a34: 0f b6 84 24 76 01 00 00      	movzbl	0x176(%rsp), %eax
     a3c: 88 84 24 1e 01 00 00         	movb	%al, 0x11e(%rsp)
     a43: 0f b6 84 24 77 01 00 00      	movzbl	0x177(%rsp), %eax
     a4b: 88 84 24 1f 01 00 00         	movb	%al, 0x11f(%rsp)
     a52: 0f b6 84 24 78 01 00 00      	movzbl	0x178(%rsp), %eax
     a5a: 0f 11 84 24 20 01 00 00      	movups	%xmm0, 0x120(%rsp)
     a62: 88 84 24 20 01 00 00         	movb	%al, 0x120(%rsp)
     a69: 0f b6 84 24 79 01 00 00      	movzbl	0x179(%rsp), %eax
     a71: 88 84 24 21 01 00 00         	movb	%al, 0x121(%rsp)
     a78: 0f b6 84 24 7a 01 00 00      	movzbl	0x17a(%rsp), %eax
     a80: 88 84 24 22 01 00 00         	movb	%al, 0x122(%rsp)
     a87: 0f b6 84 24 7b 01 00 00      	movzbl	0x17b(%rsp), %eax
     a8f: 88 84 24 23 01 00 00         	movb	%al, 0x123(%rsp)
     a96: 0f b6 84 24 7c 01 00 00      	movzbl	0x17c(%rsp), %eax
     a9e: 88 84 24 24 01 00 00         	movb	%al, 0x124(%rsp)
     aa5: 0f b6 84 24 7d 01 00 00      	movzbl	0x17d(%rsp), %eax
     aad: 88 84 24 25 01 00 00         	movb	%al, 0x125(%rsp)
     ab4: 0f b6 84 24 7e 01 00 00      	movzbl	0x17e(%rsp), %eax
     abc: 88 84 24 26 01 00 00         	movb	%al, 0x126(%rsp)
     ac3: 0f b6 84 24 7f 01 00 00      	movzbl	0x17f(%rsp), %eax
     acb: 88 84 24 27 01 00 00         	movb	%al, 0x127(%rsp)
     ad2: 0f b6 84 24 80 01 00 00      	movzbl	0x180(%rsp), %eax
     ada: 88 84 24 28 01 00 00         	movb	%al, 0x128(%rsp)
     ae1: 0f b6 84 24 81 01 00 00      	movzbl	0x181(%rsp), %eax
     ae9: 88 84 24 29 01 00 00         	movb	%al, 0x129(%rsp)
     af0: 0f b6 84 24 82 01 00 00      	movzbl	0x182(%rsp), %eax
     af8: 88 84 24 2a 01 00 00         	movb	%al, 0x12a(%rsp)
     aff: 0f b6 84 24 83 01 00 00      	movzbl	0x183(%rsp), %eax
     b07: 88 84 24 2b 01 00 00         	movb	%al, 0x12b(%rsp)
     b0e: 0f b6 84 24 84 01 00 00      	movzbl	0x184(%rsp), %eax
     b16: 88 84 24 2c 01 00 00         	movb	%al, 0x12c(%rsp)
     b1d: 0f b6 84 24 85 01 00 00      	movzbl	0x185(%rsp), %eax
     b25: 88 84 24 2d 01 00 00         	movb	%al, 0x12d(%rsp)
     b2c: 0f b6 84 24 86 01 00 00      	movzbl	0x186(%rsp), %eax
     b34: 88 84 24 2e 01 00 00         	movb	%al, 0x12e(%rsp)
     b3b: 0f b6 84 24 87 01 00 00      	movzbl	0x187(%rsp), %eax
     b43: 88 84 24 2f 01 00 00         	movb	%al, 0x12f(%rsp)
     b4a: 0f b6 84 24 88 01 00 00      	movzbl	0x188(%rsp), %eax
     b52: 0f 11 84 24 30 01 00 00      	movups	%xmm0, 0x130(%rsp)
     b5a: 88 84 24 30 01 00 00         	movb	%al, 0x130(%rsp)
     b61: 0f b6 84 24 89 01 00 00      	movzbl	0x189(%rsp), %eax
     b69: 88 84 24 31 01 00 00         	movb	%al, 0x131(%rsp)
     b70: 0f b6 84 24 8a 01 00 00      	movzbl	0x18a(%rsp), %eax
     b78: 88 84 24 32 01 00 00         	movb	%al, 0x132(%rsp)
     b7f: 0f b6 84 24 8b 01 00 00      	movzbl	0x18b(%rsp), %eax
     b87: 88 84 24 33 01 00 00         	movb	%al, 0x133(%rsp)
     b8e: 0f b6 84 24 8c 01 00 00      	movzbl	0x18c(%rsp), %eax
     b96: 88 84 24 34 01 00 00         	movb	%al, 0x134(%rsp)
     b9d: 0f b6 84 24 8d 01 00 00      	movzbl	0x18d(%rsp), %eax
     ba5: 88 84 24 35 01 00 00         	movb	%al, 0x135(%rsp)
     bac: 0f b6 84 24 8e 01 00 00      	movzbl	0x18e(%rsp), %eax
     bb4: 88 84 24 36 01 00 00         	movb	%al, 0x136(%rsp)
     bbb: 0f b6 84 24 8f 01 00 00      	movzbl	0x18f(%rsp), %eax
     bc3: 88 84 24 37 01 00 00         	movb	%al, 0x137(%rsp)
     bca: 0f b6 84 24 90 01 00 00      	movzbl	0x190(%rsp), %eax
     bd2: 88 84 24 38 01 00 00         	movb	%al, 0x138(%rsp)
     bd9: 0f b6 84 24 91 01 00 00      	movzbl	0x191(%rsp), %eax
     be1: 88 84 24 39 01 00 00         	movb	%al, 0x139(%rsp)
     be8: 0f b6 84 24 92 01 00 00      	movzbl	0x192(%rsp), %eax
     bf0: 88 84 24 3a 01 00 00         	movb	%al, 0x13a(%rsp)
     bf7: 0f b6 84 24 93 01 00 00      	movzbl	0x193(%rsp), %eax
     bff: 88 84 24 3b 01 00 00         	movb	%al, 0x13b(%rsp)
     c06: 0f b6 84 24 94 01 00 00      	movzbl	0x194(%rsp), %eax
     c0e: 88 84 24 3c 01 00 00         	movb	%al, 0x13c(%rsp)
     c15: 0f b6 84 24 95 01 00 00      	movzbl	0x195(%rsp), %eax
     c1d: 88 84 24 3d 01 00 00         	movb	%al, 0x13d(%rsp)
     c24: 0f b6 84 24 96 01 00 00      	movzbl	0x196(%rsp), %eax
     c2c: 88 84 24 3e 01 00 00         	movb	%al, 0x13e(%rsp)
     c33: 0f b6 84 24 97 01 00 00      	movzbl	0x197(%rsp), %eax
     c3b: 88 84 24 3f 01 00 00         	movb	%al, 0x13f(%rsp)
     c42: 0f b6 84 24 2e 01 00 00      	movzbl	0x12e(%rsp), %eax
     c4a: 88 84 24 76 02 00 00         	movb	%al, 0x276(%rsp)
     c51: 0f b6 84 24 2f 01 00 00      	movzbl	0x12f(%rsp), %eax
     c59: 88 84 24 77 02 00 00         	movb	%al, 0x277(%rsp)
     c60: 0f b6 84 24 34 01 00 00      	movzbl	0x134(%rsp), %eax
     c68: 88 84 24 7c 02 00 00         	movb	%al, 0x27c(%rsp)
     c6f: 0f b6 84 24 35 01 00 00      	movzbl	0x135(%rsp), %eax
     c77: 88 84 24 7d 02 00 00         	movb	%al, 0x27d(%rsp)
     c7e: 0f b6 84 24 36 01 00 00      	movzbl	0x136(%rsp), %eax
     c86: 88 84 24 7e 02 00 00         	movb	%al, 0x27e(%rsp)
     c8d: 0f b6 84 24 37 01 00 00      	movzbl	0x137(%rsp), %eax
     c95: 88 84 24 7f 02 00 00         	movb	%al, 0x27f(%rsp)
     c9c: 0f b6 84 24 38 01 00 00      	movzbl	0x138(%rsp), %eax
     ca4: 88 84 24 80 02 00 00         	movb	%al, 0x280(%rsp)
     cab: 0f b6 84 24 39 01 00 00      	movzbl	0x139(%rsp), %eax
     cb3: 88 84 24 81 02 00 00         	movb	%al, 0x281(%rsp)
     cba: 0f b6 84 24 3a 01 00 00      	movzbl	0x13a(%rsp), %eax
     cc2: 88 84 24 82 02 00 00         	movb	%al, 0x282(%rsp)
     cc9: 0f b6 84 24 3b 01 00 00      	movzbl	0x13b(%rsp), %eax
     cd1: 88 84 24 83 02 00 00         	movb	%al, 0x283(%rsp)
     cd8: 0f b6 84 24 3c 01 00 00      	movzbl	0x13c(%rsp), %eax
     ce0: 88 84 24 84 02 00 00         	movb	%al, 0x284(%rsp)
     ce7: 0f b6 84 24 3d 01 00 00      	movzbl	0x13d(%rsp), %eax
     cef: 88 84 24 85 02 00 00         	movb	%al, 0x285(%rsp)
     cf6: 0f b6 84 24 3e 01 00 00      	movzbl	0x13e(%rsp), %eax
     cfe: 88 84 24 86 02 00 00         	movb	%al, 0x286(%rsp)
     d05: 0f b6 84 24 3f 01 00 00      	movzbl	0x13f(%rsp), %eax
     d0d: 88 84 24 87 02 00 00         	movb	%al, 0x287(%rsp)
     d14: 48 8b 84 24 20 01 00 00      	movq	0x120(%rsp), %rax
     d1c: 48 89 84 24 68 02 00 00      	movq	%rax, 0x268(%rsp)
     d24: 8b 84 24 28 01 00 00         	movl	0x128(%rsp), %eax
     d2b: 89 84 24 70 02 00 00         	movl	%eax, 0x270(%rsp)
     d32: 0f b7 84 24 2c 01 00 00      	movzwl	0x12c(%rsp), %eax
     d3a: 66 89 84 24 74 02 00 00      	movw	%ax, 0x274(%rsp)
     d42: 8b 84 24 30 01 00 00         	movl	0x130(%rsp), %eax
     d49: 89 84 24 78 02 00 00         	movl	%eax, 0x278(%rsp)
     d50: 0f 10 8c 24 00 01 00 00      	movups	0x100(%rsp), %xmm1
     d58: 0f 10 94 24 10 01 00 00      	movups	0x110(%rsp), %xmm2
     d60: 0f 11 8c 24 48 02 00 00      	movups	%xmm1, 0x248(%rsp)
     d68: 0f 11 94 24 58 02 00 00      	movups	%xmm2, 0x258(%rsp)
     d70: 80 bc 24 48 02 00 00 01      	cmpb	$0x1, 0x248(%rsp)
     d78: 0f 85 98 01 00 00            	jne	0xf16 <fs_x5fops_x3a_x3amain+0xf16>
     d7e: 0f 10 8c 24 00 01 00 00      	movups	0x100(%rsp), %xmm1
     d86: 0f 10 94 24 10 01 00 00      	movups	0x110(%rsp), %xmm2
     d8e: 0f 10 9c 24 20 01 00 00      	movups	0x120(%rsp), %xmm3
     d96: 0f 10 a4 24 30 01 00 00      	movups	0x130(%rsp), %xmm4
     d9e: 0f 11 94 24 70 03 00 00      	movups	%xmm2, 0x370(%rsp)
     da6: 0f 11 9c 24 80 03 00 00      	movups	%xmm3, 0x380(%rsp)
     dae: 0f 11 8c 24 60 03 00 00      	movups	%xmm1, 0x360(%rsp)
     db6: 0f 11 a4 24 90 03 00 00      	movups	%xmm4, 0x390(%rsp)
     dbe: 0f 10 8c 24 70 03 00 00      	movups	0x370(%rsp), %xmm1
     dc6: 0f 10 94 24 80 03 00 00      	movups	0x380(%rsp), %xmm2
     dce: 48 8b 84 24 68 03 00 00      	movq	0x368(%rsp), %rax
     dd6: 48 8b 8c 24 90 03 00 00      	movq	0x390(%rsp), %rcx
     dde: 48 8b 94 24 98 03 00 00      	movq	0x398(%rsp), %rdx
     de6: 0f 11 84 24 d0 02 00 00      	movups	%xmm0, 0x2d0(%rsp)
     dee: 0f 11 84 24 b0 02 00 00      	movups	%xmm0, 0x2b0(%rsp)
     df6: 0f 11 84 24 c0 02 00 00      	movups	%xmm0, 0x2c0(%rsp)
     dfe: 48 89 8c 24 d8 02 00 00      	movq	%rcx, 0x2d8(%rsp)
     e06: 48 89 84 24 b0 02 00 00      	movq	%rax, 0x2b0(%rsp)
     e0e: 0f 11 94 24 c8 02 00 00      	movups	%xmm2, 0x2c8(%rsp)
     e16: 0f 11 8c 24 b8 02 00 00      	movups	%xmm1, 0x2b8(%rsp)
     e1e: 48 89 94 24 e0 02 00 00      	movq	%rdx, 0x2e0(%rsp)
     e26: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     e2b: c6 00 00                     	movb	$0x0, (%rax)
     e2e: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     e35: 48 8b 84 24 e0 02 00 00      	movq	0x2e0(%rsp), %rax
     e3d: 48 8b 8c 24 b0 02 00 00      	movq	0x2b0(%rsp), %rcx
     e45: 4c 8b 84 24 b8 02 00 00      	movq	0x2b8(%rsp), %r8
     e4d: 0f 10 84 24 c0 02 00 00      	movups	0x2c0(%rsp), %xmm0
     e55: 4c 8b 8c 24 d0 02 00 00      	movq	0x2d0(%rsp), %r9
     e5d: 4c 8b 94 24 d8 02 00 00      	movq	0x2d8(%rsp), %r10
     e65: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     e6a: 48 89 8c 24 e0 03 00 00      	movq	%rcx, 0x3e0(%rsp)
     e72: 4c 89 84 24 e8 03 00 00      	movq	%r8, 0x3e8(%rsp)
     e7a: 0f 11 84 24 f0 03 00 00      	movups	%xmm0, 0x3f0(%rsp)
     e82: 4c 89 8c 24 00 04 00 00      	movq	%r9, 0x400(%rsp)
     e8a: 4c 89 94 24 08 04 00 00      	movq	%r10, 0x408(%rsp)
     e92: 48 89 84 24 10 04 00 00      	movq	%rax, 0x410(%rsp)
     e9a: 48 8d 8c 24 e0 03 00 00      	leaq	0x3e0(%rsp), %rcx
     ea2: e8 00 00 00 00               	callq	0xea7 <fs_x5fops_x3a_x3amain+0xea7>
		0000000000000ea3:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aDirEntry
     ea7: 31 c0                        	xorl	%eax, %eax
     ea9: 84 c0                        	testb	%al, %al
     eab: 74 0d                        	je	0xeba <fs_x5fops_x3a_x3amain+0xeba>
     ead: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     eb2: 8b 48 04                     	movl	0x4(%rax), %ecx
     eb5: e8 00 00 00 00               	callq	0xeba <fs_x5fops_x3a_x3amain+0xeba>
		0000000000000eb6:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     eba: b0 01                        	movb	$0x1, %al
     ebc: 84 c0                        	testb	%al, %al
     ebe: 75 14                        	jne	0xed4 <fs_x5fops_x3a_x3amain+0xed4>
     ec0: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
     ec5: c6 01 01                     	movb	$0x1, (%rcx)
     ec8: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
     ecd: c7 41 04 00 00 00 00         	movl	$0x0, 0x4(%rcx)
     ed4: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
     ed9: 80 39 00                     	cmpb	$0x0, (%rcx)
     edc: 0f 84 bc 01 00 00            	je	0x109e <fs_x5fops_x3a_x3amain+0x109e>
     ee2: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     ee7: 8b 70 04                     	movl	0x4(%rax), %esi
     eea: c6 00 00                     	movb	$0x0, (%rax)
     eed: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     ef4: 48 8b 84 24 40 01 00 00      	movq	0x140(%rsp), %rax
     efc: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     f01: 48 89 84 24 e8 02 00 00      	movq	%rax, 0x2e8(%rsp)
     f09: 48 8d 8c 24 e8 02 00 00      	leaq	0x2e8(%rsp), %rcx
     f11: e9 4b f8 ff ff               	jmp	0x761 <fs_x5fops_x3a_x3amain+0x761>
     f16: 80 bc 24 48 02 00 00 00      	cmpb	$0x0, 0x248(%rsp)
     f1e: 0f 84 fa 00 00 00            	je	0x101e <fs_x5fops_x3a_x3amain+0x101e>
     f24: 80 bc 24 48 02 00 00 02      	cmpb	$0x2, 0x248(%rsp)
     f2c: 0f 85 ae 04 00 00            	jne	0x13e0 <fs_x5fops_x3a_x3amain+0x13e0>
     f32: 0f 10 84 24 00 01 00 00      	movups	0x100(%rsp), %xmm0
     f3a: 0f 10 8c 24 10 01 00 00      	movups	0x110(%rsp), %xmm1
     f42: 0f 10 94 24 20 01 00 00      	movups	0x120(%rsp), %xmm2
     f4a: 0f 10 9c 24 30 01 00 00      	movups	0x130(%rsp), %xmm3
     f52: 0f 11 84 24 28 04 00 00      	movups	%xmm0, 0x428(%rsp)
     f5a: 0f 11 8c 24 38 04 00 00      	movups	%xmm1, 0x438(%rsp)
     f62: 0f 11 94 24 48 04 00 00      	movups	%xmm2, 0x448(%rsp)
     f6a: 0f 11 9c 24 58 04 00 00      	movups	%xmm3, 0x458(%rsp)
     f72: 0f b6 84 24 30 04 00 00      	movzbl	0x430(%rsp), %eax
     f7a: 88 84 24 b8 00 00 00         	movb	%al, 0xb8(%rsp)
     f81: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     f86: c6 00 00                     	movb	$0x0, (%rax)
     f89: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     f90: 0f b6 84 24 b8 00 00 00      	movzbl	0xb8(%rsp), %eax
     f98: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
     f9d: 88 84 24 c0 00 00 00         	movb	%al, 0xc0(%rsp)
     fa4: 48 8d 8c 24 c0 00 00 00      	leaq	0xc0(%rsp), %rcx
     fac: e8 00 00 00 00               	callq	0xfb1 <fs_x5fops_x3a_x3amain+0xfb1>
		0000000000000fad:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError
     fb1: b3 01                        	movb	$0x1, %bl
     fb3: 84 db                        	testb	%bl, %bl
     fb5: 75 0d                        	jne	0xfc4 <fs_x5fops_x3a_x3amain+0xfc4>
     fb7: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     fbc: 8b 48 04                     	movl	0x4(%rax), %ecx
     fbf: e8 00 00 00 00               	callq	0xfc4 <fs_x5fops_x3a_x3amain+0xfc4>
		0000000000000fc0:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
     fc4: 84 db                        	testb	%bl, %bl
     fc6: 75 14                        	jne	0xfdc <fs_x5fops_x3a_x3amain+0xfdc>
     fc8: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     fcd: c6 00 01                     	movb	$0x1, (%rax)
     fd0: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     fd5: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     fdc: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     fe1: 80 38 00                     	cmpb	$0x0, (%rax)
     fe4: 0f 84 b2 00 00 00            	je	0x109c <fs_x5fops_x3a_x3amain+0x109c>
     fea: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
     fef: 8b 70 04                     	movl	0x4(%rax), %esi
     ff2: c6 00 00                     	movb	$0x0, (%rax)
     ff5: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
     ffc: 48 8b 84 24 40 01 00 00      	movq	0x140(%rsp), %rax
    1004: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    1009: 48 89 84 24 f8 02 00 00      	movq	%rax, 0x2f8(%rsp)
    1011: 48 8d 8c 24 f8 02 00 00      	leaq	0x2f8(%rsp), %rcx
    1019: e9 43 f7 ff ff               	jmp	0x761 <fs_x5fops_x3a_x3amain+0x761>
    101e: 0f 10 84 24 00 01 00 00      	movups	0x100(%rsp), %xmm0
    1026: 0f 10 8c 24 10 01 00 00      	movups	0x110(%rsp), %xmm1
    102e: 0f 10 94 24 20 01 00 00      	movups	0x120(%rsp), %xmm2
    1036: 0f 10 9c 24 30 01 00 00      	movups	0x130(%rsp), %xmm3
    103e: 0f 11 84 24 68 04 00 00      	movups	%xmm0, 0x468(%rsp)
    1046: 0f 11 8c 24 78 04 00 00      	movups	%xmm1, 0x478(%rsp)
    104e: 0f 11 94 24 88 04 00 00      	movups	%xmm2, 0x488(%rsp)
    1056: 0f 11 9c 24 98 04 00 00      	movups	%xmm3, 0x498(%rsp)
    105e: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    1063: 80 38 00                     	cmpb	$0x0, (%rax)
    1066: 74 34                        	je	0x109c <fs_x5fops_x3a_x3amain+0x109c>
    1068: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    106d: 8b 70 04                     	movl	0x4(%rax), %esi
    1070: c6 00 00                     	movb	$0x0, (%rax)
    1073: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
    107a: 48 8b 84 24 40 01 00 00      	movq	0x140(%rsp), %rax
    1082: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    1087: 48 89 84 24 f0 02 00 00      	movq	%rax, 0x2f0(%rsp)
    108f: 48 8d 8c 24 f0 02 00 00      	leaq	0x2f0(%rsp), %rcx
    1097: e9 c5 f6 ff ff               	jmp	0x761 <fs_x5fops_x3a_x3amain+0x761>
    109c: 31 c0                        	xorl	%eax, %eax
    109e: 88 84 24 fe 00 00 00         	movb	%al, 0xfe(%rsp)
    10a5: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    10aa: 80 38 00                     	cmpb	$0x0, (%rax)
    10ad: 0f 84 57 01 00 00            	je	0x120a <fs_x5fops_x3a_x3amain+0x120a>
    10b3: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    10b8: 8b 70 04                     	movl	0x4(%rax), %esi
    10bb: c6 00 00                     	movb	$0x0, (%rax)
    10be: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
    10c5: 48 8b 84 24 40 01 00 00      	movq	0x140(%rsp), %rax
    10cd: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    10d2: 48 89 84 24 00 03 00 00      	movq	%rax, 0x300(%rsp)
    10da: 48 8d 8c 24 00 03 00 00      	leaq	0x300(%rsp), %rcx
    10e2: e9 7a f6 ff ff               	jmp	0x761 <fs_x5fops_x3a_x3amain+0x761>
    10e7: 31 c0                        	xorl	%eax, %eax
    10e9: 84 c0                        	testb	%al, %al
    10eb: 74 0a                        	je	0x10f7 <fs_x5fops_x3a_x3amain+0x10f7>
    10ed: 0f b6 84 24 ff 00 00 00      	movzbl	0xff(%rsp), %eax
    10f5: eb 02                        	jmp	0x10f9 <fs_x5fops_x3a_x3amain+0x10f9>
    10f7: 31 c0                        	xorl	%eax, %eax
    10f9: 84 c0                        	testb	%al, %al
    10fb: 74 0e                        	je	0x110b <fs_x5fops_x3a_x3amain+0x110b>
    10fd: b8 02 00 00 00               	movl	$0x2, %eax
    1102: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x1109 <fs_x5fops_x3a_x3amain+0x1109>
		0000000000001105:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f08B05D07B5566BEF
    1109: eb 0c                        	jmp	0x1117 <fs_x5fops_x3a_x3amain+0x1117>
    110b: b8 03 00 00 00               	movl	$0x3, %eax
    1110: 48 8d 0d 00 00 00 00         	leaq	(%rip), %rcx            # 0x1117 <fs_x5fops_x3a_x3amain+0x1117>
		0000000000001113:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3aliteral_x3a_x3astring_x5f00391E19133920B8
    1117: 48 89 8c 24 10 03 00 00      	movq	%rcx, 0x310(%rsp)
    111f: 48 89 84 24 18 03 00 00      	movq	%rax, 0x318(%rsp)
    1127: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    112c: 80 38 00                     	cmpb	$0x0, (%rax)
    112f: 0f 85 4c f2 ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
    1135: 48 8b 84 24 10 03 00 00      	movq	0x310(%rsp), %rax
    113d: 48 89 84 24 50 03 00 00      	movq	%rax, 0x350(%rsp)
    1145: 48 8b 84 24 18 03 00 00      	movq	0x318(%rsp), %rax
    114d: 48 89 84 24 58 03 00 00      	movq	%rax, 0x358(%rsp)
    1155: 48 8d 94 24 50 03 00 00      	leaq	0x350(%rsp), %rdx
    115d: 48 89 f1                     	movq	%rsi, %rcx
    1160: e8 00 00 00 00               	callq	0x1165 <fs_x5fops_x3a_x3amain+0x1165>
		0000000000001161:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3afs_x3a_x3awrite_x5fstdout
    1165: 89 c6                        	movl	%eax, %esi
    1167: 89 d3                        	movl	%edx, %ebx
    1169: 88 84 24 d8 00 00 00         	movb	%al, 0xd8(%rsp)
    1170: 88 94 24 d9 00 00 00         	movb	%dl, 0xd9(%rsp)
    1177: 84 c0                        	testb	%al, %al
    1179: 0f 84 2d 01 00 00            	je	0x12ac <fs_x5fops_x3a_x3amain+0x12ac>
    117f: 80 bc 24 d8 00 00 00 01      	cmpb	$0x1, 0xd8(%rsp)
    1187: 0f 85 66 02 00 00            	jne	0x13f3 <fs_x5fops_x3a_x3amain+0x13f3>
    118d: 40 88 b4 24 f8 01 00 00      	movb	%sil, 0x1f8(%rsp)
    1195: 88 9c 24 f9 01 00 00         	movb	%bl, 0x1f9(%rsp)
    119c: 88 9c 24 e0 00 00 00         	movb	%bl, 0xe0(%rsp)
    11a3: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    11a8: c6 00 00                     	movb	$0x0, (%rax)
    11ab: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
    11b2: 0f b6 84 24 e0 00 00 00      	movzbl	0xe0(%rsp), %eax
    11ba: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    11bf: 88 84 24 e8 00 00 00         	movb	%al, 0xe8(%rsp)
    11c6: 48 8d 8c 24 e8 00 00 00      	leaq	0xe8(%rsp), %rcx
    11ce: e8 00 00 00 00               	callq	0x11d3 <fs_x5fops_x3a_x3amain+0x11d3>
		00000000000011cf:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError
    11d3: 40 b7 01                     	movb	$0x1, %dil
    11d6: 40 84 ff                     	testb	%dil, %dil
    11d9: 75 0d                        	jne	0x11e8 <fs_x5fops_x3a_x3amain+0x11e8>
    11db: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    11e0: 8b 48 04                     	movl	0x4(%rax), %ecx
    11e3: e8 00 00 00 00               	callq	0x11e8 <fs_x5fops_x3a_x3amain+0x11e8>
		00000000000011e4:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    11e8: 40 84 ff                     	testb	%dil, %dil
    11eb: 0f 85 ca 00 00 00            	jne	0x12bb <fs_x5fops_x3a_x3amain+0x12bb>
    11f1: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    11f6: c6 00 01                     	movb	$0x1, (%rax)
    11f9: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    11fe: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
    1205: e9 b1 00 00 00               	jmp	0x12bb <fs_x5fops_x3a_x3amain+0x12bb>
    120a: 48 8b 8c 24 40 01 00 00      	movq	0x140(%rsp), %rcx
    1212: e8 00 00 00 00               	callq	0x1217 <fs_x5fops_x3a_x3amain+0x1217>
		0000000000001213:  IMAGE_REL_AMD64_REL32	DirIter_x3a_x3aOpen_x3a_x3aclose
    1217: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    121c: 80 38 00                     	cmpb	$0x0, (%rax)
    121f: 0f 85 5c f1 ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
    1225: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    122a: c6 00 00                     	movb	$0x0, (%rax)
    122d: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
    1234: 48 8b 84 24 40 01 00 00      	movq	0x140(%rsp), %rax
    123c: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    1241: 48 89 84 24 08 03 00 00      	movq	%rax, 0x308(%rsp)
    1249: 48 8d 8c 24 08 03 00 00      	leaq	0x308(%rsp), %rcx
    1251: e8 00 00 00 00               	callq	0x1256 <fs_x5fops_x3a_x3amain+0x1256>
		0000000000001252:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aDirIter_x3a_x3aOpen
    1256: 31 c0                        	xorl	%eax, %eax
    1258: 84 c0                        	testb	%al, %al
    125a: 74 0d                        	je	0x1269 <fs_x5fops_x3a_x3amain+0x1269>
    125c: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    1261: 8b 48 04                     	movl	0x4(%rax), %ecx
    1264: e8 00 00 00 00               	callq	0x1269 <fs_x5fops_x3a_x3amain+0x1269>
		0000000000001265:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    1269: b0 01                        	movb	$0x1, %al
    126b: 84 c0                        	testb	%al, %al
    126d: 75 14                        	jne	0x1283 <fs_x5fops_x3a_x3amain+0x1283>
    126f: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    1274: c6 00 01                     	movb	$0x1, (%rax)
    1277: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    127c: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
    1283: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    1288: 80 38 00                     	cmpb	$0x0, (%rax)
    128b: 0f 85 f0 f0 ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
    1291: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    1296: 80 38 00                     	cmpb	$0x0, (%rax)
    1299: 0f 85 e2 f0 ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
    129f: 0f b6 84 24 fe 00 00 00      	movzbl	0xfe(%rsp), %eax
    12a7: e9 83 f5 ff ff               	jmp	0x82f <fs_x5fops_x3a_x3amain+0x82f>
    12ac: 40 88 b4 24 f0 01 00 00      	movb	%sil, 0x1f0(%rsp)
    12b4: 88 9c 24 f1 01 00 00         	movb	%bl, 0x1f1(%rsp)
    12bb: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    12c0: 80 38 00                     	cmpb	$0x0, (%rax)
    12c3: 0f 85 b8 f0 ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
    12c9: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    12ce: c6 00 00                     	movb	$0x0, (%rax)
    12d1: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
    12d8: 88 9c 24 f1 00 00 00         	movb	%bl, 0xf1(%rsp)
    12df: 40 88 b4 24 f0 00 00 00      	movb	%sil, 0xf0(%rsp)
    12e7: 40 84 f6                     	testb	%sil, %sil
    12ea: 74 36                        	je	0x1322 <fs_x5fops_x3a_x3amain+0x1322>
    12ec: 80 bc 24 f0 00 00 00 01      	cmpb	$0x1, 0xf0(%rsp)
    12f4: 0f 85 0c 01 00 00            	jne	0x1406 <fs_x5fops_x3a_x3amain+0x1406>
    12fa: 40 88 b4 24 00 02 00 00      	movb	%sil, 0x200(%rsp)
    1302: 88 9c 24 01 02 00 00         	movb	%bl, 0x201(%rsp)
    1309: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    130e: 88 9c 24 f8 00 00 00         	movb	%bl, 0xf8(%rsp)
    1315: 48 8d 8c 24 f8 00 00 00      	leaq	0xf8(%rsp), %rcx
    131d: e8 00 00 00 00               	callq	0x1322 <fs_x5fops_x3a_x3amain+0x1322>
		000000000000131e:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError
    1322: 31 c0                        	xorl	%eax, %eax
    1324: 84 c0                        	testb	%al, %al
    1326: 74 0d                        	je	0x1335 <fs_x5fops_x3a_x3amain+0x1335>
    1328: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    132d: 8b 48 04                     	movl	0x4(%rax), %ecx
    1330: e8 00 00 00 00               	callq	0x1335 <fs_x5fops_x3a_x3amain+0x1335>
		0000000000001331:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    1335: b0 01                        	movb	$0x1, %al
    1337: 84 c0                        	testb	%al, %al
    1339: 75 14                        	jne	0x134f <fs_x5fops_x3a_x3amain+0x134f>
    133b: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    1340: c6 00 01                     	movb	$0x1, (%rax)
    1343: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    1348: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%rax)
    134f: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    1354: 80 38 00                     	cmpb	$0x0, (%rax)
    1357: 0f 85 24 f0 ff ff            	jne	0x381 <fs_x5fops_x3a_x3amain+0x381>
    135d: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    1362: 80 38 00                     	cmpb	$0x0, (%rax)
    1365: e9 17 f0 ff ff               	jmp	0x381 <fs_x5fops_x3a_x3amain+0x381>
    136a: 0f b6 4c 24 28               	movzbl	0x28(%rsp), %ecx
    136f: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
    1375: e8 00 00 00 00               	callq	0x137a <fs_x5fops_x3a_x3amain+0x137a>
		0000000000001376:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    137a: 0f b6 4c 24 40               	movzbl	0x40(%rsp), %ecx
    137f: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
    1385: e8 00 00 00 00               	callq	0x138a <fs_x5fops_x3a_x3amain+0x138a>
		0000000000001386:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    138a: 0f b6 4c 24 50               	movzbl	0x50(%rsp), %ecx
    138f: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
    1395: e8 00 00 00 00               	callq	0x139a <fs_x5fops_x3a_x3amain+0x139a>
		0000000000001396:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    139a: 0f b6 4c 24 68               	movzbl	0x68(%rsp), %ecx
    139f: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
    13a5: e8 00 00 00 00               	callq	0x13aa <fs_x5fops_x3a_x3amain+0x13aa>
		00000000000013a6:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    13aa: 0f b6 4c 24 78               	movzbl	0x78(%rsp), %ecx
    13af: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
    13b5: e8 00 00 00 00               	callq	0x13ba <fs_x5fops_x3a_x3amain+0x13ba>
		00000000000013b6:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    13ba: 0f b6 8c 24 a0 00 00 00      	movzbl	0xa0(%rsp), %ecx
    13c2: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
    13c8: e8 00 00 00 00               	callq	0x13cd <fs_x5fops_x3a_x3amain+0x13cd>
		00000000000013c9:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    13cd: 0f b6 8c 24 08 02 00 00      	movzbl	0x208(%rsp), %ecx
    13d5: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
    13db: e8 00 00 00 00               	callq	0x13e0 <fs_x5fops_x3a_x3amain+0x13e0>
		00000000000013dc:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    13e0: 0f b6 8c 24 48 02 00 00      	movzbl	0x248(%rsp), %ecx
    13e8: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
    13ee: e8 00 00 00 00               	callq	0x13f3 <fs_x5fops_x3a_x3amain+0x13f3>
		00000000000013ef:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    13f3: 0f b6 8c 24 d8 00 00 00      	movzbl	0xd8(%rsp), %ecx
    13fb: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
    1401: e8 00 00 00 00               	callq	0x1406 <fs_x5fops_x3a_x3amain+0x1406>
		0000000000001402:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    1406: 0f b6 8c 24 f0 00 00 00      	movzbl	0xf0(%rsp), %ecx
    140e: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
    1414: e8 00 00 00 00               	callq	0x1419 <fs_x5fops_x3a_x3amain+0x1419>
		0000000000001415:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    1419: cc                           	int3
    141a: 66 0f 1f 44 00 00            	nopw	(%rax,%rax)

0000000000001420 <cursive_x3a_x3aruntime_x3a_x3ainit_x3a_x3afs_x5fops>:
    1420: 50                           	pushq	%rax
    1421: 48 89 0c 24                  	movq	%rcx, (%rsp)
    1425: c6 01 00                     	movb	$0x0, (%rcx)
    1428: c7 41 04 00 00 00 00         	movl	$0x0, 0x4(%rcx)
    142f: 58                           	popq	%rax
    1430: c3                           	retq
    1431: 66 66 66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	nopw	%cs:(%rax,%rax)

0000000000001440 <cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3afs_x5fops>:
    1440: 50                           	pushq	%rax
    1441: 48 89 0c 24                  	movq	%rcx, (%rsp)
    1445: c6 01 00                     	movb	$0x0, (%rcx)
    1448: c7 41 04 00 00 00 00         	movl	$0x0, 0x4(%rcx)
    144f: 58                           	popq	%rax
    1450: c3                           	retq
    1451: 66 66 66 66 66 66 2e 0f 1f 84 00 00 00 00 00 	nopw	%cs:(%rax,%rax)

0000000000001460 <main>:
    1460: 56                           	pushq	%rsi
    1461: 48 83 ec 70                  	subq	$0x70, %rsp
    1465: 48 8d 44 24 28               	leaq	0x28(%rsp), %rax
    146a: 48 89 44 24 20               	movq	%rax, 0x20(%rsp)
    146f: 48 8d 4c 24 30               	leaq	0x30(%rsp), %rcx
    1474: e8 00 00 00 00               	callq	0x1479 <main+0x19>
		0000000000001475:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3acontext_x5finit
    1479: 48 8b 44 24 48               	movq	0x48(%rsp), %rax
    147e: 48 8b 4c 24 40               	movq	0x40(%rsp), %rcx
    1483: 4c 8b 44 24 30               	movq	0x30(%rsp), %r8
    1488: 4c 8b 4c 24 38               	movq	0x38(%rsp), %r9
    148d: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    1492: c6 02 00                     	movb	$0x0, (%rdx)
    1495: c7 42 04 00 00 00 00         	movl	$0x0, 0x4(%rdx)
    149c: 80 3d ff ff ff ff 00         	cmpb	$0x0, -0x1(%rip)        # 0x14a2 <main+0x42>
		000000000000149e:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apoison_x3a_x3afs_x5fops
    14a3: 74 0f                        	je	0x14b4 <main+0x54>
    14a5: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    14aa: c6 02 01                     	movb	$0x1, (%rdx)
    14ad: c7 42 04 0a 00 00 00         	movl	$0xa, 0x4(%rdx)
    14b4: 48 8b 54 24 20               	movq	0x20(%rsp), %rdx
    14b9: 4c 89 44 24 50               	movq	%r8, 0x50(%rsp)
    14be: 4c 89 4c 24 58               	movq	%r9, 0x58(%rsp)
    14c3: 48 89 4c 24 60               	movq	%rcx, 0x60(%rsp)
    14c8: 48 89 44 24 68               	movq	%rax, 0x68(%rsp)
    14cd: 48 8d 4c 24 50               	leaq	0x50(%rsp), %rcx
    14d2: e8 00 00 00 00               	callq	0x14d7 <main+0x77>
		00000000000014d3:  IMAGE_REL_AMD64_REL32	fs_x5fops_x3a_x3amain
    14d7: 80 7c 24 28 00               	cmpb	$0x0, 0x28(%rsp)
    14dc: 75 22                        	jne	0x1500 <main+0xa0>
    14de: 89 c6                        	movl	%eax, %esi
    14e0: 48 8b 4c 24 20               	movq	0x20(%rsp), %rcx
    14e5: e8 00 00 00 00               	callq	0x14ea <main+0x8a>
		00000000000014e6:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adeinit_x3a_x3afs_x5fops
    14ea: 48 8b 44 24 20               	movq	0x20(%rsp), %rax
    14ef: 80 38 00                     	cmpb	$0x0, (%rax)
    14f2: 74 04                        	je	0x14f8 <main+0x98>
    14f4: 31 c0                        	xorl	%eax, %eax
    14f6: eb 02                        	jmp	0x14fa <main+0x9a>
    14f8: 89 f0                        	movl	%esi, %eax
    14fa: 48 83 c4 70                  	addq	$0x70, %rsp
    14fe: 5e                           	popq	%rsi
    14ff: c3                           	retq
    1500: 8b 4c 24 2c                  	movl	0x2c(%rsp), %ecx
    1504: e8 00 00 00 00               	callq	0x1509 <main+0xa9>
		0000000000001505:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
    1509: cc                           	int3

Disassembly of section .text:

0000000000000000 <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError>:
       0: 48 83 ec 38                  	subq	$0x38, %rsp
       4: 48 89 4c 24 28               	movq	%rcx, 0x28(%rsp)
       9: 48 89 54 24 30               	movq	%rdx, 0x30(%rsp)
       e: c6 02 00                     	movb	$0x0, (%rdx)
      11: c7 42 04 00 00 00 00         	movl	$0x0, 0x4(%rdx)
      18: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
      1d: 0f b6 00                     	movzbl	(%rax), %eax
      20: 88 44 24 20                  	movb	%al, 0x20(%rsp)
      24: 84 c0                        	testb	%al, %al
      26: 74 23                        	je	0x4b <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError+0x4b>
      28: 80 7c 24 20 01               	cmpb	$0x1, 0x20(%rsp)
      2d: 74 1c                        	je	0x4b <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError+0x4b>
      2f: 80 7c 24 20 02               	cmpb	$0x2, 0x20(%rsp)
      34: 74 15                        	je	0x4b <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError+0x4b>
      36: 80 7c 24 20 03               	cmpb	$0x3, 0x20(%rsp)
      3b: 74 0e                        	je	0x4b <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError+0x4b>
      3d: 80 7c 24 20 04               	cmpb	$0x4, 0x20(%rsp)
      42: 74 07                        	je	0x4b <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError+0x4b>
      44: 80 7c 24 20 05               	cmpb	$0x5, 0x20(%rsp)
      49: 75 05                        	jne	0x50 <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError+0x50>
      4b: 48 83 c4 38                  	addq	$0x38, %rsp
      4f: c3                           	retq
      50: 0f b6 4c 24 20               	movzbl	0x20(%rsp), %ecx
      55: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
      5b: e8 00 00 00 00               	callq	0x60 <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aIoError+0x60>
		000000000000005c:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
      60: cc                           	int3

Disassembly of section .text:

0000000000000000 <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aFileKind>:
       0: 48 83 ec 38                  	subq	$0x38, %rsp
       4: 48 89 4c 24 28               	movq	%rcx, 0x28(%rsp)
       9: 48 89 54 24 30               	movq	%rdx, 0x30(%rsp)
       e: c6 02 00                     	movb	$0x0, (%rdx)
      11: c7 42 04 00 00 00 00         	movl	$0x0, 0x4(%rdx)
      18: 48 8b 44 24 28               	movq	0x28(%rsp), %rax
      1d: 0f b6 00                     	movzbl	(%rax), %eax
      20: 88 44 24 20                  	movb	%al, 0x20(%rsp)
      24: 84 c0                        	testb	%al, %al
      26: 74 0e                        	je	0x36 <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aFileKind+0x36>
      28: 80 7c 24 20 01               	cmpb	$0x1, 0x20(%rsp)
      2d: 74 07                        	je	0x36 <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aFileKind+0x36>
      2f: 80 7c 24 20 02               	cmpb	$0x2, 0x20(%rsp)
      34: 75 05                        	jne	0x3b <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aFileKind+0x3b>
      36: 48 83 c4 38                  	addq	$0x38, %rsp
      3a: c3                           	retq
      3b: 0f b6 4c 24 20               	movzbl	0x20(%rsp), %ecx
      40: 81 c1 50 c3 00 00            	addl	$0xc350, %ecx           # imm = 0xC350
      46: e8 00 00 00 00               	callq	0x4b <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aFileKind+0x4b>
		0000000000000047:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3apanic
      4b: cc                           	int3

Disassembly of section .text:

0000000000000000 <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aDirIter_x3a_x3aOpen>:
       0: 48 83 ec 10                  	subq	$0x10, %rsp
       4: 48 89 0c 24                  	movq	%rcx, (%rsp)
       8: 48 89 54 24 08               	movq	%rdx, 0x8(%rsp)
       d: c6 02 00                     	movb	$0x0, (%rdx)
      10: c7 42 04 00 00 00 00         	movl	$0x0, 0x4(%rdx)
      17: 48 83 c4 10                  	addq	$0x10, %rsp
      1b: c3                           	retq

Disassembly of section .text:

0000000000000000 <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aDirEntry>:
       0: 41 57                        	pushq	%r15
       2: 41 56                        	pushq	%r14
       4: 41 55                        	pushq	%r13
       6: 41 54                        	pushq	%r12
       8: 56                           	pushq	%rsi
       9: 57                           	pushq	%rdi
       a: 55                           	pushq	%rbp
       b: 53                           	pushq	%rbx
       c: 48 81 ec 18 01 00 00         	subq	$0x118, %rsp            # imm = 0x118
      13: 48 89 8c 24 e0 00 00 00      	movq	%rcx, 0xe0(%rsp)
      1b: 48 89 54 24 30               	movq	%rdx, 0x30(%rsp)
      20: c6 02 00                     	movb	$0x0, (%rdx)
      23: c7 42 04 00 00 00 00         	movl	$0x0, 0x4(%rdx)
      2a: 48 8b 84 24 e0 00 00 00      	movq	0xe0(%rsp), %rax
      32: 0f b6 48 37                  	movzbl	0x37(%rax), %ecx
      36: 0f b6 50 36                  	movzbl	0x36(%rax), %edx
      3a: 44 0f b6 40 35               	movzbl	0x35(%rax), %r8d
      3f: 44 0f b6 48 34               	movzbl	0x34(%rax), %r9d
      44: 44 0f b6 50 33               	movzbl	0x33(%rax), %r10d
      49: 44 0f b6 58 32               	movzbl	0x32(%rax), %r11d
      4e: 44 0f b6 68 31               	movzbl	0x31(%rax), %r13d
      53: 0f b6 58 30                  	movzbl	0x30(%rax), %ebx
      57: 48 8b 70 28                  	movq	0x28(%rax), %rsi
      5b: 48 8b 78 20                  	movq	0x20(%rax), %rdi
      5f: 48 8b 68 18                  	movq	0x18(%rax), %rbp
      63: 4c 8b 70 10                  	movq	0x10(%rax), %r14
      67: 4c 8b 38                     	movq	(%rax), %r15
      6a: 4c 8b 60 08                  	movq	0x8(%rax), %r12
      6e: 4c 89 bc 24 a8 00 00 00      	movq	%r15, 0xa8(%rsp)
      76: 4c 89 a4 24 b0 00 00 00      	movq	%r12, 0xb0(%rsp)
      7e: 4c 89 b4 24 b8 00 00 00      	movq	%r14, 0xb8(%rsp)
      86: 48 89 ac 24 c0 00 00 00      	movq	%rbp, 0xc0(%rsp)
      8e: 48 89 bc 24 c8 00 00 00      	movq	%rdi, 0xc8(%rsp)
      96: 48 89 b4 24 d0 00 00 00      	movq	%rsi, 0xd0(%rsp)
      9e: 88 9c 24 d8 00 00 00         	movb	%bl, 0xd8(%rsp)
      a5: 44 88 ac 24 d9 00 00 00      	movb	%r13b, 0xd9(%rsp)
      ad: 44 88 5c 24 22               	movb	%r11b, 0x22(%rsp)
      b2: 44 88 9c 24 da 00 00 00      	movb	%r11b, 0xda(%rsp)
      ba: 44 88 54 24 23               	movb	%r10b, 0x23(%rsp)
      bf: 44 88 94 24 db 00 00 00      	movb	%r10b, 0xdb(%rsp)
      c7: 44 88 4c 24 24               	movb	%r9b, 0x24(%rsp)
      cc: 44 88 8c 24 dc 00 00 00      	movb	%r9b, 0xdc(%rsp)
      d4: 44 88 44 24 25               	movb	%r8b, 0x25(%rsp)
      d9: 44 88 84 24 dd 00 00 00      	movb	%r8b, 0xdd(%rsp)
      e1: 88 54 24 26                  	movb	%dl, 0x26(%rsp)
      e5: 88 94 24 de 00 00 00         	movb	%dl, 0xde(%rsp)
      ec: 88 4c 24 27                  	movb	%cl, 0x27(%rsp)
      f0: 88 8c 24 df 00 00 00         	movb	%cl, 0xdf(%rsp)
      f7: 0f b6 84 24 d8 00 00 00      	movzbl	0xd8(%rsp), %eax
      ff: 48 8b 54 24 30               	movq	0x30(%rsp), %rdx
     104: 88 44 24 28                  	movb	%al, 0x28(%rsp)
     108: 48 8d 4c 24 28               	leaq	0x28(%rsp), %rcx
     10d: e8 00 00 00 00               	callq	0x112 <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aDirEntry+0x112>
		000000000000010e:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aFileKind
     112: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
     117: 80 38 00                     	cmpb	$0x0, (%rax)
     11a: 0f 85 4a 01 00 00            	jne	0x26a <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aDirEntry+0x26a>
     120: 48 89 6c 24 50               	movq	%rbp, 0x50(%rsp)
     125: 48 89 7c 24 58               	movq	%rdi, 0x58(%rsp)
     12a: 4c 89 7c 24 38               	movq	%r15, 0x38(%rsp)
     12f: 4c 89 64 24 40               	movq	%r12, 0x40(%rsp)
     134: 4c 89 74 24 48               	movq	%r14, 0x48(%rsp)
     139: 48 89 74 24 60               	movq	%rsi, 0x60(%rsp)
     13e: 88 5c 24 68                  	movb	%bl, 0x68(%rsp)
     142: 44 88 6c 24 69               	movb	%r13b, 0x69(%rsp)
     147: 0f b6 44 24 22               	movzbl	0x22(%rsp), %eax
     14c: 88 44 24 6a                  	movb	%al, 0x6a(%rsp)
     150: 0f b6 44 24 23               	movzbl	0x23(%rsp), %eax
     155: 88 44 24 6b                  	movb	%al, 0x6b(%rsp)
     159: 0f b6 44 24 24               	movzbl	0x24(%rsp), %eax
     15e: 88 44 24 6c                  	movb	%al, 0x6c(%rsp)
     162: 0f b6 44 24 25               	movzbl	0x25(%rsp), %eax
     167: 88 44 24 6d                  	movb	%al, 0x6d(%rsp)
     16b: 0f b6 44 24 26               	movzbl	0x26(%rsp), %eax
     170: 88 44 24 6e                  	movb	%al, 0x6e(%rsp)
     174: 0f b6 44 24 27               	movzbl	0x27(%rsp), %eax
     179: 88 44 24 6f                  	movb	%al, 0x6f(%rsp)
     17d: 48 8b 44 24 50               	movq	0x50(%rsp), %rax
     182: 48 8b 4c 24 58               	movq	0x58(%rsp), %rcx
     187: 48 89 b4 24 f8 00 00 00      	movq	%rsi, 0xf8(%rsp)
     18f: 48 89 84 24 e8 00 00 00      	movq	%rax, 0xe8(%rsp)
     197: 48 89 8c 24 f0 00 00 00      	movq	%rcx, 0xf0(%rsp)
     19f: 48 8d 8c 24 e8 00 00 00      	leaq	0xe8(%rsp), %rcx
     1a7: e8 00 00 00 00               	callq	0x1ac <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aDirEntry+0x1ac>
		00000000000001a8:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3adrop_x5fmanaged
     1ac: 48 8b 44 24 30               	movq	0x30(%rsp), %rax
     1b1: 80 38 00                     	cmpb	$0x0, (%rax)
     1b4: 0f 85 b0 00 00 00            	jne	0x26a <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aDirEntry+0x26a>
     1ba: 4c 89 7c 24 70               	movq	%r15, 0x70(%rsp)
     1bf: 4c 89 64 24 78               	movq	%r12, 0x78(%rsp)
     1c4: 4c 89 b4 24 80 00 00 00      	movq	%r14, 0x80(%rsp)
     1cc: 48 89 ac 24 88 00 00 00      	movq	%rbp, 0x88(%rsp)
     1d4: 48 89 bc 24 90 00 00 00      	movq	%rdi, 0x90(%rsp)
     1dc: 48 89 b4 24 98 00 00 00      	movq	%rsi, 0x98(%rsp)
     1e4: 88 9c 24 a0 00 00 00         	movb	%bl, 0xa0(%rsp)
     1eb: 44 88 ac 24 a1 00 00 00      	movb	%r13b, 0xa1(%rsp)
     1f3: 0f b6 44 24 22               	movzbl	0x22(%rsp), %eax
     1f8: 88 84 24 a2 00 00 00         	movb	%al, 0xa2(%rsp)
     1ff: 0f b6 44 24 23               	movzbl	0x23(%rsp), %eax
     204: 88 84 24 a3 00 00 00         	movb	%al, 0xa3(%rsp)
     20b: 0f b6 44 24 24               	movzbl	0x24(%rsp), %eax
     210: 88 84 24 a4 00 00 00         	movb	%al, 0xa4(%rsp)
     217: 0f b6 44 24 25               	movzbl	0x25(%rsp), %eax
     21c: 88 84 24 a5 00 00 00         	movb	%al, 0xa5(%rsp)
     223: 0f b6 44 24 26               	movzbl	0x26(%rsp), %eax
     228: 88 84 24 a6 00 00 00         	movb	%al, 0xa6(%rsp)
     22f: 0f b6 44 24 27               	movzbl	0x27(%rsp), %eax
     234: 88 84 24 a7 00 00 00         	movb	%al, 0xa7(%rsp)
     23b: 48 8b 44 24 70               	movq	0x70(%rsp), %rax
     240: 48 8b 4c 24 78               	movq	0x78(%rsp), %rcx
     245: 4c 89 b4 24 10 01 00 00      	movq	%r14, 0x110(%rsp)
     24d: 48 89 84 24 00 01 00 00      	movq	%rax, 0x100(%rsp)
     255: 48 89 8c 24 08 01 00 00      	movq	%rcx, 0x108(%rsp)
     25d: 48 8d 8c 24 00 01 00 00      	leaq	0x100(%rsp), %rcx
     265: e8 00 00 00 00               	callq	0x26a <cursive_x3a_x3aruntime_x3a_x3adrop_x3a_x3aDirEntry+0x26a>
		0000000000000266:  IMAGE_REL_AMD64_REL32	cursive_x3a_x3aruntime_x3a_x3astring_x3a_x3adrop_x5fmanaged
     26a: 90                           	nop
     26b: 48 81 c4 18 01 00 00         	addq	$0x118, %rsp            # imm = 0x118
     272: 5b                           	popq	%rbx
     273: 5d                           	popq	%rbp
     274: 5f                           	popq	%rdi
     275: 5e                           	popq	%rsi
     276: 41 5c                        	popq	%r12
     278: 41 5d                        	popq	%r13
     27a: 41 5e                        	popq	%r14
     27c: 41 5f                        	popq	%r15
     27e: c3                           	retq

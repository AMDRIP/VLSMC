
ANIM.ELF:     file format elf32-i386


Disassembly of section .text:

40000000 <_start>:
40000000:	8b 0c 24             	mov    (%esp),%ecx
40000003:	8d 54 24 04          	lea    0x4(%esp),%edx
40000007:	83 e4 f0             	and    $0xfffffff0,%esp
4000000a:	54                   	push   %esp
4000000b:	6a 00                	push   $0x0
4000000d:	6a 00                	push   $0x0
4000000f:	6a 00                	push   $0x0
40000011:	52                   	push   %edx
40000012:	51                   	push   %ecx
40000013:	68 1f 00 00 40       	push   $0x4000001f
40000018:	e8 9d 02 00 00       	call   400002ba <__libc_start_main>

4000001d <_start.hang>:
4000001d:	eb fe                	jmp    4000001d <_start.hang>

4000001f <main>:
4000001f:	8d 4c 24 04          	lea    0x4(%esp),%ecx
40000023:	83 e4 f0             	and    $0xfffffff0,%esp
40000026:	ff 71 fc             	push   -0x4(%ecx)
40000029:	55                   	push   %ebp
4000002a:	89 e5                	mov    %esp,%ebp
4000002c:	51                   	push   %ecx
4000002d:	83 ec 34             	sub    $0x34,%esp
40000030:	83 ec 0c             	sub    $0xc,%esp
40000033:	68 00 10 00 40       	push   $0x40001000
40000038:	e8 dd 01 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
4000003d:	83 c4 10             	add    $0x10,%esp
40000040:	83 ec 0c             	sub    $0xc,%esp
40000043:	68 04 10 00 40       	push   $0x40001004
40000048:	e8 cd 01 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
4000004d:	83 c4 10             	add    $0x10,%esp
40000050:	83 ec 0c             	sub    $0xc,%esp
40000053:	68 28 10 00 40       	push   $0x40001028
40000058:	e8 bd 01 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
4000005d:	83 c4 10             	add    $0x10,%esp
40000060:	83 ec 0c             	sub    $0xc,%esp
40000063:	68 4c 10 00 40       	push   $0x4000104c
40000068:	e8 ad 01 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
4000006d:	83 c4 10             	add    $0x10,%esp
40000070:	c7 45 d4 71 10 00 40 	movl   $0x40001071,-0x2c(%ebp)
40000077:	c7 45 d8 88 10 00 40 	movl   $0x40001088,-0x28(%ebp)
4000007e:	c7 45 dc a8 10 00 40 	movl   $0x400010a8,-0x24(%ebp)
40000085:	c7 45 e0 c8 10 00 40 	movl   $0x400010c8,-0x20(%ebp)
4000008c:	c7 45 e4 df 10 00 40 	movl   $0x400010df,-0x1c(%ebp)
40000093:	c7 45 e8 f9 10 00 40 	movl   $0x400010f9,-0x18(%ebp)
4000009a:	c7 45 ec 06 00 00 00 	movl   $0x6,-0x14(%ebp)
400000a1:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
400000a8:	e9 bf 00 00 00       	jmp    4000016c <main+0x14d>
400000ad:	83 ec 0c             	sub    $0xc,%esp
400000b0:	68 14 11 00 40       	push   $0x40001114
400000b5:	e8 60 01 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
400000ba:	83 c4 10             	add    $0x10,%esp
400000bd:	c7 45 d0 7c 2f 2d 5c 	movl   $0x5c2d2f7c,-0x30(%ebp)
400000c4:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
400000cb:	eb 52                	jmp    4000011f <main+0x100>
400000cd:	66 c7 45 ce 00 00    	movw   $0x0,-0x32(%ebp)
400000d3:	8b 55 f0             	mov    -0x10(%ebp),%edx
400000d6:	89 d0                	mov    %edx,%eax
400000d8:	c1 f8 1f             	sar    $0x1f,%eax
400000db:	c1 e8 1e             	shr    $0x1e,%eax
400000de:	01 c2                	add    %eax,%edx
400000e0:	83 e2 03             	and    $0x3,%edx
400000e3:	29 c2                	sub    %eax,%edx
400000e5:	89 d0                	mov    %edx,%eax
400000e7:	0f b6 44 05 d0       	movzbl -0x30(%ebp,%eax,1),%eax
400000ec:	88 45 ce             	mov    %al,-0x32(%ebp)
400000ef:	83 ec 0c             	sub    $0xc,%esp
400000f2:	8d 45 ce             	lea    -0x32(%ebp),%eax
400000f5:	50                   	push   %eax
400000f6:	e8 1f 01 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
400000fb:	83 c4 10             	add    $0x10,%esp
400000fe:	83 ec 0c             	sub    $0xc,%esp
40000101:	6a 32                	push   $0x32
40000103:	e8 45 01 00 00       	call   4000024d <_ZN5vlsmc3App5sleepEj>
40000108:	83 c4 10             	add    $0x10,%esp
4000010b:	83 ec 0c             	sub    $0xc,%esp
4000010e:	68 18 11 00 40       	push   $0x40001118
40000113:	e8 02 01 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
40000118:	83 c4 10             	add    $0x10,%esp
4000011b:	83 45 f0 01          	addl   $0x1,-0x10(%ebp)
4000011f:	83 7d f0 09          	cmpl   $0x9,-0x10(%ebp)
40000123:	7e a8                	jle    400000cd <main+0xae>
40000125:	83 ec 0c             	sub    $0xc,%esp
40000128:	68 1a 11 00 40       	push   $0x4000111a
4000012d:	e8 e8 00 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
40000132:	83 c4 10             	add    $0x10,%esp
40000135:	8b 45 f4             	mov    -0xc(%ebp),%eax
40000138:	8b 44 85 d4          	mov    -0x2c(%ebp,%eax,4),%eax
4000013c:	83 ec 0c             	sub    $0xc,%esp
4000013f:	50                   	push   %eax
40000140:	e8 d5 00 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
40000145:	83 c4 10             	add    $0x10,%esp
40000148:	83 ec 0c             	sub    $0xc,%esp
4000014b:	68 1f 11 00 40       	push   $0x4000111f
40000150:	e8 c5 00 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
40000155:	83 c4 10             	add    $0x10,%esp
40000158:	83 ec 0c             	sub    $0xc,%esp
4000015b:	68 c8 00 00 00       	push   $0xc8
40000160:	e8 e8 00 00 00       	call   4000024d <_ZN5vlsmc3App5sleepEj>
40000165:	83 c4 10             	add    $0x10,%esp
40000168:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
4000016c:	8b 45 f4             	mov    -0xc(%ebp),%eax
4000016f:	3b 45 ec             	cmp    -0x14(%ebp),%eax
40000172:	0f 8c 35 ff ff ff    	jl     400000ad <main+0x8e>
40000178:	83 ec 0c             	sub    $0xc,%esp
4000017b:	68 28 11 00 40       	push   $0x40001128
40000180:	e8 95 00 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
40000185:	83 c4 10             	add    $0x10,%esp
40000188:	83 ec 0c             	sub    $0xc,%esp
4000018b:	68 50 11 00 40       	push   $0x40001150
40000190:	e8 85 00 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
40000195:	83 c4 10             	add    $0x10,%esp
40000198:	83 ec 0c             	sub    $0xc,%esp
4000019b:	68 4c 10 00 40       	push   $0x4000104c
400001a0:	e8 75 00 00 00       	call   4000021a <_ZN5vlsmc3App5printEPKc>
400001a5:	83 c4 10             	add    $0x10,%esp
400001a8:	b8 00 00 00 00       	mov    $0x0,%eax
400001ad:	8b 4d fc             	mov    -0x4(%ebp),%ecx
400001b0:	c9                   	leave
400001b1:	8d 61 fc             	lea    -0x4(%ecx),%esp
400001b4:	c3                   	ret

400001b5 <_ZL8syscall1jj>:
400001b5:	55                   	push   %ebp
400001b6:	89 e5                	mov    %esp,%ebp
400001b8:	53                   	push   %ebx
400001b9:	83 ec 10             	sub    $0x10,%esp
400001bc:	8b 45 08             	mov    0x8(%ebp),%eax
400001bf:	8b 55 0c             	mov    0xc(%ebp),%edx
400001c2:	89 d3                	mov    %edx,%ebx
400001c4:	cd 80                	int    $0x80
400001c6:	89 45 f8             	mov    %eax,-0x8(%ebp)
400001c9:	8b 45 f8             	mov    -0x8(%ebp),%eax
400001cc:	8b 5d fc             	mov    -0x4(%ebp),%ebx
400001cf:	c9                   	leave
400001d0:	c3                   	ret

400001d1 <_ZL8syscall2jjj>:
400001d1:	55                   	push   %ebp
400001d2:	89 e5                	mov    %esp,%ebp
400001d4:	53                   	push   %ebx
400001d5:	83 ec 10             	sub    $0x10,%esp
400001d8:	8b 45 08             	mov    0x8(%ebp),%eax
400001db:	8b 55 0c             	mov    0xc(%ebp),%edx
400001de:	8b 4d 10             	mov    0x10(%ebp),%ecx
400001e1:	89 d3                	mov    %edx,%ebx
400001e3:	cd 80                	int    $0x80
400001e5:	89 45 f8             	mov    %eax,-0x8(%ebp)
400001e8:	8b 45 f8             	mov    -0x8(%ebp),%eax
400001eb:	8b 5d fc             	mov    -0x4(%ebp),%ebx
400001ee:	c9                   	leave
400001ef:	c3                   	ret

400001f0 <_ZL9sys_printPKcj>:
400001f0:	55                   	push   %ebp
400001f1:	89 e5                	mov    %esp,%ebp
400001f3:	8b 45 08             	mov    0x8(%ebp),%eax
400001f6:	ff 75 0c             	push   0xc(%ebp)
400001f9:	50                   	push   %eax
400001fa:	6a 01                	push   $0x1
400001fc:	e8 d0 ff ff ff       	call   400001d1 <_ZL8syscall2jjj>
40000201:	83 c4 0c             	add    $0xc,%esp
40000204:	90                   	nop
40000205:	c9                   	leave
40000206:	c3                   	ret

40000207 <_ZL9sys_sleepj>:
40000207:	55                   	push   %ebp
40000208:	89 e5                	mov    %esp,%ebp
4000020a:	ff 75 08             	push   0x8(%ebp)
4000020d:	6a 03                	push   $0x3
4000020f:	e8 a1 ff ff ff       	call   400001b5 <_ZL8syscall1jj>
40000214:	83 c4 08             	add    $0x8,%esp
40000217:	90                   	nop
40000218:	c9                   	leave
40000219:	c3                   	ret

4000021a <_ZN5vlsmc3App5printEPKc>:
4000021a:	55                   	push   %ebp
4000021b:	89 e5                	mov    %esp,%ebp
4000021d:	83 ec 10             	sub    $0x10,%esp
40000220:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
40000227:	eb 04                	jmp    4000022d <_ZN5vlsmc3App5printEPKc+0x13>
40000229:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
4000022d:	8b 55 08             	mov    0x8(%ebp),%edx
40000230:	8b 45 fc             	mov    -0x4(%ebp),%eax
40000233:	01 d0                	add    %edx,%eax
40000235:	0f b6 00             	movzbl (%eax),%eax
40000238:	84 c0                	test   %al,%al
4000023a:	75 ed                	jne    40000229 <_ZN5vlsmc3App5printEPKc+0xf>
4000023c:	ff 75 fc             	push   -0x4(%ebp)
4000023f:	ff 75 08             	push   0x8(%ebp)
40000242:	e8 a9 ff ff ff       	call   400001f0 <_ZL9sys_printPKcj>
40000247:	83 c4 08             	add    $0x8,%esp
4000024a:	90                   	nop
4000024b:	c9                   	leave
4000024c:	c3                   	ret

4000024d <_ZN5vlsmc3App5sleepEj>:
4000024d:	55                   	push   %ebp
4000024e:	89 e5                	mov    %esp,%ebp
40000250:	ff 75 08             	push   0x8(%ebp)
40000253:	e8 af ff ff ff       	call   40000207 <_ZL9sys_sleepj>
40000258:	83 c4 04             	add    $0x4,%esp
4000025b:	90                   	nop
4000025c:	c9                   	leave
4000025d:	c3                   	ret

4000025e <__libc_csu_init>:
4000025e:	55                   	push   %ebp
4000025f:	89 e5                	mov    %esp,%ebp
40000261:	83 ec 18             	sub    $0x18,%esp
40000264:	b8 04 20 00 40       	mov    $0x40002004,%eax
40000269:	85 c0                	test   %eax,%eax
4000026b:	74 4a                	je     400002b7 <__libc_csu_init+0x59>
4000026d:	b8 04 20 00 40       	mov    $0x40002004,%eax
40000272:	85 c0                	test   %eax,%eax
40000274:	74 41                	je     400002b7 <__libc_csu_init+0x59>
40000276:	b8 04 20 00 40       	mov    $0x40002004,%eax
4000027b:	2d 04 20 00 40       	sub    $0x40002004,%eax
40000280:	c1 f8 02             	sar    $0x2,%eax
40000283:	89 45 f0             	mov    %eax,-0x10(%ebp)
40000286:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
4000028d:	eb 1e                	jmp    400002ad <__libc_csu_init+0x4f>
4000028f:	8b 45 f4             	mov    -0xc(%ebp),%eax
40000292:	8b 04 85 04 20 00 40 	mov    0x40002004(,%eax,4),%eax
40000299:	85 c0                	test   %eax,%eax
4000029b:	74 0c                	je     400002a9 <__libc_csu_init+0x4b>
4000029d:	8b 45 f4             	mov    -0xc(%ebp),%eax
400002a0:	8b 04 85 04 20 00 40 	mov    0x40002004(,%eax,4),%eax
400002a7:	ff d0                	call   *%eax
400002a9:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
400002ad:	8b 45 f4             	mov    -0xc(%ebp),%eax
400002b0:	3b 45 f0             	cmp    -0x10(%ebp),%eax
400002b3:	72 da                	jb     4000028f <__libc_csu_init+0x31>
400002b5:	eb 01                	jmp    400002b8 <__libc_csu_init+0x5a>
400002b7:	90                   	nop
400002b8:	c9                   	leave
400002b9:	c3                   	ret

400002ba <__libc_start_main>:
400002ba:	55                   	push   %ebp
400002bb:	89 e5                	mov    %esp,%ebp
400002bd:	83 ec 18             	sub    $0x18,%esp
400002c0:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
400002c4:	74 15                	je     400002db <__libc_start_main+0x21>
400002c6:	8b 45 10             	mov    0x10(%ebp),%eax
400002c9:	8b 00                	mov    (%eax),%eax
400002cb:	85 c0                	test   %eax,%eax
400002cd:	74 0c                	je     400002db <__libc_start_main+0x21>
400002cf:	8b 45 10             	mov    0x10(%ebp),%eax
400002d2:	8b 00                	mov    (%eax),%eax
400002d4:	a3 04 30 00 40       	mov    %eax,0x40003004
400002d9:	eb 0a                	jmp    400002e5 <__libc_start_main+0x2b>
400002db:	c7 05 04 30 00 40 74 	movl   $0x40001174,0x40003004
400002e2:	11 00 40 
400002e5:	8b 45 0c             	mov    0xc(%ebp),%eax
400002e8:	83 c0 01             	add    $0x1,%eax
400002eb:	8d 14 85 00 00 00 00 	lea    0x0(,%eax,4),%edx
400002f2:	8b 45 10             	mov    0x10(%ebp),%eax
400002f5:	01 d0                	add    %edx,%eax
400002f7:	a3 00 30 00 40       	mov    %eax,0x40003000
400002fc:	83 7d 14 00          	cmpl   $0x0,0x14(%ebp)
40000300:	74 05                	je     40000307 <__libc_start_main+0x4d>
40000302:	8b 45 14             	mov    0x14(%ebp),%eax
40000305:	ff d0                	call   *%eax
40000307:	e8 52 ff ff ff       	call   4000025e <__libc_csu_init>
4000030c:	8b 45 08             	mov    0x8(%ebp),%eax
4000030f:	8b 15 00 30 00 40    	mov    0x40003000,%edx
40000315:	83 ec 04             	sub    $0x4,%esp
40000318:	52                   	push   %edx
40000319:	ff 75 10             	push   0x10(%ebp)
4000031c:	ff 75 0c             	push   0xc(%ebp)
4000031f:	ff d0                	call   *%eax
40000321:	83 c4 10             	add    $0x10,%esp
40000324:	89 45 f4             	mov    %eax,-0xc(%ebp)
40000327:	83 ec 0c             	sub    $0xc,%esp
4000032a:	ff 75 f4             	push   -0xc(%ebp)
4000032d:	e8 08 00 00 00       	call   4000033a <exit>
40000332:	83 c4 10             	add    $0x10,%esp
40000335:	8b 45 f4             	mov    -0xc(%ebp),%eax
40000338:	c9                   	leave
40000339:	c3                   	ret

4000033a <exit>:
4000033a:	55                   	push   %ebp
4000033b:	89 e5                	mov    %esp,%ebp
4000033d:	83 ec 08             	sub    $0x8,%esp
40000340:	83 ec 08             	sub    $0x8,%esp
40000343:	ff 75 08             	push   0x8(%ebp)
40000346:	6a 00                	push   $0x0
40000348:	e8 71 03 00 00       	call   400006be <syscall>
4000034d:	83 c4 10             	add    $0x10,%esp
40000350:	90                   	nop
40000351:	eb fd                	jmp    40000350 <exit+0x16>

40000353 <abort>:
40000353:	55                   	push   %ebp
40000354:	89 e5                	mov    %esp,%ebp
40000356:	83 ec 08             	sub    $0x8,%esp
40000359:	83 ec 0c             	sub    $0xc,%esp
4000035c:	6a 01                	push   $0x1
4000035e:	e8 d7 ff ff ff       	call   4000033a <exit>
40000363:	83 c4 10             	add    $0x10,%esp
40000366:	90                   	nop
40000367:	c9                   	leave
40000368:	c3                   	ret

40000369 <atoi>:
40000369:	55                   	push   %ebp
4000036a:	89 e5                	mov    %esp,%ebp
4000036c:	83 ec 10             	sub    $0x10,%esp
4000036f:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
40000376:	c7 45 f8 01 00 00 00 	movl   $0x1,-0x8(%ebp)
4000037d:	eb 04                	jmp    40000383 <atoi+0x1a>
4000037f:	83 45 08 01          	addl   $0x1,0x8(%ebp)
40000383:	8b 45 08             	mov    0x8(%ebp),%eax
40000386:	0f b6 00             	movzbl (%eax),%eax
40000389:	3c 20                	cmp    $0x20,%al
4000038b:	74 f2                	je     4000037f <atoi+0x16>
4000038d:	8b 45 08             	mov    0x8(%ebp),%eax
40000390:	0f b6 00             	movzbl (%eax),%eax
40000393:	3c 09                	cmp    $0x9,%al
40000395:	74 e8                	je     4000037f <atoi+0x16>
40000397:	8b 45 08             	mov    0x8(%ebp),%eax
4000039a:	0f b6 00             	movzbl (%eax),%eax
4000039d:	3c 0a                	cmp    $0xa,%al
4000039f:	74 de                	je     4000037f <atoi+0x16>
400003a1:	8b 45 08             	mov    0x8(%ebp),%eax
400003a4:	0f b6 00             	movzbl (%eax),%eax
400003a7:	3c 2d                	cmp    $0x2d,%al
400003a9:	75 0d                	jne    400003b8 <atoi+0x4f>
400003ab:	c7 45 f8 ff ff ff ff 	movl   $0xffffffff,-0x8(%ebp)
400003b2:	83 45 08 01          	addl   $0x1,0x8(%ebp)
400003b6:	eb 33                	jmp    400003eb <atoi+0x82>
400003b8:	8b 45 08             	mov    0x8(%ebp),%eax
400003bb:	0f b6 00             	movzbl (%eax),%eax
400003be:	3c 2b                	cmp    $0x2b,%al
400003c0:	75 29                	jne    400003eb <atoi+0x82>
400003c2:	83 45 08 01          	addl   $0x1,0x8(%ebp)
400003c6:	eb 23                	jmp    400003eb <atoi+0x82>
400003c8:	8b 55 fc             	mov    -0x4(%ebp),%edx
400003cb:	89 d0                	mov    %edx,%eax
400003cd:	c1 e0 02             	shl    $0x2,%eax
400003d0:	01 d0                	add    %edx,%eax
400003d2:	01 c0                	add    %eax,%eax
400003d4:	89 c2                	mov    %eax,%edx
400003d6:	8b 45 08             	mov    0x8(%ebp),%eax
400003d9:	0f b6 00             	movzbl (%eax),%eax
400003dc:	0f be c0             	movsbl %al,%eax
400003df:	83 e8 30             	sub    $0x30,%eax
400003e2:	01 d0                	add    %edx,%eax
400003e4:	89 45 fc             	mov    %eax,-0x4(%ebp)
400003e7:	83 45 08 01          	addl   $0x1,0x8(%ebp)
400003eb:	8b 45 08             	mov    0x8(%ebp),%eax
400003ee:	0f b6 00             	movzbl (%eax),%eax
400003f1:	3c 2f                	cmp    $0x2f,%al
400003f3:	7e 0a                	jle    400003ff <atoi+0x96>
400003f5:	8b 45 08             	mov    0x8(%ebp),%eax
400003f8:	0f b6 00             	movzbl (%eax),%eax
400003fb:	3c 39                	cmp    $0x39,%al
400003fd:	7e c9                	jle    400003c8 <atoi+0x5f>
400003ff:	8b 45 fc             	mov    -0x4(%ebp),%eax
40000402:	0f af 45 f8          	imul   -0x8(%ebp),%eax
40000406:	c9                   	leave
40000407:	c3                   	ret

40000408 <_ZL14reverse_stringPci>:
40000408:	55                   	push   %ebp
40000409:	89 e5                	mov    %esp,%ebp
4000040b:	83 ec 10             	sub    $0x10,%esp
4000040e:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
40000415:	8b 45 0c             	mov    0xc(%ebp),%eax
40000418:	83 e8 01             	sub    $0x1,%eax
4000041b:	89 45 f8             	mov    %eax,-0x8(%ebp)
4000041e:	eb 39                	jmp    40000459 <_ZL14reverse_stringPci+0x51>
40000420:	8b 55 fc             	mov    -0x4(%ebp),%edx
40000423:	8b 45 08             	mov    0x8(%ebp),%eax
40000426:	01 d0                	add    %edx,%eax
40000428:	0f b6 00             	movzbl (%eax),%eax
4000042b:	88 45 f7             	mov    %al,-0x9(%ebp)
4000042e:	8b 55 f8             	mov    -0x8(%ebp),%edx
40000431:	8b 45 08             	mov    0x8(%ebp),%eax
40000434:	01 d0                	add    %edx,%eax
40000436:	8b 4d fc             	mov    -0x4(%ebp),%ecx
40000439:	8b 55 08             	mov    0x8(%ebp),%edx
4000043c:	01 ca                	add    %ecx,%edx
4000043e:	0f b6 00             	movzbl (%eax),%eax
40000441:	88 02                	mov    %al,(%edx)
40000443:	8b 55 f8             	mov    -0x8(%ebp),%edx
40000446:	8b 45 08             	mov    0x8(%ebp),%eax
40000449:	01 c2                	add    %eax,%edx
4000044b:	0f b6 45 f7          	movzbl -0x9(%ebp),%eax
4000044f:	88 02                	mov    %al,(%edx)
40000451:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
40000455:	83 6d f8 01          	subl   $0x1,-0x8(%ebp)
40000459:	8b 45 fc             	mov    -0x4(%ebp),%eax
4000045c:	3b 45 f8             	cmp    -0x8(%ebp),%eax
4000045f:	7c bf                	jl     40000420 <_ZL14reverse_stringPci+0x18>
40000461:	90                   	nop
40000462:	90                   	nop
40000463:	c9                   	leave
40000464:	c3                   	ret

40000465 <itoa>:
40000465:	55                   	push   %ebp
40000466:	89 e5                	mov    %esp,%ebp
40000468:	53                   	push   %ebx
40000469:	83 ec 10             	sub    $0x10,%esp
4000046c:	83 7d 10 01          	cmpl   $0x1,0x10(%ebp)
40000470:	7e 06                	jle    40000478 <itoa+0x13>
40000472:	83 7d 10 24          	cmpl   $0x24,0x10(%ebp)
40000476:	7e 0e                	jle    40000486 <itoa+0x21>
40000478:	8b 45 0c             	mov    0xc(%ebp),%eax
4000047b:	c6 00 00             	movb   $0x0,(%eax)
4000047e:	8b 45 0c             	mov    0xc(%ebp),%eax
40000481:	e9 d9 00 00 00       	jmp    4000055f <itoa+0xfa>
40000486:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
4000048d:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
40000494:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
40000498:	75 24                	jne    400004be <itoa+0x59>
4000049a:	8b 4d 0c             	mov    0xc(%ebp),%ecx
4000049d:	8b 45 f8             	mov    -0x8(%ebp),%eax
400004a0:	8d 50 01             	lea    0x1(%eax),%edx
400004a3:	89 55 f8             	mov    %edx,-0x8(%ebp)
400004a6:	01 c8                	add    %ecx,%eax
400004a8:	c6 00 30             	movb   $0x30,(%eax)
400004ab:	8b 55 f8             	mov    -0x8(%ebp),%edx
400004ae:	8b 45 0c             	mov    0xc(%ebp),%eax
400004b1:	01 d0                	add    %edx,%eax
400004b3:	c6 00 00             	movb   $0x0,(%eax)
400004b6:	8b 45 0c             	mov    0xc(%ebp),%eax
400004b9:	e9 a1 00 00 00       	jmp    4000055f <itoa+0xfa>
400004be:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
400004c2:	79 10                	jns    400004d4 <itoa+0x6f>
400004c4:	83 7d 10 0a          	cmpl   $0xa,0x10(%ebp)
400004c8:	75 0a                	jne    400004d4 <itoa+0x6f>
400004ca:	c7 45 f4 01 00 00 00 	movl   $0x1,-0xc(%ebp)
400004d1:	f7 5d 08             	negl   0x8(%ebp)
400004d4:	8b 45 08             	mov    0x8(%ebp),%eax
400004d7:	89 45 f0             	mov    %eax,-0x10(%ebp)
400004da:	eb 4a                	jmp    40000526 <itoa+0xc1>
400004dc:	8b 4d 10             	mov    0x10(%ebp),%ecx
400004df:	8b 45 f0             	mov    -0x10(%ebp),%eax
400004e2:	ba 00 00 00 00       	mov    $0x0,%edx
400004e7:	f7 f1                	div    %ecx
400004e9:	89 d0                	mov    %edx,%eax
400004eb:	89 45 ec             	mov    %eax,-0x14(%ebp)
400004ee:	83 7d ec 09          	cmpl   $0x9,-0x14(%ebp)
400004f2:	7e 0a                	jle    400004fe <itoa+0x99>
400004f4:	8b 45 ec             	mov    -0x14(%ebp),%eax
400004f7:	83 c0 57             	add    $0x57,%eax
400004fa:	89 c3                	mov    %eax,%ebx
400004fc:	eb 08                	jmp    40000506 <itoa+0xa1>
400004fe:	8b 45 ec             	mov    -0x14(%ebp),%eax
40000501:	83 c0 30             	add    $0x30,%eax
40000504:	89 c3                	mov    %eax,%ebx
40000506:	8b 4d 0c             	mov    0xc(%ebp),%ecx
40000509:	8b 45 f8             	mov    -0x8(%ebp),%eax
4000050c:	8d 50 01             	lea    0x1(%eax),%edx
4000050f:	89 55 f8             	mov    %edx,-0x8(%ebp)
40000512:	01 c8                	add    %ecx,%eax
40000514:	88 18                	mov    %bl,(%eax)
40000516:	8b 5d 10             	mov    0x10(%ebp),%ebx
40000519:	8b 45 f0             	mov    -0x10(%ebp),%eax
4000051c:	ba 00 00 00 00       	mov    $0x0,%edx
40000521:	f7 f3                	div    %ebx
40000523:	89 45 f0             	mov    %eax,-0x10(%ebp)
40000526:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
4000052a:	75 b0                	jne    400004dc <itoa+0x77>
4000052c:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
40000530:	74 11                	je     40000543 <itoa+0xde>
40000532:	8b 4d 0c             	mov    0xc(%ebp),%ecx
40000535:	8b 45 f8             	mov    -0x8(%ebp),%eax
40000538:	8d 50 01             	lea    0x1(%eax),%edx
4000053b:	89 55 f8             	mov    %edx,-0x8(%ebp)
4000053e:	01 c8                	add    %ecx,%eax
40000540:	c6 00 2d             	movb   $0x2d,(%eax)
40000543:	8b 55 f8             	mov    -0x8(%ebp),%edx
40000546:	8b 45 0c             	mov    0xc(%ebp),%eax
40000549:	01 d0                	add    %edx,%eax
4000054b:	c6 00 00             	movb   $0x0,(%eax)
4000054e:	ff 75 f8             	push   -0x8(%ebp)
40000551:	ff 75 0c             	push   0xc(%ebp)
40000554:	e8 af fe ff ff       	call   40000408 <_ZL14reverse_stringPci>
40000559:	83 c4 08             	add    $0x8,%esp
4000055c:	8b 45 0c             	mov    0xc(%ebp),%eax
4000055f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
40000562:	c9                   	leave
40000563:	c3                   	ret

40000564 <abs>:
40000564:	55                   	push   %ebp
40000565:	89 e5                	mov    %esp,%ebp
40000567:	8b 45 08             	mov    0x8(%ebp),%eax
4000056a:	89 c2                	mov    %eax,%edx
4000056c:	f7 da                	neg    %edx
4000056e:	0f 49 c2             	cmovns %edx,%eax
40000571:	5d                   	pop    %ebp
40000572:	c3                   	ret

40000573 <labs>:
40000573:	55                   	push   %ebp
40000574:	89 e5                	mov    %esp,%ebp
40000576:	8b 45 08             	mov    0x8(%ebp),%eax
40000579:	89 c2                	mov    %eax,%edx
4000057b:	f7 da                	neg    %edx
4000057d:	0f 49 c2             	cmovns %edx,%eax
40000580:	5d                   	pop    %ebp
40000581:	c3                   	ret

40000582 <div>:
40000582:	55                   	push   %ebp
40000583:	89 e5                	mov    %esp,%ebp
40000585:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
40000589:	75 15                	jne    400005a0 <div+0x1e>
4000058b:	8b 45 08             	mov    0x8(%ebp),%eax
4000058e:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
40000594:	8b 45 08             	mov    0x8(%ebp),%eax
40000597:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
4000059e:	eb 1c                	jmp    400005bc <div+0x3a>
400005a0:	8b 45 0c             	mov    0xc(%ebp),%eax
400005a3:	99                   	cltd
400005a4:	f7 7d 10             	idivl  0x10(%ebp)
400005a7:	89 c2                	mov    %eax,%edx
400005a9:	8b 45 08             	mov    0x8(%ebp),%eax
400005ac:	89 10                	mov    %edx,(%eax)
400005ae:	8b 45 0c             	mov    0xc(%ebp),%eax
400005b1:	99                   	cltd
400005b2:	f7 7d 10             	idivl  0x10(%ebp)
400005b5:	8b 45 08             	mov    0x8(%ebp),%eax
400005b8:	89 50 04             	mov    %edx,0x4(%eax)
400005bb:	90                   	nop
400005bc:	8b 45 08             	mov    0x8(%ebp),%eax
400005bf:	5d                   	pop    %ebp
400005c0:	c2 04 00             	ret    $0x4

400005c3 <ldiv>:
400005c3:	55                   	push   %ebp
400005c4:	89 e5                	mov    %esp,%ebp
400005c6:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
400005ca:	75 15                	jne    400005e1 <ldiv+0x1e>
400005cc:	8b 45 08             	mov    0x8(%ebp),%eax
400005cf:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
400005d5:	8b 45 08             	mov    0x8(%ebp),%eax
400005d8:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
400005df:	eb 1c                	jmp    400005fd <ldiv+0x3a>
400005e1:	8b 45 0c             	mov    0xc(%ebp),%eax
400005e4:	99                   	cltd
400005e5:	f7 7d 10             	idivl  0x10(%ebp)
400005e8:	89 c2                	mov    %eax,%edx
400005ea:	8b 45 08             	mov    0x8(%ebp),%eax
400005ed:	89 10                	mov    %edx,(%eax)
400005ef:	8b 45 0c             	mov    0xc(%ebp),%eax
400005f2:	99                   	cltd
400005f3:	f7 7d 10             	idivl  0x10(%ebp)
400005f6:	8b 45 08             	mov    0x8(%ebp),%eax
400005f9:	89 50 04             	mov    %edx,0x4(%eax)
400005fc:	90                   	nop
400005fd:	8b 45 08             	mov    0x8(%ebp),%eax
40000600:	5d                   	pop    %ebp
40000601:	c2 04 00             	ret    $0x4

40000604 <rand>:
40000604:	55                   	push   %ebp
40000605:	89 e5                	mov    %esp,%ebp
40000607:	a1 00 20 00 40       	mov    0x40002000,%eax
4000060c:	69 c0 6d 4e c6 41    	imul   $0x41c64e6d,%eax,%eax
40000612:	05 39 30 00 00       	add    $0x3039,%eax
40000617:	a3 00 20 00 40       	mov    %eax,0x40002000
4000061c:	a1 00 20 00 40       	mov    0x40002000,%eax
40000621:	c1 e8 10             	shr    $0x10,%eax
40000624:	25 ff 7f 00 00       	and    $0x7fff,%eax
40000629:	5d                   	pop    %ebp
4000062a:	c3                   	ret

4000062b <srand>:
4000062b:	55                   	push   %ebp
4000062c:	89 e5                	mov    %esp,%ebp
4000062e:	8b 45 08             	mov    0x8(%ebp),%eax
40000631:	a3 00 20 00 40       	mov    %eax,0x40002000
40000636:	90                   	nop
40000637:	5d                   	pop    %ebp
40000638:	c3                   	ret

40000639 <fork>:
40000639:	55                   	push   %ebp
4000063a:	89 e5                	mov    %esp,%ebp
4000063c:	83 ec 08             	sub    $0x8,%esp
4000063f:	83 ec 0c             	sub    $0xc,%esp
40000642:	6a 22                	push   $0x22
40000644:	e8 75 00 00 00       	call   400006be <syscall>
40000649:	83 c4 10             	add    $0x10,%esp
4000064c:	c9                   	leave
4000064d:	c3                   	ret

4000064e <execve>:
4000064e:	55                   	push   %ebp
4000064f:	89 e5                	mov    %esp,%ebp
40000651:	83 ec 08             	sub    $0x8,%esp
40000654:	8b 4d 10             	mov    0x10(%ebp),%ecx
40000657:	8b 55 0c             	mov    0xc(%ebp),%edx
4000065a:	8b 45 08             	mov    0x8(%ebp),%eax
4000065d:	51                   	push   %ecx
4000065e:	52                   	push   %edx
4000065f:	50                   	push   %eax
40000660:	6a 23                	push   $0x23
40000662:	e8 57 00 00 00       	call   400006be <syscall>
40000667:	83 c4 10             	add    $0x10,%esp
4000066a:	c9                   	leave
4000066b:	c3                   	ret

4000066c <exec>:
4000066c:	55                   	push   %ebp
4000066d:	89 e5                	mov    %esp,%ebp
4000066f:	83 ec 18             	sub    $0x18,%esp
40000672:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
40000679:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
40000680:	8b 45 08             	mov    0x8(%ebp),%eax
40000683:	89 45 f0             	mov    %eax,-0x10(%ebp)
40000686:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
4000068d:	83 ec 04             	sub    $0x4,%esp
40000690:	8d 45 ec             	lea    -0x14(%ebp),%eax
40000693:	50                   	push   %eax
40000694:	8d 45 f0             	lea    -0x10(%ebp),%eax
40000697:	50                   	push   %eax
40000698:	ff 75 08             	push   0x8(%ebp)
4000069b:	e8 ae ff ff ff       	call   4000064e <execve>
400006a0:	83 c4 10             	add    $0x10,%esp
400006a3:	c9                   	leave
400006a4:	c3                   	ret

400006a5 <wait>:
400006a5:	55                   	push   %ebp
400006a6:	89 e5                	mov    %esp,%ebp
400006a8:	83 ec 08             	sub    $0x8,%esp
400006ab:	8b 45 08             	mov    0x8(%ebp),%eax
400006ae:	83 ec 08             	sub    $0x8,%esp
400006b1:	50                   	push   %eax
400006b2:	6a 24                	push   $0x24
400006b4:	e8 05 00 00 00       	call   400006be <syscall>
400006b9:	83 c4 10             	add    $0x10,%esp
400006bc:	c9                   	leave
400006bd:	c3                   	ret

400006be <syscall>:
400006be:	55                   	push   %ebp
400006bf:	89 e5                	mov    %esp,%ebp
400006c1:	57                   	push   %edi
400006c2:	56                   	push   %esi
400006c3:	53                   	push   %ebx
400006c4:	83 ec 2c             	sub    $0x2c,%esp
400006c7:	8d 45 0c             	lea    0xc(%ebp),%eax
400006ca:	89 45 cc             	mov    %eax,-0x34(%ebp)
400006cd:	8b 45 cc             	mov    -0x34(%ebp),%eax
400006d0:	8d 50 04             	lea    0x4(%eax),%edx
400006d3:	89 55 cc             	mov    %edx,-0x34(%ebp)
400006d6:	8b 00                	mov    (%eax),%eax
400006d8:	89 45 e4             	mov    %eax,-0x1c(%ebp)
400006db:	8b 45 cc             	mov    -0x34(%ebp),%eax
400006de:	8d 50 04             	lea    0x4(%eax),%edx
400006e1:	89 55 cc             	mov    %edx,-0x34(%ebp)
400006e4:	8b 00                	mov    (%eax),%eax
400006e6:	89 45 e0             	mov    %eax,-0x20(%ebp)
400006e9:	8b 45 cc             	mov    -0x34(%ebp),%eax
400006ec:	8d 50 04             	lea    0x4(%eax),%edx
400006ef:	89 55 cc             	mov    %edx,-0x34(%ebp)
400006f2:	8b 00                	mov    (%eax),%eax
400006f4:	89 45 dc             	mov    %eax,-0x24(%ebp)
400006f7:	8b 45 cc             	mov    -0x34(%ebp),%eax
400006fa:	8d 50 04             	lea    0x4(%eax),%edx
400006fd:	89 55 cc             	mov    %edx,-0x34(%ebp)
40000700:	8b 00                	mov    (%eax),%eax
40000702:	89 45 d8             	mov    %eax,-0x28(%ebp)
40000705:	8b 45 cc             	mov    -0x34(%ebp),%eax
40000708:	8d 50 04             	lea    0x4(%eax),%edx
4000070b:	89 55 cc             	mov    %edx,-0x34(%ebp)
4000070e:	8b 00                	mov    (%eax),%eax
40000710:	89 45 d4             	mov    %eax,-0x2c(%ebp)
40000713:	8b 45 08             	mov    0x8(%ebp),%eax
40000716:	8b 5d e4             	mov    -0x1c(%ebp),%ebx
40000719:	8b 4d e0             	mov    -0x20(%ebp),%ecx
4000071c:	8b 55 dc             	mov    -0x24(%ebp),%edx
4000071f:	8b 75 d8             	mov    -0x28(%ebp),%esi
40000722:	8b 7d d4             	mov    -0x2c(%ebp),%edi
40000725:	cd 80                	int    $0x80
40000727:	89 45 d0             	mov    %eax,-0x30(%ebp)
4000072a:	83 7d d0 00          	cmpl   $0x0,-0x30(%ebp)
4000072e:	79 1e                	jns    4000074e <syscall+0x90>
40000730:	81 7d d0 01 f0 ff ff 	cmpl   $0xfffff001,-0x30(%ebp)
40000737:	7c 15                	jl     4000074e <syscall+0x90>
40000739:	8b 45 d0             	mov    -0x30(%ebp),%eax
4000073c:	f7 d8                	neg    %eax
4000073e:	89 c3                	mov    %eax,%ebx
40000740:	e8 14 00 00 00       	call   40000759 <__errno_location>
40000745:	89 18                	mov    %ebx,(%eax)
40000747:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
4000074c:	eb 03                	jmp    40000751 <syscall+0x93>
4000074e:	8b 45 d0             	mov    -0x30(%ebp),%eax
40000751:	83 c4 2c             	add    $0x2c,%esp
40000754:	5b                   	pop    %ebx
40000755:	5e                   	pop    %esi
40000756:	5f                   	pop    %edi
40000757:	5d                   	pop    %ebp
40000758:	c3                   	ret

40000759 <__errno_location>:
40000759:	55                   	push   %ebp
4000075a:	89 e5                	mov    %esp,%ebp
4000075c:	b8 08 30 00 40       	mov    $0x40003008,%eax
40000761:	5d                   	pop    %ebp
40000762:	c3                   	ret

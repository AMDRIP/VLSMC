
CAT_TEST.ELF:     file format elf32-i386


Disassembly of section .text:

00400000 <_start>:
  400000:	e8 45 00 00 00       	call   40004a <main>
  400005:	b8 00 00 00 00       	mov    $0x0,%eax
  40000a:	cd 80                	int    $0x80

0040000c <_start.hang>:
  40000c:	eb fe                	jmp    40000c <_start.hang>

0040000e <_ZL8syscall3jjjj>:
  40000e:	55                   	push   %ebp
  40000f:	89 e5                	mov    %esp,%ebp
  400011:	53                   	push   %ebx
  400012:	83 ec 10             	sub    $0x10,%esp
  400015:	8b 45 08             	mov    0x8(%ebp),%eax
  400018:	8b 5d 0c             	mov    0xc(%ebp),%ebx
  40001b:	8b 4d 10             	mov    0x10(%ebp),%ecx
  40001e:	8b 55 14             	mov    0x14(%ebp),%edx
  400021:	cd 80                	int    $0x80
  400023:	89 45 f8             	mov    %eax,-0x8(%ebp)
  400026:	8b 45 f8             	mov    -0x8(%ebp),%eax
  400029:	8b 5d fc             	mov    -0x4(%ebp),%ebx
  40002c:	c9                   	leave
  40002d:	c3                   	ret

0040002e <__cxa_pure_virtual>:
  40002e:	55                   	push   %ebp
  40002f:	89 e5                	mov    %esp,%ebp
  400031:	90                   	nop
  400032:	eb fd                	jmp    400031 <__cxa_pure_virtual+0x3>

00400034 <_Znwj>:
  400034:	55                   	push   %ebp
  400035:	89 e5                	mov    %esp,%ebp
  400037:	b8 00 00 00 00       	mov    $0x0,%eax
  40003c:	5d                   	pop    %ebp
  40003d:	c3                   	ret

0040003e <_ZdlPv>:
  40003e:	55                   	push   %ebp
  40003f:	89 e5                	mov    %esp,%ebp
  400041:	90                   	nop
  400042:	5d                   	pop    %ebp
  400043:	c3                   	ret

00400044 <_ZdlPvj>:
  400044:	55                   	push   %ebp
  400045:	89 e5                	mov    %esp,%ebp
  400047:	90                   	nop
  400048:	5d                   	pop    %ebp
  400049:	c3                   	ret

0040004a <main>:
  40004a:	8d 4c 24 04          	lea    0x4(%esp),%ecx
  40004e:	83 e4 f0             	and    $0xfffffff0,%esp
  400051:	ff 71 fc             	push   -0x4(%ecx)
  400054:	55                   	push   %ebp
  400055:	89 e5                	mov    %esp,%ebp
  400057:	51                   	push   %ecx
  400058:	81 ec 24 02 00 00    	sub    $0x224,%esp
  40005e:	83 ec 0c             	sub    $0xc,%esp
  400061:	68 00 10 40 00       	push   $0x401000
  400066:	e8 ee 01 00 00       	call   400259 <_ZN5vlsmc3App5printEPKc>
  40006b:	83 c4 10             	add    $0x10,%esp
  40006e:	83 ec 0c             	sub    $0xc,%esp
  400071:	68 35 10 40 00       	push   $0x401035
  400076:	e8 3d 02 00 00       	call   4002b8 <_ZN5vlsmc3App11find_threadEPKc>
  40007b:	83 c4 10             	add    $0x10,%esp
  40007e:	89 45 f0             	mov    %eax,-0x10(%ebp)
  400081:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
  400085:	79 1a                	jns    4000a1 <main+0x57>
  400087:	83 ec 0c             	sub    $0xc,%esp
  40008a:	68 44 10 40 00       	push   $0x401044
  40008f:	e8 c5 01 00 00       	call   400259 <_ZN5vlsmc3App5printEPKc>
  400094:	83 c4 10             	add    $0x10,%esp
  400097:	b8 01 00 00 00       	mov    $0x1,%eax
  40009c:	e9 17 01 00 00       	jmp    4001b8 <main+0x16e>
  4000a1:	c7 45 ec 71 10 40 00 	movl   $0x401071,-0x14(%ebp)
  4000a8:	c7 45 e8 0b 00 00 00 	movl   $0xb,-0x18(%ebp)
  4000af:	8b 45 e8             	mov    -0x18(%ebp),%eax
  4000b2:	83 ec 04             	sub    $0x4,%esp
  4000b5:	50                   	push   %eax
  4000b6:	ff 75 ec             	push   -0x14(%ebp)
  4000b9:	ff 75 f0             	push   -0x10(%ebp)
  4000bc:	e8 cb 01 00 00       	call   40028c <_ZN5vlsmc3App8msg_sendEiPKvj>
  4000c1:	83 c4 10             	add    $0x10,%esp
  4000c4:	89 45 e4             	mov    %eax,-0x1c(%ebp)
  4000c7:	83 7d e4 00          	cmpl   $0x0,-0x1c(%ebp)
  4000cb:	79 1a                	jns    4000e7 <main+0x9d>
  4000cd:	83 ec 0c             	sub    $0xc,%esp
  4000d0:	68 80 10 40 00       	push   $0x401080
  4000d5:	e8 7f 01 00 00       	call   400259 <_ZN5vlsmc3App5printEPKc>
  4000da:	83 c4 10             	add    $0x10,%esp
  4000dd:	b8 01 00 00 00       	mov    $0x1,%eax
  4000e2:	e9 d1 00 00 00       	jmp    4001b8 <main+0x16e>
  4000e7:	83 ec 0c             	sub    $0xc,%esp
  4000ea:	68 ac 10 40 00       	push   $0x4010ac
  4000ef:	e8 65 01 00 00       	call   400259 <_ZN5vlsmc3App5printEPKc>
  4000f4:	83 c4 10             	add    $0x10,%esp
  4000f7:	83 ec 04             	sub    $0x4,%esp
  4000fa:	68 00 02 00 00       	push   $0x200
  4000ff:	8d 85 e0 fd ff ff    	lea    -0x220(%ebp),%eax
  400105:	50                   	push   %eax
  400106:	8d 85 dc fd ff ff    	lea    -0x224(%ebp),%eax
  40010c:	50                   	push   %eax
  40010d:	e8 90 01 00 00       	call   4002a2 <_ZN5vlsmc3App8msg_recvEPiPvj>
  400112:	83 c4 10             	add    $0x10,%esp
  400115:	89 45 e0             	mov    %eax,-0x20(%ebp)
  400118:	83 7d e0 00          	cmpl   $0x0,-0x20(%ebp)
  40011c:	7e d9                	jle    4000f7 <main+0xad>
  40011e:	83 ec 0c             	sub    $0xc,%esp
  400121:	68 d9 10 40 00       	push   $0x4010d9
  400126:	e8 2e 01 00 00       	call   400259 <_ZN5vlsmc3App5printEPKc>
  40012b:	83 c4 10             	add    $0x10,%esp
  40012e:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
  400135:	eb 4a                	jmp    400181 <main+0x137>
  400137:	8d 95 e0 fd ff ff    	lea    -0x220(%ebp),%edx
  40013d:	8b 45 f4             	mov    -0xc(%ebp),%eax
  400140:	01 d0                	add    %edx,%eax
  400142:	83 ec 0c             	sub    $0xc,%esp
  400145:	50                   	push   %eax
  400146:	e8 0e 01 00 00       	call   400259 <_ZN5vlsmc3App5printEPKc>
  40014b:	83 c4 10             	add    $0x10,%esp
  40014e:	66 c7 85 da fd ff ff 	movw   $0x0,-0x226(%ebp)
  400155:	00 00 
  400157:	8d 95 e0 fd ff ff    	lea    -0x220(%ebp),%edx
  40015d:	8b 45 f4             	mov    -0xc(%ebp),%eax
  400160:	01 d0                	add    %edx,%eax
  400162:	0f b6 00             	movzbl (%eax),%eax
  400165:	88 85 da fd ff ff    	mov    %al,-0x226(%ebp)
  40016b:	83 ec 0c             	sub    $0xc,%esp
  40016e:	8d 85 da fd ff ff    	lea    -0x226(%ebp),%eax
  400174:	50                   	push   %eax
  400175:	e8 df 00 00 00       	call   400259 <_ZN5vlsmc3App5printEPKc>
  40017a:	83 c4 10             	add    $0x10,%esp
  40017d:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
  400181:	8b 45 f4             	mov    -0xc(%ebp),%eax
  400184:	3b 45 e0             	cmp    -0x20(%ebp),%eax
  400187:	7d 09                	jge    400192 <main+0x148>
  400189:	81 7d f4 c7 00 00 00 	cmpl   $0xc7,-0xc(%ebp)
  400190:	7e a5                	jle    400137 <main+0xed>
  400192:	83 ec 0c             	sub    $0xc,%esp
  400195:	68 f0 10 40 00       	push   $0x4010f0
  40019a:	e8 ba 00 00 00       	call   400259 <_ZN5vlsmc3App5printEPKc>
  40019f:	83 c4 10             	add    $0x10,%esp
  4001a2:	90                   	nop
  4001a3:	83 ec 0c             	sub    $0xc,%esp
  4001a6:	68 07 11 40 00       	push   $0x401107
  4001ab:	e8 a9 00 00 00       	call   400259 <_ZN5vlsmc3App5printEPKc>
  4001b0:	83 c4 10             	add    $0x10,%esp
  4001b3:	b8 00 00 00 00       	mov    $0x0,%eax
  4001b8:	8b 4d fc             	mov    -0x4(%ebp),%ecx
  4001bb:	c9                   	leave
  4001bc:	8d 61 fc             	lea    -0x4(%ecx),%esp
  4001bf:	c3                   	ret

004001c0 <_ZL8syscall1jj>:
  4001c0:	55                   	push   %ebp
  4001c1:	89 e5                	mov    %esp,%ebp
  4001c3:	53                   	push   %ebx
  4001c4:	83 ec 10             	sub    $0x10,%esp
  4001c7:	8b 45 08             	mov    0x8(%ebp),%eax
  4001ca:	8b 55 0c             	mov    0xc(%ebp),%edx
  4001cd:	89 d3                	mov    %edx,%ebx
  4001cf:	cd 80                	int    $0x80
  4001d1:	89 45 f8             	mov    %eax,-0x8(%ebp)
  4001d4:	8b 45 f8             	mov    -0x8(%ebp),%eax
  4001d7:	8b 5d fc             	mov    -0x4(%ebp),%ebx
  4001da:	c9                   	leave
  4001db:	c3                   	ret

004001dc <_ZL8syscall2jjj>:
  4001dc:	55                   	push   %ebp
  4001dd:	89 e5                	mov    %esp,%ebp
  4001df:	53                   	push   %ebx
  4001e0:	83 ec 10             	sub    $0x10,%esp
  4001e3:	8b 45 08             	mov    0x8(%ebp),%eax
  4001e6:	8b 55 0c             	mov    0xc(%ebp),%edx
  4001e9:	8b 4d 10             	mov    0x10(%ebp),%ecx
  4001ec:	89 d3                	mov    %edx,%ebx
  4001ee:	cd 80                	int    $0x80
  4001f0:	89 45 f8             	mov    %eax,-0x8(%ebp)
  4001f3:	8b 45 f8             	mov    -0x8(%ebp),%eax
  4001f6:	8b 5d fc             	mov    -0x4(%ebp),%ebx
  4001f9:	c9                   	leave
  4001fa:	c3                   	ret

004001fb <_ZL9sys_printPKcj>:
  4001fb:	55                   	push   %ebp
  4001fc:	89 e5                	mov    %esp,%ebp
  4001fe:	8b 45 08             	mov    0x8(%ebp),%eax
  400201:	ff 75 0c             	push   0xc(%ebp)
  400204:	50                   	push   %eax
  400205:	6a 01                	push   $0x1
  400207:	e8 d0 ff ff ff       	call   4001dc <_ZL8syscall2jjj>
  40020c:	83 c4 0c             	add    $0xc,%esp
  40020f:	90                   	nop
  400210:	c9                   	leave
  400211:	c3                   	ret

00400212 <_ZL12sys_send_msgiPKvj>:
  400212:	55                   	push   %ebp
  400213:	89 e5                	mov    %esp,%ebp
  400215:	8b 55 0c             	mov    0xc(%ebp),%edx
  400218:	8b 45 08             	mov    0x8(%ebp),%eax
  40021b:	ff 75 10             	push   0x10(%ebp)
  40021e:	52                   	push   %edx
  40021f:	50                   	push   %eax
  400220:	6a 17                	push   $0x17
  400222:	e8 e7 fd ff ff       	call   40000e <_ZL8syscall3jjjj>
  400227:	83 c4 10             	add    $0x10,%esp
  40022a:	c9                   	leave
  40022b:	c3                   	ret

0040022c <_ZL12sys_recv_msgPiPvj>:
  40022c:	55                   	push   %ebp
  40022d:	89 e5                	mov    %esp,%ebp
  40022f:	8b 55 0c             	mov    0xc(%ebp),%edx
  400232:	8b 45 08             	mov    0x8(%ebp),%eax
  400235:	ff 75 10             	push   0x10(%ebp)
  400238:	52                   	push   %edx
  400239:	50                   	push   %eax
  40023a:	6a 18                	push   $0x18
  40023c:	e8 cd fd ff ff       	call   40000e <_ZL8syscall3jjjj>
  400241:	83 c4 10             	add    $0x10,%esp
  400244:	c9                   	leave
  400245:	c3                   	ret

00400246 <_ZL15sys_find_threadPKc>:
  400246:	55                   	push   %ebp
  400247:	89 e5                	mov    %esp,%ebp
  400249:	8b 45 08             	mov    0x8(%ebp),%eax
  40024c:	50                   	push   %eax
  40024d:	6a 1b                	push   $0x1b
  40024f:	e8 6c ff ff ff       	call   4001c0 <_ZL8syscall1jj>
  400254:	83 c4 08             	add    $0x8,%esp
  400257:	c9                   	leave
  400258:	c3                   	ret

00400259 <_ZN5vlsmc3App5printEPKc>:
  400259:	55                   	push   %ebp
  40025a:	89 e5                	mov    %esp,%ebp
  40025c:	83 ec 10             	sub    $0x10,%esp
  40025f:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
  400266:	eb 04                	jmp    40026c <_ZN5vlsmc3App5printEPKc+0x13>
  400268:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
  40026c:	8b 55 08             	mov    0x8(%ebp),%edx
  40026f:	8b 45 fc             	mov    -0x4(%ebp),%eax
  400272:	01 d0                	add    %edx,%eax
  400274:	0f b6 00             	movzbl (%eax),%eax
  400277:	84 c0                	test   %al,%al
  400279:	75 ed                	jne    400268 <_ZN5vlsmc3App5printEPKc+0xf>
  40027b:	ff 75 fc             	push   -0x4(%ebp)
  40027e:	ff 75 08             	push   0x8(%ebp)
  400281:	e8 75 ff ff ff       	call   4001fb <_ZL9sys_printPKcj>
  400286:	83 c4 08             	add    $0x8,%esp
  400289:	90                   	nop
  40028a:	c9                   	leave
  40028b:	c3                   	ret

0040028c <_ZN5vlsmc3App8msg_sendEiPKvj>:
  40028c:	55                   	push   %ebp
  40028d:	89 e5                	mov    %esp,%ebp
  40028f:	ff 75 10             	push   0x10(%ebp)
  400292:	ff 75 0c             	push   0xc(%ebp)
  400295:	ff 75 08             	push   0x8(%ebp)
  400298:	e8 75 ff ff ff       	call   400212 <_ZL12sys_send_msgiPKvj>
  40029d:	83 c4 0c             	add    $0xc,%esp
  4002a0:	c9                   	leave
  4002a1:	c3                   	ret

004002a2 <_ZN5vlsmc3App8msg_recvEPiPvj>:
  4002a2:	55                   	push   %ebp
  4002a3:	89 e5                	mov    %esp,%ebp
  4002a5:	ff 75 10             	push   0x10(%ebp)
  4002a8:	ff 75 0c             	push   0xc(%ebp)
  4002ab:	ff 75 08             	push   0x8(%ebp)
  4002ae:	e8 79 ff ff ff       	call   40022c <_ZL12sys_recv_msgPiPvj>
  4002b3:	83 c4 0c             	add    $0xc,%esp
  4002b6:	c9                   	leave
  4002b7:	c3                   	ret

004002b8 <_ZN5vlsmc3App11find_threadEPKc>:
  4002b8:	55                   	push   %ebp
  4002b9:	89 e5                	mov    %esp,%ebp
  4002bb:	ff 75 08             	push   0x8(%ebp)
  4002be:	e8 83 ff ff ff       	call   400246 <_ZL15sys_find_threadPKc>
  4002c3:	83 c4 04             	add    $0x4,%esp
  4002c6:	c9                   	leave
  4002c7:	c3                   	ret

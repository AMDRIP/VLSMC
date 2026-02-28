
LIBC.SO:     file format elf32-i386


Disassembly of section .text:

00001000 <syscall>:
    1000:	55                   	push   %ebp
    1001:	89 e5                	mov    %esp,%ebp
    1003:	57                   	push   %edi
    1004:	56                   	push   %esi
    1005:	53                   	push   %ebx
    1006:	83 ec 3c             	sub    $0x3c,%esp
    1009:	e8 9d 00 00 00       	call   10ab <__x86.get_pc_thunk.ax>
    100e:	05 2e 3c 00 00       	add    $0x3c2e,%eax
    1013:	89 45 c4             	mov    %eax,-0x3c(%ebp)
    1016:	8d 45 0c             	lea    0xc(%ebp),%eax
    1019:	89 45 cc             	mov    %eax,-0x34(%ebp)
    101c:	8b 45 cc             	mov    -0x34(%ebp),%eax
    101f:	8d 50 04             	lea    0x4(%eax),%edx
    1022:	89 55 cc             	mov    %edx,-0x34(%ebp)
    1025:	8b 00                	mov    (%eax),%eax
    1027:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    102a:	8b 45 cc             	mov    -0x34(%ebp),%eax
    102d:	8d 50 04             	lea    0x4(%eax),%edx
    1030:	89 55 cc             	mov    %edx,-0x34(%ebp)
    1033:	8b 00                	mov    (%eax),%eax
    1035:	89 45 e0             	mov    %eax,-0x20(%ebp)
    1038:	8b 45 cc             	mov    -0x34(%ebp),%eax
    103b:	8d 50 04             	lea    0x4(%eax),%edx
    103e:	89 55 cc             	mov    %edx,-0x34(%ebp)
    1041:	8b 00                	mov    (%eax),%eax
    1043:	89 45 dc             	mov    %eax,-0x24(%ebp)
    1046:	8b 45 cc             	mov    -0x34(%ebp),%eax
    1049:	8d 50 04             	lea    0x4(%eax),%edx
    104c:	89 55 cc             	mov    %edx,-0x34(%ebp)
    104f:	8b 00                	mov    (%eax),%eax
    1051:	89 45 d8             	mov    %eax,-0x28(%ebp)
    1054:	8b 45 cc             	mov    -0x34(%ebp),%eax
    1057:	8d 50 04             	lea    0x4(%eax),%edx
    105a:	89 55 cc             	mov    %edx,-0x34(%ebp)
    105d:	8b 00                	mov    (%eax),%eax
    105f:	89 45 d4             	mov    %eax,-0x2c(%ebp)
    1062:	8b 45 08             	mov    0x8(%ebp),%eax
    1065:	8b 5d e4             	mov    -0x1c(%ebp),%ebx
    1068:	8b 4d e0             	mov    -0x20(%ebp),%ecx
    106b:	8b 55 dc             	mov    -0x24(%ebp),%edx
    106e:	8b 75 d8             	mov    -0x28(%ebp),%esi
    1071:	8b 7d d4             	mov    -0x2c(%ebp),%edi
    1074:	cd 80                	int    $0x80
    1076:	89 45 d0             	mov    %eax,-0x30(%ebp)
    1079:	83 7d d0 00          	cmpl   $0x0,-0x30(%ebp)
    107d:	79 21                	jns    10a0 <syscall+0xa0>
    107f:	81 7d d0 01 f0 ff ff 	cmpl   $0xfffff001,-0x30(%ebp)
    1086:	7c 18                	jl     10a0 <syscall+0xa0>
    1088:	8b 45 d0             	mov    -0x30(%ebp),%eax
    108b:	89 c6                	mov    %eax,%esi
    108d:	f7 de                	neg    %esi
    108f:	8b 5d c4             	mov    -0x3c(%ebp),%ebx
    1092:	e8 79 2d 00 00       	call   3e10 <__errno_location@plt>
    1097:	89 30                	mov    %esi,(%eax)
    1099:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    109e:	eb 03                	jmp    10a3 <syscall+0xa3>
    10a0:	8b 45 d0             	mov    -0x30(%ebp),%eax
    10a3:	83 c4 3c             	add    $0x3c,%esp
    10a6:	5b                   	pop    %ebx
    10a7:	5e                   	pop    %esi
    10a8:	5f                   	pop    %edi
    10a9:	5d                   	pop    %ebp
    10aa:	c3                   	ret

000010ab <__x86.get_pc_thunk.ax>:
    10ab:	8b 04 24             	mov    (%esp),%eax
    10ae:	c3                   	ret

000010af <__errno_location>:
    10af:	55                   	push   %ebp
    10b0:	89 e5                	mov    %esp,%ebp
    10b2:	e8 f4 ff ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    10b7:	05 85 3b 00 00       	add    $0x3b85,%eax
    10bc:	8d 80 c4 13 00 00    	lea    0x13c4(%eax),%eax
    10c2:	5d                   	pop    %ebp
    10c3:	c3                   	ret

000010c4 <memcpy>:
    10c4:	55                   	push   %ebp
    10c5:	89 e5                	mov    %esp,%ebp
    10c7:	83 ec 20             	sub    $0x20,%esp
    10ca:	e8 dc ff ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    10cf:	05 6d 3b 00 00       	add    $0x3b6d,%eax
    10d4:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    10d8:	74 0c                	je     10e6 <memcpy+0x22>
    10da:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    10de:	74 06                	je     10e6 <memcpy+0x22>
    10e0:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    10e4:	75 08                	jne    10ee <memcpy+0x2a>
    10e6:	8b 45 08             	mov    0x8(%ebp),%eax
    10e9:	e9 9d 00 00 00       	jmp    118b <memcpy+0xc7>
    10ee:	8b 45 08             	mov    0x8(%ebp),%eax
    10f1:	89 45 fc             	mov    %eax,-0x4(%ebp)
    10f4:	8b 45 0c             	mov    0xc(%ebp),%eax
    10f7:	89 45 f8             	mov    %eax,-0x8(%ebp)
    10fa:	8b 45 fc             	mov    -0x4(%ebp),%eax
    10fd:	83 e0 03             	and    $0x3,%eax
    1100:	85 c0                	test   %eax,%eax
    1102:	75 72                	jne    1176 <memcpy+0xb2>
    1104:	8b 45 f8             	mov    -0x8(%ebp),%eax
    1107:	83 e0 03             	and    $0x3,%eax
    110a:	85 c0                	test   %eax,%eax
    110c:	75 68                	jne    1176 <memcpy+0xb2>
    110e:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1111:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1114:	8b 45 f8             	mov    -0x8(%ebp),%eax
    1117:	89 45 f0             	mov    %eax,-0x10(%ebp)
    111a:	8b 45 10             	mov    0x10(%ebp),%eax
    111d:	c1 e8 02             	shr    $0x2,%eax
    1120:	89 45 ec             	mov    %eax,-0x14(%ebp)
    1123:	eb 16                	jmp    113b <memcpy+0x77>
    1125:	8b 45 f0             	mov    -0x10(%ebp),%eax
    1128:	8d 50 04             	lea    0x4(%eax),%edx
    112b:	89 55 f0             	mov    %edx,-0x10(%ebp)
    112e:	8b 10                	mov    (%eax),%edx
    1130:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1133:	8d 48 04             	lea    0x4(%eax),%ecx
    1136:	89 4d f4             	mov    %ecx,-0xc(%ebp)
    1139:	89 10                	mov    %edx,(%eax)
    113b:	8b 45 ec             	mov    -0x14(%ebp),%eax
    113e:	8d 50 ff             	lea    -0x1(%eax),%edx
    1141:	89 55 ec             	mov    %edx,-0x14(%ebp)
    1144:	85 c0                	test   %eax,%eax
    1146:	0f 95 c0             	setne  %al
    1149:	84 c0                	test   %al,%al
    114b:	75 d8                	jne    1125 <memcpy+0x61>
    114d:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1150:	89 45 fc             	mov    %eax,-0x4(%ebp)
    1153:	8b 45 f0             	mov    -0x10(%ebp),%eax
    1156:	89 45 f8             	mov    %eax,-0x8(%ebp)
    1159:	83 65 10 03          	andl   $0x3,0x10(%ebp)
    115d:	eb 17                	jmp    1176 <memcpy+0xb2>
    115f:	8b 45 f8             	mov    -0x8(%ebp),%eax
    1162:	8d 50 01             	lea    0x1(%eax),%edx
    1165:	89 55 f8             	mov    %edx,-0x8(%ebp)
    1168:	0f b6 10             	movzbl (%eax),%edx
    116b:	8b 45 fc             	mov    -0x4(%ebp),%eax
    116e:	8d 48 01             	lea    0x1(%eax),%ecx
    1171:	89 4d fc             	mov    %ecx,-0x4(%ebp)
    1174:	88 10                	mov    %dl,(%eax)
    1176:	8b 45 10             	mov    0x10(%ebp),%eax
    1179:	8d 50 ff             	lea    -0x1(%eax),%edx
    117c:	89 55 10             	mov    %edx,0x10(%ebp)
    117f:	85 c0                	test   %eax,%eax
    1181:	0f 95 c0             	setne  %al
    1184:	84 c0                	test   %al,%al
    1186:	75 d7                	jne    115f <memcpy+0x9b>
    1188:	8b 45 08             	mov    0x8(%ebp),%eax
    118b:	c9                   	leave
    118c:	c3                   	ret

0000118d <memmove>:
    118d:	55                   	push   %ebp
    118e:	89 e5                	mov    %esp,%ebp
    1190:	53                   	push   %ebx
    1191:	83 ec 14             	sub    $0x14,%esp
    1194:	e8 12 ff ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1199:	05 a3 3a 00 00       	add    $0x3aa3,%eax
    119e:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    11a2:	74 14                	je     11b8 <memmove+0x2b>
    11a4:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    11a8:	74 0e                	je     11b8 <memmove+0x2b>
    11aa:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    11ae:	74 08                	je     11b8 <memmove+0x2b>
    11b0:	8b 55 08             	mov    0x8(%ebp),%edx
    11b3:	3b 55 0c             	cmp    0xc(%ebp),%edx
    11b6:	75 05                	jne    11bd <memmove+0x30>
    11b8:	8b 45 08             	mov    0x8(%ebp),%eax
    11bb:	eb 62                	jmp    121f <memmove+0x92>
    11bd:	8b 55 08             	mov    0x8(%ebp),%edx
    11c0:	89 55 f4             	mov    %edx,-0xc(%ebp)
    11c3:	8b 55 0c             	mov    0xc(%ebp),%edx
    11c6:	89 55 f0             	mov    %edx,-0x10(%ebp)
    11c9:	8b 55 f4             	mov    -0xc(%ebp),%edx
    11cc:	3b 55 f0             	cmp    -0x10(%ebp),%edx
    11cf:	73 18                	jae    11e9 <memmove+0x5c>
    11d1:	83 ec 04             	sub    $0x4,%esp
    11d4:	ff 75 10             	push   0x10(%ebp)
    11d7:	ff 75 0c             	push   0xc(%ebp)
    11da:	ff 75 08             	push   0x8(%ebp)
    11dd:	89 c3                	mov    %eax,%ebx
    11df:	e8 9c 2b 00 00       	call   3d80 <memcpy@plt>
    11e4:	83 c4 10             	add    $0x10,%esp
    11e7:	eb 36                	jmp    121f <memmove+0x92>
    11e9:	8b 45 10             	mov    0x10(%ebp),%eax
    11ec:	01 45 f4             	add    %eax,-0xc(%ebp)
    11ef:	8b 45 10             	mov    0x10(%ebp),%eax
    11f2:	01 45 f0             	add    %eax,-0x10(%ebp)
    11f5:	eb 13                	jmp    120a <memmove+0x7d>
    11f7:	83 6d f0 01          	subl   $0x1,-0x10(%ebp)
    11fb:	8b 45 f0             	mov    -0x10(%ebp),%eax
    11fe:	0f b6 10             	movzbl (%eax),%edx
    1201:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
    1205:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1208:	88 10                	mov    %dl,(%eax)
    120a:	8b 45 10             	mov    0x10(%ebp),%eax
    120d:	8d 50 ff             	lea    -0x1(%eax),%edx
    1210:	89 55 10             	mov    %edx,0x10(%ebp)
    1213:	85 c0                	test   %eax,%eax
    1215:	0f 95 c0             	setne  %al
    1218:	84 c0                	test   %al,%al
    121a:	75 db                	jne    11f7 <memmove+0x6a>
    121c:	8b 45 08             	mov    0x8(%ebp),%eax
    121f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1222:	c9                   	leave
    1223:	c3                   	ret

00001224 <memset>:
    1224:	55                   	push   %ebp
    1225:	89 e5                	mov    %esp,%ebp
    1227:	83 ec 20             	sub    $0x20,%esp
    122a:	e8 7c fe ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    122f:	05 0d 3a 00 00       	add    $0x3a0d,%eax
    1234:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1238:	74 06                	je     1240 <memset+0x1c>
    123a:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    123e:	75 08                	jne    1248 <memset+0x24>
    1240:	8b 45 08             	mov    0x8(%ebp),%eax
    1243:	e9 ba 00 00 00       	jmp    1302 <memset+0xde>
    1248:	8b 45 08             	mov    0x8(%ebp),%eax
    124b:	89 45 fc             	mov    %eax,-0x4(%ebp)
    124e:	8b 45 0c             	mov    0xc(%ebp),%eax
    1251:	88 45 f3             	mov    %al,-0xd(%ebp)
    1254:	eb 13                	jmp    1269 <memset+0x45>
    1256:	0f b6 55 f3          	movzbl -0xd(%ebp),%edx
    125a:	8b 45 fc             	mov    -0x4(%ebp),%eax
    125d:	8d 48 01             	lea    0x1(%eax),%ecx
    1260:	89 4d fc             	mov    %ecx,-0x4(%ebp)
    1263:	88 10                	mov    %dl,(%eax)
    1265:	83 6d 10 01          	subl   $0x1,0x10(%ebp)
    1269:	8b 45 fc             	mov    -0x4(%ebp),%eax
    126c:	83 e0 03             	and    $0x3,%eax
    126f:	85 c0                	test   %eax,%eax
    1271:	74 06                	je     1279 <memset+0x55>
    1273:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    1277:	75 dd                	jne    1256 <memset+0x32>
    1279:	83 7d 10 03          	cmpl   $0x3,0x10(%ebp)
    127d:	76 6e                	jbe    12ed <memset+0xc9>
    127f:	0f b6 55 f3          	movzbl -0xd(%ebp),%edx
    1283:	89 d0                	mov    %edx,%eax
    1285:	c1 e0 08             	shl    $0x8,%eax
    1288:	01 d0                	add    %edx,%eax
    128a:	89 c2                	mov    %eax,%edx
    128c:	0f b6 45 f3          	movzbl -0xd(%ebp),%eax
    1290:	c1 e0 10             	shl    $0x10,%eax
    1293:	09 c2                	or     %eax,%edx
    1295:	0f b6 45 f3          	movzbl -0xd(%ebp),%eax
    1299:	c1 e0 18             	shl    $0x18,%eax
    129c:	09 d0                	or     %edx,%eax
    129e:	89 45 ec             	mov    %eax,-0x14(%ebp)
    12a1:	8b 45 fc             	mov    -0x4(%ebp),%eax
    12a4:	89 45 f8             	mov    %eax,-0x8(%ebp)
    12a7:	8b 45 10             	mov    0x10(%ebp),%eax
    12aa:	c1 e8 02             	shr    $0x2,%eax
    12ad:	89 45 f4             	mov    %eax,-0xc(%ebp)
    12b0:	eb 0e                	jmp    12c0 <memset+0x9c>
    12b2:	8b 55 ec             	mov    -0x14(%ebp),%edx
    12b5:	8b 45 f8             	mov    -0x8(%ebp),%eax
    12b8:	8d 48 04             	lea    0x4(%eax),%ecx
    12bb:	89 4d f8             	mov    %ecx,-0x8(%ebp)
    12be:	89 10                	mov    %edx,(%eax)
    12c0:	8b 45 f4             	mov    -0xc(%ebp),%eax
    12c3:	8d 50 ff             	lea    -0x1(%eax),%edx
    12c6:	89 55 f4             	mov    %edx,-0xc(%ebp)
    12c9:	85 c0                	test   %eax,%eax
    12cb:	0f 95 c0             	setne  %al
    12ce:	84 c0                	test   %al,%al
    12d0:	75 e0                	jne    12b2 <memset+0x8e>
    12d2:	8b 45 f8             	mov    -0x8(%ebp),%eax
    12d5:	89 45 fc             	mov    %eax,-0x4(%ebp)
    12d8:	83 65 10 03          	andl   $0x3,0x10(%ebp)
    12dc:	eb 0f                	jmp    12ed <memset+0xc9>
    12de:	0f b6 55 f3          	movzbl -0xd(%ebp),%edx
    12e2:	8b 45 fc             	mov    -0x4(%ebp),%eax
    12e5:	8d 48 01             	lea    0x1(%eax),%ecx
    12e8:	89 4d fc             	mov    %ecx,-0x4(%ebp)
    12eb:	88 10                	mov    %dl,(%eax)
    12ed:	8b 45 10             	mov    0x10(%ebp),%eax
    12f0:	8d 50 ff             	lea    -0x1(%eax),%edx
    12f3:	89 55 10             	mov    %edx,0x10(%ebp)
    12f6:	85 c0                	test   %eax,%eax
    12f8:	0f 95 c0             	setne  %al
    12fb:	84 c0                	test   %al,%al
    12fd:	75 df                	jne    12de <memset+0xba>
    12ff:	8b 45 08             	mov    0x8(%ebp),%eax
    1302:	c9                   	leave
    1303:	c3                   	ret

00001304 <memcmp>:
    1304:	55                   	push   %ebp
    1305:	89 e5                	mov    %esp,%ebp
    1307:	83 ec 20             	sub    $0x20,%esp
    130a:	e8 9c fd ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    130f:	05 2d 39 00 00       	add    $0x392d,%eax
    1314:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    1318:	75 0a                	jne    1324 <memcmp+0x20>
    131a:	b8 00 00 00 00       	mov    $0x0,%eax
    131f:	e9 c8 00 00 00       	jmp    13ec <memcmp+0xe8>
    1324:	8b 45 08             	mov    0x8(%ebp),%eax
    1327:	89 45 fc             	mov    %eax,-0x4(%ebp)
    132a:	8b 45 0c             	mov    0xc(%ebp),%eax
    132d:	89 45 f8             	mov    %eax,-0x8(%ebp)
    1330:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1333:	83 e0 03             	and    $0x3,%eax
    1336:	85 c0                	test   %eax,%eax
    1338:	0f 85 97 00 00 00    	jne    13d5 <memcmp+0xd1>
    133e:	8b 45 f8             	mov    -0x8(%ebp),%eax
    1341:	83 e0 03             	and    $0x3,%eax
    1344:	85 c0                	test   %eax,%eax
    1346:	0f 85 89 00 00 00    	jne    13d5 <memcmp+0xd1>
    134c:	8b 45 fc             	mov    -0x4(%ebp),%eax
    134f:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1352:	8b 45 f8             	mov    -0x8(%ebp),%eax
    1355:	89 45 f0             	mov    %eax,-0x10(%ebp)
    1358:	8b 45 10             	mov    0x10(%ebp),%eax
    135b:	c1 e8 02             	shr    $0x2,%eax
    135e:	89 45 ec             	mov    %eax,-0x14(%ebp)
    1361:	eb 16                	jmp    1379 <memcmp+0x75>
    1363:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1366:	8b 10                	mov    (%eax),%edx
    1368:	8b 45 f0             	mov    -0x10(%ebp),%eax
    136b:	8b 00                	mov    (%eax),%eax
    136d:	39 c2                	cmp    %eax,%edx
    136f:	75 1c                	jne    138d <memcmp+0x89>
    1371:	83 45 f4 04          	addl   $0x4,-0xc(%ebp)
    1375:	83 45 f0 04          	addl   $0x4,-0x10(%ebp)
    1379:	8b 45 ec             	mov    -0x14(%ebp),%eax
    137c:	8d 50 ff             	lea    -0x1(%eax),%edx
    137f:	89 55 ec             	mov    %edx,-0x14(%ebp)
    1382:	85 c0                	test   %eax,%eax
    1384:	0f 95 c0             	setne  %al
    1387:	84 c0                	test   %al,%al
    1389:	75 d8                	jne    1363 <memcmp+0x5f>
    138b:	eb 01                	jmp    138e <memcmp+0x8a>
    138d:	90                   	nop
    138e:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1391:	89 45 fc             	mov    %eax,-0x4(%ebp)
    1394:	8b 45 f0             	mov    -0x10(%ebp),%eax
    1397:	89 45 f8             	mov    %eax,-0x8(%ebp)
    139a:	8b 45 f4             	mov    -0xc(%ebp),%eax
    139d:	2b 45 08             	sub    0x8(%ebp),%eax
    13a0:	29 45 10             	sub    %eax,0x10(%ebp)
    13a3:	eb 30                	jmp    13d5 <memcmp+0xd1>
    13a5:	8b 45 fc             	mov    -0x4(%ebp),%eax
    13a8:	0f b6 10             	movzbl (%eax),%edx
    13ab:	8b 45 f8             	mov    -0x8(%ebp),%eax
    13ae:	0f b6 00             	movzbl (%eax),%eax
    13b1:	38 c2                	cmp    %al,%dl
    13b3:	74 18                	je     13cd <memcmp+0xc9>
    13b5:	8b 45 fc             	mov    -0x4(%ebp),%eax
    13b8:	0f b6 00             	movzbl (%eax),%eax
    13bb:	0f b6 c8             	movzbl %al,%ecx
    13be:	8b 45 f8             	mov    -0x8(%ebp),%eax
    13c1:	0f b6 00             	movzbl (%eax),%eax
    13c4:	0f b6 d0             	movzbl %al,%edx
    13c7:	89 c8                	mov    %ecx,%eax
    13c9:	29 d0                	sub    %edx,%eax
    13cb:	eb 1f                	jmp    13ec <memcmp+0xe8>
    13cd:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
    13d1:	83 45 f8 01          	addl   $0x1,-0x8(%ebp)
    13d5:	8b 45 10             	mov    0x10(%ebp),%eax
    13d8:	8d 50 ff             	lea    -0x1(%eax),%edx
    13db:	89 55 10             	mov    %edx,0x10(%ebp)
    13de:	85 c0                	test   %eax,%eax
    13e0:	0f 95 c0             	setne  %al
    13e3:	84 c0                	test   %al,%al
    13e5:	75 be                	jne    13a5 <memcmp+0xa1>
    13e7:	b8 00 00 00 00       	mov    $0x0,%eax
    13ec:	c9                   	leave
    13ed:	c3                   	ret

000013ee <memcmp_s>:
    13ee:	55                   	push   %ebp
    13ef:	89 e5                	mov    %esp,%ebp
    13f1:	53                   	push   %ebx
    13f2:	83 ec 14             	sub    $0x14,%esp
    13f5:	e8 fd 06 00 00       	call   1af7 <__x86.get_pc_thunk.dx>
    13fa:	81 c2 42 38 00 00    	add    $0x3842,%edx
    1400:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1404:	74 06                	je     140c <memcmp_s+0x1e>
    1406:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    140a:	75 07                	jne    1413 <memcmp_s+0x25>
    140c:	b8 00 00 00 00       	mov    $0x0,%eax
    1411:	eb 2a                	jmp    143d <memcmp_s+0x4f>
    1413:	8b 45 0c             	mov    0xc(%ebp),%eax
    1416:	3b 45 14             	cmp    0x14(%ebp),%eax
    1419:	73 05                	jae    1420 <memcmp_s+0x32>
    141b:	8b 45 0c             	mov    0xc(%ebp),%eax
    141e:	eb 03                	jmp    1423 <memcmp_s+0x35>
    1420:	8b 45 14             	mov    0x14(%ebp),%eax
    1423:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1426:	83 ec 04             	sub    $0x4,%esp
    1429:	ff 75 f4             	push   -0xc(%ebp)
    142c:	ff 75 10             	push   0x10(%ebp)
    142f:	ff 75 08             	push   0x8(%ebp)
    1432:	89 d3                	mov    %edx,%ebx
    1434:	e8 97 29 00 00       	call   3dd0 <memcmp@plt>
    1439:	83 c4 10             	add    $0x10,%esp
    143c:	90                   	nop
    143d:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1440:	c9                   	leave
    1441:	c3                   	ret

00001442 <strlen>:
    1442:	55                   	push   %ebp
    1443:	89 e5                	mov    %esp,%ebp
    1445:	83 ec 10             	sub    $0x10,%esp
    1448:	e8 5e fc ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    144d:	05 ef 37 00 00       	add    $0x37ef,%eax
    1452:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1456:	75 0a                	jne    1462 <strlen+0x20>
    1458:	b8 00 00 00 00       	mov    $0x0,%eax
    145d:	e9 bb 00 00 00       	jmp    151d <strlen+0xdb>
    1462:	8b 45 08             	mov    0x8(%ebp),%eax
    1465:	89 45 fc             	mov    %eax,-0x4(%ebp)
    1468:	eb 19                	jmp    1483 <strlen+0x41>
    146a:	8b 45 fc             	mov    -0x4(%ebp),%eax
    146d:	0f b6 00             	movzbl (%eax),%eax
    1470:	84 c0                	test   %al,%al
    1472:	75 0b                	jne    147f <strlen+0x3d>
    1474:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1477:	2b 45 08             	sub    0x8(%ebp),%eax
    147a:	e9 9e 00 00 00       	jmp    151d <strlen+0xdb>
    147f:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
    1483:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1486:	83 e0 03             	and    $0x3,%eax
    1489:	85 c0                	test   %eax,%eax
    148b:	75 dd                	jne    146a <strlen+0x28>
    148d:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1490:	89 45 f8             	mov    %eax,-0x8(%ebp)
    1493:	8b 45 f8             	mov    -0x8(%ebp),%eax
    1496:	8b 00                	mov    (%eax),%eax
    1498:	89 45 f4             	mov    %eax,-0xc(%ebp)
    149b:	8b 45 f4             	mov    -0xc(%ebp),%eax
    149e:	8d 90 ff fe fe fe    	lea    -0x1010101(%eax),%edx
    14a4:	8b 45 f4             	mov    -0xc(%ebp),%eax
    14a7:	f7 d0                	not    %eax
    14a9:	21 d0                	and    %edx,%eax
    14ab:	25 80 80 80 80       	and    $0x80808080,%eax
    14b0:	85 c0                	test   %eax,%eax
    14b2:	74 60                	je     1514 <strlen+0xd2>
    14b4:	8b 45 f8             	mov    -0x8(%ebp),%eax
    14b7:	89 45 f0             	mov    %eax,-0x10(%ebp)
    14ba:	8b 45 f0             	mov    -0x10(%ebp),%eax
    14bd:	0f b6 00             	movzbl (%eax),%eax
    14c0:	84 c0                	test   %al,%al
    14c2:	75 08                	jne    14cc <strlen+0x8a>
    14c4:	8b 45 f0             	mov    -0x10(%ebp),%eax
    14c7:	2b 45 08             	sub    0x8(%ebp),%eax
    14ca:	eb 51                	jmp    151d <strlen+0xdb>
    14cc:	8b 45 f0             	mov    -0x10(%ebp),%eax
    14cf:	83 c0 01             	add    $0x1,%eax
    14d2:	0f b6 00             	movzbl (%eax),%eax
    14d5:	84 c0                	test   %al,%al
    14d7:	75 0b                	jne    14e4 <strlen+0xa2>
    14d9:	8b 45 f0             	mov    -0x10(%ebp),%eax
    14dc:	83 c0 01             	add    $0x1,%eax
    14df:	2b 45 08             	sub    0x8(%ebp),%eax
    14e2:	eb 39                	jmp    151d <strlen+0xdb>
    14e4:	8b 45 f0             	mov    -0x10(%ebp),%eax
    14e7:	83 c0 02             	add    $0x2,%eax
    14ea:	0f b6 00             	movzbl (%eax),%eax
    14ed:	84 c0                	test   %al,%al
    14ef:	75 0b                	jne    14fc <strlen+0xba>
    14f1:	8b 45 f0             	mov    -0x10(%ebp),%eax
    14f4:	83 c0 02             	add    $0x2,%eax
    14f7:	2b 45 08             	sub    0x8(%ebp),%eax
    14fa:	eb 21                	jmp    151d <strlen+0xdb>
    14fc:	8b 45 f0             	mov    -0x10(%ebp),%eax
    14ff:	83 c0 03             	add    $0x3,%eax
    1502:	0f b6 00             	movzbl (%eax),%eax
    1505:	84 c0                	test   %al,%al
    1507:	75 0b                	jne    1514 <strlen+0xd2>
    1509:	8b 45 f0             	mov    -0x10(%ebp),%eax
    150c:	83 c0 03             	add    $0x3,%eax
    150f:	2b 45 08             	sub    0x8(%ebp),%eax
    1512:	eb 09                	jmp    151d <strlen+0xdb>
    1514:	83 45 f8 04          	addl   $0x4,-0x8(%ebp)
    1518:	e9 76 ff ff ff       	jmp    1493 <strlen+0x51>
    151d:	c9                   	leave
    151e:	c3                   	ret

0000151f <strnlen>:
    151f:	55                   	push   %ebp
    1520:	89 e5                	mov    %esp,%ebp
    1522:	83 ec 10             	sub    $0x10,%esp
    1525:	e8 81 fb ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    152a:	05 12 37 00 00       	add    $0x3712,%eax
    152f:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1533:	75 07                	jne    153c <strnlen+0x1d>
    1535:	b8 00 00 00 00       	mov    $0x0,%eax
    153a:	eb 27                	jmp    1563 <strnlen+0x44>
    153c:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
    1543:	eb 04                	jmp    1549 <strnlen+0x2a>
    1545:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
    1549:	8b 45 fc             	mov    -0x4(%ebp),%eax
    154c:	3b 45 0c             	cmp    0xc(%ebp),%eax
    154f:	73 0f                	jae    1560 <strnlen+0x41>
    1551:	8b 55 08             	mov    0x8(%ebp),%edx
    1554:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1557:	01 d0                	add    %edx,%eax
    1559:	0f b6 00             	movzbl (%eax),%eax
    155c:	84 c0                	test   %al,%al
    155e:	75 e5                	jne    1545 <strnlen+0x26>
    1560:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1563:	c9                   	leave
    1564:	c3                   	ret

00001565 <strcpy>:
    1565:	55                   	push   %ebp
    1566:	89 e5                	mov    %esp,%ebp
    1568:	83 ec 20             	sub    $0x20,%esp
    156b:	e8 3b fb ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1570:	05 cc 36 00 00       	add    $0x36cc,%eax
    1575:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1579:	74 06                	je     1581 <strcpy+0x1c>
    157b:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    157f:	75 08                	jne    1589 <strcpy+0x24>
    1581:	8b 45 08             	mov    0x8(%ebp),%eax
    1584:	e9 95 00 00 00       	jmp    161e <strcpy+0xb9>
    1589:	8b 45 08             	mov    0x8(%ebp),%eax
    158c:	89 45 fc             	mov    %eax,-0x4(%ebp)
    158f:	8b 45 0c             	mov    0xc(%ebp),%eax
    1592:	89 45 f8             	mov    %eax,-0x8(%ebp)
    1595:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1598:	83 e0 03             	and    $0x3,%eax
    159b:	85 c0                	test   %eax,%eax
    159d:	75 58                	jne    15f7 <strcpy+0x92>
    159f:	8b 45 f8             	mov    -0x8(%ebp),%eax
    15a2:	83 e0 03             	and    $0x3,%eax
    15a5:	85 c0                	test   %eax,%eax
    15a7:	75 4e                	jne    15f7 <strcpy+0x92>
    15a9:	8b 45 fc             	mov    -0x4(%ebp),%eax
    15ac:	89 45 f4             	mov    %eax,-0xc(%ebp)
    15af:	8b 45 f8             	mov    -0x8(%ebp),%eax
    15b2:	89 45 f0             	mov    %eax,-0x10(%ebp)
    15b5:	8b 45 f0             	mov    -0x10(%ebp),%eax
    15b8:	8b 00                	mov    (%eax),%eax
    15ba:	89 45 ec             	mov    %eax,-0x14(%ebp)
    15bd:	8b 45 ec             	mov    -0x14(%ebp),%eax
    15c0:	8d 90 ff fe fe fe    	lea    -0x1010101(%eax),%edx
    15c6:	8b 45 ec             	mov    -0x14(%ebp),%eax
    15c9:	f7 d0                	not    %eax
    15cb:	21 d0                	and    %edx,%eax
    15cd:	25 80 80 80 80       	and    $0x80808080,%eax
    15d2:	85 c0                	test   %eax,%eax
    15d4:	75 14                	jne    15ea <strcpy+0x85>
    15d6:	8b 55 ec             	mov    -0x14(%ebp),%edx
    15d9:	8b 45 f4             	mov    -0xc(%ebp),%eax
    15dc:	8d 48 04             	lea    0x4(%eax),%ecx
    15df:	89 4d f4             	mov    %ecx,-0xc(%ebp)
    15e2:	89 10                	mov    %edx,(%eax)
    15e4:	83 45 f0 04          	addl   $0x4,-0x10(%ebp)
    15e8:	eb cb                	jmp    15b5 <strcpy+0x50>
    15ea:	90                   	nop
    15eb:	8b 45 f4             	mov    -0xc(%ebp),%eax
    15ee:	89 45 fc             	mov    %eax,-0x4(%ebp)
    15f1:	8b 45 f0             	mov    -0x10(%ebp),%eax
    15f4:	89 45 f8             	mov    %eax,-0x8(%ebp)
    15f7:	90                   	nop
    15f8:	8b 45 f8             	mov    -0x8(%ebp),%eax
    15fb:	8d 50 01             	lea    0x1(%eax),%edx
    15fe:	89 55 f8             	mov    %edx,-0x8(%ebp)
    1601:	0f b6 10             	movzbl (%eax),%edx
    1604:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1607:	8d 48 01             	lea    0x1(%eax),%ecx
    160a:	89 4d fc             	mov    %ecx,-0x4(%ebp)
    160d:	88 10                	mov    %dl,(%eax)
    160f:	0f b6 00             	movzbl (%eax),%eax
    1612:	84 c0                	test   %al,%al
    1614:	0f 95 c0             	setne  %al
    1617:	84 c0                	test   %al,%al
    1619:	75 dd                	jne    15f8 <strcpy+0x93>
    161b:	8b 45 08             	mov    0x8(%ebp),%eax
    161e:	c9                   	leave
    161f:	c3                   	ret

00001620 <strlcpy>:
    1620:	55                   	push   %ebp
    1621:	89 e5                	mov    %esp,%ebp
    1623:	53                   	push   %ebx
    1624:	83 ec 14             	sub    $0x14,%esp
    1627:	e8 cf 04 00 00       	call   1afb <__x86.get_pc_thunk.bx>
    162c:	81 c3 10 36 00 00    	add    $0x3610,%ebx
    1632:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1636:	74 0c                	je     1644 <strlcpy+0x24>
    1638:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    163c:	74 06                	je     1644 <strlcpy+0x24>
    163e:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    1642:	75 07                	jne    164b <strlcpy+0x2b>
    1644:	b8 00 00 00 00       	mov    $0x0,%eax
    1649:	eb 44                	jmp    168f <strlcpy+0x6f>
    164b:	83 ec 0c             	sub    $0xc,%esp
    164e:	ff 75 0c             	push   0xc(%ebp)
    1651:	e8 da 27 00 00       	call   3e30 <strlen@plt>
    1656:	83 c4 10             	add    $0x10,%esp
    1659:	89 45 f4             	mov    %eax,-0xc(%ebp)
    165c:	8b 45 10             	mov    0x10(%ebp),%eax
    165f:	83 e8 01             	sub    $0x1,%eax
    1662:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1665:	39 c2                	cmp    %eax,%edx
    1667:	0f 46 c2             	cmovbe %edx,%eax
    166a:	89 45 f0             	mov    %eax,-0x10(%ebp)
    166d:	83 ec 04             	sub    $0x4,%esp
    1670:	ff 75 f0             	push   -0x10(%ebp)
    1673:	ff 75 0c             	push   0xc(%ebp)
    1676:	ff 75 08             	push   0x8(%ebp)
    1679:	e8 02 27 00 00       	call   3d80 <memcpy@plt>
    167e:	83 c4 10             	add    $0x10,%esp
    1681:	8b 55 08             	mov    0x8(%ebp),%edx
    1684:	8b 45 f0             	mov    -0x10(%ebp),%eax
    1687:	01 d0                	add    %edx,%eax
    1689:	c6 00 00             	movb   $0x0,(%eax)
    168c:	8b 45 f4             	mov    -0xc(%ebp),%eax
    168f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1692:	c9                   	leave
    1693:	c3                   	ret

00001694 <strcat>:
    1694:	55                   	push   %ebp
    1695:	89 e5                	mov    %esp,%ebp
    1697:	53                   	push   %ebx
    1698:	83 ec 14             	sub    $0x14,%esp
    169b:	e8 5b 04 00 00       	call   1afb <__x86.get_pc_thunk.bx>
    16a0:	81 c3 9c 35 00 00    	add    $0x359c,%ebx
    16a6:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    16aa:	74 06                	je     16b2 <strcat+0x1e>
    16ac:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    16b0:	75 05                	jne    16b7 <strcat+0x23>
    16b2:	8b 45 08             	mov    0x8(%ebp),%eax
    16b5:	eb 2a                	jmp    16e1 <strcat+0x4d>
    16b7:	83 ec 0c             	sub    $0xc,%esp
    16ba:	ff 75 08             	push   0x8(%ebp)
    16bd:	e8 6e 27 00 00       	call   3e30 <strlen@plt>
    16c2:	83 c4 10             	add    $0x10,%esp
    16c5:	8b 55 08             	mov    0x8(%ebp),%edx
    16c8:	01 d0                	add    %edx,%eax
    16ca:	89 45 f4             	mov    %eax,-0xc(%ebp)
    16cd:	83 ec 08             	sub    $0x8,%esp
    16d0:	ff 75 0c             	push   0xc(%ebp)
    16d3:	ff 75 f4             	push   -0xc(%ebp)
    16d6:	e8 65 26 00 00       	call   3d40 <strcpy@plt>
    16db:	83 c4 10             	add    $0x10,%esp
    16de:	8b 45 08             	mov    0x8(%ebp),%eax
    16e1:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    16e4:	c9                   	leave
    16e5:	c3                   	ret

000016e6 <strncat>:
    16e6:	55                   	push   %ebp
    16e7:	89 e5                	mov    %esp,%ebp
    16e9:	53                   	push   %ebx
    16ea:	83 ec 14             	sub    $0x14,%esp
    16ed:	e8 b9 f9 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    16f2:	05 4a 35 00 00       	add    $0x354a,%eax
    16f7:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    16fb:	74 0c                	je     1709 <strncat+0x23>
    16fd:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    1701:	74 06                	je     1709 <strncat+0x23>
    1703:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    1707:	75 05                	jne    170e <strncat+0x28>
    1709:	8b 45 08             	mov    0x8(%ebp),%eax
    170c:	eb 61                	jmp    176f <strncat+0x89>
    170e:	83 ec 0c             	sub    $0xc,%esp
    1711:	ff 75 08             	push   0x8(%ebp)
    1714:	89 c3                	mov    %eax,%ebx
    1716:	e8 15 27 00 00       	call   3e30 <strlen@plt>
    171b:	83 c4 10             	add    $0x10,%esp
    171e:	8b 55 08             	mov    0x8(%ebp),%edx
    1721:	01 d0                	add    %edx,%eax
    1723:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1726:	eb 17                	jmp    173f <strncat+0x59>
    1728:	8b 45 0c             	mov    0xc(%ebp),%eax
    172b:	8d 50 01             	lea    0x1(%eax),%edx
    172e:	89 55 0c             	mov    %edx,0xc(%ebp)
    1731:	0f b6 10             	movzbl (%eax),%edx
    1734:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1737:	8d 48 01             	lea    0x1(%eax),%ecx
    173a:	89 4d f4             	mov    %ecx,-0xc(%ebp)
    173d:	88 10                	mov    %dl,(%eax)
    173f:	8b 45 10             	mov    0x10(%ebp),%eax
    1742:	8d 50 ff             	lea    -0x1(%eax),%edx
    1745:	89 55 10             	mov    %edx,0x10(%ebp)
    1748:	85 c0                	test   %eax,%eax
    174a:	74 11                	je     175d <strncat+0x77>
    174c:	8b 45 0c             	mov    0xc(%ebp),%eax
    174f:	0f b6 00             	movzbl (%eax),%eax
    1752:	84 c0                	test   %al,%al
    1754:	74 07                	je     175d <strncat+0x77>
    1756:	b8 01 00 00 00       	mov    $0x1,%eax
    175b:	eb 05                	jmp    1762 <strncat+0x7c>
    175d:	b8 00 00 00 00       	mov    $0x0,%eax
    1762:	84 c0                	test   %al,%al
    1764:	75 c2                	jne    1728 <strncat+0x42>
    1766:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1769:	c6 00 00             	movb   $0x0,(%eax)
    176c:	8b 45 08             	mov    0x8(%ebp),%eax
    176f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1772:	c9                   	leave
    1773:	c3                   	ret

00001774 <strlcat>:
    1774:	55                   	push   %ebp
    1775:	89 e5                	mov    %esp,%ebp
    1777:	53                   	push   %ebx
    1778:	83 ec 14             	sub    $0x14,%esp
    177b:	e8 7b 03 00 00       	call   1afb <__x86.get_pc_thunk.bx>
    1780:	81 c3 bc 34 00 00    	add    $0x34bc,%ebx
    1786:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    178a:	74 0c                	je     1798 <strlcat+0x24>
    178c:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    1790:	74 06                	je     1798 <strlcat+0x24>
    1792:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    1796:	75 0a                	jne    17a2 <strlcat+0x2e>
    1798:	b8 00 00 00 00       	mov    $0x0,%eax
    179d:	e9 80 00 00 00       	jmp    1822 <strlcat+0xae>
    17a2:	83 ec 0c             	sub    $0xc,%esp
    17a5:	ff 75 08             	push   0x8(%ebp)
    17a8:	e8 83 26 00 00       	call   3e30 <strlen@plt>
    17ad:	83 c4 10             	add    $0x10,%esp
    17b0:	89 45 f0             	mov    %eax,-0x10(%ebp)
    17b3:	83 ec 0c             	sub    $0xc,%esp
    17b6:	ff 75 0c             	push   0xc(%ebp)
    17b9:	e8 72 26 00 00       	call   3e30 <strlen@plt>
    17be:	83 c4 10             	add    $0x10,%esp
    17c1:	89 45 ec             	mov    %eax,-0x14(%ebp)
    17c4:	8b 45 f0             	mov    -0x10(%ebp),%eax
    17c7:	3b 45 10             	cmp    0x10(%ebp),%eax
    17ca:	72 0a                	jb     17d6 <strlcat+0x62>
    17cc:	8b 55 10             	mov    0x10(%ebp),%edx
    17cf:	8b 45 ec             	mov    -0x14(%ebp),%eax
    17d2:	01 d0                	add    %edx,%eax
    17d4:	eb 4c                	jmp    1822 <strlcat+0xae>
    17d6:	8b 45 10             	mov    0x10(%ebp),%eax
    17d9:	2b 45 f0             	sub    -0x10(%ebp),%eax
    17dc:	83 e8 01             	sub    $0x1,%eax
    17df:	89 45 f4             	mov    %eax,-0xc(%ebp)
    17e2:	8b 45 f4             	mov    -0xc(%ebp),%eax
    17e5:	39 45 ec             	cmp    %eax,-0x14(%ebp)
    17e8:	73 06                	jae    17f0 <strlcat+0x7c>
    17ea:	8b 45 ec             	mov    -0x14(%ebp),%eax
    17ed:	89 45 f4             	mov    %eax,-0xc(%ebp)
    17f0:	8b 55 08             	mov    0x8(%ebp),%edx
    17f3:	8b 45 f0             	mov    -0x10(%ebp),%eax
    17f6:	01 d0                	add    %edx,%eax
    17f8:	83 ec 04             	sub    $0x4,%esp
    17fb:	ff 75 f4             	push   -0xc(%ebp)
    17fe:	ff 75 0c             	push   0xc(%ebp)
    1801:	50                   	push   %eax
    1802:	e8 79 25 00 00       	call   3d80 <memcpy@plt>
    1807:	83 c4 10             	add    $0x10,%esp
    180a:	8b 55 f0             	mov    -0x10(%ebp),%edx
    180d:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1810:	01 c2                	add    %eax,%edx
    1812:	8b 45 08             	mov    0x8(%ebp),%eax
    1815:	01 d0                	add    %edx,%eax
    1817:	c6 00 00             	movb   $0x0,(%eax)
    181a:	8b 55 f0             	mov    -0x10(%ebp),%edx
    181d:	8b 45 ec             	mov    -0x14(%ebp),%eax
    1820:	01 d0                	add    %edx,%eax
    1822:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1825:	c9                   	leave
    1826:	c3                   	ret

00001827 <strcmp>:
    1827:	55                   	push   %ebp
    1828:	89 e5                	mov    %esp,%ebp
    182a:	83 ec 10             	sub    $0x10,%esp
    182d:	e8 79 f8 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1832:	05 0a 34 00 00       	add    $0x340a,%eax
    1837:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    183b:	74 06                	je     1843 <strcmp+0x1c>
    183d:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    1841:	75 0a                	jne    184d <strcmp+0x26>
    1843:	ba 00 00 00 00       	mov    $0x0,%edx
    1848:	e9 9f 00 00 00       	jmp    18ec <strcmp+0xc5>
    184d:	8b 45 08             	mov    0x8(%ebp),%eax
    1850:	83 e0 03             	and    $0x3,%eax
    1853:	85 c0                	test   %eax,%eax
    1855:	75 67                	jne    18be <strcmp+0x97>
    1857:	8b 45 0c             	mov    0xc(%ebp),%eax
    185a:	83 e0 03             	and    $0x3,%eax
    185d:	85 c0                	test   %eax,%eax
    185f:	75 5d                	jne    18be <strcmp+0x97>
    1861:	8b 45 08             	mov    0x8(%ebp),%eax
    1864:	89 45 fc             	mov    %eax,-0x4(%ebp)
    1867:	8b 45 0c             	mov    0xc(%ebp),%eax
    186a:	89 45 f8             	mov    %eax,-0x8(%ebp)
    186d:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1870:	8b 00                	mov    (%eax),%eax
    1872:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1875:	8b 45 f8             	mov    -0x8(%ebp),%eax
    1878:	8b 00                	mov    (%eax),%eax
    187a:	89 45 f0             	mov    %eax,-0x10(%ebp)
    187d:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1880:	3b 45 f0             	cmp    -0x10(%ebp),%eax
    1883:	75 23                	jne    18a8 <strcmp+0x81>
    1885:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1888:	8d 90 ff fe fe fe    	lea    -0x1010101(%eax),%edx
    188e:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1891:	f7 d0                	not    %eax
    1893:	21 d0                	and    %edx,%eax
    1895:	25 80 80 80 80       	and    $0x80808080,%eax
    189a:	85 c0                	test   %eax,%eax
    189c:	75 0a                	jne    18a8 <strcmp+0x81>
    189e:	83 45 fc 04          	addl   $0x4,-0x4(%ebp)
    18a2:	83 45 f8 04          	addl   $0x4,-0x8(%ebp)
    18a6:	eb c5                	jmp    186d <strcmp+0x46>
    18a8:	8b 45 fc             	mov    -0x4(%ebp),%eax
    18ab:	89 45 08             	mov    %eax,0x8(%ebp)
    18ae:	8b 45 f8             	mov    -0x8(%ebp),%eax
    18b1:	89 45 0c             	mov    %eax,0xc(%ebp)
    18b4:	eb 08                	jmp    18be <strcmp+0x97>
    18b6:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    18ba:	83 45 0c 01          	addl   $0x1,0xc(%ebp)
    18be:	8b 45 08             	mov    0x8(%ebp),%eax
    18c1:	0f b6 00             	movzbl (%eax),%eax
    18c4:	84 c0                	test   %al,%al
    18c6:	74 10                	je     18d8 <strcmp+0xb1>
    18c8:	8b 45 08             	mov    0x8(%ebp),%eax
    18cb:	0f b6 10             	movzbl (%eax),%edx
    18ce:	8b 45 0c             	mov    0xc(%ebp),%eax
    18d1:	0f b6 00             	movzbl (%eax),%eax
    18d4:	38 c2                	cmp    %al,%dl
    18d6:	74 de                	je     18b6 <strcmp+0x8f>
    18d8:	8b 45 08             	mov    0x8(%ebp),%eax
    18db:	0f b6 00             	movzbl (%eax),%eax
    18de:	0f b6 d0             	movzbl %al,%edx
    18e1:	8b 45 0c             	mov    0xc(%ebp),%eax
    18e4:	0f b6 00             	movzbl (%eax),%eax
    18e7:	0f b6 c0             	movzbl %al,%eax
    18ea:	29 c2                	sub    %eax,%edx
    18ec:	89 d0                	mov    %edx,%eax
    18ee:	c9                   	leave
    18ef:	c3                   	ret

000018f0 <strncmp>:
    18f0:	55                   	push   %ebp
    18f1:	89 e5                	mov    %esp,%ebp
    18f3:	e8 b3 f7 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    18f8:	05 44 33 00 00       	add    $0x3344,%eax
    18fd:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1901:	74 0c                	je     190f <strncmp+0x1f>
    1903:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    1907:	74 06                	je     190f <strncmp+0x1f>
    1909:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    190d:	75 48                	jne    1957 <strncmp+0x67>
    190f:	b8 00 00 00 00       	mov    $0x0,%eax
    1914:	eb 58                	jmp    196e <strncmp+0x7e>
    1916:	8b 45 08             	mov    0x8(%ebp),%eax
    1919:	0f b6 10             	movzbl (%eax),%edx
    191c:	8b 45 0c             	mov    0xc(%ebp),%eax
    191f:	0f b6 00             	movzbl (%eax),%eax
    1922:	38 c2                	cmp    %al,%dl
    1924:	74 18                	je     193e <strncmp+0x4e>
    1926:	8b 45 08             	mov    0x8(%ebp),%eax
    1929:	0f b6 00             	movzbl (%eax),%eax
    192c:	0f b6 c8             	movzbl %al,%ecx
    192f:	8b 45 0c             	mov    0xc(%ebp),%eax
    1932:	0f b6 00             	movzbl (%eax),%eax
    1935:	0f b6 d0             	movzbl %al,%edx
    1938:	89 c8                	mov    %ecx,%eax
    193a:	29 d0                	sub    %edx,%eax
    193c:	eb 30                	jmp    196e <strncmp+0x7e>
    193e:	8b 45 08             	mov    0x8(%ebp),%eax
    1941:	0f b6 00             	movzbl (%eax),%eax
    1944:	84 c0                	test   %al,%al
    1946:	75 07                	jne    194f <strncmp+0x5f>
    1948:	b8 00 00 00 00       	mov    $0x0,%eax
    194d:	eb 1f                	jmp    196e <strncmp+0x7e>
    194f:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    1953:	83 45 0c 01          	addl   $0x1,0xc(%ebp)
    1957:	8b 45 10             	mov    0x10(%ebp),%eax
    195a:	8d 50 ff             	lea    -0x1(%eax),%edx
    195d:	89 55 10             	mov    %edx,0x10(%ebp)
    1960:	85 c0                	test   %eax,%eax
    1962:	0f 95 c0             	setne  %al
    1965:	84 c0                	test   %al,%al
    1967:	75 ad                	jne    1916 <strncmp+0x26>
    1969:	b8 00 00 00 00       	mov    $0x0,%eax
    196e:	5d                   	pop    %ebp
    196f:	c3                   	ret

00001970 <strncmp_s>:
    1970:	55                   	push   %ebp
    1971:	89 e5                	mov    %esp,%ebp
    1973:	53                   	push   %ebx
    1974:	83 ec 14             	sub    $0x14,%esp
    1977:	e8 7b 01 00 00       	call   1af7 <__x86.get_pc_thunk.dx>
    197c:	81 c2 c0 32 00 00    	add    $0x32c0,%edx
    1982:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1986:	74 06                	je     198e <strncmp_s+0x1e>
    1988:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    198c:	75 07                	jne    1995 <strncmp_s+0x25>
    198e:	b8 00 00 00 00       	mov    $0x0,%eax
    1993:	eb 2a                	jmp    19bf <strncmp_s+0x4f>
    1995:	8b 45 0c             	mov    0xc(%ebp),%eax
    1998:	3b 45 14             	cmp    0x14(%ebp),%eax
    199b:	73 05                	jae    19a2 <strncmp_s+0x32>
    199d:	8b 45 0c             	mov    0xc(%ebp),%eax
    19a0:	eb 03                	jmp    19a5 <strncmp_s+0x35>
    19a2:	8b 45 14             	mov    0x14(%ebp),%eax
    19a5:	89 45 f4             	mov    %eax,-0xc(%ebp)
    19a8:	83 ec 04             	sub    $0x4,%esp
    19ab:	ff 75 f4             	push   -0xc(%ebp)
    19ae:	ff 75 10             	push   0x10(%ebp)
    19b1:	ff 75 08             	push   0x8(%ebp)
    19b4:	89 d3                	mov    %edx,%ebx
    19b6:	e8 e5 23 00 00       	call   3da0 <strncmp@plt>
    19bb:	83 c4 10             	add    $0x10,%esp
    19be:	90                   	nop
    19bf:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    19c2:	c9                   	leave
    19c3:	c3                   	ret

000019c4 <strchr>:
    19c4:	55                   	push   %ebp
    19c5:	89 e5                	mov    %esp,%ebp
    19c7:	e8 df f6 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    19cc:	05 70 32 00 00       	add    $0x3270,%eax
    19d1:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    19d5:	75 1d                	jne    19f4 <strchr+0x30>
    19d7:	b8 00 00 00 00       	mov    $0x0,%eax
    19dc:	eb 30                	jmp    1a0e <strchr+0x4a>
    19de:	8b 45 08             	mov    0x8(%ebp),%eax
    19e1:	0f b6 00             	movzbl (%eax),%eax
    19e4:	8b 55 0c             	mov    0xc(%ebp),%edx
    19e7:	38 d0                	cmp    %dl,%al
    19e9:	75 05                	jne    19f0 <strchr+0x2c>
    19eb:	8b 45 08             	mov    0x8(%ebp),%eax
    19ee:	eb 1e                	jmp    1a0e <strchr+0x4a>
    19f0:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    19f4:	8b 45 08             	mov    0x8(%ebp),%eax
    19f7:	0f b6 00             	movzbl (%eax),%eax
    19fa:	84 c0                	test   %al,%al
    19fc:	75 e0                	jne    19de <strchr+0x1a>
    19fe:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    1a02:	75 05                	jne    1a09 <strchr+0x45>
    1a04:	8b 45 08             	mov    0x8(%ebp),%eax
    1a07:	eb 05                	jmp    1a0e <strchr+0x4a>
    1a09:	b8 00 00 00 00       	mov    $0x0,%eax
    1a0e:	5d                   	pop    %ebp
    1a0f:	c3                   	ret

00001a10 <strrchr>:
    1a10:	55                   	push   %ebp
    1a11:	89 e5                	mov    %esp,%ebp
    1a13:	83 ec 10             	sub    $0x10,%esp
    1a16:	e8 90 f6 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1a1b:	05 21 32 00 00       	add    $0x3221,%eax
    1a20:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1a24:	75 07                	jne    1a2d <strrchr+0x1d>
    1a26:	b8 00 00 00 00       	mov    $0x0,%eax
    1a2b:	eb 38                	jmp    1a65 <strrchr+0x55>
    1a2d:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
    1a34:	eb 17                	jmp    1a4d <strrchr+0x3d>
    1a36:	8b 45 08             	mov    0x8(%ebp),%eax
    1a39:	0f b6 00             	movzbl (%eax),%eax
    1a3c:	8b 55 0c             	mov    0xc(%ebp),%edx
    1a3f:	38 d0                	cmp    %dl,%al
    1a41:	75 06                	jne    1a49 <strrchr+0x39>
    1a43:	8b 45 08             	mov    0x8(%ebp),%eax
    1a46:	89 45 fc             	mov    %eax,-0x4(%ebp)
    1a49:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    1a4d:	8b 45 08             	mov    0x8(%ebp),%eax
    1a50:	0f b6 00             	movzbl (%eax),%eax
    1a53:	84 c0                	test   %al,%al
    1a55:	75 df                	jne    1a36 <strrchr+0x26>
    1a57:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    1a5b:	75 05                	jne    1a62 <strrchr+0x52>
    1a5d:	8b 45 08             	mov    0x8(%ebp),%eax
    1a60:	eb 03                	jmp    1a65 <strrchr+0x55>
    1a62:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1a65:	c9                   	leave
    1a66:	c3                   	ret

00001a67 <strstr>:
    1a67:	55                   	push   %ebp
    1a68:	89 e5                	mov    %esp,%ebp
    1a6a:	83 ec 10             	sub    $0x10,%esp
    1a6d:	e8 39 f6 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1a72:	05 ca 31 00 00       	add    $0x31ca,%eax
    1a77:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1a7b:	74 06                	je     1a83 <strstr+0x1c>
    1a7d:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    1a81:	75 07                	jne    1a8a <strstr+0x23>
    1a83:	b8 00 00 00 00       	mov    $0x0,%eax
    1a88:	eb 6b                	jmp    1af5 <strstr+0x8e>
    1a8a:	8b 45 0c             	mov    0xc(%ebp),%eax
    1a8d:	0f b6 00             	movzbl (%eax),%eax
    1a90:	84 c0                	test   %al,%al
    1a92:	75 52                	jne    1ae6 <strstr+0x7f>
    1a94:	8b 45 08             	mov    0x8(%ebp),%eax
    1a97:	eb 5c                	jmp    1af5 <strstr+0x8e>
    1a99:	8b 45 08             	mov    0x8(%ebp),%eax
    1a9c:	89 45 fc             	mov    %eax,-0x4(%ebp)
    1a9f:	8b 45 0c             	mov    0xc(%ebp),%eax
    1aa2:	89 45 f8             	mov    %eax,-0x8(%ebp)
    1aa5:	eb 08                	jmp    1aaf <strstr+0x48>
    1aa7:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
    1aab:	83 45 f8 01          	addl   $0x1,-0x8(%ebp)
    1aaf:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1ab2:	0f b6 00             	movzbl (%eax),%eax
    1ab5:	84 c0                	test   %al,%al
    1ab7:	74 1a                	je     1ad3 <strstr+0x6c>
    1ab9:	8b 45 f8             	mov    -0x8(%ebp),%eax
    1abc:	0f b6 00             	movzbl (%eax),%eax
    1abf:	84 c0                	test   %al,%al
    1ac1:	74 10                	je     1ad3 <strstr+0x6c>
    1ac3:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1ac6:	0f b6 10             	movzbl (%eax),%edx
    1ac9:	8b 45 f8             	mov    -0x8(%ebp),%eax
    1acc:	0f b6 00             	movzbl (%eax),%eax
    1acf:	38 c2                	cmp    %al,%dl
    1ad1:	74 d4                	je     1aa7 <strstr+0x40>
    1ad3:	8b 45 f8             	mov    -0x8(%ebp),%eax
    1ad6:	0f b6 00             	movzbl (%eax),%eax
    1ad9:	84 c0                	test   %al,%al
    1adb:	75 05                	jne    1ae2 <strstr+0x7b>
    1add:	8b 45 08             	mov    0x8(%ebp),%eax
    1ae0:	eb 13                	jmp    1af5 <strstr+0x8e>
    1ae2:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    1ae6:	8b 45 08             	mov    0x8(%ebp),%eax
    1ae9:	0f b6 00             	movzbl (%eax),%eax
    1aec:	84 c0                	test   %al,%al
    1aee:	75 a9                	jne    1a99 <strstr+0x32>
    1af0:	b8 00 00 00 00       	mov    $0x0,%eax
    1af5:	c9                   	leave
    1af6:	c3                   	ret

00001af7 <__x86.get_pc_thunk.dx>:
    1af7:	8b 14 24             	mov    (%esp),%edx
    1afa:	c3                   	ret

00001afb <__x86.get_pc_thunk.bx>:
    1afb:	8b 1c 24             	mov    (%esp),%ebx
    1afe:	c3                   	ret

00001aff <mutex_lock>:
    1aff:	55                   	push   %ebp
    1b00:	89 e5                	mov    %esp,%ebp
    1b02:	e8 a4 f5 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1b07:	05 35 31 00 00       	add    $0x3135,%eax
    1b0c:	eb 02                	jmp    1b10 <mutex_lock+0x11>
    1b0e:	f3 90                	pause
    1b10:	8b 55 08             	mov    0x8(%ebp),%edx
    1b13:	b8 01 00 00 00       	mov    $0x1,%eax
    1b18:	86 02                	xchg   %al,(%edx)
    1b1a:	84 c0                	test   %al,%al
    1b1c:	75 f0                	jne    1b0e <mutex_lock+0xf>
    1b1e:	90                   	nop
    1b1f:	90                   	nop
    1b20:	5d                   	pop    %ebp
    1b21:	c3                   	ret

00001b22 <mutex_unlock>:
    1b22:	55                   	push   %ebp
    1b23:	89 e5                	mov    %esp,%ebp
    1b25:	e8 81 f5 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1b2a:	05 12 31 00 00       	add    $0x3112,%eax
    1b2f:	8b 45 08             	mov    0x8(%ebp),%eax
    1b32:	ba 00 00 00 00       	mov    $0x0,%edx
    1b37:	88 10                	mov    %dl,(%eax)
    1b39:	90                   	nop
    1b3a:	5d                   	pop    %ebp
    1b3b:	c3                   	ret

00001b3c <_ZL13request_spaceP8MemBlockj>:
    1b3c:	55                   	push   %ebp
    1b3d:	89 e5                	mov    %esp,%ebp
    1b3f:	53                   	push   %ebx
    1b40:	83 ec 14             	sub    $0x14,%esp
    1b43:	e8 63 f5 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1b48:	05 f4 30 00 00       	add    $0x30f4,%eax
    1b4d:	83 ec 08             	sub    $0x8,%esp
    1b50:	ff 75 0c             	push   0xc(%ebp)
    1b53:	6a 1c                	push   $0x1c
    1b55:	89 c3                	mov    %eax,%ebx
    1b57:	e8 04 22 00 00       	call   3d60 <syscall@plt>
    1b5c:	83 c4 10             	add    $0x10,%esp
    1b5f:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1b62:	83 7d f4 ff          	cmpl   $0xffffffff,-0xc(%ebp)
    1b66:	75 07                	jne    1b6f <_ZL13request_spaceP8MemBlockj+0x33>
    1b68:	b8 00 00 00 00       	mov    $0x0,%eax
    1b6d:	eb 44                	jmp    1bb3 <_ZL13request_spaceP8MemBlockj+0x77>
    1b6f:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1b72:	89 45 f0             	mov    %eax,-0x10(%ebp)
    1b75:	8b 45 f0             	mov    -0x10(%ebp),%eax
    1b78:	8b 55 0c             	mov    0xc(%ebp),%edx
    1b7b:	89 10                	mov    %edx,(%eax)
    1b7d:	8b 45 f0             	mov    -0x10(%ebp),%eax
    1b80:	c6 40 04 00          	movb   $0x0,0x4(%eax)
    1b84:	8b 45 f0             	mov    -0x10(%ebp),%eax
    1b87:	c7 40 08 de c0 ad de 	movl   $0xdeadc0de,0x8(%eax)
    1b8e:	8b 45 f0             	mov    -0x10(%ebp),%eax
    1b91:	c7 40 0c 00 00 00 00 	movl   $0x0,0xc(%eax)
    1b98:	8b 45 f0             	mov    -0x10(%ebp),%eax
    1b9b:	8b 55 08             	mov    0x8(%ebp),%edx
    1b9e:	89 50 10             	mov    %edx,0x10(%eax)
    1ba1:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1ba5:	74 09                	je     1bb0 <_ZL13request_spaceP8MemBlockj+0x74>
    1ba7:	8b 45 08             	mov    0x8(%ebp),%eax
    1baa:	8b 55 f0             	mov    -0x10(%ebp),%edx
    1bad:	89 50 0c             	mov    %edx,0xc(%eax)
    1bb0:	8b 45 f0             	mov    -0x10(%ebp),%eax
    1bb3:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1bb6:	c9                   	leave
    1bb7:	c3                   	ret

00001bb8 <_ZL15find_free_blockPP8MemBlockj>:
    1bb8:	55                   	push   %ebp
    1bb9:	89 e5                	mov    %esp,%ebp
    1bbb:	83 ec 10             	sub    $0x10,%esp
    1bbe:	e8 e8 f4 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1bc3:	05 79 30 00 00       	add    $0x3079,%eax
    1bc8:	8b 80 cc 13 00 00    	mov    0x13cc(%eax),%eax
    1bce:	89 45 fc             	mov    %eax,-0x4(%ebp)
    1bd1:	eb 11                	jmp    1be4 <_ZL15find_free_blockPP8MemBlockj+0x2c>
    1bd3:	8b 45 08             	mov    0x8(%ebp),%eax
    1bd6:	8b 55 fc             	mov    -0x4(%ebp),%edx
    1bd9:	89 10                	mov    %edx,(%eax)
    1bdb:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1bde:	8b 40 0c             	mov    0xc(%eax),%eax
    1be1:	89 45 fc             	mov    %eax,-0x4(%ebp)
    1be4:	83 7d fc 00          	cmpl   $0x0,-0x4(%ebp)
    1be8:	74 18                	je     1c02 <_ZL15find_free_blockPP8MemBlockj+0x4a>
    1bea:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1bed:	0f b6 40 04          	movzbl 0x4(%eax),%eax
    1bf1:	83 f0 01             	xor    $0x1,%eax
    1bf4:	84 c0                	test   %al,%al
    1bf6:	75 db                	jne    1bd3 <_ZL15find_free_blockPP8MemBlockj+0x1b>
    1bf8:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1bfb:	8b 00                	mov    (%eax),%eax
    1bfd:	3b 45 0c             	cmp    0xc(%ebp),%eax
    1c00:	72 d1                	jb     1bd3 <_ZL15find_free_blockPP8MemBlockj+0x1b>
    1c02:	8b 45 fc             	mov    -0x4(%ebp),%eax
    1c05:	c9                   	leave
    1c06:	c3                   	ret

00001c07 <_ZL11split_blockP8MemBlockj>:
    1c07:	55                   	push   %ebp
    1c08:	89 e5                	mov    %esp,%ebp
    1c0a:	83 ec 10             	sub    $0x10,%esp
    1c0d:	e8 99 f4 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1c12:	05 2a 30 00 00       	add    $0x302a,%eax
    1c17:	8b 55 08             	mov    0x8(%ebp),%edx
    1c1a:	8b 12                	mov    (%edx),%edx
    1c1c:	8b 4d 0c             	mov    0xc(%ebp),%ecx
    1c1f:	83 c1 1c             	add    $0x1c,%ecx
    1c22:	39 d1                	cmp    %edx,%ecx
    1c24:	73 72                	jae    1c98 <_ZL11split_blockP8MemBlockj+0x91>
    1c26:	8b 4d 08             	mov    0x8(%ebp),%ecx
    1c29:	8b 55 0c             	mov    0xc(%ebp),%edx
    1c2c:	01 ca                	add    %ecx,%edx
    1c2e:	89 55 fc             	mov    %edx,-0x4(%ebp)
    1c31:	8b 55 08             	mov    0x8(%ebp),%edx
    1c34:	8b 12                	mov    (%edx),%edx
    1c36:	89 d1                	mov    %edx,%ecx
    1c38:	2b 4d 0c             	sub    0xc(%ebp),%ecx
    1c3b:	8b 55 fc             	mov    -0x4(%ebp),%edx
    1c3e:	89 0a                	mov    %ecx,(%edx)
    1c40:	8b 55 fc             	mov    -0x4(%ebp),%edx
    1c43:	c6 42 04 01          	movb   $0x1,0x4(%edx)
    1c47:	8b 55 fc             	mov    -0x4(%ebp),%edx
    1c4a:	c7 42 08 de c0 ad de 	movl   $0xdeadc0de,0x8(%edx)
    1c51:	8b 55 08             	mov    0x8(%ebp),%edx
    1c54:	8b 4a 0c             	mov    0xc(%edx),%ecx
    1c57:	8b 55 fc             	mov    -0x4(%ebp),%edx
    1c5a:	89 4a 0c             	mov    %ecx,0xc(%edx)
    1c5d:	8b 55 fc             	mov    -0x4(%ebp),%edx
    1c60:	8b 4d 08             	mov    0x8(%ebp),%ecx
    1c63:	89 4a 10             	mov    %ecx,0x10(%edx)
    1c66:	8b 55 08             	mov    0x8(%ebp),%edx
    1c69:	8b 52 0c             	mov    0xc(%edx),%edx
    1c6c:	85 d2                	test   %edx,%edx
    1c6e:	74 0e                	je     1c7e <_ZL11split_blockP8MemBlockj+0x77>
    1c70:	8b 45 08             	mov    0x8(%ebp),%eax
    1c73:	8b 40 0c             	mov    0xc(%eax),%eax
    1c76:	8b 55 fc             	mov    -0x4(%ebp),%edx
    1c79:	89 50 10             	mov    %edx,0x10(%eax)
    1c7c:	eb 09                	jmp    1c87 <_ZL11split_blockP8MemBlockj+0x80>
    1c7e:	8b 55 fc             	mov    -0x4(%ebp),%edx
    1c81:	89 90 d0 13 00 00    	mov    %edx,0x13d0(%eax)
    1c87:	8b 45 08             	mov    0x8(%ebp),%eax
    1c8a:	8b 55 fc             	mov    -0x4(%ebp),%edx
    1c8d:	89 50 0c             	mov    %edx,0xc(%eax)
    1c90:	8b 45 08             	mov    0x8(%ebp),%eax
    1c93:	8b 55 0c             	mov    0xc(%ebp),%edx
    1c96:	89 10                	mov    %edx,(%eax)
    1c98:	90                   	nop
    1c99:	c9                   	leave
    1c9a:	c3                   	ret

00001c9b <_ZL16_malloc_unlockedj>:
    1c9b:	55                   	push   %ebp
    1c9c:	89 e5                	mov    %esp,%ebp
    1c9e:	53                   	push   %ebx
    1c9f:	83 ec 14             	sub    $0x14,%esp
    1ca2:	e8 54 fe ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    1ca7:	81 c3 95 2f 00 00    	add    $0x2f95,%ebx
    1cad:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1cb1:	75 0a                	jne    1cbd <_ZL16_malloc_unlockedj+0x22>
    1cb3:	b8 00 00 00 00       	mov    $0x0,%eax
    1cb8:	e9 bc 00 00 00       	jmp    1d79 <_ZL16_malloc_unlockedj+0xde>
    1cbd:	8b 45 08             	mov    0x8(%ebp),%eax
    1cc0:	83 c0 1b             	add    $0x1b,%eax
    1cc3:	83 e0 f8             	and    $0xfffffff8,%eax
    1cc6:	89 45 f0             	mov    %eax,-0x10(%ebp)
    1cc9:	8b 83 cc 13 00 00    	mov    0x13cc(%ebx),%eax
    1ccf:	85 c0                	test   %eax,%eax
    1cd1:	75 37                	jne    1d0a <_ZL16_malloc_unlockedj+0x6f>
    1cd3:	83 ec 08             	sub    $0x8,%esp
    1cd6:	ff 75 f0             	push   -0x10(%ebp)
    1cd9:	6a 00                	push   $0x0
    1cdb:	e8 5c fe ff ff       	call   1b3c <_ZL13request_spaceP8MemBlockj>
    1ce0:	83 c4 10             	add    $0x10,%esp
    1ce3:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1ce6:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    1cea:	75 0a                	jne    1cf6 <_ZL16_malloc_unlockedj+0x5b>
    1cec:	b8 00 00 00 00       	mov    $0x0,%eax
    1cf1:	e9 83 00 00 00       	jmp    1d79 <_ZL16_malloc_unlockedj+0xde>
    1cf6:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1cf9:	89 83 cc 13 00 00    	mov    %eax,0x13cc(%ebx)
    1cff:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1d02:	89 83 d0 13 00 00    	mov    %eax,0x13d0(%ebx)
    1d08:	eb 69                	jmp    1d73 <_ZL16_malloc_unlockedj+0xd8>
    1d0a:	8b 83 cc 13 00 00    	mov    0x13cc(%ebx),%eax
    1d10:	89 45 ec             	mov    %eax,-0x14(%ebp)
    1d13:	83 ec 08             	sub    $0x8,%esp
    1d16:	ff 75 f0             	push   -0x10(%ebp)
    1d19:	8d 45 ec             	lea    -0x14(%ebp),%eax
    1d1c:	50                   	push   %eax
    1d1d:	e8 96 fe ff ff       	call   1bb8 <_ZL15find_free_blockPP8MemBlockj>
    1d22:	83 c4 10             	add    $0x10,%esp
    1d25:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1d28:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    1d2c:	75 2d                	jne    1d5b <_ZL16_malloc_unlockedj+0xc0>
    1d2e:	8b 45 ec             	mov    -0x14(%ebp),%eax
    1d31:	83 ec 08             	sub    $0x8,%esp
    1d34:	ff 75 f0             	push   -0x10(%ebp)
    1d37:	50                   	push   %eax
    1d38:	e8 ff fd ff ff       	call   1b3c <_ZL13request_spaceP8MemBlockj>
    1d3d:	83 c4 10             	add    $0x10,%esp
    1d40:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1d43:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    1d47:	75 07                	jne    1d50 <_ZL16_malloc_unlockedj+0xb5>
    1d49:	b8 00 00 00 00       	mov    $0x0,%eax
    1d4e:	eb 29                	jmp    1d79 <_ZL16_malloc_unlockedj+0xde>
    1d50:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1d53:	89 83 d0 13 00 00    	mov    %eax,0x13d0(%ebx)
    1d59:	eb 18                	jmp    1d73 <_ZL16_malloc_unlockedj+0xd8>
    1d5b:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1d5e:	c6 40 04 00          	movb   $0x0,0x4(%eax)
    1d62:	83 ec 08             	sub    $0x8,%esp
    1d65:	ff 75 f0             	push   -0x10(%ebp)
    1d68:	ff 75 f4             	push   -0xc(%ebp)
    1d6b:	e8 97 fe ff ff       	call   1c07 <_ZL11split_blockP8MemBlockj>
    1d70:	83 c4 10             	add    $0x10,%esp
    1d73:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1d76:	83 c0 14             	add    $0x14,%eax
    1d79:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1d7c:	c9                   	leave
    1d7d:	c3                   	ret

00001d7e <malloc>:
    1d7e:	55                   	push   %ebp
    1d7f:	89 e5                	mov    %esp,%ebp
    1d81:	53                   	push   %ebx
    1d82:	83 ec 14             	sub    $0x14,%esp
    1d85:	e8 71 fd ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    1d8a:	81 c3 b2 2e 00 00    	add    $0x2eb2,%ebx
    1d90:	8d 83 c8 13 00 00    	lea    0x13c8(%ebx),%eax
    1d96:	50                   	push   %eax
    1d97:	e8 63 fd ff ff       	call   1aff <mutex_lock>
    1d9c:	83 c4 04             	add    $0x4,%esp
    1d9f:	83 ec 0c             	sub    $0xc,%esp
    1da2:	ff 75 08             	push   0x8(%ebp)
    1da5:	e8 f1 fe ff ff       	call   1c9b <_ZL16_malloc_unlockedj>
    1daa:	83 c4 10             	add    $0x10,%esp
    1dad:	89 45 f4             	mov    %eax,-0xc(%ebp)
    1db0:	83 ec 0c             	sub    $0xc,%esp
    1db3:	8d 83 c8 13 00 00    	lea    0x13c8(%ebx),%eax
    1db9:	50                   	push   %eax
    1dba:	e8 63 fd ff ff       	call   1b22 <mutex_unlock>
    1dbf:	83 c4 10             	add    $0x10,%esp
    1dc2:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1dc5:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1dc8:	c9                   	leave
    1dc9:	c3                   	ret

00001dca <_ZL14_free_unlockedPv>:
    1dca:	55                   	push   %ebp
    1dcb:	89 e5                	mov    %esp,%ebp
    1dcd:	53                   	push   %ebx
    1dce:	83 ec 14             	sub    $0x14,%esp
    1dd1:	e8 d5 f2 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1dd6:	05 66 2e 00 00       	add    $0x2e66,%eax
    1ddb:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    1ddf:	0f 84 59 01 00 00    	je     1f3e <_ZL14_free_unlockedPv+0x174>
    1de5:	8b 55 08             	mov    0x8(%ebp),%edx
    1de8:	83 ea 14             	sub    $0x14,%edx
    1deb:	89 55 f4             	mov    %edx,-0xc(%ebp)
    1dee:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1df1:	8b 52 08             	mov    0x8(%edx),%edx
    1df4:	81 fa de c0 ad de    	cmp    $0xdeadc0de,%edx
    1dfa:	74 14                	je     1e10 <_ZL14_free_unlockedPv+0x46>
    1dfc:	83 ec 08             	sub    $0x8,%esp
    1dff:	6a 01                	push   $0x1
    1e01:	6a 00                	push   $0x0
    1e03:	89 c3                	mov    %eax,%ebx
    1e05:	e8 56 1f 00 00       	call   3d60 <syscall@plt>
    1e0a:	83 c4 10             	add    $0x10,%esp
    1e0d:	90                   	nop
    1e0e:	eb fd                	jmp    1e0d <_ZL14_free_unlockedPv+0x43>
    1e10:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e13:	c6 42 04 01          	movb   $0x1,0x4(%edx)
    1e17:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e1a:	8b 52 0c             	mov    0xc(%edx),%edx
    1e1d:	85 d2                	test   %edx,%edx
    1e1f:	74 52                	je     1e73 <_ZL14_free_unlockedPv+0xa9>
    1e21:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e24:	8b 52 0c             	mov    0xc(%edx),%edx
    1e27:	0f b6 52 04          	movzbl 0x4(%edx),%edx
    1e2b:	84 d2                	test   %dl,%dl
    1e2d:	74 44                	je     1e73 <_ZL14_free_unlockedPv+0xa9>
    1e2f:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e32:	8b 0a                	mov    (%edx),%ecx
    1e34:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e37:	8b 52 0c             	mov    0xc(%edx),%edx
    1e3a:	8b 12                	mov    (%edx),%edx
    1e3c:	01 d1                	add    %edx,%ecx
    1e3e:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e41:	89 0a                	mov    %ecx,(%edx)
    1e43:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e46:	8b 52 0c             	mov    0xc(%edx),%edx
    1e49:	8b 4a 0c             	mov    0xc(%edx),%ecx
    1e4c:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e4f:	89 4a 0c             	mov    %ecx,0xc(%edx)
    1e52:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e55:	8b 52 0c             	mov    0xc(%edx),%edx
    1e58:	85 d2                	test   %edx,%edx
    1e5a:	74 0e                	je     1e6a <_ZL14_free_unlockedPv+0xa0>
    1e5c:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e5f:	8b 52 0c             	mov    0xc(%edx),%edx
    1e62:	8b 4d f4             	mov    -0xc(%ebp),%ecx
    1e65:	89 4a 10             	mov    %ecx,0x10(%edx)
    1e68:	eb 09                	jmp    1e73 <_ZL14_free_unlockedPv+0xa9>
    1e6a:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e6d:	89 90 d0 13 00 00    	mov    %edx,0x13d0(%eax)
    1e73:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e76:	8b 52 10             	mov    0x10(%edx),%edx
    1e79:	85 d2                	test   %edx,%edx
    1e7b:	74 64                	je     1ee1 <_ZL14_free_unlockedPv+0x117>
    1e7d:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e80:	8b 52 10             	mov    0x10(%edx),%edx
    1e83:	0f b6 52 04          	movzbl 0x4(%edx),%edx
    1e87:	84 d2                	test   %dl,%dl
    1e89:	74 56                	je     1ee1 <_ZL14_free_unlockedPv+0x117>
    1e8b:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e8e:	8b 52 10             	mov    0x10(%edx),%edx
    1e91:	8b 1a                	mov    (%edx),%ebx
    1e93:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e96:	8b 0a                	mov    (%edx),%ecx
    1e98:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1e9b:	8b 52 10             	mov    0x10(%edx),%edx
    1e9e:	01 d9                	add    %ebx,%ecx
    1ea0:	89 0a                	mov    %ecx,(%edx)
    1ea2:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1ea5:	8b 52 10             	mov    0x10(%edx),%edx
    1ea8:	8b 4d f4             	mov    -0xc(%ebp),%ecx
    1eab:	8b 49 0c             	mov    0xc(%ecx),%ecx
    1eae:	89 4a 0c             	mov    %ecx,0xc(%edx)
    1eb1:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1eb4:	8b 52 0c             	mov    0xc(%edx),%edx
    1eb7:	85 d2                	test   %edx,%edx
    1eb9:	74 11                	je     1ecc <_ZL14_free_unlockedPv+0x102>
    1ebb:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1ebe:	8b 52 0c             	mov    0xc(%edx),%edx
    1ec1:	8b 4d f4             	mov    -0xc(%ebp),%ecx
    1ec4:	8b 49 10             	mov    0x10(%ecx),%ecx
    1ec7:	89 4a 10             	mov    %ecx,0x10(%edx)
    1eca:	eb 0c                	jmp    1ed8 <_ZL14_free_unlockedPv+0x10e>
    1ecc:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1ecf:	8b 52 10             	mov    0x10(%edx),%edx
    1ed2:	89 90 d0 13 00 00    	mov    %edx,0x13d0(%eax)
    1ed8:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1edb:	8b 52 10             	mov    0x10(%edx),%edx
    1ede:	89 55 f4             	mov    %edx,-0xc(%ebp)
    1ee1:	8b 90 d0 13 00 00    	mov    0x13d0(%eax),%edx
    1ee7:	39 55 f4             	cmp    %edx,-0xc(%ebp)
    1eea:	75 53                	jne    1f3f <_ZL14_free_unlockedPv+0x175>
    1eec:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1eef:	8b 52 10             	mov    0x10(%edx),%edx
    1ef2:	85 d2                	test   %edx,%edx
    1ef4:	74 1b                	je     1f11 <_ZL14_free_unlockedPv+0x147>
    1ef6:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1ef9:	8b 52 10             	mov    0x10(%edx),%edx
    1efc:	c7 42 0c 00 00 00 00 	movl   $0x0,0xc(%edx)
    1f03:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1f06:	8b 52 10             	mov    0x10(%edx),%edx
    1f09:	89 90 d0 13 00 00    	mov    %edx,0x13d0(%eax)
    1f0f:	eb 14                	jmp    1f25 <_ZL14_free_unlockedPv+0x15b>
    1f11:	c7 80 cc 13 00 00 00 	movl   $0x0,0x13cc(%eax)
    1f18:	00 00 00 
    1f1b:	c7 80 d0 13 00 00 00 	movl   $0x0,0x13d0(%eax)
    1f22:	00 00 00 
    1f25:	8b 55 f4             	mov    -0xc(%ebp),%edx
    1f28:	8b 12                	mov    (%edx),%edx
    1f2a:	f7 da                	neg    %edx
    1f2c:	83 ec 08             	sub    $0x8,%esp
    1f2f:	52                   	push   %edx
    1f30:	6a 1c                	push   $0x1c
    1f32:	89 c3                	mov    %eax,%ebx
    1f34:	e8 27 1e 00 00       	call   3d60 <syscall@plt>
    1f39:	83 c4 10             	add    $0x10,%esp
    1f3c:	eb 01                	jmp    1f3f <_ZL14_free_unlockedPv+0x175>
    1f3e:	90                   	nop
    1f3f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1f42:	c9                   	leave
    1f43:	c3                   	ret

00001f44 <free>:
    1f44:	55                   	push   %ebp
    1f45:	89 e5                	mov    %esp,%ebp
    1f47:	53                   	push   %ebx
    1f48:	83 ec 04             	sub    $0x4,%esp
    1f4b:	e8 ab fb ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    1f50:	81 c3 ec 2c 00 00    	add    $0x2cec,%ebx
    1f56:	8d 83 c8 13 00 00    	lea    0x13c8(%ebx),%eax
    1f5c:	50                   	push   %eax
    1f5d:	e8 9d fb ff ff       	call   1aff <mutex_lock>
    1f62:	83 c4 04             	add    $0x4,%esp
    1f65:	83 ec 0c             	sub    $0xc,%esp
    1f68:	ff 75 08             	push   0x8(%ebp)
    1f6b:	e8 5a fe ff ff       	call   1dca <_ZL14_free_unlockedPv>
    1f70:	83 c4 10             	add    $0x10,%esp
    1f73:	83 ec 0c             	sub    $0xc,%esp
    1f76:	8d 83 c8 13 00 00    	lea    0x13c8(%ebx),%eax
    1f7c:	50                   	push   %eax
    1f7d:	e8 a0 fb ff ff       	call   1b22 <mutex_unlock>
    1f82:	83 c4 10             	add    $0x10,%esp
    1f85:	90                   	nop
    1f86:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1f89:	c9                   	leave
    1f8a:	c3                   	ret

00001f8b <calloc>:
    1f8b:	55                   	push   %ebp
    1f8c:	89 e5                	mov    %esp,%ebp
    1f8e:	53                   	push   %ebx
    1f8f:	83 ec 14             	sub    $0x14,%esp
    1f92:	e8 14 f1 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    1f97:	05 a5 2c 00 00       	add    $0x2ca5,%eax
    1f9c:	8b 55 08             	mov    0x8(%ebp),%edx
    1f9f:	0f af 55 0c          	imul   0xc(%ebp),%edx
    1fa3:	89 55 f0             	mov    %edx,-0x10(%ebp)
    1fa6:	83 ec 0c             	sub    $0xc,%esp
    1fa9:	ff 75 f0             	push   -0x10(%ebp)
    1fac:	89 c3                	mov    %eax,%ebx
    1fae:	e8 dd 1d 00 00       	call   3d90 <malloc@plt>
    1fb3:	83 c4 10             	add    $0x10,%esp
    1fb6:	89 45 ec             	mov    %eax,-0x14(%ebp)
    1fb9:	83 7d ec 00          	cmpl   $0x0,-0x14(%ebp)
    1fbd:	74 26                	je     1fe5 <calloc+0x5a>
    1fbf:	8b 45 ec             	mov    -0x14(%ebp),%eax
    1fc2:	89 45 e8             	mov    %eax,-0x18(%ebp)
    1fc5:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    1fcc:	eb 0f                	jmp    1fdd <calloc+0x52>
    1fce:	8b 55 e8             	mov    -0x18(%ebp),%edx
    1fd1:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1fd4:	01 d0                	add    %edx,%eax
    1fd6:	c6 00 00             	movb   $0x0,(%eax)
    1fd9:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    1fdd:	8b 45 f4             	mov    -0xc(%ebp),%eax
    1fe0:	3b 45 f0             	cmp    -0x10(%ebp),%eax
    1fe3:	72 e9                	jb     1fce <calloc+0x43>
    1fe5:	8b 45 ec             	mov    -0x14(%ebp),%eax
    1fe8:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    1feb:	c9                   	leave
    1fec:	c3                   	ret

00001fed <realloc>:
    1fed:	55                   	push   %ebp
    1fee:	89 e5                	mov    %esp,%ebp
    1ff0:	53                   	push   %ebx
    1ff1:	83 ec 24             	sub    $0x24,%esp
    1ff4:	e8 02 fb ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    1ff9:	81 c3 43 2c 00 00    	add    $0x2c43,%ebx
    1fff:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    2003:	75 13                	jne    2018 <realloc+0x2b>
    2005:	83 ec 0c             	sub    $0xc,%esp
    2008:	ff 75 0c             	push   0xc(%ebp)
    200b:	e8 80 1d 00 00       	call   3d90 <malloc@plt>
    2010:	83 c4 10             	add    $0x10,%esp
    2013:	e9 b8 00 00 00       	jmp    20d0 <realloc+0xe3>
    2018:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    201c:	75 18                	jne    2036 <realloc+0x49>
    201e:	83 ec 0c             	sub    $0xc,%esp
    2021:	ff 75 08             	push   0x8(%ebp)
    2024:	e8 17 1e 00 00       	call   3e40 <free@plt>
    2029:	83 c4 10             	add    $0x10,%esp
    202c:	b8 00 00 00 00       	mov    $0x0,%eax
    2031:	e9 9a 00 00 00       	jmp    20d0 <realloc+0xe3>
    2036:	8b 45 08             	mov    0x8(%ebp),%eax
    2039:	83 e8 14             	sub    $0x14,%eax
    203c:	89 45 f0             	mov    %eax,-0x10(%ebp)
    203f:	8b 45 f0             	mov    -0x10(%ebp),%eax
    2042:	8b 40 08             	mov    0x8(%eax),%eax
    2045:	3d de c0 ad de       	cmp    $0xdeadc0de,%eax
    204a:	74 07                	je     2053 <realloc+0x66>
    204c:	b8 00 00 00 00       	mov    $0x0,%eax
    2051:	eb 7d                	jmp    20d0 <realloc+0xe3>
    2053:	8b 45 f0             	mov    -0x10(%ebp),%eax
    2056:	8b 00                	mov    (%eax),%eax
    2058:	83 e8 14             	sub    $0x14,%eax
    205b:	89 45 ec             	mov    %eax,-0x14(%ebp)
    205e:	8b 45 0c             	mov    0xc(%ebp),%eax
    2061:	39 45 ec             	cmp    %eax,-0x14(%ebp)
    2064:	72 05                	jb     206b <realloc+0x7e>
    2066:	8b 45 08             	mov    0x8(%ebp),%eax
    2069:	eb 65                	jmp    20d0 <realloc+0xe3>
    206b:	83 ec 0c             	sub    $0xc,%esp
    206e:	ff 75 0c             	push   0xc(%ebp)
    2071:	e8 1a 1d 00 00       	call   3d90 <malloc@plt>
    2076:	83 c4 10             	add    $0x10,%esp
    2079:	89 45 e8             	mov    %eax,-0x18(%ebp)
    207c:	83 7d e8 00          	cmpl   $0x0,-0x18(%ebp)
    2080:	75 07                	jne    2089 <realloc+0x9c>
    2082:	b8 00 00 00 00       	mov    $0x0,%eax
    2087:	eb 47                	jmp    20d0 <realloc+0xe3>
    2089:	8b 45 e8             	mov    -0x18(%ebp),%eax
    208c:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    208f:	8b 45 08             	mov    0x8(%ebp),%eax
    2092:	89 45 e0             	mov    %eax,-0x20(%ebp)
    2095:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    209c:	eb 19                	jmp    20b7 <realloc+0xca>
    209e:	8b 55 e0             	mov    -0x20(%ebp),%edx
    20a1:	8b 45 f4             	mov    -0xc(%ebp),%eax
    20a4:	01 d0                	add    %edx,%eax
    20a6:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
    20a9:	8b 55 f4             	mov    -0xc(%ebp),%edx
    20ac:	01 ca                	add    %ecx,%edx
    20ae:	0f b6 00             	movzbl (%eax),%eax
    20b1:	88 02                	mov    %al,(%edx)
    20b3:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    20b7:	8b 45 f4             	mov    -0xc(%ebp),%eax
    20ba:	3b 45 ec             	cmp    -0x14(%ebp),%eax
    20bd:	72 df                	jb     209e <realloc+0xb1>
    20bf:	83 ec 0c             	sub    $0xc,%esp
    20c2:	ff 75 08             	push   0x8(%ebp)
    20c5:	e8 76 1d 00 00       	call   3e40 <free@plt>
    20ca:	83 c4 10             	add    $0x10,%esp
    20cd:	8b 45 e8             	mov    -0x18(%ebp),%eax
    20d0:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    20d3:	c9                   	leave
    20d4:	c3                   	ret

000020d5 <_Znwj>:
    20d5:	55                   	push   %ebp
    20d6:	89 e5                	mov    %esp,%ebp
    20d8:	53                   	push   %ebx
    20d9:	83 ec 04             	sub    $0x4,%esp
    20dc:	e8 ca ef ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    20e1:	05 5b 2b 00 00       	add    $0x2b5b,%eax
    20e6:	83 ec 0c             	sub    $0xc,%esp
    20e9:	ff 75 08             	push   0x8(%ebp)
    20ec:	89 c3                	mov    %eax,%ebx
    20ee:	e8 9d 1c 00 00       	call   3d90 <malloc@plt>
    20f3:	83 c4 10             	add    $0x10,%esp
    20f6:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    20f9:	c9                   	leave
    20fa:	c3                   	ret

000020fb <_Znaj>:
    20fb:	55                   	push   %ebp
    20fc:	89 e5                	mov    %esp,%ebp
    20fe:	53                   	push   %ebx
    20ff:	83 ec 04             	sub    $0x4,%esp
    2102:	e8 a4 ef ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    2107:	05 35 2b 00 00       	add    $0x2b35,%eax
    210c:	83 ec 0c             	sub    $0xc,%esp
    210f:	ff 75 08             	push   0x8(%ebp)
    2112:	89 c3                	mov    %eax,%ebx
    2114:	e8 77 1c 00 00       	call   3d90 <malloc@plt>
    2119:	83 c4 10             	add    $0x10,%esp
    211c:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    211f:	c9                   	leave
    2120:	c3                   	ret

00002121 <_ZdlPv>:
    2121:	55                   	push   %ebp
    2122:	89 e5                	mov    %esp,%ebp
    2124:	53                   	push   %ebx
    2125:	83 ec 04             	sub    $0x4,%esp
    2128:	e8 7e ef ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    212d:	05 0f 2b 00 00       	add    $0x2b0f,%eax
    2132:	83 ec 0c             	sub    $0xc,%esp
    2135:	ff 75 08             	push   0x8(%ebp)
    2138:	89 c3                	mov    %eax,%ebx
    213a:	e8 01 1d 00 00       	call   3e40 <free@plt>
    213f:	83 c4 10             	add    $0x10,%esp
    2142:	90                   	nop
    2143:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2146:	c9                   	leave
    2147:	c3                   	ret

00002148 <_ZdaPv>:
    2148:	55                   	push   %ebp
    2149:	89 e5                	mov    %esp,%ebp
    214b:	53                   	push   %ebx
    214c:	83 ec 04             	sub    $0x4,%esp
    214f:	e8 57 ef ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    2154:	05 e8 2a 00 00       	add    $0x2ae8,%eax
    2159:	83 ec 0c             	sub    $0xc,%esp
    215c:	ff 75 08             	push   0x8(%ebp)
    215f:	89 c3                	mov    %eax,%ebx
    2161:	e8 da 1c 00 00       	call   3e40 <free@plt>
    2166:	83 c4 10             	add    $0x10,%esp
    2169:	90                   	nop
    216a:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    216d:	c9                   	leave
    216e:	c3                   	ret

0000216f <_ZdlPvj>:
    216f:	55                   	push   %ebp
    2170:	89 e5                	mov    %esp,%ebp
    2172:	53                   	push   %ebx
    2173:	83 ec 04             	sub    $0x4,%esp
    2176:	e8 30 ef ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    217b:	05 c1 2a 00 00       	add    $0x2ac1,%eax
    2180:	83 ec 0c             	sub    $0xc,%esp
    2183:	ff 75 08             	push   0x8(%ebp)
    2186:	89 c3                	mov    %eax,%ebx
    2188:	e8 b3 1c 00 00       	call   3e40 <free@plt>
    218d:	83 c4 10             	add    $0x10,%esp
    2190:	90                   	nop
    2191:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2194:	c9                   	leave
    2195:	c3                   	ret

00002196 <_ZdaPvj>:
    2196:	55                   	push   %ebp
    2197:	89 e5                	mov    %esp,%ebp
    2199:	53                   	push   %ebx
    219a:	83 ec 04             	sub    $0x4,%esp
    219d:	e8 09 ef ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    21a2:	05 9a 2a 00 00       	add    $0x2a9a,%eax
    21a7:	83 ec 0c             	sub    $0xc,%esp
    21aa:	ff 75 08             	push   0x8(%ebp)
    21ad:	89 c3                	mov    %eax,%ebx
    21af:	e8 8c 1c 00 00       	call   3e40 <free@plt>
    21b4:	83 c4 10             	add    $0x10,%esp
    21b7:	90                   	nop
    21b8:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    21bb:	c9                   	leave
    21bc:	c3                   	ret

000021bd <mutex_lock>:
    21bd:	55                   	push   %ebp
    21be:	89 e5                	mov    %esp,%ebp
    21c0:	e8 e6 ee ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    21c5:	05 77 2a 00 00       	add    $0x2a77,%eax
    21ca:	eb 02                	jmp    21ce <mutex_lock+0x11>
    21cc:	f3 90                	pause
    21ce:	8b 55 08             	mov    0x8(%ebp),%edx
    21d1:	b8 01 00 00 00       	mov    $0x1,%eax
    21d6:	86 02                	xchg   %al,(%edx)
    21d8:	84 c0                	test   %al,%al
    21da:	75 f0                	jne    21cc <mutex_lock+0xf>
    21dc:	90                   	nop
    21dd:	90                   	nop
    21de:	5d                   	pop    %ebp
    21df:	c3                   	ret

000021e0 <mutex_unlock>:
    21e0:	55                   	push   %ebp
    21e1:	89 e5                	mov    %esp,%ebp
    21e3:	e8 c3 ee ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    21e8:	05 54 2a 00 00       	add    $0x2a54,%eax
    21ed:	8b 45 08             	mov    0x8(%ebp),%eax
    21f0:	ba 00 00 00 00       	mov    $0x0,%edx
    21f5:	88 10                	mov    %dl,(%eax)
    21f7:	90                   	nop
    21f8:	5d                   	pop    %ebp
    21f9:	c3                   	ret

000021fa <_ZL23_fflush_stdout_unlockedv>:
    21fa:	55                   	push   %ebp
    21fb:	89 e5                	mov    %esp,%ebp
    21fd:	53                   	push   %ebx
    21fe:	83 ec 04             	sub    $0x4,%esp
    2201:	e8 f5 f8 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    2206:	81 c3 36 2a 00 00    	add    $0x2a36,%ebx
    220c:	8b 83 e4 17 00 00    	mov    0x17e4(%ebx),%eax
    2212:	85 c0                	test   %eax,%eax
    2214:	74 27                	je     223d <_ZL23_fflush_stdout_unlockedv+0x43>
    2216:	8b 83 e4 17 00 00    	mov    0x17e4(%ebx),%eax
    221c:	89 c2                	mov    %eax,%edx
    221e:	8d 83 e4 13 00 00    	lea    0x13e4(%ebx),%eax
    2224:	83 ec 04             	sub    $0x4,%esp
    2227:	52                   	push   %edx
    2228:	50                   	push   %eax
    2229:	6a 01                	push   $0x1
    222b:	e8 30 1b 00 00       	call   3d60 <syscall@plt>
    2230:	83 c4 10             	add    $0x10,%esp
    2233:	c7 83 e4 17 00 00 00 	movl   $0x0,0x17e4(%ebx)
    223a:	00 00 00 
    223d:	90                   	nop
    223e:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2241:	c9                   	leave
    2242:	c3                   	ret

00002243 <_ZL17_putchar_unlockedi>:
    2243:	55                   	push   %ebp
    2244:	89 e5                	mov    %esp,%ebp
    2246:	53                   	push   %ebx
    2247:	83 ec 04             	sub    $0x4,%esp
    224a:	e8 ac f8 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    224f:	81 c3 ed 29 00 00    	add    $0x29ed,%ebx
    2255:	8b 83 e4 17 00 00    	mov    0x17e4(%ebx),%eax
    225b:	3d ff 03 00 00       	cmp    $0x3ff,%eax
    2260:	76 05                	jbe    2267 <_ZL17_putchar_unlockedi+0x24>
    2262:	e8 93 ff ff ff       	call   21fa <_ZL23_fflush_stdout_unlockedv>
    2267:	8b 45 08             	mov    0x8(%ebp),%eax
    226a:	89 c1                	mov    %eax,%ecx
    226c:	8b 83 e4 17 00 00    	mov    0x17e4(%ebx),%eax
    2272:	8d 50 01             	lea    0x1(%eax),%edx
    2275:	89 93 e4 17 00 00    	mov    %edx,0x17e4(%ebx)
    227b:	88 8c 03 e4 13 00 00 	mov    %cl,0x13e4(%ebx,%eax,1)
    2282:	83 7d 08 0a          	cmpl   $0xa,0x8(%ebp)
    2286:	75 05                	jne    228d <_ZL17_putchar_unlockedi+0x4a>
    2288:	e8 6d ff ff ff       	call   21fa <_ZL23_fflush_stdout_unlockedv>
    228d:	8b 45 08             	mov    0x8(%ebp),%eax
    2290:	0f b6 c0             	movzbl %al,%eax
    2293:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2296:	c9                   	leave
    2297:	c3                   	ret

00002298 <putchar>:
    2298:	55                   	push   %ebp
    2299:	89 e5                	mov    %esp,%ebp
    229b:	53                   	push   %ebx
    229c:	83 ec 14             	sub    $0x14,%esp
    229f:	e8 57 f8 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    22a4:	81 c3 98 29 00 00    	add    $0x2998,%ebx
    22aa:	8d 83 e8 17 00 00    	lea    0x17e8(%ebx),%eax
    22b0:	50                   	push   %eax
    22b1:	e8 07 ff ff ff       	call   21bd <mutex_lock>
    22b6:	83 c4 04             	add    $0x4,%esp
    22b9:	83 ec 0c             	sub    $0xc,%esp
    22bc:	ff 75 08             	push   0x8(%ebp)
    22bf:	e8 7f ff ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    22c4:	83 c4 10             	add    $0x10,%esp
    22c7:	89 45 f4             	mov    %eax,-0xc(%ebp)
    22ca:	83 ec 0c             	sub    $0xc,%esp
    22cd:	8d 83 e8 17 00 00    	lea    0x17e8(%ebx),%eax
    22d3:	50                   	push   %eax
    22d4:	e8 07 ff ff ff       	call   21e0 <mutex_unlock>
    22d9:	83 c4 10             	add    $0x10,%esp
    22dc:	8b 45 f4             	mov    -0xc(%ebp),%eax
    22df:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    22e2:	c9                   	leave
    22e3:	c3                   	ret

000022e4 <puts>:
    22e4:	55                   	push   %ebp
    22e5:	89 e5                	mov    %esp,%ebp
    22e7:	53                   	push   %ebx
    22e8:	83 ec 14             	sub    $0x14,%esp
    22eb:	e8 0b f8 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    22f0:	81 c3 4c 29 00 00    	add    $0x294c,%ebx
    22f6:	8d 83 e8 17 00 00    	lea    0x17e8(%ebx),%eax
    22fc:	50                   	push   %eax
    22fd:	e8 bb fe ff ff       	call   21bd <mutex_lock>
    2302:	83 c4 04             	add    $0x4,%esp
    2305:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    230c:	eb 1f                	jmp    232d <puts+0x49>
    230e:	8b 45 08             	mov    0x8(%ebp),%eax
    2311:	8d 50 01             	lea    0x1(%eax),%edx
    2314:	89 55 08             	mov    %edx,0x8(%ebp)
    2317:	0f b6 00             	movzbl (%eax),%eax
    231a:	0f be c0             	movsbl %al,%eax
    231d:	83 ec 0c             	sub    $0xc,%esp
    2320:	50                   	push   %eax
    2321:	e8 1d ff ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    2326:	83 c4 10             	add    $0x10,%esp
    2329:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    232d:	8b 45 08             	mov    0x8(%ebp),%eax
    2330:	0f b6 00             	movzbl (%eax),%eax
    2333:	84 c0                	test   %al,%al
    2335:	75 d7                	jne    230e <puts+0x2a>
    2337:	83 ec 0c             	sub    $0xc,%esp
    233a:	6a 0a                	push   $0xa
    233c:	e8 02 ff ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    2341:	83 c4 10             	add    $0x10,%esp
    2344:	83 ec 0c             	sub    $0xc,%esp
    2347:	8d 83 e8 17 00 00    	lea    0x17e8(%ebx),%eax
    234d:	50                   	push   %eax
    234e:	e8 8d fe ff ff       	call   21e0 <mutex_unlock>
    2353:	83 c4 10             	add    $0x10,%esp
    2356:	8b 45 f4             	mov    -0xc(%ebp),%eax
    2359:	83 c0 01             	add    $0x1,%eax
    235c:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    235f:	c9                   	leave
    2360:	c3                   	ret

00002361 <_ZL15print_uint_corejiibbicb>:
    2361:	55                   	push   %ebp
    2362:	89 e5                	mov    %esp,%ebp
    2364:	53                   	push   %ebx
    2365:	83 ec 54             	sub    $0x54,%esp
    2368:	e8 3e ed ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    236d:	05 cf 28 00 00       	add    $0x28cf,%eax
    2372:	8b 5d 14             	mov    0x14(%ebp),%ebx
    2375:	8b 4d 18             	mov    0x18(%ebp),%ecx
    2378:	8b 55 20             	mov    0x20(%ebp),%edx
    237b:	8b 45 24             	mov    0x24(%ebp),%eax
    237e:	88 5d b4             	mov    %bl,-0x4c(%ebp)
    2381:	88 4d b0             	mov    %cl,-0x50(%ebp)
    2384:	88 55 ac             	mov    %dl,-0x54(%ebp)
    2387:	88 45 a8             	mov    %al,-0x58(%ebp)
    238a:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    2391:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    2395:	75 76                	jne    240d <_ZL15print_uint_corejiibbicb+0xac>
    2397:	8b 45 f4             	mov    -0xc(%ebp),%eax
    239a:	8d 50 01             	lea    0x1(%eax),%edx
    239d:	89 55 f4             	mov    %edx,-0xc(%ebp)
    23a0:	c6 44 05 b8 30       	movb   $0x30,-0x48(%ebp,%eax,1)
    23a5:	eb 6c                	jmp    2413 <_ZL15print_uint_corejiibbicb+0xb2>
    23a7:	8b 4d 0c             	mov    0xc(%ebp),%ecx
    23aa:	8b 45 08             	mov    0x8(%ebp),%eax
    23ad:	ba 00 00 00 00       	mov    $0x0,%edx
    23b2:	f7 f1                	div    %ecx
    23b4:	89 55 dc             	mov    %edx,-0x24(%ebp)
    23b7:	83 7d dc 09          	cmpl   $0x9,-0x24(%ebp)
    23bb:	77 17                	ja     23d4 <_ZL15print_uint_corejiibbicb+0x73>
    23bd:	8b 45 dc             	mov    -0x24(%ebp),%eax
    23c0:	83 c0 30             	add    $0x30,%eax
    23c3:	89 c1                	mov    %eax,%ecx
    23c5:	8b 45 f4             	mov    -0xc(%ebp),%eax
    23c8:	8d 50 01             	lea    0x1(%eax),%edx
    23cb:	89 55 f4             	mov    %edx,-0xc(%ebp)
    23ce:	88 4c 05 b8          	mov    %cl,-0x48(%ebp,%eax,1)
    23d2:	eb 29                	jmp    23fd <_ZL15print_uint_corejiibbicb+0x9c>
    23d4:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    23d8:	74 07                	je     23e1 <_ZL15print_uint_corejiibbicb+0x80>
    23da:	ba 41 00 00 00       	mov    $0x41,%edx
    23df:	eb 05                	jmp    23e6 <_ZL15print_uint_corejiibbicb+0x85>
    23e1:	ba 61 00 00 00       	mov    $0x61,%edx
    23e6:	8b 45 dc             	mov    -0x24(%ebp),%eax
    23e9:	01 d0                	add    %edx,%eax
    23eb:	83 e8 0a             	sub    $0xa,%eax
    23ee:	89 c1                	mov    %eax,%ecx
    23f0:	8b 45 f4             	mov    -0xc(%ebp),%eax
    23f3:	8d 50 01             	lea    0x1(%eax),%edx
    23f6:	89 55 f4             	mov    %edx,-0xc(%ebp)
    23f9:	88 4c 05 b8          	mov    %cl,-0x48(%ebp,%eax,1)
    23fd:	8b 5d 0c             	mov    0xc(%ebp),%ebx
    2400:	8b 45 08             	mov    0x8(%ebp),%eax
    2403:	ba 00 00 00 00       	mov    $0x0,%edx
    2408:	f7 f3                	div    %ebx
    240a:	89 45 08             	mov    %eax,0x8(%ebp)
    240d:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    2411:	75 94                	jne    23a7 <_ZL15print_uint_corejiibbicb+0x46>
    2413:	80 7d b4 00          	cmpb   $0x0,-0x4c(%ebp)
    2417:	74 0d                	je     2426 <_ZL15print_uint_corejiibbicb+0xc5>
    2419:	80 7d b0 00          	cmpb   $0x0,-0x50(%ebp)
    241d:	74 07                	je     2426 <_ZL15print_uint_corejiibbicb+0xc5>
    241f:	b8 01 00 00 00       	mov    $0x1,%eax
    2424:	eb 05                	jmp    242b <_ZL15print_uint_corejiibbicb+0xca>
    2426:	b8 00 00 00 00       	mov    $0x0,%eax
    242b:	89 45 d8             	mov    %eax,-0x28(%ebp)
    242e:	8b 45 1c             	mov    0x1c(%ebp),%eax
    2431:	2b 45 f4             	sub    -0xc(%ebp),%eax
    2434:	2b 45 d8             	sub    -0x28(%ebp),%eax
    2437:	89 45 f0             	mov    %eax,-0x10(%ebp)
    243a:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
    243e:	79 07                	jns    2447 <_ZL15print_uint_corejiibbicb+0xe6>
    2440:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
    2447:	0f b6 45 a8          	movzbl -0x58(%ebp),%eax
    244b:	83 f0 01             	xor    $0x1,%eax
    244e:	84 c0                	test   %al,%al
    2450:	74 28                	je     247a <_ZL15print_uint_corejiibbicb+0x119>
    2452:	80 7d ac 20          	cmpb   $0x20,-0x54(%ebp)
    2456:	75 22                	jne    247a <_ZL15print_uint_corejiibbicb+0x119>
    2458:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
    245f:	eb 11                	jmp    2472 <_ZL15print_uint_corejiibbicb+0x111>
    2461:	83 ec 0c             	sub    $0xc,%esp
    2464:	6a 20                	push   $0x20
    2466:	e8 d8 fd ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    246b:	83 c4 10             	add    $0x10,%esp
    246e:	83 45 ec 01          	addl   $0x1,-0x14(%ebp)
    2472:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2475:	3b 45 f0             	cmp    -0x10(%ebp),%eax
    2478:	7c e7                	jl     2461 <_ZL15print_uint_corejiibbicb+0x100>
    247a:	80 7d b4 00          	cmpb   $0x0,-0x4c(%ebp)
    247e:	74 13                	je     2493 <_ZL15print_uint_corejiibbicb+0x132>
    2480:	80 7d b0 00          	cmpb   $0x0,-0x50(%ebp)
    2484:	74 0d                	je     2493 <_ZL15print_uint_corejiibbicb+0x132>
    2486:	83 ec 0c             	sub    $0xc,%esp
    2489:	6a 2d                	push   $0x2d
    248b:	e8 b3 fd ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    2490:	83 c4 10             	add    $0x10,%esp
    2493:	0f b6 45 a8          	movzbl -0x58(%ebp),%eax
    2497:	83 f0 01             	xor    $0x1,%eax
    249a:	84 c0                	test   %al,%al
    249c:	74 28                	je     24c6 <_ZL15print_uint_corejiibbicb+0x165>
    249e:	80 7d ac 30          	cmpb   $0x30,-0x54(%ebp)
    24a2:	75 22                	jne    24c6 <_ZL15print_uint_corejiibbicb+0x165>
    24a4:	c7 45 e8 00 00 00 00 	movl   $0x0,-0x18(%ebp)
    24ab:	eb 11                	jmp    24be <_ZL15print_uint_corejiibbicb+0x15d>
    24ad:	83 ec 0c             	sub    $0xc,%esp
    24b0:	6a 30                	push   $0x30
    24b2:	e8 8c fd ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    24b7:	83 c4 10             	add    $0x10,%esp
    24ba:	83 45 e8 01          	addl   $0x1,-0x18(%ebp)
    24be:	8b 45 e8             	mov    -0x18(%ebp),%eax
    24c1:	3b 45 f0             	cmp    -0x10(%ebp),%eax
    24c4:	7c e7                	jl     24ad <_ZL15print_uint_corejiibbicb+0x14c>
    24c6:	8b 45 f4             	mov    -0xc(%ebp),%eax
    24c9:	83 e8 01             	sub    $0x1,%eax
    24cc:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    24cf:	eb 1e                	jmp    24ef <_ZL15print_uint_corejiibbicb+0x18e>
    24d1:	8d 55 b8             	lea    -0x48(%ebp),%edx
    24d4:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    24d7:	01 d0                	add    %edx,%eax
    24d9:	0f b6 00             	movzbl (%eax),%eax
    24dc:	0f be c0             	movsbl %al,%eax
    24df:	83 ec 0c             	sub    $0xc,%esp
    24e2:	50                   	push   %eax
    24e3:	e8 5b fd ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    24e8:	83 c4 10             	add    $0x10,%esp
    24eb:	83 6d e4 01          	subl   $0x1,-0x1c(%ebp)
    24ef:	83 7d e4 00          	cmpl   $0x0,-0x1c(%ebp)
    24f3:	79 dc                	jns    24d1 <_ZL15print_uint_corejiibbicb+0x170>
    24f5:	80 7d a8 00          	cmpb   $0x0,-0x58(%ebp)
    24f9:	74 22                	je     251d <_ZL15print_uint_corejiibbicb+0x1bc>
    24fb:	c7 45 e0 00 00 00 00 	movl   $0x0,-0x20(%ebp)
    2502:	eb 11                	jmp    2515 <_ZL15print_uint_corejiibbicb+0x1b4>
    2504:	83 ec 0c             	sub    $0xc,%esp
    2507:	6a 20                	push   $0x20
    2509:	e8 35 fd ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    250e:	83 c4 10             	add    $0x10,%esp
    2511:	83 45 e0 01          	addl   $0x1,-0x20(%ebp)
    2515:	8b 45 e0             	mov    -0x20(%ebp),%eax
    2518:	3b 45 f0             	cmp    -0x10(%ebp),%eax
    251b:	7c e7                	jl     2504 <_ZL15print_uint_corejiibbicb+0x1a3>
    251d:	90                   	nop
    251e:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2521:	c9                   	leave
    2522:	c3                   	ret

00002523 <_ZL9print_intiicb>:
    2523:	55                   	push   %ebp
    2524:	89 e5                	mov    %esp,%ebp
    2526:	83 ec 28             	sub    $0x28,%esp
    2529:	e8 7d eb ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    252e:	05 0e 27 00 00       	add    $0x270e,%eax
    2533:	8b 55 10             	mov    0x10(%ebp),%edx
    2536:	8b 45 14             	mov    0x14(%ebp),%eax
    2539:	88 55 e4             	mov    %dl,-0x1c(%ebp)
    253c:	88 45 e0             	mov    %al,-0x20(%ebp)
    253f:	8b 45 08             	mov    0x8(%ebp),%eax
    2542:	c1 e8 1f             	shr    $0x1f,%eax
    2545:	88 45 f7             	mov    %al,-0x9(%ebp)
    2548:	80 7d f7 00          	cmpb   $0x0,-0x9(%ebp)
    254c:	74 07                	je     2555 <_ZL9print_intiicb+0x32>
    254e:	8b 45 08             	mov    0x8(%ebp),%eax
    2551:	f7 d8                	neg    %eax
    2553:	eb 03                	jmp    2558 <_ZL9print_intiicb+0x35>
    2555:	8b 45 08             	mov    0x8(%ebp),%eax
    2558:	89 45 f0             	mov    %eax,-0x10(%ebp)
    255b:	0f b6 4d e0          	movzbl -0x20(%ebp),%ecx
    255f:	0f be 55 e4          	movsbl -0x1c(%ebp),%edx
    2563:	0f b6 45 f7          	movzbl -0x9(%ebp),%eax
    2567:	51                   	push   %ecx
    2568:	52                   	push   %edx
    2569:	ff 75 0c             	push   0xc(%ebp)
    256c:	50                   	push   %eax
    256d:	6a 01                	push   $0x1
    256f:	6a 00                	push   $0x0
    2571:	6a 0a                	push   $0xa
    2573:	ff 75 f0             	push   -0x10(%ebp)
    2576:	e8 e6 fd ff ff       	call   2361 <_ZL15print_uint_corejiibbicb>
    257b:	83 c4 20             	add    $0x20,%esp
    257e:	90                   	nop
    257f:	c9                   	leave
    2580:	c3                   	ret

00002581 <_ZL9print_hexjiicb>:
    2581:	55                   	push   %ebp
    2582:	89 e5                	mov    %esp,%ebp
    2584:	83 ec 18             	sub    $0x18,%esp
    2587:	e8 1f eb ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    258c:	05 b0 26 00 00       	add    $0x26b0,%eax
    2591:	8b 55 14             	mov    0x14(%ebp),%edx
    2594:	8b 45 18             	mov    0x18(%ebp),%eax
    2597:	88 55 f4             	mov    %dl,-0xc(%ebp)
    259a:	88 45 f0             	mov    %al,-0x10(%ebp)
    259d:	0f b6 55 f0          	movzbl -0x10(%ebp),%edx
    25a1:	0f be 45 f4          	movsbl -0xc(%ebp),%eax
    25a5:	52                   	push   %edx
    25a6:	50                   	push   %eax
    25a7:	ff 75 10             	push   0x10(%ebp)
    25aa:	6a 00                	push   $0x0
    25ac:	6a 00                	push   $0x0
    25ae:	ff 75 0c             	push   0xc(%ebp)
    25b1:	6a 10                	push   $0x10
    25b3:	ff 75 08             	push   0x8(%ebp)
    25b6:	e8 a6 fd ff ff       	call   2361 <_ZL15print_uint_corejiibbicb>
    25bb:	83 c4 20             	add    $0x20,%esp
    25be:	90                   	nop
    25bf:	c9                   	leave
    25c0:	c3                   	ret

000025c1 <_ZL9print_ptrPv>:
    25c1:	55                   	push   %ebp
    25c2:	89 e5                	mov    %esp,%ebp
    25c4:	83 ec 28             	sub    $0x28,%esp
    25c7:	e8 df ea ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    25cc:	05 70 26 00 00       	add    $0x2670,%eax
    25d1:	83 ec 0c             	sub    $0xc,%esp
    25d4:	6a 30                	push   $0x30
    25d6:	e8 68 fc ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    25db:	83 c4 10             	add    $0x10,%esp
    25de:	83 ec 0c             	sub    $0xc,%esp
    25e1:	6a 78                	push   $0x78
    25e3:	e8 5b fc ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    25e8:	83 c4 10             	add    $0x10,%esp
    25eb:	8b 45 08             	mov    0x8(%ebp),%eax
    25ee:	89 45 ec             	mov    %eax,-0x14(%ebp)
    25f1:	83 7d ec 00          	cmpl   $0x0,-0x14(%ebp)
    25f5:	75 12                	jne    2609 <_ZL9print_ptrPv+0x48>
    25f7:	83 ec 0c             	sub    $0xc,%esp
    25fa:	6a 30                	push   $0x30
    25fc:	e8 42 fc ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    2601:	83 c4 10             	add    $0x10,%esp
    2604:	e9 ad 00 00 00       	jmp    26b6 <_ZL9print_ptrPv+0xf5>
    2609:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    2610:	eb 4a                	jmp    265c <_ZL9print_ptrPv+0x9b>
    2612:	b8 07 00 00 00       	mov    $0x7,%eax
    2617:	2b 45 f4             	sub    -0xc(%ebp),%eax
    261a:	c1 e0 02             	shl    $0x2,%eax
    261d:	8b 55 ec             	mov    -0x14(%ebp),%edx
    2620:	89 c1                	mov    %eax,%ecx
    2622:	d3 ea                	shr    %cl,%edx
    2624:	89 d0                	mov    %edx,%eax
    2626:	83 e0 0f             	and    $0xf,%eax
    2629:	89 45 e8             	mov    %eax,-0x18(%ebp)
    262c:	83 7d e8 09          	cmpl   $0x9,-0x18(%ebp)
    2630:	7f 14                	jg     2646 <_ZL9print_ptrPv+0x85>
    2632:	8b 45 e8             	mov    -0x18(%ebp),%eax
    2635:	83 c0 30             	add    $0x30,%eax
    2638:	89 c1                	mov    %eax,%ecx
    263a:	8d 55 df             	lea    -0x21(%ebp),%edx
    263d:	8b 45 f4             	mov    -0xc(%ebp),%eax
    2640:	01 d0                	add    %edx,%eax
    2642:	88 08                	mov    %cl,(%eax)
    2644:	eb 12                	jmp    2658 <_ZL9print_ptrPv+0x97>
    2646:	8b 45 e8             	mov    -0x18(%ebp),%eax
    2649:	83 c0 57             	add    $0x57,%eax
    264c:	89 c1                	mov    %eax,%ecx
    264e:	8d 55 df             	lea    -0x21(%ebp),%edx
    2651:	8b 45 f4             	mov    -0xc(%ebp),%eax
    2654:	01 d0                	add    %edx,%eax
    2656:	88 08                	mov    %cl,(%eax)
    2658:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    265c:	83 7d f4 07          	cmpl   $0x7,-0xc(%ebp)
    2660:	7e b0                	jle    2612 <_ZL9print_ptrPv+0x51>
    2662:	c6 45 e7 00          	movb   $0x0,-0x19(%ebp)
    2666:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
    266d:	eb 04                	jmp    2673 <_ZL9print_ptrPv+0xb2>
    266f:	83 45 f0 01          	addl   $0x1,-0x10(%ebp)
    2673:	8d 55 df             	lea    -0x21(%ebp),%edx
    2676:	8b 45 f0             	mov    -0x10(%ebp),%eax
    2679:	01 d0                	add    %edx,%eax
    267b:	0f b6 00             	movzbl (%eax),%eax
    267e:	3c 30                	cmp    $0x30,%al
    2680:	75 25                	jne    26a7 <_ZL9print_ptrPv+0xe6>
    2682:	83 7d f0 06          	cmpl   $0x6,-0x10(%ebp)
    2686:	7e e7                	jle    266f <_ZL9print_ptrPv+0xae>
    2688:	eb 1d                	jmp    26a7 <_ZL9print_ptrPv+0xe6>
    268a:	8b 45 f0             	mov    -0x10(%ebp),%eax
    268d:	8d 50 01             	lea    0x1(%eax),%edx
    2690:	89 55 f0             	mov    %edx,-0x10(%ebp)
    2693:	0f b6 44 05 df       	movzbl -0x21(%ebp,%eax,1),%eax
    2698:	0f be c0             	movsbl %al,%eax
    269b:	83 ec 0c             	sub    $0xc,%esp
    269e:	50                   	push   %eax
    269f:	e8 9f fb ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    26a4:	83 c4 10             	add    $0x10,%esp
    26a7:	8d 55 df             	lea    -0x21(%ebp),%edx
    26aa:	8b 45 f0             	mov    -0x10(%ebp),%eax
    26ad:	01 d0                	add    %edx,%eax
    26af:	0f b6 00             	movzbl (%eax),%eax
    26b2:	84 c0                	test   %al,%al
    26b4:	75 d4                	jne    268a <_ZL9print_ptrPv+0xc9>
    26b6:	c9                   	leave
    26b7:	c3                   	ret

000026b8 <_ZL11print_floatdi>:
    26b8:	55                   	push   %ebp
    26b9:	89 e5                	mov    %esp,%ebp
    26bb:	53                   	push   %ebx
    26bc:	83 ec 54             	sub    $0x54,%esp
    26bf:	e8 37 f4 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    26c4:	81 c3 78 25 00 00    	add    $0x2578,%ebx
    26ca:	8b 45 08             	mov    0x8(%ebp),%eax
    26cd:	89 45 c0             	mov    %eax,-0x40(%ebp)
    26d0:	8b 45 0c             	mov    0xc(%ebp),%eax
    26d3:	89 45 c4             	mov    %eax,-0x3c(%ebp)
    26d6:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    26da:	7f 07                	jg     26e3 <_ZL11print_floatdi+0x2b>
    26dc:	c7 45 10 06 00 00 00 	movl   $0x6,0x10(%ebp)
    26e3:	dd 45 c0             	fldl   -0x40(%ebp)
    26e6:	d9 ee                	fldz
    26e8:	df f1                	fcomip %st(1),%st
    26ea:	dd d8                	fstp   %st(0)
    26ec:	76 15                	jbe    2703 <_ZL11print_floatdi+0x4b>
    26ee:	83 ec 0c             	sub    $0xc,%esp
    26f1:	6a 2d                	push   $0x2d
    26f3:	e8 4b fb ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    26f8:	83 c4 10             	add    $0x10,%esp
    26fb:	dd 45 c0             	fldl   -0x40(%ebp)
    26fe:	d9 e0                	fchs
    2700:	dd 5d c0             	fstpl  -0x40(%ebp)
    2703:	dd 45 c0             	fldl   -0x40(%ebp)
    2706:	d9 7d be             	fnstcw -0x42(%ebp)
    2709:	0f b7 45 be          	movzwl -0x42(%ebp),%eax
    270d:	80 cc 0c             	or     $0xc,%ah
    2710:	66 89 45 bc          	mov    %ax,-0x44(%ebp)
    2714:	d9 6d bc             	fldcw  -0x44(%ebp)
    2717:	df 7d b0             	fistpll -0x50(%ebp)
    271a:	d9 6d be             	fldcw  -0x42(%ebp)
    271d:	8b 45 b0             	mov    -0x50(%ebp),%eax
    2720:	89 45 f4             	mov    %eax,-0xc(%ebp)
    2723:	8b 45 f4             	mov    -0xc(%ebp),%eax
    2726:	ba 00 00 00 00       	mov    $0x0,%edx
    272b:	89 45 b0             	mov    %eax,-0x50(%ebp)
    272e:	89 55 b4             	mov    %edx,-0x4c(%ebp)
    2731:	df 6d b0             	fildll -0x50(%ebp)
    2734:	dd 45 c0             	fldl   -0x40(%ebp)
    2737:	de e1                	fsubp  %st,%st(1)
    2739:	dd 5d e8             	fstpl  -0x18(%ebp)
    273c:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    2740:	75 12                	jne    2754 <_ZL11print_floatdi+0x9c>
    2742:	83 ec 0c             	sub    $0xc,%esp
    2745:	6a 30                	push   $0x30
    2747:	e8 f7 fa ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    274c:	83 c4 10             	add    $0x10,%esp
    274f:	e9 8b 00 00 00       	jmp    27df <_ZL11print_floatdi+0x127>
    2754:	c7 45 e4 0b 00 00 00 	movl   $0xb,-0x1c(%ebp)
    275b:	8d 55 d0             	lea    -0x30(%ebp),%edx
    275e:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    2761:	01 d0                	add    %edx,%eax
    2763:	c6 00 00             	movb   $0x0,(%eax)
    2766:	eb 43                	jmp    27ab <_ZL11print_floatdi+0xf3>
    2768:	83 6d e4 01          	subl   $0x1,-0x1c(%ebp)
    276c:	8b 4d f4             	mov    -0xc(%ebp),%ecx
    276f:	ba cd cc cc cc       	mov    $0xcccccccd,%edx
    2774:	89 c8                	mov    %ecx,%eax
    2776:	f7 e2                	mul    %edx
    2778:	c1 ea 03             	shr    $0x3,%edx
    277b:	89 d0                	mov    %edx,%eax
    277d:	c1 e0 02             	shl    $0x2,%eax
    2780:	01 d0                	add    %edx,%eax
    2782:	01 c0                	add    %eax,%eax
    2784:	29 c1                	sub    %eax,%ecx
    2786:	89 ca                	mov    %ecx,%edx
    2788:	89 d0                	mov    %edx,%eax
    278a:	83 c0 30             	add    $0x30,%eax
    278d:	89 c1                	mov    %eax,%ecx
    278f:	8d 55 d0             	lea    -0x30(%ebp),%edx
    2792:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    2795:	01 d0                	add    %edx,%eax
    2797:	88 08                	mov    %cl,(%eax)
    2799:	8b 45 f4             	mov    -0xc(%ebp),%eax
    279c:	ba cd cc cc cc       	mov    $0xcccccccd,%edx
    27a1:	f7 e2                	mul    %edx
    27a3:	89 d0                	mov    %edx,%eax
    27a5:	c1 e8 03             	shr    $0x3,%eax
    27a8:	89 45 f4             	mov    %eax,-0xc(%ebp)
    27ab:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    27af:	75 b7                	jne    2768 <_ZL11print_floatdi+0xb0>
    27b1:	eb 1d                	jmp    27d0 <_ZL11print_floatdi+0x118>
    27b3:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    27b6:	8d 50 01             	lea    0x1(%eax),%edx
    27b9:	89 55 e4             	mov    %edx,-0x1c(%ebp)
    27bc:	0f b6 44 05 d0       	movzbl -0x30(%ebp,%eax,1),%eax
    27c1:	0f be c0             	movsbl %al,%eax
    27c4:	83 ec 0c             	sub    $0xc,%esp
    27c7:	50                   	push   %eax
    27c8:	e8 76 fa ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    27cd:	83 c4 10             	add    $0x10,%esp
    27d0:	8d 55 d0             	lea    -0x30(%ebp),%edx
    27d3:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    27d6:	01 d0                	add    %edx,%eax
    27d8:	0f b6 00             	movzbl (%eax),%eax
    27db:	84 c0                	test   %al,%al
    27dd:	75 d4                	jne    27b3 <_ZL11print_floatdi+0xfb>
    27df:	83 ec 0c             	sub    $0xc,%esp
    27e2:	6a 2e                	push   $0x2e
    27e4:	e8 5a fa ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    27e9:	83 c4 10             	add    $0x10,%esp
    27ec:	c7 45 e0 00 00 00 00 	movl   $0x0,-0x20(%ebp)
    27f3:	eb 49                	jmp    283e <_ZL11print_floatdi+0x186>
    27f5:	dd 45 e8             	fldl   -0x18(%ebp)
    27f8:	dd 83 a4 f2 ff ff    	fldl   -0xd5c(%ebx)
    27fe:	de c9                	fmulp  %st,%st(1)
    2800:	dd 5d e8             	fstpl  -0x18(%ebp)
    2803:	dd 45 e8             	fldl   -0x18(%ebp)
    2806:	d9 7d be             	fnstcw -0x42(%ebp)
    2809:	0f b7 45 be          	movzwl -0x42(%ebp),%eax
    280d:	80 cc 0c             	or     $0xc,%ah
    2810:	66 89 45 bc          	mov    %ax,-0x44(%ebp)
    2814:	d9 6d bc             	fldcw  -0x44(%ebp)
    2817:	db 5d dc             	fistpl -0x24(%ebp)
    281a:	d9 6d be             	fldcw  -0x42(%ebp)
    281d:	8b 45 dc             	mov    -0x24(%ebp),%eax
    2820:	83 c0 30             	add    $0x30,%eax
    2823:	83 ec 0c             	sub    $0xc,%esp
    2826:	50                   	push   %eax
    2827:	e8 17 fa ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    282c:	83 c4 10             	add    $0x10,%esp
    282f:	db 45 dc             	fildl  -0x24(%ebp)
    2832:	dd 45 e8             	fldl   -0x18(%ebp)
    2835:	de e1                	fsubp  %st,%st(1)
    2837:	dd 5d e8             	fstpl  -0x18(%ebp)
    283a:	83 45 e0 01          	addl   $0x1,-0x20(%ebp)
    283e:	8b 45 e0             	mov    -0x20(%ebp),%eax
    2841:	3b 45 10             	cmp    0x10(%ebp),%eax
    2844:	7c af                	jl     27f5 <_ZL11print_floatdi+0x13d>
    2846:	90                   	nop
    2847:	90                   	nop
    2848:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    284b:	c9                   	leave
    284c:	c3                   	ret

0000284d <printf>:
    284d:	55                   	push   %ebp
    284e:	89 e5                	mov    %esp,%ebp
    2850:	53                   	push   %ebx
    2851:	83 ec 54             	sub    $0x54,%esp
    2854:	e8 a2 f2 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    2859:	81 c3 e3 23 00 00    	add    $0x23e3,%ebx
    285f:	8d 83 e8 17 00 00    	lea    0x17e8(%ebx),%eax
    2865:	50                   	push   %eax
    2866:	e8 52 f9 ff ff       	call   21bd <mutex_lock>
    286b:	83 c4 04             	add    $0x4,%esp
    286e:	8d 45 0c             	lea    0xc(%ebp),%eax
    2871:	89 45 b0             	mov    %eax,-0x50(%ebp)
    2874:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    287b:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
    2882:	e9 53 03 00 00       	jmp    2bda <.L83+0x3e>
    2887:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
    288b:	75 34                	jne    28c1 <printf+0x74>
    288d:	8b 45 08             	mov    0x8(%ebp),%eax
    2890:	0f b6 00             	movzbl (%eax),%eax
    2893:	3c 25                	cmp    $0x25,%al
    2895:	75 0c                	jne    28a3 <printf+0x56>
    2897:	c7 45 f0 01 00 00 00 	movl   $0x1,-0x10(%ebp)
    289e:	e9 33 03 00 00       	jmp    2bd6 <.L83+0x3a>
    28a3:	8b 45 08             	mov    0x8(%ebp),%eax
    28a6:	0f b6 00             	movzbl (%eax),%eax
    28a9:	0f be c0             	movsbl %al,%eax
    28ac:	83 ec 0c             	sub    $0xc,%esp
    28af:	50                   	push   %eax
    28b0:	e8 8e f9 ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    28b5:	83 c4 10             	add    $0x10,%esp
    28b8:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    28bc:	e9 15 03 00 00       	jmp    2bd6 <.L83+0x3a>
    28c1:	c6 45 ef 00          	movb   $0x0,-0x11(%ebp)
    28c5:	c6 45 ee 20          	movb   $0x20,-0x12(%ebp)
    28c9:	c7 45 e8 00 00 00 00 	movl   $0x0,-0x18(%ebp)
    28d0:	8b 45 08             	mov    0x8(%ebp),%eax
    28d3:	0f b6 00             	movzbl (%eax),%eax
    28d6:	3c 2d                	cmp    $0x2d,%al
    28d8:	75 08                	jne    28e2 <printf+0x95>
    28da:	c6 45 ef 01          	movb   $0x1,-0x11(%ebp)
    28de:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    28e2:	8b 45 08             	mov    0x8(%ebp),%eax
    28e5:	0f b6 00             	movzbl (%eax),%eax
    28e8:	3c 30                	cmp    $0x30,%al
    28ea:	75 2d                	jne    2919 <printf+0xcc>
    28ec:	c6 45 ee 30          	movb   $0x30,-0x12(%ebp)
    28f0:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    28f4:	eb 23                	jmp    2919 <printf+0xcc>
    28f6:	8b 55 e8             	mov    -0x18(%ebp),%edx
    28f9:	89 d0                	mov    %edx,%eax
    28fb:	c1 e0 02             	shl    $0x2,%eax
    28fe:	01 d0                	add    %edx,%eax
    2900:	01 c0                	add    %eax,%eax
    2902:	89 c2                	mov    %eax,%edx
    2904:	8b 45 08             	mov    0x8(%ebp),%eax
    2907:	0f b6 00             	movzbl (%eax),%eax
    290a:	0f be c0             	movsbl %al,%eax
    290d:	83 e8 30             	sub    $0x30,%eax
    2910:	01 d0                	add    %edx,%eax
    2912:	89 45 e8             	mov    %eax,-0x18(%ebp)
    2915:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    2919:	8b 45 08             	mov    0x8(%ebp),%eax
    291c:	0f b6 00             	movzbl (%eax),%eax
    291f:	3c 2f                	cmp    $0x2f,%al
    2921:	7e 0a                	jle    292d <printf+0xe0>
    2923:	8b 45 08             	mov    0x8(%ebp),%eax
    2926:	0f b6 00             	movzbl (%eax),%eax
    2929:	3c 39                	cmp    $0x39,%al
    292b:	7e c9                	jle    28f6 <printf+0xa9>
    292d:	8b 45 08             	mov    0x8(%ebp),%eax
    2930:	0f b6 00             	movzbl (%eax),%eax
    2933:	0f be c0             	movsbl %al,%eax
    2936:	83 f8 25             	cmp    $0x25,%eax
    2939:	0f 84 18 02 00 00    	je     2b57 <.L88+0x1e>
    293f:	83 f8 25             	cmp    $0x25,%eax
    2942:	0f 8c 54 02 00 00    	jl     2b9c <.L83>
    2948:	83 f8 78             	cmp    $0x78,%eax
    294b:	0f 8f 4b 02 00 00    	jg     2b9c <.L83>
    2951:	83 f8 58             	cmp    $0x58,%eax
    2954:	0f 8c 42 02 00 00    	jl     2b9c <.L83>
    295a:	83 e8 58             	sub    $0x58,%eax
    295d:	83 f8 20             	cmp    $0x20,%eax
    2960:	0f 87 36 02 00 00    	ja     2b9c <.L83>
    2966:	c1 e0 02             	shl    $0x2,%eax
    2969:	8b 84 18 1c f2 ff ff 	mov    -0xde4(%eax,%ebx,1),%eax
    2970:	01 d8                	add    %ebx,%eax
    2972:	ff e0                	jmp    *%eax

00002974 <.L87>:
    2974:	8b 45 b0             	mov    -0x50(%ebp),%eax
    2977:	8d 50 04             	lea    0x4(%eax),%edx
    297a:	89 55 b0             	mov    %edx,-0x50(%ebp)
    297d:	8b 00                	mov    (%eax),%eax
    297f:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    2982:	83 7d e4 00          	cmpl   $0x0,-0x1c(%ebp)
    2986:	75 09                	jne    2991 <.L87+0x1d>
    2988:	8d 83 14 f2 ff ff    	lea    -0xdec(%ebx),%eax
    298e:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    2991:	c7 45 e0 00 00 00 00 	movl   $0x0,-0x20(%ebp)
    2998:	eb 04                	jmp    299e <.L87+0x2a>
    299a:	83 45 e0 01          	addl   $0x1,-0x20(%ebp)
    299e:	8b 55 e0             	mov    -0x20(%ebp),%edx
    29a1:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    29a4:	01 d0                	add    %edx,%eax
    29a6:	0f b6 00             	movzbl (%eax),%eax
    29a9:	84 c0                	test   %al,%al
    29ab:	75 ed                	jne    299a <.L87+0x26>
    29ad:	8b 45 e8             	mov    -0x18(%ebp),%eax
    29b0:	2b 45 e0             	sub    -0x20(%ebp),%eax
    29b3:	89 45 dc             	mov    %eax,-0x24(%ebp)
    29b6:	83 7d dc 00          	cmpl   $0x0,-0x24(%ebp)
    29ba:	79 07                	jns    29c3 <.L87+0x4f>
    29bc:	c7 45 dc 00 00 00 00 	movl   $0x0,-0x24(%ebp)
    29c3:	0f b6 45 ef          	movzbl -0x11(%ebp),%eax
    29c7:	83 f0 01             	xor    $0x1,%eax
    29ca:	84 c0                	test   %al,%al
    29cc:	74 47                	je     2a15 <.L87+0xa1>
    29ce:	c7 45 d8 00 00 00 00 	movl   $0x0,-0x28(%ebp)
    29d5:	eb 15                	jmp    29ec <.L87+0x78>
    29d7:	83 ec 0c             	sub    $0xc,%esp
    29da:	6a 20                	push   $0x20
    29dc:	e8 62 f8 ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    29e1:	83 c4 10             	add    $0x10,%esp
    29e4:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    29e8:	83 45 d8 01          	addl   $0x1,-0x28(%ebp)
    29ec:	8b 45 d8             	mov    -0x28(%ebp),%eax
    29ef:	3b 45 dc             	cmp    -0x24(%ebp),%eax
    29f2:	7c e3                	jl     29d7 <.L87+0x63>
    29f4:	eb 1f                	jmp    2a15 <.L87+0xa1>
    29f6:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    29f9:	8d 50 01             	lea    0x1(%eax),%edx
    29fc:	89 55 e4             	mov    %edx,-0x1c(%ebp)
    29ff:	0f b6 00             	movzbl (%eax),%eax
    2a02:	0f be c0             	movsbl %al,%eax
    2a05:	83 ec 0c             	sub    $0xc,%esp
    2a08:	50                   	push   %eax
    2a09:	e8 35 f8 ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    2a0e:	83 c4 10             	add    $0x10,%esp
    2a11:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    2a15:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    2a18:	0f b6 00             	movzbl (%eax),%eax
    2a1b:	84 c0                	test   %al,%al
    2a1d:	75 d7                	jne    29f6 <.L87+0x82>
    2a1f:	80 7d ef 00          	cmpb   $0x0,-0x11(%ebp)
    2a23:	0f 84 a5 01 00 00    	je     2bce <.L83+0x32>
    2a29:	c7 45 d4 00 00 00 00 	movl   $0x0,-0x2c(%ebp)
    2a30:	eb 15                	jmp    2a47 <.L87+0xd3>
    2a32:	83 ec 0c             	sub    $0xc,%esp
    2a35:	6a 20                	push   $0x20
    2a37:	e8 07 f8 ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    2a3c:	83 c4 10             	add    $0x10,%esp
    2a3f:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    2a43:	83 45 d4 01          	addl   $0x1,-0x2c(%ebp)
    2a47:	8b 45 d4             	mov    -0x2c(%ebp),%eax
    2a4a:	3b 45 dc             	cmp    -0x24(%ebp),%eax
    2a4d:	7c e3                	jl     2a32 <.L87+0xbe>
    2a4f:	e9 7a 01 00 00       	jmp    2bce <.L83+0x32>

00002a54 <.L89>:
    2a54:	8b 45 b0             	mov    -0x50(%ebp),%eax
    2a57:	8d 50 04             	lea    0x4(%eax),%edx
    2a5a:	89 55 b0             	mov    %edx,-0x50(%ebp)
    2a5d:	8b 00                	mov    (%eax),%eax
    2a5f:	89 45 bc             	mov    %eax,-0x44(%ebp)
    2a62:	0f b6 55 ef          	movzbl -0x11(%ebp),%edx
    2a66:	0f be 45 ee          	movsbl -0x12(%ebp),%eax
    2a6a:	52                   	push   %edx
    2a6b:	50                   	push   %eax
    2a6c:	ff 75 e8             	push   -0x18(%ebp)
    2a6f:	ff 75 bc             	push   -0x44(%ebp)
    2a72:	e8 ac fa ff ff       	call   2523 <_ZL9print_intiicb>
    2a77:	83 c4 10             	add    $0x10,%esp
    2a7a:	e9 50 01 00 00       	jmp    2bcf <.L83+0x33>

00002a7f <.L86>:
    2a7f:	8b 45 b0             	mov    -0x50(%ebp),%eax
    2a82:	8d 50 04             	lea    0x4(%eax),%edx
    2a85:	89 55 b0             	mov    %edx,-0x50(%ebp)
    2a88:	8b 00                	mov    (%eax),%eax
    2a8a:	89 45 cc             	mov    %eax,-0x34(%ebp)
    2a8d:	0f b6 55 ef          	movzbl -0x11(%ebp),%edx
    2a91:	0f be 45 ee          	movsbl -0x12(%ebp),%eax
    2a95:	52                   	push   %edx
    2a96:	50                   	push   %eax
    2a97:	ff 75 e8             	push   -0x18(%ebp)
    2a9a:	6a 00                	push   $0x0
    2a9c:	6a 00                	push   $0x0
    2a9e:	6a 00                	push   $0x0
    2aa0:	6a 0a                	push   $0xa
    2aa2:	ff 75 cc             	push   -0x34(%ebp)
    2aa5:	e8 b7 f8 ff ff       	call   2361 <_ZL15print_uint_corejiibbicb>
    2aaa:	83 c4 20             	add    $0x20,%esp
    2aad:	e9 1d 01 00 00       	jmp    2bcf <.L83+0x33>

00002ab2 <.L91>:
    2ab2:	8b 45 b0             	mov    -0x50(%ebp),%eax
    2ab5:	8d 50 04             	lea    0x4(%eax),%edx
    2ab8:	89 55 b0             	mov    %edx,-0x50(%ebp)
    2abb:	8b 00                	mov    (%eax),%eax
    2abd:	88 45 bb             	mov    %al,-0x45(%ebp)
    2ac0:	0f be 45 bb          	movsbl -0x45(%ebp),%eax
    2ac4:	83 ec 0c             	sub    $0xc,%esp
    2ac7:	50                   	push   %eax
    2ac8:	e8 76 f7 ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    2acd:	83 c4 10             	add    $0x10,%esp
    2ad0:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    2ad4:	e9 f6 00 00 00       	jmp    2bcf <.L83+0x33>

00002ad9 <.L84>:
    2ad9:	8b 45 b0             	mov    -0x50(%ebp),%eax
    2adc:	8d 50 04             	lea    0x4(%eax),%edx
    2adf:	89 55 b0             	mov    %edx,-0x50(%ebp)
    2ae2:	8b 00                	mov    (%eax),%eax
    2ae4:	89 45 d0             	mov    %eax,-0x30(%ebp)
    2ae7:	0f b6 55 ef          	movzbl -0x11(%ebp),%edx
    2aeb:	0f be 45 ee          	movsbl -0x12(%ebp),%eax
    2aef:	83 ec 0c             	sub    $0xc,%esp
    2af2:	52                   	push   %edx
    2af3:	50                   	push   %eax
    2af4:	ff 75 e8             	push   -0x18(%ebp)
    2af7:	6a 00                	push   $0x0
    2af9:	ff 75 d0             	push   -0x30(%ebp)
    2afc:	e8 80 fa ff ff       	call   2581 <_ZL9print_hexjiicb>
    2b01:	83 c4 20             	add    $0x20,%esp
    2b04:	e9 c6 00 00 00       	jmp    2bcf <.L83+0x33>

00002b09 <.L92>:
    2b09:	8b 45 b0             	mov    -0x50(%ebp),%eax
    2b0c:	8d 50 04             	lea    0x4(%eax),%edx
    2b0f:	89 55 b0             	mov    %edx,-0x50(%ebp)
    2b12:	8b 00                	mov    (%eax),%eax
    2b14:	89 45 b4             	mov    %eax,-0x4c(%ebp)
    2b17:	0f b6 55 ef          	movzbl -0x11(%ebp),%edx
    2b1b:	0f be 45 ee          	movsbl -0x12(%ebp),%eax
    2b1f:	83 ec 0c             	sub    $0xc,%esp
    2b22:	52                   	push   %edx
    2b23:	50                   	push   %eax
    2b24:	ff 75 e8             	push   -0x18(%ebp)
    2b27:	6a 01                	push   $0x1
    2b29:	ff 75 b4             	push   -0x4c(%ebp)
    2b2c:	e8 50 fa ff ff       	call   2581 <_ZL9print_hexjiicb>
    2b31:	83 c4 20             	add    $0x20,%esp
    2b34:	e9 96 00 00 00       	jmp    2bcf <.L83+0x33>

00002b39 <.L88>:
    2b39:	8b 45 b0             	mov    -0x50(%ebp),%eax
    2b3c:	8d 50 04             	lea    0x4(%eax),%edx
    2b3f:	89 55 b0             	mov    %edx,-0x50(%ebp)
    2b42:	8b 00                	mov    (%eax),%eax
    2b44:	89 45 c8             	mov    %eax,-0x38(%ebp)
    2b47:	83 ec 0c             	sub    $0xc,%esp
    2b4a:	ff 75 c8             	push   -0x38(%ebp)
    2b4d:	e8 6f fa ff ff       	call   25c1 <_ZL9print_ptrPv>
    2b52:	83 c4 10             	add    $0x10,%esp
    2b55:	eb 78                	jmp    2bcf <.L83+0x33>
    2b57:	83 ec 0c             	sub    $0xc,%esp
    2b5a:	6a 25                	push   $0x25
    2b5c:	e8 e2 f6 ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    2b61:	83 c4 10             	add    $0x10,%esp
    2b64:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    2b68:	eb 65                	jmp    2bcf <.L83+0x33>

00002b6a <.L90>:
    2b6a:	8b 45 b0             	mov    -0x50(%ebp),%eax
    2b6d:	8d 50 08             	lea    0x8(%eax),%edx
    2b70:	89 55 b0             	mov    %edx,-0x50(%ebp)
    2b73:	dd 00                	fldl   (%eax)
    2b75:	dd 5d c0             	fstpl  -0x40(%ebp)
    2b78:	83 7d e8 00          	cmpl   $0x0,-0x18(%ebp)
    2b7c:	7e 05                	jle    2b83 <.L90+0x19>
    2b7e:	8b 45 e8             	mov    -0x18(%ebp),%eax
    2b81:	eb 05                	jmp    2b88 <.L90+0x1e>
    2b83:	b8 06 00 00 00       	mov    $0x6,%eax
    2b88:	83 ec 04             	sub    $0x4,%esp
    2b8b:	50                   	push   %eax
    2b8c:	ff 75 c4             	push   -0x3c(%ebp)
    2b8f:	ff 75 c0             	push   -0x40(%ebp)
    2b92:	e8 21 fb ff ff       	call   26b8 <_ZL11print_floatdi>
    2b97:	83 c4 10             	add    $0x10,%esp
    2b9a:	eb 33                	jmp    2bcf <.L83+0x33>

00002b9c <.L83>:
    2b9c:	83 ec 0c             	sub    $0xc,%esp
    2b9f:	6a 25                	push   $0x25
    2ba1:	e8 9d f6 ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    2ba6:	83 c4 10             	add    $0x10,%esp
    2ba9:	8b 45 08             	mov    0x8(%ebp),%eax
    2bac:	0f b6 00             	movzbl (%eax),%eax
    2baf:	84 c0                	test   %al,%al
    2bb1:	74 15                	je     2bc8 <.L83+0x2c>
    2bb3:	8b 45 08             	mov    0x8(%ebp),%eax
    2bb6:	0f b6 00             	movzbl (%eax),%eax
    2bb9:	0f be c0             	movsbl %al,%eax
    2bbc:	83 ec 0c             	sub    $0xc,%esp
    2bbf:	50                   	push   %eax
    2bc0:	e8 7e f6 ff ff       	call   2243 <_ZL17_putchar_unlockedi>
    2bc5:	83 c4 10             	add    $0x10,%esp
    2bc8:	83 45 f4 02          	addl   $0x2,-0xc(%ebp)
    2bcc:	eb 01                	jmp    2bcf <.L83+0x33>
    2bce:	90                   	nop
    2bcf:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
    2bd6:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    2bda:	8b 45 08             	mov    0x8(%ebp),%eax
    2bdd:	0f b6 00             	movzbl (%eax),%eax
    2be0:	84 c0                	test   %al,%al
    2be2:	0f 85 9f fc ff ff    	jne    2887 <printf+0x3a>
    2be8:	e8 0d f6 ff ff       	call   21fa <_ZL23_fflush_stdout_unlockedv>
    2bed:	83 ec 0c             	sub    $0xc,%esp
    2bf0:	8d 83 e8 17 00 00    	lea    0x17e8(%ebx),%eax
    2bf6:	50                   	push   %eax
    2bf7:	e8 e4 f5 ff ff       	call   21e0 <mutex_unlock>
    2bfc:	83 c4 10             	add    $0x10,%esp
    2bff:	8b 45 f4             	mov    -0xc(%ebp),%eax
    2c02:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2c05:	c9                   	leave
    2c06:	c3                   	ret

00002c07 <getchar>:
    2c07:	55                   	push   %ebp
    2c08:	89 e5                	mov    %esp,%ebp
    2c0a:	53                   	push   %ebx
    2c0b:	83 ec 14             	sub    $0x14,%esp
    2c0e:	e8 98 e4 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    2c13:	05 29 20 00 00       	add    $0x2029,%eax
    2c18:	83 ec 0c             	sub    $0xc,%esp
    2c1b:	6a 02                	push   $0x2
    2c1d:	89 c3                	mov    %eax,%ebx
    2c1f:	e8 3c 11 00 00       	call   3d60 <syscall@plt>
    2c24:	83 c4 10             	add    $0x10,%esp
    2c27:	89 45 f4             	mov    %eax,-0xc(%ebp)
    2c2a:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    2c2e:	79 07                	jns    2c37 <getchar+0x30>
    2c30:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    2c35:	eb 03                	jmp    2c3a <getchar+0x33>
    2c37:	8b 45 f4             	mov    -0xc(%ebp),%eax
    2c3a:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2c3d:	c9                   	leave
    2c3e:	c3                   	ret

00002c3f <gets_s>:
    2c3f:	55                   	push   %ebp
    2c40:	89 e5                	mov    %esp,%ebp
    2c42:	53                   	push   %ebx
    2c43:	83 ec 24             	sub    $0x24,%esp
    2c46:	e8 b0 ee ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    2c4b:	81 c3 f1 1f 00 00    	add    $0x1ff1,%ebx
    2c51:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    2c55:	74 06                	je     2c5d <gets_s+0x1e>
    2c57:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    2c5b:	75 15                	jne    2c72 <gets_s+0x33>
    2c5d:	e8 ae 11 00 00       	call   3e10 <__errno_location@plt>
    2c62:	c7 00 16 00 00 00    	movl   $0x16,(%eax)
    2c68:	b8 00 00 00 00       	mov    $0x0,%eax
    2c6d:	e9 d0 00 00 00       	jmp    2d42 <gets_s+0x103>
    2c72:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    2c79:	e9 a4 00 00 00       	jmp    2d22 <gets_s+0xe3>
    2c7e:	e8 6d 11 00 00       	call   3df0 <getchar@plt>
    2c83:	89 45 f0             	mov    %eax,-0x10(%ebp)
    2c86:	83 7d f0 ff          	cmpl   $0xffffffff,-0x10(%ebp)
    2c8a:	75 14                	jne    2ca0 <gets_s+0x61>
    2c8c:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    2c90:	0f 85 9d 00 00 00    	jne    2d33 <gets_s+0xf4>
    2c96:	b8 00 00 00 00       	mov    $0x0,%eax
    2c9b:	e9 a2 00 00 00       	jmp    2d42 <gets_s+0x103>
    2ca0:	83 7d f0 0a          	cmpl   $0xa,-0x10(%ebp)
    2ca4:	74 06                	je     2cac <gets_s+0x6d>
    2ca6:	83 7d f0 0d          	cmpl   $0xd,-0x10(%ebp)
    2caa:	75 0f                	jne    2cbb <gets_s+0x7c>
    2cac:	83 ec 0c             	sub    $0xc,%esp
    2caf:	6a 0a                	push   $0xa
    2cb1:	e8 7a 10 00 00       	call   3d30 <putchar@plt>
    2cb6:	83 c4 10             	add    $0x10,%esp
    2cb9:	eb 79                	jmp    2d34 <gets_s+0xf5>
    2cbb:	83 7d f0 08          	cmpl   $0x8,-0x10(%ebp)
    2cbf:	74 06                	je     2cc7 <gets_s+0x88>
    2cc1:	83 7d f0 7f          	cmpl   $0x7f,-0x10(%ebp)
    2cc5:	75 33                	jne    2cfa <gets_s+0xbb>
    2cc7:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    2ccb:	74 55                	je     2d22 <gets_s+0xe3>
    2ccd:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
    2cd1:	83 ec 0c             	sub    $0xc,%esp
    2cd4:	6a 08                	push   $0x8
    2cd6:	e8 55 10 00 00       	call   3d30 <putchar@plt>
    2cdb:	83 c4 10             	add    $0x10,%esp
    2cde:	83 ec 0c             	sub    $0xc,%esp
    2ce1:	6a 20                	push   $0x20
    2ce3:	e8 48 10 00 00       	call   3d30 <putchar@plt>
    2ce8:	83 c4 10             	add    $0x10,%esp
    2ceb:	83 ec 0c             	sub    $0xc,%esp
    2cee:	6a 08                	push   $0x8
    2cf0:	e8 3b 10 00 00       	call   3d30 <putchar@plt>
    2cf5:	83 c4 10             	add    $0x10,%esp
    2cf8:	eb 28                	jmp    2d22 <gets_s+0xe3>
    2cfa:	8b 45 f0             	mov    -0x10(%ebp),%eax
    2cfd:	88 45 e7             	mov    %al,-0x19(%ebp)
    2d00:	8b 4d 08             	mov    0x8(%ebp),%ecx
    2d03:	8b 45 f4             	mov    -0xc(%ebp),%eax
    2d06:	8d 50 01             	lea    0x1(%eax),%edx
    2d09:	89 55 f4             	mov    %edx,-0xc(%ebp)
    2d0c:	01 c8                	add    %ecx,%eax
    2d0e:	0f b6 4d e7          	movzbl -0x19(%ebp),%ecx
    2d12:	88 08                	mov    %cl,(%eax)
    2d14:	83 ec 0c             	sub    $0xc,%esp
    2d17:	ff 75 f0             	push   -0x10(%ebp)
    2d1a:	e8 11 10 00 00       	call   3d30 <putchar@plt>
    2d1f:	83 c4 10             	add    $0x10,%esp
    2d22:	8b 45 0c             	mov    0xc(%ebp),%eax
    2d25:	83 e8 01             	sub    $0x1,%eax
    2d28:	39 45 f4             	cmp    %eax,-0xc(%ebp)
    2d2b:	0f 82 4d ff ff ff    	jb     2c7e <gets_s+0x3f>
    2d31:	eb 01                	jmp    2d34 <gets_s+0xf5>
    2d33:	90                   	nop
    2d34:	8b 55 08             	mov    0x8(%ebp),%edx
    2d37:	8b 45 f4             	mov    -0xc(%ebp),%eax
    2d3a:	01 d0                	add    %edx,%eax
    2d3c:	c6 00 00             	movb   $0x0,(%eax)
    2d3f:	8b 45 08             	mov    0x8(%ebp),%eax
    2d42:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2d45:	c9                   	leave
    2d46:	c3                   	ret

00002d47 <flockfile>:
    2d47:	55                   	push   %ebp
    2d48:	89 e5                	mov    %esp,%ebp
    2d4a:	e8 5c e3 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    2d4f:	05 ed 1e 00 00       	add    $0x1eed,%eax
    2d54:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    2d58:	74 0f                	je     2d69 <flockfile+0x22>
    2d5a:	8b 45 08             	mov    0x8(%ebp),%eax
    2d5d:	83 c0 20             	add    $0x20,%eax
    2d60:	50                   	push   %eax
    2d61:	e8 57 f4 ff ff       	call   21bd <mutex_lock>
    2d66:	83 c4 04             	add    $0x4,%esp
    2d69:	90                   	nop
    2d6a:	c9                   	leave
    2d6b:	c3                   	ret

00002d6c <funlockfile>:
    2d6c:	55                   	push   %ebp
    2d6d:	89 e5                	mov    %esp,%ebp
    2d6f:	e8 37 e3 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    2d74:	05 c8 1e 00 00       	add    $0x1ec8,%eax
    2d79:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    2d7d:	74 0f                	je     2d8e <funlockfile+0x22>
    2d7f:	8b 45 08             	mov    0x8(%ebp),%eax
    2d82:	83 c0 20             	add    $0x20,%eax
    2d85:	50                   	push   %eax
    2d86:	e8 55 f4 ff ff       	call   21e0 <mutex_unlock>
    2d8b:	83 c4 04             	add    $0x4,%esp
    2d8e:	90                   	nop
    2d8f:	c9                   	leave
    2d90:	c3                   	ret

00002d91 <fopen>:
    2d91:	55                   	push   %ebp
    2d92:	89 e5                	mov    %esp,%ebp
    2d94:	53                   	push   %ebx
    2d95:	83 ec 14             	sub    $0x14,%esp
    2d98:	e8 5e ed ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    2d9d:	81 c3 9f 1e 00 00    	add    $0x1e9f,%ebx
    2da3:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    2da7:	74 06                	je     2daf <fopen+0x1e>
    2da9:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    2dad:	75 0a                	jne    2db9 <fopen+0x28>
    2daf:	b8 00 00 00 00       	mov    $0x0,%eax
    2db4:	e9 23 01 00 00       	jmp    2edc <fopen+0x14b>
    2db9:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    2dc0:	8b 45 0c             	mov    0xc(%ebp),%eax
    2dc3:	0f b6 00             	movzbl (%eax),%eax
    2dc6:	3c 72                	cmp    $0x72,%al
    2dc8:	75 09                	jne    2dd3 <fopen+0x42>
    2dca:	c7 45 f4 01 00 00 00 	movl   $0x1,-0xc(%ebp)
    2dd1:	eb 1d                	jmp    2df0 <fopen+0x5f>
    2dd3:	8b 45 0c             	mov    0xc(%ebp),%eax
    2dd6:	0f b6 00             	movzbl (%eax),%eax
    2dd9:	3c 77                	cmp    $0x77,%al
    2ddb:	75 09                	jne    2de6 <fopen+0x55>
    2ddd:	c7 45 f4 02 00 00 00 	movl   $0x2,-0xc(%ebp)
    2de4:	eb 0a                	jmp    2df0 <fopen+0x5f>
    2de6:	b8 00 00 00 00       	mov    $0x0,%eax
    2deb:	e9 ec 00 00 00       	jmp    2edc <fopen+0x14b>
    2df0:	8b 45 08             	mov    0x8(%ebp),%eax
    2df3:	83 ec 04             	sub    $0x4,%esp
    2df6:	ff 75 f4             	push   -0xc(%ebp)
    2df9:	50                   	push   %eax
    2dfa:	6a 1d                	push   $0x1d
    2dfc:	e8 5f 0f 00 00       	call   3d60 <syscall@plt>
    2e01:	83 c4 10             	add    $0x10,%esp
    2e04:	89 45 f0             	mov    %eax,-0x10(%ebp)
    2e07:	83 7d f0 ff          	cmpl   $0xffffffff,-0x10(%ebp)
    2e0b:	75 0a                	jne    2e17 <fopen+0x86>
    2e0d:	b8 00 00 00 00       	mov    $0x0,%eax
    2e12:	e9 c5 00 00 00       	jmp    2edc <fopen+0x14b>
    2e17:	83 ec 0c             	sub    $0xc,%esp
    2e1a:	6a 24                	push   $0x24
    2e1c:	e8 6f 0f 00 00       	call   3d90 <malloc@plt>
    2e21:	83 c4 10             	add    $0x10,%esp
    2e24:	89 45 ec             	mov    %eax,-0x14(%ebp)
    2e27:	83 7d ec 00          	cmpl   $0x0,-0x14(%ebp)
    2e2b:	75 1a                	jne    2e47 <fopen+0xb6>
    2e2d:	83 ec 08             	sub    $0x8,%esp
    2e30:	ff 75 f0             	push   -0x10(%ebp)
    2e33:	6a 20                	push   $0x20
    2e35:	e8 26 0f 00 00       	call   3d60 <syscall@plt>
    2e3a:	83 c4 10             	add    $0x10,%esp
    2e3d:	b8 00 00 00 00       	mov    $0x0,%eax
    2e42:	e9 95 00 00 00       	jmp    2edc <fopen+0x14b>
    2e47:	83 ec 0c             	sub    $0xc,%esp
    2e4a:	68 00 10 00 00       	push   $0x1000
    2e4f:	e8 3c 0f 00 00       	call   3d90 <malloc@plt>
    2e54:	83 c4 10             	add    $0x10,%esp
    2e57:	8b 55 ec             	mov    -0x14(%ebp),%edx
    2e5a:	89 42 10             	mov    %eax,0x10(%edx)
    2e5d:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2e60:	8b 40 10             	mov    0x10(%eax),%eax
    2e63:	85 c0                	test   %eax,%eax
    2e65:	75 25                	jne    2e8c <fopen+0xfb>
    2e67:	83 ec 08             	sub    $0x8,%esp
    2e6a:	ff 75 f0             	push   -0x10(%ebp)
    2e6d:	6a 20                	push   $0x20
    2e6f:	e8 ec 0e 00 00       	call   3d60 <syscall@plt>
    2e74:	83 c4 10             	add    $0x10,%esp
    2e77:	83 ec 0c             	sub    $0xc,%esp
    2e7a:	ff 75 ec             	push   -0x14(%ebp)
    2e7d:	e8 be 0f 00 00       	call   3e40 <free@plt>
    2e82:	83 c4 10             	add    $0x10,%esp
    2e85:	b8 00 00 00 00       	mov    $0x0,%eax
    2e8a:	eb 50                	jmp    2edc <fopen+0x14b>
    2e8c:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2e8f:	8b 55 f0             	mov    -0x10(%ebp),%edx
    2e92:	89 10                	mov    %edx,(%eax)
    2e94:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2e97:	8b 55 f4             	mov    -0xc(%ebp),%edx
    2e9a:	89 50 04             	mov    %edx,0x4(%eax)
    2e9d:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2ea0:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%eax)
    2ea7:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2eaa:	c7 40 0c 00 00 00 00 	movl   $0x0,0xc(%eax)
    2eb1:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2eb4:	c7 40 14 00 10 00 00 	movl   $0x1000,0x14(%eax)
    2ebb:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2ebe:	c7 40 18 00 00 00 00 	movl   $0x0,0x18(%eax)
    2ec5:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2ec8:	c7 40 1c 00 00 00 00 	movl   $0x0,0x1c(%eax)
    2ecf:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2ed2:	c7 40 20 00 00 00 00 	movl   $0x0,0x20(%eax)
    2ed9:	8b 45 ec             	mov    -0x14(%ebp),%eax
    2edc:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2edf:	c9                   	leave
    2ee0:	c3                   	ret

00002ee1 <_ZL16_fflush_unlockedP4FILE>:
    2ee1:	55                   	push   %ebp
    2ee2:	89 e5                	mov    %esp,%ebp
    2ee4:	53                   	push   %ebx
    2ee5:	83 ec 14             	sub    $0x14,%esp
    2ee8:	e8 be e1 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    2eed:	05 4f 1d 00 00       	add    $0x1d4f,%eax
    2ef2:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    2ef6:	75 07                	jne    2eff <_ZL16_fflush_unlockedP4FILE+0x1e>
    2ef8:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    2efd:	eb 6c                	jmp    2f6b <_ZL16_fflush_unlockedP4FILE+0x8a>
    2eff:	8b 55 08             	mov    0x8(%ebp),%edx
    2f02:	8b 52 04             	mov    0x4(%edx),%edx
    2f05:	83 fa 02             	cmp    $0x2,%edx
    2f08:	75 48                	jne    2f52 <_ZL16_fflush_unlockedP4FILE+0x71>
    2f0a:	8b 55 08             	mov    0x8(%ebp),%edx
    2f0d:	8b 52 18             	mov    0x18(%edx),%edx
    2f10:	85 d2                	test   %edx,%edx
    2f12:	74 3e                	je     2f52 <_ZL16_fflush_unlockedP4FILE+0x71>
    2f14:	8b 55 08             	mov    0x8(%ebp),%edx
    2f17:	8b 52 18             	mov    0x18(%edx),%edx
    2f1a:	89 d3                	mov    %edx,%ebx
    2f1c:	8b 55 08             	mov    0x8(%ebp),%edx
    2f1f:	8b 52 10             	mov    0x10(%edx),%edx
    2f22:	89 d1                	mov    %edx,%ecx
    2f24:	8b 55 08             	mov    0x8(%ebp),%edx
    2f27:	8b 12                	mov    (%edx),%edx
    2f29:	53                   	push   %ebx
    2f2a:	51                   	push   %ecx
    2f2b:	52                   	push   %edx
    2f2c:	6a 1f                	push   $0x1f
    2f2e:	89 c3                	mov    %eax,%ebx
    2f30:	e8 2b 0e 00 00       	call   3d60 <syscall@plt>
    2f35:	83 c4 10             	add    $0x10,%esp
    2f38:	89 45 f4             	mov    %eax,-0xc(%ebp)
    2f3b:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    2f3f:	7f 11                	jg     2f52 <_ZL16_fflush_unlockedP4FILE+0x71>
    2f41:	8b 45 08             	mov    0x8(%ebp),%eax
    2f44:	c7 40 0c 01 00 00 00 	movl   $0x1,0xc(%eax)
    2f4b:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    2f50:	eb 19                	jmp    2f6b <_ZL16_fflush_unlockedP4FILE+0x8a>
    2f52:	8b 45 08             	mov    0x8(%ebp),%eax
    2f55:	c7 40 18 00 00 00 00 	movl   $0x0,0x18(%eax)
    2f5c:	8b 45 08             	mov    0x8(%ebp),%eax
    2f5f:	c7 40 1c 00 00 00 00 	movl   $0x0,0x1c(%eax)
    2f66:	b8 00 00 00 00       	mov    $0x0,%eax
    2f6b:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2f6e:	c9                   	leave
    2f6f:	c3                   	ret

00002f70 <fflush>:
    2f70:	55                   	push   %ebp
    2f71:	89 e5                	mov    %esp,%ebp
    2f73:	53                   	push   %ebx
    2f74:	83 ec 14             	sub    $0x14,%esp
    2f77:	e8 7f eb ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    2f7c:	81 c3 c0 1c 00 00    	add    $0x1cc0,%ebx
    2f82:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    2f86:	75 07                	jne    2f8f <fflush+0x1f>
    2f88:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    2f8d:	eb 30                	jmp    2fbf <fflush+0x4f>
    2f8f:	83 ec 0c             	sub    $0xc,%esp
    2f92:	ff 75 08             	push   0x8(%ebp)
    2f95:	e8 66 0e 00 00       	call   3e00 <flockfile@plt>
    2f9a:	83 c4 10             	add    $0x10,%esp
    2f9d:	83 ec 0c             	sub    $0xc,%esp
    2fa0:	ff 75 08             	push   0x8(%ebp)
    2fa3:	e8 39 ff ff ff       	call   2ee1 <_ZL16_fflush_unlockedP4FILE>
    2fa8:	83 c4 10             	add    $0x10,%esp
    2fab:	89 45 f4             	mov    %eax,-0xc(%ebp)
    2fae:	83 ec 0c             	sub    $0xc,%esp
    2fb1:	ff 75 08             	push   0x8(%ebp)
    2fb4:	e8 07 0e 00 00       	call   3dc0 <funlockfile@plt>
    2fb9:	83 c4 10             	add    $0x10,%esp
    2fbc:	8b 45 f4             	mov    -0xc(%ebp),%eax
    2fbf:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    2fc2:	c9                   	leave
    2fc3:	c3                   	ret

00002fc4 <fclose>:
    2fc4:	55                   	push   %ebp
    2fc5:	89 e5                	mov    %esp,%ebp
    2fc7:	53                   	push   %ebx
    2fc8:	83 ec 14             	sub    $0x14,%esp
    2fcb:	e8 2b eb ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    2fd0:	81 c3 6c 1c 00 00    	add    $0x1c6c,%ebx
    2fd6:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    2fda:	75 0a                	jne    2fe6 <fclose+0x22>
    2fdc:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    2fe1:	e9 86 00 00 00       	jmp    306c <fclose+0xa8>
    2fe6:	83 ec 0c             	sub    $0xc,%esp
    2fe9:	ff 75 08             	push   0x8(%ebp)
    2fec:	e8 0f 0e 00 00       	call   3e00 <flockfile@plt>
    2ff1:	83 c4 10             	add    $0x10,%esp
    2ff4:	83 ec 0c             	sub    $0xc,%esp
    2ff7:	ff 75 08             	push   0x8(%ebp)
    2ffa:	e8 e2 fe ff ff       	call   2ee1 <_ZL16_fflush_unlockedP4FILE>
    2fff:	83 c4 10             	add    $0x10,%esp
    3002:	89 45 f4             	mov    %eax,-0xc(%ebp)
    3005:	8b 45 08             	mov    0x8(%ebp),%eax
    3008:	8b 00                	mov    (%eax),%eax
    300a:	83 ec 08             	sub    $0x8,%esp
    300d:	50                   	push   %eax
    300e:	6a 20                	push   $0x20
    3010:	e8 4b 0d 00 00       	call   3d60 <syscall@plt>
    3015:	83 c4 10             	add    $0x10,%esp
    3018:	89 45 f0             	mov    %eax,-0x10(%ebp)
    301b:	8b 45 08             	mov    0x8(%ebp),%eax
    301e:	8b 40 10             	mov    0x10(%eax),%eax
    3021:	85 c0                	test   %eax,%eax
    3023:	74 12                	je     3037 <fclose+0x73>
    3025:	8b 45 08             	mov    0x8(%ebp),%eax
    3028:	8b 40 10             	mov    0x10(%eax),%eax
    302b:	83 ec 0c             	sub    $0xc,%esp
    302e:	50                   	push   %eax
    302f:	e8 0c 0e 00 00       	call   3e40 <free@plt>
    3034:	83 c4 10             	add    $0x10,%esp
    3037:	83 ec 0c             	sub    $0xc,%esp
    303a:	ff 75 08             	push   0x8(%ebp)
    303d:	e8 7e 0d 00 00       	call   3dc0 <funlockfile@plt>
    3042:	83 c4 10             	add    $0x10,%esp
    3045:	83 ec 0c             	sub    $0xc,%esp
    3048:	ff 75 08             	push   0x8(%ebp)
    304b:	e8 f0 0d 00 00       	call   3e40 <free@plt>
    3050:	83 c4 10             	add    $0x10,%esp
    3053:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    3057:	75 0d                	jne    3066 <fclose+0xa2>
    3059:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
    305d:	75 07                	jne    3066 <fclose+0xa2>
    305f:	b8 00 00 00 00       	mov    $0x0,%eax
    3064:	eb 05                	jmp    306b <fclose+0xa7>
    3066:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    306b:	90                   	nop
    306c:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    306f:	c9                   	leave
    3070:	c3                   	ret

00003071 <fgetc>:
    3071:	55                   	push   %ebp
    3072:	89 e5                	mov    %esp,%ebp
    3074:	56                   	push   %esi
    3075:	53                   	push   %ebx
    3076:	83 ec 10             	sub    $0x10,%esp
    3079:	e8 7d ea ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    307e:	81 c3 be 1b 00 00    	add    $0x1bbe,%ebx
    3084:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    3088:	75 0a                	jne    3094 <fgetc+0x23>
    308a:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    308f:	e9 e3 00 00 00       	jmp    3177 <fgetc+0x106>
    3094:	83 ec 0c             	sub    $0xc,%esp
    3097:	ff 75 08             	push   0x8(%ebp)
    309a:	e8 61 0d 00 00       	call   3e00 <flockfile@plt>
    309f:	83 c4 10             	add    $0x10,%esp
    30a2:	8b 45 08             	mov    0x8(%ebp),%eax
    30a5:	8b 40 08             	mov    0x8(%eax),%eax
    30a8:	85 c0                	test   %eax,%eax
    30aa:	75 15                	jne    30c1 <fgetc+0x50>
    30ac:	8b 45 08             	mov    0x8(%ebp),%eax
    30af:	8b 40 0c             	mov    0xc(%eax),%eax
    30b2:	85 c0                	test   %eax,%eax
    30b4:	75 0b                	jne    30c1 <fgetc+0x50>
    30b6:	8b 45 08             	mov    0x8(%ebp),%eax
    30b9:	8b 40 04             	mov    0x4(%eax),%eax
    30bc:	83 f8 01             	cmp    $0x1,%eax
    30bf:	74 18                	je     30d9 <fgetc+0x68>
    30c1:	83 ec 0c             	sub    $0xc,%esp
    30c4:	ff 75 08             	push   0x8(%ebp)
    30c7:	e8 f4 0c 00 00       	call   3dc0 <funlockfile@plt>
    30cc:	83 c4 10             	add    $0x10,%esp
    30cf:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    30d4:	e9 9e 00 00 00       	jmp    3177 <fgetc+0x106>
    30d9:	8b 45 08             	mov    0x8(%ebp),%eax
    30dc:	8b 50 18             	mov    0x18(%eax),%edx
    30df:	8b 45 08             	mov    0x8(%ebp),%eax
    30e2:	8b 40 1c             	mov    0x1c(%eax),%eax
    30e5:	39 c2                	cmp    %eax,%edx
    30e7:	72 5d                	jb     3146 <fgetc+0xd5>
    30e9:	8b 45 08             	mov    0x8(%ebp),%eax
    30ec:	8b 40 14             	mov    0x14(%eax),%eax
    30ef:	89 c1                	mov    %eax,%ecx
    30f1:	8b 45 08             	mov    0x8(%ebp),%eax
    30f4:	8b 40 10             	mov    0x10(%eax),%eax
    30f7:	89 c2                	mov    %eax,%edx
    30f9:	8b 45 08             	mov    0x8(%ebp),%eax
    30fc:	8b 00                	mov    (%eax),%eax
    30fe:	51                   	push   %ecx
    30ff:	52                   	push   %edx
    3100:	50                   	push   %eax
    3101:	6a 1e                	push   $0x1e
    3103:	e8 58 0c 00 00       	call   3d60 <syscall@plt>
    3108:	83 c4 10             	add    $0x10,%esp
    310b:	89 45 f4             	mov    %eax,-0xc(%ebp)
    310e:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    3112:	7f 1f                	jg     3133 <fgetc+0xc2>
    3114:	8b 45 08             	mov    0x8(%ebp),%eax
    3117:	c7 40 08 01 00 00 00 	movl   $0x1,0x8(%eax)
    311e:	83 ec 0c             	sub    $0xc,%esp
    3121:	ff 75 08             	push   0x8(%ebp)
    3124:	e8 97 0c 00 00       	call   3dc0 <funlockfile@plt>
    3129:	83 c4 10             	add    $0x10,%esp
    312c:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    3131:	eb 44                	jmp    3177 <fgetc+0x106>
    3133:	8b 55 f4             	mov    -0xc(%ebp),%edx
    3136:	8b 45 08             	mov    0x8(%ebp),%eax
    3139:	89 50 1c             	mov    %edx,0x1c(%eax)
    313c:	8b 45 08             	mov    0x8(%ebp),%eax
    313f:	c7 40 18 00 00 00 00 	movl   $0x0,0x18(%eax)
    3146:	8b 45 08             	mov    0x8(%ebp),%eax
    3149:	8b 70 10             	mov    0x10(%eax),%esi
    314c:	8b 45 08             	mov    0x8(%ebp),%eax
    314f:	8b 40 18             	mov    0x18(%eax),%eax
    3152:	8d 48 01             	lea    0x1(%eax),%ecx
    3155:	8b 55 08             	mov    0x8(%ebp),%edx
    3158:	89 4a 18             	mov    %ecx,0x18(%edx)
    315b:	01 f0                	add    %esi,%eax
    315d:	0f b6 00             	movzbl (%eax),%eax
    3160:	0f b6 c0             	movzbl %al,%eax
    3163:	89 45 f0             	mov    %eax,-0x10(%ebp)
    3166:	83 ec 0c             	sub    $0xc,%esp
    3169:	ff 75 08             	push   0x8(%ebp)
    316c:	e8 4f 0c 00 00       	call   3dc0 <funlockfile@plt>
    3171:	83 c4 10             	add    $0x10,%esp
    3174:	8b 45 f0             	mov    -0x10(%ebp),%eax
    3177:	8d 65 f8             	lea    -0x8(%ebp),%esp
    317a:	5b                   	pop    %ebx
    317b:	5e                   	pop    %esi
    317c:	5d                   	pop    %ebp
    317d:	c3                   	ret

0000317e <fputc>:
    317e:	55                   	push   %ebp
    317f:	89 e5                	mov    %esp,%ebp
    3181:	56                   	push   %esi
    3182:	53                   	push   %ebx
    3183:	83 ec 10             	sub    $0x10,%esp
    3186:	e8 70 e9 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    318b:	81 c3 b1 1a 00 00    	add    $0x1ab1,%ebx
    3191:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    3195:	75 0a                	jne    31a1 <fputc+0x23>
    3197:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    319c:	e9 ab 00 00 00       	jmp    324c <fputc+0xce>
    31a1:	83 ec 0c             	sub    $0xc,%esp
    31a4:	ff 75 0c             	push   0xc(%ebp)
    31a7:	e8 54 0c 00 00       	call   3e00 <flockfile@plt>
    31ac:	83 c4 10             	add    $0x10,%esp
    31af:	8b 45 0c             	mov    0xc(%ebp),%eax
    31b2:	8b 40 0c             	mov    0xc(%eax),%eax
    31b5:	85 c0                	test   %eax,%eax
    31b7:	75 0b                	jne    31c4 <fputc+0x46>
    31b9:	8b 45 0c             	mov    0xc(%ebp),%eax
    31bc:	8b 40 04             	mov    0x4(%eax),%eax
    31bf:	83 f8 02             	cmp    $0x2,%eax
    31c2:	74 15                	je     31d9 <fputc+0x5b>
    31c4:	83 ec 0c             	sub    $0xc,%esp
    31c7:	ff 75 0c             	push   0xc(%ebp)
    31ca:	e8 f1 0b 00 00       	call   3dc0 <funlockfile@plt>
    31cf:	83 c4 10             	add    $0x10,%esp
    31d2:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    31d7:	eb 73                	jmp    324c <fputc+0xce>
    31d9:	8b 45 0c             	mov    0xc(%ebp),%eax
    31dc:	8b 50 18             	mov    0x18(%eax),%edx
    31df:	8b 45 0c             	mov    0xc(%ebp),%eax
    31e2:	8b 40 14             	mov    0x14(%eax),%eax
    31e5:	39 c2                	cmp    %eax,%edx
    31e7:	72 2c                	jb     3215 <fputc+0x97>
    31e9:	83 ec 0c             	sub    $0xc,%esp
    31ec:	ff 75 0c             	push   0xc(%ebp)
    31ef:	e8 ed fc ff ff       	call   2ee1 <_ZL16_fflush_unlockedP4FILE>
    31f4:	83 c4 10             	add    $0x10,%esp
    31f7:	85 c0                	test   %eax,%eax
    31f9:	0f 95 c0             	setne  %al
    31fc:	84 c0                	test   %al,%al
    31fe:	74 15                	je     3215 <fputc+0x97>
    3200:	83 ec 0c             	sub    $0xc,%esp
    3203:	ff 75 0c             	push   0xc(%ebp)
    3206:	e8 b5 0b 00 00       	call   3dc0 <funlockfile@plt>
    320b:	83 c4 10             	add    $0x10,%esp
    320e:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
    3213:	eb 37                	jmp    324c <fputc+0xce>
    3215:	8b 45 08             	mov    0x8(%ebp),%eax
    3218:	88 45 f7             	mov    %al,-0x9(%ebp)
    321b:	8b 45 0c             	mov    0xc(%ebp),%eax
    321e:	8b 70 10             	mov    0x10(%eax),%esi
    3221:	8b 45 0c             	mov    0xc(%ebp),%eax
    3224:	8b 40 18             	mov    0x18(%eax),%eax
    3227:	8d 48 01             	lea    0x1(%eax),%ecx
    322a:	8b 55 0c             	mov    0xc(%ebp),%edx
    322d:	89 4a 18             	mov    %ecx,0x18(%edx)
    3230:	01 f0                	add    %esi,%eax
    3232:	0f b6 4d f7          	movzbl -0x9(%ebp),%ecx
    3236:	88 08                	mov    %cl,(%eax)
    3238:	83 ec 0c             	sub    $0xc,%esp
    323b:	ff 75 0c             	push   0xc(%ebp)
    323e:	e8 7d 0b 00 00       	call   3dc0 <funlockfile@plt>
    3243:	83 c4 10             	add    $0x10,%esp
    3246:	8b 45 08             	mov    0x8(%ebp),%eax
    3249:	0f b6 c0             	movzbl %al,%eax
    324c:	8d 65 f8             	lea    -0x8(%ebp),%esp
    324f:	5b                   	pop    %ebx
    3250:	5e                   	pop    %esi
    3251:	5d                   	pop    %ebp
    3252:	c3                   	ret

00003253 <fread>:
    3253:	55                   	push   %ebp
    3254:	89 e5                	mov    %esp,%ebp
    3256:	53                   	push   %ebx
    3257:	83 ec 24             	sub    $0x24,%esp
    325a:	e8 9c e8 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    325f:	81 c3 dd 19 00 00    	add    $0x19dd,%ebx
    3265:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    3269:	74 12                	je     327d <fread+0x2a>
    326b:	83 7d 14 00          	cmpl   $0x0,0x14(%ebp)
    326f:	74 0c                	je     327d <fread+0x2a>
    3271:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    3275:	74 06                	je     327d <fread+0x2a>
    3277:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    327b:	75 0a                	jne    3287 <fread+0x34>
    327d:	b8 00 00 00 00       	mov    $0x0,%eax
    3282:	e9 3d 01 00 00       	jmp    33c4 <fread+0x171>
    3287:	83 ec 0c             	sub    $0xc,%esp
    328a:	ff 75 14             	push   0x14(%ebp)
    328d:	e8 6e 0b 00 00       	call   3e00 <flockfile@plt>
    3292:	83 c4 10             	add    $0x10,%esp
    3295:	8b 45 14             	mov    0x14(%ebp),%eax
    3298:	8b 40 04             	mov    0x4(%eax),%eax
    329b:	83 f8 01             	cmp    $0x1,%eax
    329e:	74 18                	je     32b8 <fread+0x65>
    32a0:	83 ec 0c             	sub    $0xc,%esp
    32a3:	ff 75 14             	push   0x14(%ebp)
    32a6:	e8 15 0b 00 00       	call   3dc0 <funlockfile@plt>
    32ab:	83 c4 10             	add    $0x10,%esp
    32ae:	b8 00 00 00 00       	mov    $0x0,%eax
    32b3:	e9 0c 01 00 00       	jmp    33c4 <fread+0x171>
    32b8:	8b 45 0c             	mov    0xc(%ebp),%eax
    32bb:	0f af 45 10          	imul   0x10(%ebp),%eax
    32bf:	89 45 e8             	mov    %eax,-0x18(%ebp)
    32c2:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    32c9:	8b 45 08             	mov    0x8(%ebp),%eax
    32cc:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    32cf:	e9 cb 00 00 00       	jmp    339f <fread+0x14c>
    32d4:	8b 45 14             	mov    0x14(%ebp),%eax
    32d7:	8b 50 1c             	mov    0x1c(%eax),%edx
    32da:	8b 45 14             	mov    0x14(%ebp),%eax
    32dd:	8b 40 18             	mov    0x18(%eax),%eax
    32e0:	29 c2                	sub    %eax,%edx
    32e2:	89 55 e0             	mov    %edx,-0x20(%ebp)
    32e5:	83 7d e0 00          	cmpl   $0x0,-0x20(%ebp)
    32e9:	74 6a                	je     3355 <fread+0x102>
    32eb:	8b 45 e8             	mov    -0x18(%ebp),%eax
    32ee:	2b 45 f4             	sub    -0xc(%ebp),%eax
    32f1:	89 45 f0             	mov    %eax,-0x10(%ebp)
    32f4:	8b 45 f0             	mov    -0x10(%ebp),%eax
    32f7:	39 45 e0             	cmp    %eax,-0x20(%ebp)
    32fa:	73 06                	jae    3302 <fread+0xaf>
    32fc:	8b 45 e0             	mov    -0x20(%ebp),%eax
    32ff:	89 45 f0             	mov    %eax,-0x10(%ebp)
    3302:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
    3309:	eb 29                	jmp    3334 <fread+0xe1>
    330b:	8b 45 14             	mov    0x14(%ebp),%eax
    330e:	8b 50 10             	mov    0x10(%eax),%edx
    3311:	8b 45 14             	mov    0x14(%ebp),%eax
    3314:	8b 48 18             	mov    0x18(%eax),%ecx
    3317:	8b 45 ec             	mov    -0x14(%ebp),%eax
    331a:	01 c8                	add    %ecx,%eax
    331c:	01 d0                	add    %edx,%eax
    331e:	8b 4d f4             	mov    -0xc(%ebp),%ecx
    3321:	8b 55 ec             	mov    -0x14(%ebp),%edx
    3324:	01 d1                	add    %edx,%ecx
    3326:	8b 55 e4             	mov    -0x1c(%ebp),%edx
    3329:	01 ca                	add    %ecx,%edx
    332b:	0f b6 00             	movzbl (%eax),%eax
    332e:	88 02                	mov    %al,(%edx)
    3330:	83 45 ec 01          	addl   $0x1,-0x14(%ebp)
    3334:	8b 45 ec             	mov    -0x14(%ebp),%eax
    3337:	3b 45 f0             	cmp    -0x10(%ebp),%eax
    333a:	72 cf                	jb     330b <fread+0xb8>
    333c:	8b 45 f0             	mov    -0x10(%ebp),%eax
    333f:	01 45 f4             	add    %eax,-0xc(%ebp)
    3342:	8b 45 14             	mov    0x14(%ebp),%eax
    3345:	8b 50 18             	mov    0x18(%eax),%edx
    3348:	8b 45 f0             	mov    -0x10(%ebp),%eax
    334b:	01 c2                	add    %eax,%edx
    334d:	8b 45 14             	mov    0x14(%ebp),%eax
    3350:	89 50 18             	mov    %edx,0x18(%eax)
    3353:	eb 4a                	jmp    339f <fread+0x14c>
    3355:	8b 45 14             	mov    0x14(%ebp),%eax
    3358:	8b 40 14             	mov    0x14(%eax),%eax
    335b:	89 c1                	mov    %eax,%ecx
    335d:	8b 45 14             	mov    0x14(%ebp),%eax
    3360:	8b 40 10             	mov    0x10(%eax),%eax
    3363:	89 c2                	mov    %eax,%edx
    3365:	8b 45 14             	mov    0x14(%ebp),%eax
    3368:	8b 00                	mov    (%eax),%eax
    336a:	51                   	push   %ecx
    336b:	52                   	push   %edx
    336c:	50                   	push   %eax
    336d:	6a 1e                	push   $0x1e
    336f:	e8 ec 09 00 00       	call   3d60 <syscall@plt>
    3374:	83 c4 10             	add    $0x10,%esp
    3377:	89 45 dc             	mov    %eax,-0x24(%ebp)
    337a:	83 7d dc 00          	cmpl   $0x0,-0x24(%ebp)
    337e:	7f 0c                	jg     338c <fread+0x139>
    3380:	8b 45 14             	mov    0x14(%ebp),%eax
    3383:	c7 40 08 01 00 00 00 	movl   $0x1,0x8(%eax)
    338a:	eb 1f                	jmp    33ab <fread+0x158>
    338c:	8b 55 dc             	mov    -0x24(%ebp),%edx
    338f:	8b 45 14             	mov    0x14(%ebp),%eax
    3392:	89 50 1c             	mov    %edx,0x1c(%eax)
    3395:	8b 45 14             	mov    0x14(%ebp),%eax
    3398:	c7 40 18 00 00 00 00 	movl   $0x0,0x18(%eax)
    339f:	8b 45 f4             	mov    -0xc(%ebp),%eax
    33a2:	3b 45 e8             	cmp    -0x18(%ebp),%eax
    33a5:	0f 82 29 ff ff ff    	jb     32d4 <fread+0x81>
    33ab:	83 ec 0c             	sub    $0xc,%esp
    33ae:	ff 75 14             	push   0x14(%ebp)
    33b1:	e8 0a 0a 00 00       	call   3dc0 <funlockfile@plt>
    33b6:	83 c4 10             	add    $0x10,%esp
    33b9:	8b 45 f4             	mov    -0xc(%ebp),%eax
    33bc:	ba 00 00 00 00       	mov    $0x0,%edx
    33c1:	f7 75 0c             	divl   0xc(%ebp)
    33c4:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    33c7:	c9                   	leave
    33c8:	c3                   	ret

000033c9 <fwrite>:
    33c9:	55                   	push   %ebp
    33ca:	89 e5                	mov    %esp,%ebp
    33cc:	56                   	push   %esi
    33cd:	53                   	push   %ebx
    33ce:	83 ec 20             	sub    $0x20,%esp
    33d1:	e8 25 e7 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    33d6:	81 c3 66 18 00 00    	add    $0x1866,%ebx
    33dc:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    33e0:	74 12                	je     33f4 <fwrite+0x2b>
    33e2:	83 7d 14 00          	cmpl   $0x0,0x14(%ebp)
    33e6:	74 0c                	je     33f4 <fwrite+0x2b>
    33e8:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
    33ec:	74 06                	je     33f4 <fwrite+0x2b>
    33ee:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    33f2:	75 0a                	jne    33fe <fwrite+0x35>
    33f4:	b8 00 00 00 00       	mov    $0x0,%eax
    33f9:	e9 0e 01 00 00       	jmp    350c <fwrite+0x143>
    33fe:	83 ec 0c             	sub    $0xc,%esp
    3401:	ff 75 14             	push   0x14(%ebp)
    3404:	e8 f7 09 00 00       	call   3e00 <flockfile@plt>
    3409:	83 c4 10             	add    $0x10,%esp
    340c:	8b 45 14             	mov    0x14(%ebp),%eax
    340f:	8b 40 04             	mov    0x4(%eax),%eax
    3412:	83 f8 02             	cmp    $0x2,%eax
    3415:	74 18                	je     342f <fwrite+0x66>
    3417:	83 ec 0c             	sub    $0xc,%esp
    341a:	ff 75 14             	push   0x14(%ebp)
    341d:	e8 9e 09 00 00       	call   3dc0 <funlockfile@plt>
    3422:	83 c4 10             	add    $0x10,%esp
    3425:	b8 00 00 00 00       	mov    $0x0,%eax
    342a:	e9 dd 00 00 00       	jmp    350c <fwrite+0x143>
    342f:	8b 45 0c             	mov    0xc(%ebp),%eax
    3432:	0f af 45 10          	imul   0x10(%ebp),%eax
    3436:	89 45 e8             	mov    %eax,-0x18(%ebp)
    3439:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    3440:	8b 45 08             	mov    0x8(%ebp),%eax
    3443:	89 45 e4             	mov    %eax,-0x1c(%ebp)
    3446:	e9 99 00 00 00       	jmp    34e4 <fwrite+0x11b>
    344b:	8b 45 14             	mov    0x14(%ebp),%eax
    344e:	8b 50 14             	mov    0x14(%eax),%edx
    3451:	8b 45 14             	mov    0x14(%ebp),%eax
    3454:	8b 40 18             	mov    0x18(%eax),%eax
    3457:	29 c2                	sub    %eax,%edx
    3459:	89 55 e0             	mov    %edx,-0x20(%ebp)
    345c:	83 7d e0 00          	cmpl   $0x0,-0x20(%ebp)
    3460:	74 6b                	je     34cd <fwrite+0x104>
    3462:	8b 45 e8             	mov    -0x18(%ebp),%eax
    3465:	2b 45 f4             	sub    -0xc(%ebp),%eax
    3468:	89 45 f0             	mov    %eax,-0x10(%ebp)
    346b:	8b 45 f0             	mov    -0x10(%ebp),%eax
    346e:	39 45 e0             	cmp    %eax,-0x20(%ebp)
    3471:	73 06                	jae    3479 <fwrite+0xb0>
    3473:	8b 45 e0             	mov    -0x20(%ebp),%eax
    3476:	89 45 f0             	mov    %eax,-0x10(%ebp)
    3479:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
    3480:	eb 2a                	jmp    34ac <fwrite+0xe3>
    3482:	8b 55 f4             	mov    -0xc(%ebp),%edx
    3485:	8b 45 ec             	mov    -0x14(%ebp),%eax
    3488:	01 c2                	add    %eax,%edx
    348a:	8b 45 e4             	mov    -0x1c(%ebp),%eax
    348d:	8d 0c 02             	lea    (%edx,%eax,1),%ecx
    3490:	8b 45 14             	mov    0x14(%ebp),%eax
    3493:	8b 50 10             	mov    0x10(%eax),%edx
    3496:	8b 45 14             	mov    0x14(%ebp),%eax
    3499:	8b 70 18             	mov    0x18(%eax),%esi
    349c:	8b 45 ec             	mov    -0x14(%ebp),%eax
    349f:	01 f0                	add    %esi,%eax
    34a1:	01 c2                	add    %eax,%edx
    34a3:	0f b6 01             	movzbl (%ecx),%eax
    34a6:	88 02                	mov    %al,(%edx)
    34a8:	83 45 ec 01          	addl   $0x1,-0x14(%ebp)
    34ac:	8b 45 ec             	mov    -0x14(%ebp),%eax
    34af:	3b 45 f0             	cmp    -0x10(%ebp),%eax
    34b2:	72 ce                	jb     3482 <fwrite+0xb9>
    34b4:	8b 45 f0             	mov    -0x10(%ebp),%eax
    34b7:	01 45 f4             	add    %eax,-0xc(%ebp)
    34ba:	8b 45 14             	mov    0x14(%ebp),%eax
    34bd:	8b 50 18             	mov    0x18(%eax),%edx
    34c0:	8b 45 f0             	mov    -0x10(%ebp),%eax
    34c3:	01 c2                	add    %eax,%edx
    34c5:	8b 45 14             	mov    0x14(%ebp),%eax
    34c8:	89 50 18             	mov    %edx,0x18(%eax)
    34cb:	eb 17                	jmp    34e4 <fwrite+0x11b>
    34cd:	83 ec 0c             	sub    $0xc,%esp
    34d0:	ff 75 14             	push   0x14(%ebp)
    34d3:	e8 09 fa ff ff       	call   2ee1 <_ZL16_fflush_unlockedP4FILE>
    34d8:	83 c4 10             	add    $0x10,%esp
    34db:	85 c0                	test   %eax,%eax
    34dd:	0f 95 c0             	setne  %al
    34e0:	84 c0                	test   %al,%al
    34e2:	75 0e                	jne    34f2 <fwrite+0x129>
    34e4:	8b 45 f4             	mov    -0xc(%ebp),%eax
    34e7:	3b 45 e8             	cmp    -0x18(%ebp),%eax
    34ea:	0f 82 5b ff ff ff    	jb     344b <fwrite+0x82>
    34f0:	eb 01                	jmp    34f3 <fwrite+0x12a>
    34f2:	90                   	nop
    34f3:	83 ec 0c             	sub    $0xc,%esp
    34f6:	ff 75 14             	push   0x14(%ebp)
    34f9:	e8 c2 08 00 00       	call   3dc0 <funlockfile@plt>
    34fe:	83 c4 10             	add    $0x10,%esp
    3501:	8b 45 f4             	mov    -0xc(%ebp),%eax
    3504:	ba 00 00 00 00       	mov    $0x0,%edx
    3509:	f7 75 0c             	divl   0xc(%ebp)
    350c:	8d 65 f8             	lea    -0x8(%ebp),%esp
    350f:	5b                   	pop    %ebx
    3510:	5e                   	pop    %esi
    3511:	5d                   	pop    %ebp
    3512:	c3                   	ret

00003513 <feof>:
    3513:	55                   	push   %ebp
    3514:	89 e5                	mov    %esp,%ebp
    3516:	53                   	push   %ebx
    3517:	83 ec 14             	sub    $0x14,%esp
    351a:	e8 dc e5 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    351f:	81 c3 1d 17 00 00    	add    $0x171d,%ebx
    3525:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    3529:	75 07                	jne    3532 <feof+0x1f>
    352b:	b8 01 00 00 00       	mov    $0x1,%eax
    3530:	eb 28                	jmp    355a <feof+0x47>
    3532:	83 ec 0c             	sub    $0xc,%esp
    3535:	ff 75 08             	push   0x8(%ebp)
    3538:	e8 c3 08 00 00       	call   3e00 <flockfile@plt>
    353d:	83 c4 10             	add    $0x10,%esp
    3540:	8b 45 08             	mov    0x8(%ebp),%eax
    3543:	8b 40 08             	mov    0x8(%eax),%eax
    3546:	89 45 f4             	mov    %eax,-0xc(%ebp)
    3549:	83 ec 0c             	sub    $0xc,%esp
    354c:	ff 75 08             	push   0x8(%ebp)
    354f:	e8 6c 08 00 00       	call   3dc0 <funlockfile@plt>
    3554:	83 c4 10             	add    $0x10,%esp
    3557:	8b 45 f4             	mov    -0xc(%ebp),%eax
    355a:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    355d:	c9                   	leave
    355e:	c3                   	ret

0000355f <ferror>:
    355f:	55                   	push   %ebp
    3560:	89 e5                	mov    %esp,%ebp
    3562:	53                   	push   %ebx
    3563:	83 ec 14             	sub    $0x14,%esp
    3566:	e8 90 e5 ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    356b:	81 c3 d1 16 00 00    	add    $0x16d1,%ebx
    3571:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    3575:	75 07                	jne    357e <ferror+0x1f>
    3577:	b8 01 00 00 00       	mov    $0x1,%eax
    357c:	eb 28                	jmp    35a6 <ferror+0x47>
    357e:	83 ec 0c             	sub    $0xc,%esp
    3581:	ff 75 08             	push   0x8(%ebp)
    3584:	e8 77 08 00 00       	call   3e00 <flockfile@plt>
    3589:	83 c4 10             	add    $0x10,%esp
    358c:	8b 45 08             	mov    0x8(%ebp),%eax
    358f:	8b 40 0c             	mov    0xc(%eax),%eax
    3592:	89 45 f4             	mov    %eax,-0xc(%ebp)
    3595:	83 ec 0c             	sub    $0xc,%esp
    3598:	ff 75 08             	push   0x8(%ebp)
    359b:	e8 20 08 00 00       	call   3dc0 <funlockfile@plt>
    35a0:	83 c4 10             	add    $0x10,%esp
    35a3:	8b 45 f4             	mov    -0xc(%ebp),%eax
    35a6:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    35a9:	c9                   	leave
    35aa:	c3                   	ret

000035ab <exit>:
    35ab:	55                   	push   %ebp
    35ac:	89 e5                	mov    %esp,%ebp
    35ae:	53                   	push   %ebx
    35af:	83 ec 04             	sub    $0x4,%esp
    35b2:	e8 f4 da ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    35b7:	05 85 16 00 00       	add    $0x1685,%eax
    35bc:	83 ec 08             	sub    $0x8,%esp
    35bf:	ff 75 08             	push   0x8(%ebp)
    35c2:	6a 00                	push   $0x0
    35c4:	89 c3                	mov    %eax,%ebx
    35c6:	e8 95 07 00 00       	call   3d60 <syscall@plt>
    35cb:	83 c4 10             	add    $0x10,%esp
    35ce:	90                   	nop
    35cf:	eb fd                	jmp    35ce <exit+0x23>

000035d1 <abort>:
    35d1:	55                   	push   %ebp
    35d2:	89 e5                	mov    %esp,%ebp
    35d4:	53                   	push   %ebx
    35d5:	83 ec 04             	sub    $0x4,%esp
    35d8:	e8 ce da ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    35dd:	05 5f 16 00 00       	add    $0x165f,%eax
    35e2:	83 ec 0c             	sub    $0xc,%esp
    35e5:	6a 01                	push   $0x1
    35e7:	89 c3                	mov    %eax,%ebx
    35e9:	e8 32 08 00 00       	call   3e20 <exit@plt>
    35ee:	83 c4 10             	add    $0x10,%esp
    35f1:	90                   	nop
    35f2:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    35f5:	c9                   	leave
    35f6:	c3                   	ret

000035f7 <atoi>:
    35f7:	55                   	push   %ebp
    35f8:	89 e5                	mov    %esp,%ebp
    35fa:	83 ec 10             	sub    $0x10,%esp
    35fd:	e8 a9 da ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3602:	05 3a 16 00 00       	add    $0x163a,%eax
    3607:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
    360e:	c7 45 f8 01 00 00 00 	movl   $0x1,-0x8(%ebp)
    3615:	eb 04                	jmp    361b <atoi+0x24>
    3617:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    361b:	8b 45 08             	mov    0x8(%ebp),%eax
    361e:	0f b6 00             	movzbl (%eax),%eax
    3621:	3c 20                	cmp    $0x20,%al
    3623:	74 f2                	je     3617 <atoi+0x20>
    3625:	8b 45 08             	mov    0x8(%ebp),%eax
    3628:	0f b6 00             	movzbl (%eax),%eax
    362b:	3c 09                	cmp    $0x9,%al
    362d:	74 e8                	je     3617 <atoi+0x20>
    362f:	8b 45 08             	mov    0x8(%ebp),%eax
    3632:	0f b6 00             	movzbl (%eax),%eax
    3635:	3c 0a                	cmp    $0xa,%al
    3637:	74 de                	je     3617 <atoi+0x20>
    3639:	8b 45 08             	mov    0x8(%ebp),%eax
    363c:	0f b6 00             	movzbl (%eax),%eax
    363f:	3c 2d                	cmp    $0x2d,%al
    3641:	75 0d                	jne    3650 <atoi+0x59>
    3643:	c7 45 f8 ff ff ff ff 	movl   $0xffffffff,-0x8(%ebp)
    364a:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    364e:	eb 33                	jmp    3683 <atoi+0x8c>
    3650:	8b 45 08             	mov    0x8(%ebp),%eax
    3653:	0f b6 00             	movzbl (%eax),%eax
    3656:	3c 2b                	cmp    $0x2b,%al
    3658:	75 29                	jne    3683 <atoi+0x8c>
    365a:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    365e:	eb 23                	jmp    3683 <atoi+0x8c>
    3660:	8b 55 fc             	mov    -0x4(%ebp),%edx
    3663:	89 d0                	mov    %edx,%eax
    3665:	c1 e0 02             	shl    $0x2,%eax
    3668:	01 d0                	add    %edx,%eax
    366a:	01 c0                	add    %eax,%eax
    366c:	89 c2                	mov    %eax,%edx
    366e:	8b 45 08             	mov    0x8(%ebp),%eax
    3671:	0f b6 00             	movzbl (%eax),%eax
    3674:	0f be c0             	movsbl %al,%eax
    3677:	83 e8 30             	sub    $0x30,%eax
    367a:	01 d0                	add    %edx,%eax
    367c:	89 45 fc             	mov    %eax,-0x4(%ebp)
    367f:	83 45 08 01          	addl   $0x1,0x8(%ebp)
    3683:	8b 45 08             	mov    0x8(%ebp),%eax
    3686:	0f b6 00             	movzbl (%eax),%eax
    3689:	3c 2f                	cmp    $0x2f,%al
    368b:	7e 0a                	jle    3697 <atoi+0xa0>
    368d:	8b 45 08             	mov    0x8(%ebp),%eax
    3690:	0f b6 00             	movzbl (%eax),%eax
    3693:	3c 39                	cmp    $0x39,%al
    3695:	7e c9                	jle    3660 <atoi+0x69>
    3697:	8b 45 fc             	mov    -0x4(%ebp),%eax
    369a:	0f af 45 f8          	imul   -0x8(%ebp),%eax
    369e:	c9                   	leave
    369f:	c3                   	ret

000036a0 <_ZL14reverse_stringPci>:
    36a0:	55                   	push   %ebp
    36a1:	89 e5                	mov    %esp,%ebp
    36a3:	83 ec 10             	sub    $0x10,%esp
    36a6:	e8 00 da ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    36ab:	05 91 15 00 00       	add    $0x1591,%eax
    36b0:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
    36b7:	8b 45 0c             	mov    0xc(%ebp),%eax
    36ba:	83 e8 01             	sub    $0x1,%eax
    36bd:	89 45 f8             	mov    %eax,-0x8(%ebp)
    36c0:	eb 39                	jmp    36fb <_ZL14reverse_stringPci+0x5b>
    36c2:	8b 55 fc             	mov    -0x4(%ebp),%edx
    36c5:	8b 45 08             	mov    0x8(%ebp),%eax
    36c8:	01 d0                	add    %edx,%eax
    36ca:	0f b6 00             	movzbl (%eax),%eax
    36cd:	88 45 f7             	mov    %al,-0x9(%ebp)
    36d0:	8b 55 f8             	mov    -0x8(%ebp),%edx
    36d3:	8b 45 08             	mov    0x8(%ebp),%eax
    36d6:	01 d0                	add    %edx,%eax
    36d8:	8b 4d fc             	mov    -0x4(%ebp),%ecx
    36db:	8b 55 08             	mov    0x8(%ebp),%edx
    36de:	01 ca                	add    %ecx,%edx
    36e0:	0f b6 00             	movzbl (%eax),%eax
    36e3:	88 02                	mov    %al,(%edx)
    36e5:	8b 55 f8             	mov    -0x8(%ebp),%edx
    36e8:	8b 45 08             	mov    0x8(%ebp),%eax
    36eb:	01 c2                	add    %eax,%edx
    36ed:	0f b6 45 f7          	movzbl -0x9(%ebp),%eax
    36f1:	88 02                	mov    %al,(%edx)
    36f3:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
    36f7:	83 6d f8 01          	subl   $0x1,-0x8(%ebp)
    36fb:	8b 45 fc             	mov    -0x4(%ebp),%eax
    36fe:	3b 45 f8             	cmp    -0x8(%ebp),%eax
    3701:	7c bf                	jl     36c2 <_ZL14reverse_stringPci+0x22>
    3703:	90                   	nop
    3704:	90                   	nop
    3705:	c9                   	leave
    3706:	c3                   	ret

00003707 <itoa>:
    3707:	55                   	push   %ebp
    3708:	89 e5                	mov    %esp,%ebp
    370a:	53                   	push   %ebx
    370b:	83 ec 10             	sub    $0x10,%esp
    370e:	e8 98 d9 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3713:	05 29 15 00 00       	add    $0x1529,%eax
    3718:	83 7d 10 01          	cmpl   $0x1,0x10(%ebp)
    371c:	7e 06                	jle    3724 <itoa+0x1d>
    371e:	83 7d 10 24          	cmpl   $0x24,0x10(%ebp)
    3722:	7e 0e                	jle    3732 <itoa+0x2b>
    3724:	8b 45 0c             	mov    0xc(%ebp),%eax
    3727:	c6 00 00             	movb   $0x0,(%eax)
    372a:	8b 45 0c             	mov    0xc(%ebp),%eax
    372d:	e9 d9 00 00 00       	jmp    380b <itoa+0x104>
    3732:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
    3739:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    3740:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    3744:	75 24                	jne    376a <itoa+0x63>
    3746:	8b 4d 0c             	mov    0xc(%ebp),%ecx
    3749:	8b 45 f8             	mov    -0x8(%ebp),%eax
    374c:	8d 50 01             	lea    0x1(%eax),%edx
    374f:	89 55 f8             	mov    %edx,-0x8(%ebp)
    3752:	01 c8                	add    %ecx,%eax
    3754:	c6 00 30             	movb   $0x30,(%eax)
    3757:	8b 55 f8             	mov    -0x8(%ebp),%edx
    375a:	8b 45 0c             	mov    0xc(%ebp),%eax
    375d:	01 d0                	add    %edx,%eax
    375f:	c6 00 00             	movb   $0x0,(%eax)
    3762:	8b 45 0c             	mov    0xc(%ebp),%eax
    3765:	e9 a1 00 00 00       	jmp    380b <itoa+0x104>
    376a:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
    376e:	79 10                	jns    3780 <itoa+0x79>
    3770:	83 7d 10 0a          	cmpl   $0xa,0x10(%ebp)
    3774:	75 0a                	jne    3780 <itoa+0x79>
    3776:	c7 45 f4 01 00 00 00 	movl   $0x1,-0xc(%ebp)
    377d:	f7 5d 08             	negl   0x8(%ebp)
    3780:	8b 45 08             	mov    0x8(%ebp),%eax
    3783:	89 45 f0             	mov    %eax,-0x10(%ebp)
    3786:	eb 4a                	jmp    37d2 <itoa+0xcb>
    3788:	8b 4d 10             	mov    0x10(%ebp),%ecx
    378b:	8b 45 f0             	mov    -0x10(%ebp),%eax
    378e:	ba 00 00 00 00       	mov    $0x0,%edx
    3793:	f7 f1                	div    %ecx
    3795:	89 d0                	mov    %edx,%eax
    3797:	89 45 ec             	mov    %eax,-0x14(%ebp)
    379a:	83 7d ec 09          	cmpl   $0x9,-0x14(%ebp)
    379e:	7e 0a                	jle    37aa <itoa+0xa3>
    37a0:	8b 45 ec             	mov    -0x14(%ebp),%eax
    37a3:	83 c0 57             	add    $0x57,%eax
    37a6:	89 c3                	mov    %eax,%ebx
    37a8:	eb 08                	jmp    37b2 <itoa+0xab>
    37aa:	8b 45 ec             	mov    -0x14(%ebp),%eax
    37ad:	83 c0 30             	add    $0x30,%eax
    37b0:	89 c3                	mov    %eax,%ebx
    37b2:	8b 4d 0c             	mov    0xc(%ebp),%ecx
    37b5:	8b 45 f8             	mov    -0x8(%ebp),%eax
    37b8:	8d 50 01             	lea    0x1(%eax),%edx
    37bb:	89 55 f8             	mov    %edx,-0x8(%ebp)
    37be:	01 c8                	add    %ecx,%eax
    37c0:	88 18                	mov    %bl,(%eax)
    37c2:	8b 5d 10             	mov    0x10(%ebp),%ebx
    37c5:	8b 45 f0             	mov    -0x10(%ebp),%eax
    37c8:	ba 00 00 00 00       	mov    $0x0,%edx
    37cd:	f7 f3                	div    %ebx
    37cf:	89 45 f0             	mov    %eax,-0x10(%ebp)
    37d2:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
    37d6:	75 b0                	jne    3788 <itoa+0x81>
    37d8:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
    37dc:	74 11                	je     37ef <itoa+0xe8>
    37de:	8b 4d 0c             	mov    0xc(%ebp),%ecx
    37e1:	8b 45 f8             	mov    -0x8(%ebp),%eax
    37e4:	8d 50 01             	lea    0x1(%eax),%edx
    37e7:	89 55 f8             	mov    %edx,-0x8(%ebp)
    37ea:	01 c8                	add    %ecx,%eax
    37ec:	c6 00 2d             	movb   $0x2d,(%eax)
    37ef:	8b 55 f8             	mov    -0x8(%ebp),%edx
    37f2:	8b 45 0c             	mov    0xc(%ebp),%eax
    37f5:	01 d0                	add    %edx,%eax
    37f7:	c6 00 00             	movb   $0x0,(%eax)
    37fa:	ff 75 f8             	push   -0x8(%ebp)
    37fd:	ff 75 0c             	push   0xc(%ebp)
    3800:	e8 9b fe ff ff       	call   36a0 <_ZL14reverse_stringPci>
    3805:	83 c4 08             	add    $0x8,%esp
    3808:	8b 45 0c             	mov    0xc(%ebp),%eax
    380b:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    380e:	c9                   	leave
    380f:	c3                   	ret

00003810 <abs>:
    3810:	55                   	push   %ebp
    3811:	89 e5                	mov    %esp,%ebp
    3813:	e8 93 d8 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3818:	05 24 14 00 00       	add    $0x1424,%eax
    381d:	8b 45 08             	mov    0x8(%ebp),%eax
    3820:	89 c2                	mov    %eax,%edx
    3822:	f7 da                	neg    %edx
    3824:	0f 49 c2             	cmovns %edx,%eax
    3827:	5d                   	pop    %ebp
    3828:	c3                   	ret

00003829 <labs>:
    3829:	55                   	push   %ebp
    382a:	89 e5                	mov    %esp,%ebp
    382c:	e8 7a d8 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3831:	05 0b 14 00 00       	add    $0x140b,%eax
    3836:	8b 45 08             	mov    0x8(%ebp),%eax
    3839:	89 c2                	mov    %eax,%edx
    383b:	f7 da                	neg    %edx
    383d:	0f 49 c2             	cmovns %edx,%eax
    3840:	5d                   	pop    %ebp
    3841:	c3                   	ret

00003842 <div>:
    3842:	55                   	push   %ebp
    3843:	89 e5                	mov    %esp,%ebp
    3845:	e8 61 d8 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    384a:	05 f2 13 00 00       	add    $0x13f2,%eax
    384f:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    3853:	75 15                	jne    386a <div+0x28>
    3855:	8b 45 08             	mov    0x8(%ebp),%eax
    3858:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
    385e:	8b 45 08             	mov    0x8(%ebp),%eax
    3861:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
    3868:	eb 1c                	jmp    3886 <div+0x44>
    386a:	8b 45 0c             	mov    0xc(%ebp),%eax
    386d:	99                   	cltd
    386e:	f7 7d 10             	idivl  0x10(%ebp)
    3871:	89 c2                	mov    %eax,%edx
    3873:	8b 45 08             	mov    0x8(%ebp),%eax
    3876:	89 10                	mov    %edx,(%eax)
    3878:	8b 45 0c             	mov    0xc(%ebp),%eax
    387b:	99                   	cltd
    387c:	f7 7d 10             	idivl  0x10(%ebp)
    387f:	8b 45 08             	mov    0x8(%ebp),%eax
    3882:	89 50 04             	mov    %edx,0x4(%eax)
    3885:	90                   	nop
    3886:	8b 45 08             	mov    0x8(%ebp),%eax
    3889:	5d                   	pop    %ebp
    388a:	c2 04 00             	ret    $0x4

0000388d <ldiv>:
    388d:	55                   	push   %ebp
    388e:	89 e5                	mov    %esp,%ebp
    3890:	e8 16 d8 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3895:	05 a7 13 00 00       	add    $0x13a7,%eax
    389a:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    389e:	75 15                	jne    38b5 <ldiv+0x28>
    38a0:	8b 45 08             	mov    0x8(%ebp),%eax
    38a3:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
    38a9:	8b 45 08             	mov    0x8(%ebp),%eax
    38ac:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
    38b3:	eb 1c                	jmp    38d1 <ldiv+0x44>
    38b5:	8b 45 0c             	mov    0xc(%ebp),%eax
    38b8:	99                   	cltd
    38b9:	f7 7d 10             	idivl  0x10(%ebp)
    38bc:	89 c2                	mov    %eax,%edx
    38be:	8b 45 08             	mov    0x8(%ebp),%eax
    38c1:	89 10                	mov    %edx,(%eax)
    38c3:	8b 45 0c             	mov    0xc(%ebp),%eax
    38c6:	99                   	cltd
    38c7:	f7 7d 10             	idivl  0x10(%ebp)
    38ca:	8b 45 08             	mov    0x8(%ebp),%eax
    38cd:	89 50 04             	mov    %edx,0x4(%eax)
    38d0:	90                   	nop
    38d1:	8b 45 08             	mov    0x8(%ebp),%eax
    38d4:	5d                   	pop    %ebp
    38d5:	c2 04 00             	ret    $0x4

000038d8 <rand>:
    38d8:	55                   	push   %ebp
    38d9:	89 e5                	mov    %esp,%ebp
    38db:	e8 cb d7 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    38e0:	05 5c 13 00 00       	add    $0x135c,%eax
    38e5:	8b 90 c4 03 00 00    	mov    0x3c4(%eax),%edx
    38eb:	69 d2 6d 4e c6 41    	imul   $0x41c64e6d,%edx,%edx
    38f1:	81 c2 39 30 00 00    	add    $0x3039,%edx
    38f7:	89 90 c4 03 00 00    	mov    %edx,0x3c4(%eax)
    38fd:	8b 80 c4 03 00 00    	mov    0x3c4(%eax),%eax
    3903:	c1 e8 10             	shr    $0x10,%eax
    3906:	25 ff 7f 00 00       	and    $0x7fff,%eax
    390b:	5d                   	pop    %ebp
    390c:	c3                   	ret

0000390d <srand>:
    390d:	55                   	push   %ebp
    390e:	89 e5                	mov    %esp,%ebp
    3910:	e8 96 d7 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3915:	05 27 13 00 00       	add    $0x1327,%eax
    391a:	8b 55 08             	mov    0x8(%ebp),%edx
    391d:	89 90 c4 03 00 00    	mov    %edx,0x3c4(%eax)
    3923:	90                   	nop
    3924:	5d                   	pop    %ebp
    3925:	c3                   	ret

00003926 <fork>:
    3926:	55                   	push   %ebp
    3927:	89 e5                	mov    %esp,%ebp
    3929:	53                   	push   %ebx
    392a:	83 ec 04             	sub    $0x4,%esp
    392d:	e8 79 d7 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3932:	05 0a 13 00 00       	add    $0x130a,%eax
    3937:	83 ec 0c             	sub    $0xc,%esp
    393a:	6a 22                	push   $0x22
    393c:	89 c3                	mov    %eax,%ebx
    393e:	e8 1d 04 00 00       	call   3d60 <syscall@plt>
    3943:	83 c4 10             	add    $0x10,%esp
    3946:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3949:	c9                   	leave
    394a:	c3                   	ret

0000394b <execve>:
    394b:	55                   	push   %ebp
    394c:	89 e5                	mov    %esp,%ebp
    394e:	53                   	push   %ebx
    394f:	83 ec 04             	sub    $0x4,%esp
    3952:	e8 54 d7 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3957:	05 e5 12 00 00       	add    $0x12e5,%eax
    395c:	8b 5d 10             	mov    0x10(%ebp),%ebx
    395f:	8b 4d 0c             	mov    0xc(%ebp),%ecx
    3962:	8b 55 08             	mov    0x8(%ebp),%edx
    3965:	53                   	push   %ebx
    3966:	51                   	push   %ecx
    3967:	52                   	push   %edx
    3968:	6a 23                	push   $0x23
    396a:	89 c3                	mov    %eax,%ebx
    396c:	e8 ef 03 00 00       	call   3d60 <syscall@plt>
    3971:	83 c4 10             	add    $0x10,%esp
    3974:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3977:	c9                   	leave
    3978:	c3                   	ret

00003979 <exec>:
    3979:	55                   	push   %ebp
    397a:	89 e5                	mov    %esp,%ebp
    397c:	53                   	push   %ebx
    397d:	83 ec 14             	sub    $0x14,%esp
    3980:	e8 26 d7 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3985:	05 b7 12 00 00       	add    $0x12b7,%eax
    398a:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
    3991:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    3998:	8b 55 08             	mov    0x8(%ebp),%edx
    399b:	89 55 f0             	mov    %edx,-0x10(%ebp)
    399e:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
    39a5:	83 ec 04             	sub    $0x4,%esp
    39a8:	8d 55 ec             	lea    -0x14(%ebp),%edx
    39ab:	52                   	push   %edx
    39ac:	8d 55 f0             	lea    -0x10(%ebp),%edx
    39af:	52                   	push   %edx
    39b0:	ff 75 08             	push   0x8(%ebp)
    39b3:	89 c3                	mov    %eax,%ebx
    39b5:	e8 b6 03 00 00       	call   3d70 <execve@plt>
    39ba:	83 c4 10             	add    $0x10,%esp
    39bd:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    39c0:	c9                   	leave
    39c1:	c3                   	ret

000039c2 <wait>:
    39c2:	55                   	push   %ebp
    39c3:	89 e5                	mov    %esp,%ebp
    39c5:	53                   	push   %ebx
    39c6:	83 ec 04             	sub    $0x4,%esp
    39c9:	e8 dd d6 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    39ce:	05 6e 12 00 00       	add    $0x126e,%eax
    39d3:	8b 55 08             	mov    0x8(%ebp),%edx
    39d6:	83 ec 08             	sub    $0x8,%esp
    39d9:	52                   	push   %edx
    39da:	6a 24                	push   $0x24
    39dc:	89 c3                	mov    %eax,%ebx
    39de:	e8 7d 03 00 00       	call   3d60 <syscall@plt>
    39e3:	83 c4 10             	add    $0x10,%esp
    39e6:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    39e9:	c9                   	leave
    39ea:	c3                   	ret

000039eb <fabs>:
    39eb:	55                   	push   %ebp
    39ec:	89 e5                	mov    %esp,%ebp
    39ee:	83 ec 18             	sub    $0x18,%esp
    39f1:	e8 b5 d6 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    39f6:	05 46 12 00 00       	add    $0x1246,%eax
    39fb:	8b 45 08             	mov    0x8(%ebp),%eax
    39fe:	89 45 e8             	mov    %eax,-0x18(%ebp)
    3a01:	8b 45 0c             	mov    0xc(%ebp),%eax
    3a04:	89 45 ec             	mov    %eax,-0x14(%ebp)
    3a07:	dd 45 e8             	fldl   -0x18(%ebp)
    3a0a:	d9 e1                	fabs
    3a0c:	dd 5d f8             	fstpl  -0x8(%ebp)
    3a0f:	dd 45 f8             	fldl   -0x8(%ebp)
    3a12:	c9                   	leave
    3a13:	c3                   	ret

00003a14 <fabsf>:
    3a14:	55                   	push   %ebp
    3a15:	89 e5                	mov    %esp,%ebp
    3a17:	83 ec 10             	sub    $0x10,%esp
    3a1a:	e8 8c d6 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3a1f:	05 1d 12 00 00       	add    $0x121d,%eax
    3a24:	d9 45 08             	flds   0x8(%ebp)
    3a27:	d9 e1                	fabs
    3a29:	d9 5d fc             	fstps  -0x4(%ebp)
    3a2c:	d9 45 fc             	flds   -0x4(%ebp)
    3a2f:	c9                   	leave
    3a30:	c3                   	ret

00003a31 <sqrt>:
    3a31:	55                   	push   %ebp
    3a32:	89 e5                	mov    %esp,%ebp
    3a34:	83 ec 18             	sub    $0x18,%esp
    3a37:	e8 6f d6 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3a3c:	05 00 12 00 00       	add    $0x1200,%eax
    3a41:	8b 45 08             	mov    0x8(%ebp),%eax
    3a44:	89 45 e8             	mov    %eax,-0x18(%ebp)
    3a47:	8b 45 0c             	mov    0xc(%ebp),%eax
    3a4a:	89 45 ec             	mov    %eax,-0x14(%ebp)
    3a4d:	dd 45 e8             	fldl   -0x18(%ebp)
    3a50:	d9 fa                	fsqrt
    3a52:	dd 5d f8             	fstpl  -0x8(%ebp)
    3a55:	dd 45 f8             	fldl   -0x8(%ebp)
    3a58:	c9                   	leave
    3a59:	c3                   	ret

00003a5a <sqrtf>:
    3a5a:	55                   	push   %ebp
    3a5b:	89 e5                	mov    %esp,%ebp
    3a5d:	83 ec 10             	sub    $0x10,%esp
    3a60:	e8 46 d6 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3a65:	05 d7 11 00 00       	add    $0x11d7,%eax
    3a6a:	d9 45 08             	flds   0x8(%ebp)
    3a6d:	d9 fa                	fsqrt
    3a6f:	d9 5d fc             	fstps  -0x4(%ebp)
    3a72:	d9 45 fc             	flds   -0x4(%ebp)
    3a75:	c9                   	leave
    3a76:	c3                   	ret

00003a77 <sin>:
    3a77:	55                   	push   %ebp
    3a78:	89 e5                	mov    %esp,%ebp
    3a7a:	83 ec 18             	sub    $0x18,%esp
    3a7d:	e8 29 d6 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3a82:	05 ba 11 00 00       	add    $0x11ba,%eax
    3a87:	8b 45 08             	mov    0x8(%ebp),%eax
    3a8a:	89 45 e8             	mov    %eax,-0x18(%ebp)
    3a8d:	8b 45 0c             	mov    0xc(%ebp),%eax
    3a90:	89 45 ec             	mov    %eax,-0x14(%ebp)
    3a93:	dd 45 e8             	fldl   -0x18(%ebp)
    3a96:	d9 fe                	fsin
    3a98:	dd 5d f8             	fstpl  -0x8(%ebp)
    3a9b:	dd 45 f8             	fldl   -0x8(%ebp)
    3a9e:	c9                   	leave
    3a9f:	c3                   	ret

00003aa0 <sinf>:
    3aa0:	55                   	push   %ebp
    3aa1:	89 e5                	mov    %esp,%ebp
    3aa3:	83 ec 10             	sub    $0x10,%esp
    3aa6:	e8 00 d6 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3aab:	05 91 11 00 00       	add    $0x1191,%eax
    3ab0:	d9 45 08             	flds   0x8(%ebp)
    3ab3:	d9 fe                	fsin
    3ab5:	d9 5d fc             	fstps  -0x4(%ebp)
    3ab8:	d9 45 fc             	flds   -0x4(%ebp)
    3abb:	c9                   	leave
    3abc:	c3                   	ret

00003abd <cos>:
    3abd:	55                   	push   %ebp
    3abe:	89 e5                	mov    %esp,%ebp
    3ac0:	83 ec 18             	sub    $0x18,%esp
    3ac3:	e8 e3 d5 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3ac8:	05 74 11 00 00       	add    $0x1174,%eax
    3acd:	8b 45 08             	mov    0x8(%ebp),%eax
    3ad0:	89 45 e8             	mov    %eax,-0x18(%ebp)
    3ad3:	8b 45 0c             	mov    0xc(%ebp),%eax
    3ad6:	89 45 ec             	mov    %eax,-0x14(%ebp)
    3ad9:	dd 45 e8             	fldl   -0x18(%ebp)
    3adc:	d9 ff                	fcos
    3ade:	dd 5d f8             	fstpl  -0x8(%ebp)
    3ae1:	dd 45 f8             	fldl   -0x8(%ebp)
    3ae4:	c9                   	leave
    3ae5:	c3                   	ret

00003ae6 <cosf>:
    3ae6:	55                   	push   %ebp
    3ae7:	89 e5                	mov    %esp,%ebp
    3ae9:	83 ec 10             	sub    $0x10,%esp
    3aec:	e8 ba d5 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3af1:	05 4b 11 00 00       	add    $0x114b,%eax
    3af6:	d9 45 08             	flds   0x8(%ebp)
    3af9:	d9 ff                	fcos
    3afb:	d9 5d fc             	fstps  -0x4(%ebp)
    3afe:	d9 45 fc             	flds   -0x4(%ebp)
    3b01:	c9                   	leave
    3b02:	c3                   	ret

00003b03 <pow>:
    3b03:	55                   	push   %ebp
    3b04:	89 e5                	mov    %esp,%ebp
    3b06:	83 ec 20             	sub    $0x20,%esp
    3b09:	e8 9d d5 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3b0e:	05 2e 11 00 00       	add    $0x112e,%eax
    3b13:	8b 55 08             	mov    0x8(%ebp),%edx
    3b16:	89 55 e8             	mov    %edx,-0x18(%ebp)
    3b19:	8b 55 0c             	mov    0xc(%ebp),%edx
    3b1c:	89 55 ec             	mov    %edx,-0x14(%ebp)
    3b1f:	8b 55 10             	mov    0x10(%ebp),%edx
    3b22:	89 55 e0             	mov    %edx,-0x20(%ebp)
    3b25:	8b 55 14             	mov    0x14(%ebp),%edx
    3b28:	89 55 e4             	mov    %edx,-0x1c(%ebp)
    3b2b:	dd 45 e8             	fldl   -0x18(%ebp)
    3b2e:	d9 ee                	fldz
    3b30:	df e9                	fucomip %st(1),%st
    3b32:	dd d8                	fstp   %st(0)
    3b34:	7a 0f                	jp     3b45 <pow+0x42>
    3b36:	dd 45 e8             	fldl   -0x18(%ebp)
    3b39:	d9 ee                	fldz
    3b3b:	df e9                	fucomip %st(1),%st
    3b3d:	dd d8                	fstp   %st(0)
    3b3f:	75 04                	jne    3b45 <pow+0x42>
    3b41:	d9 ee                	fldz
    3b43:	eb 3c                	jmp    3b81 <pow+0x7e>
    3b45:	dd 45 e0             	fldl   -0x20(%ebp)
    3b48:	d9 ee                	fldz
    3b4a:	df e9                	fucomip %st(1),%st
    3b4c:	dd d8                	fstp   %st(0)
    3b4e:	7a 0f                	jp     3b5f <pow+0x5c>
    3b50:	dd 45 e0             	fldl   -0x20(%ebp)
    3b53:	d9 ee                	fldz
    3b55:	df e9                	fucomip %st(1),%st
    3b57:	dd d8                	fstp   %st(0)
    3b59:	75 04                	jne    3b5f <pow+0x5c>
    3b5b:	d9 e8                	fld1
    3b5d:	eb 22                	jmp    3b81 <pow+0x7e>
    3b5f:	dd 45 e0             	fldl   -0x20(%ebp)
    3b62:	dd 45 e8             	fldl   -0x18(%ebp)
    3b65:	d9 f1                	fyl2x
    3b67:	d9 c0                	fld    %st(0)
    3b69:	d9 fc                	frndint
    3b6b:	d9 c9                	fxch   %st(1)
    3b6d:	d8 e1                	fsub   %st(1),%st
    3b6f:	d9 f0                	f2xm1
    3b71:	d9 e8                	fld1
    3b73:	de c1                	faddp  %st,%st(1)
    3b75:	d9 fd                	fscale
    3b77:	dd d9                	fstp   %st(1)
    3b79:	dd d9                	fstp   %st(1)
    3b7b:	dd 5d f8             	fstpl  -0x8(%ebp)
    3b7e:	dd 45 f8             	fldl   -0x8(%ebp)
    3b81:	c9                   	leave
    3b82:	c3                   	ret

00003b83 <powf>:
    3b83:	55                   	push   %ebp
    3b84:	89 e5                	mov    %esp,%ebp
    3b86:	53                   	push   %ebx
    3b87:	83 ec 14             	sub    $0x14,%esp
    3b8a:	e8 1c d5 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3b8f:	05 ad 10 00 00       	add    $0x10ad,%eax
    3b94:	d9 45 08             	flds   0x8(%ebp)
    3b97:	d9 45 0c             	flds   0xc(%ebp)
    3b9a:	8d 64 24 f8          	lea    -0x8(%esp),%esp
    3b9e:	dd 1c 24             	fstpl  (%esp)
    3ba1:	8d 64 24 f8          	lea    -0x8(%esp),%esp
    3ba5:	dd 1c 24             	fstpl  (%esp)
    3ba8:	89 c3                	mov    %eax,%ebx
    3baa:	e8 01 02 00 00       	call   3db0 <pow@plt>
    3baf:	83 c4 10             	add    $0x10,%esp
    3bb2:	d9 5d f4             	fstps  -0xc(%ebp)
    3bb5:	d9 45 f4             	flds   -0xc(%ebp)
    3bb8:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3bbb:	c9                   	leave
    3bbc:	c3                   	ret

00003bbd <__cxa_atexit>:
    3bbd:	55                   	push   %ebp
    3bbe:	89 e5                	mov    %esp,%ebp
    3bc0:	e8 e6 d4 ff ff       	call   10ab <__x86.get_pc_thunk.ax>
    3bc5:	05 77 10 00 00       	add    $0x1077,%eax
    3bca:	b8 00 00 00 00       	mov    $0x0,%eax
    3bcf:	5d                   	pop    %ebp
    3bd0:	c3                   	ret

00003bd1 <__cxa_pure_virtual>:
    3bd1:	55                   	push   %ebp
    3bd2:	89 e5                	mov    %esp,%ebp
    3bd4:	53                   	push   %ebx
    3bd5:	83 ec 04             	sub    $0x4,%esp
    3bd8:	e8 1e df ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    3bdd:	81 c3 5f 10 00 00    	add    $0x105f,%ebx
    3be3:	83 ec 0c             	sub    $0xc,%esp
    3be6:	8d 83 ac f2 ff ff    	lea    -0xd54(%ebx),%eax
    3bec:	50                   	push   %eax
    3bed:	e8 5e 01 00 00       	call   3d50 <printf@plt>
    3bf2:	83 c4 10             	add    $0x10,%esp
    3bf5:	83 ec 08             	sub    $0x8,%esp
    3bf8:	6a 01                	push   $0x1
    3bfa:	6a 00                	push   $0x0
    3bfc:	e8 5f 01 00 00       	call   3d60 <syscall@plt>
    3c01:	83 c4 10             	add    $0x10,%esp
    3c04:	90                   	nop
    3c05:	eb fd                	jmp    3c04 <__cxa_pure_virtual+0x33>

00003c07 <__libc_csu_init>:
    3c07:	55                   	push   %ebp
    3c08:	89 e5                	mov    %esp,%ebp
    3c0a:	53                   	push   %ebx
    3c0b:	83 ec 14             	sub    $0x14,%esp
    3c0e:	e8 e8 de ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    3c13:	81 c3 29 10 00 00    	add    $0x1029,%ebx
    3c19:	8d 83 c8 03 00 00    	lea    0x3c8(%ebx),%eax
    3c1f:	85 c0                	test   %eax,%eax
    3c21:	74 59                	je     3c7c <__libc_csu_init+0x75>
    3c23:	8d 83 c8 03 00 00    	lea    0x3c8(%ebx),%eax
    3c29:	85 c0                	test   %eax,%eax
    3c2b:	74 4f                	je     3c7c <__libc_csu_init+0x75>
    3c2d:	8d 83 c8 03 00 00    	lea    0x3c8(%ebx),%eax
    3c33:	89 c1                	mov    %eax,%ecx
    3c35:	8d 83 c8 03 00 00    	lea    0x3c8(%ebx),%eax
    3c3b:	89 c2                	mov    %eax,%edx
    3c3d:	89 c8                	mov    %ecx,%eax
    3c3f:	29 d0                	sub    %edx,%eax
    3c41:	c1 f8 02             	sar    $0x2,%eax
    3c44:	89 45 f0             	mov    %eax,-0x10(%ebp)
    3c47:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
    3c4e:	eb 22                	jmp    3c72 <__libc_csu_init+0x6b>
    3c50:	8d 83 c8 03 00 00    	lea    0x3c8(%ebx),%eax
    3c56:	8b 55 f4             	mov    -0xc(%ebp),%edx
    3c59:	8b 04 90             	mov    (%eax,%edx,4),%eax
    3c5c:	85 c0                	test   %eax,%eax
    3c5e:	74 0e                	je     3c6e <__libc_csu_init+0x67>
    3c60:	8d 83 c8 03 00 00    	lea    0x3c8(%ebx),%eax
    3c66:	8b 55 f4             	mov    -0xc(%ebp),%edx
    3c69:	8b 04 90             	mov    (%eax,%edx,4),%eax
    3c6c:	ff d0                	call   *%eax
    3c6e:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
    3c72:	8b 45 f4             	mov    -0xc(%ebp),%eax
    3c75:	3b 45 f0             	cmp    -0x10(%ebp),%eax
    3c78:	72 d6                	jb     3c50 <__libc_csu_init+0x49>
    3c7a:	eb 01                	jmp    3c7d <__libc_csu_init+0x76>
    3c7c:	90                   	nop
    3c7d:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3c80:	c9                   	leave
    3c81:	c3                   	ret

00003c82 <__libc_start_main>:
    3c82:	55                   	push   %ebp
    3c83:	89 e5                	mov    %esp,%ebp
    3c85:	53                   	push   %ebx
    3c86:	83 ec 14             	sub    $0x14,%esp
    3c89:	e8 6d de ff ff       	call   1afb <__x86.get_pc_thunk.bx>
    3c8e:	81 c3 ae 0f 00 00    	add    $0xfae,%ebx
    3c94:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
    3c98:	74 18                	je     3cb2 <__libc_start_main+0x30>
    3c9a:	8b 45 10             	mov    0x10(%ebp),%eax
    3c9d:	8b 00                	mov    (%eax),%eax
    3c9f:	85 c0                	test   %eax,%eax
    3ca1:	74 0f                	je     3cb2 <__libc_start_main+0x30>
    3ca3:	8b 45 10             	mov    0x10(%ebp),%eax
    3ca6:	8b 10                	mov    (%eax),%edx
    3ca8:	8b 83 fc ff ff ff    	mov    -0x4(%ebx),%eax
    3cae:	89 10                	mov    %edx,(%eax)
    3cb0:	eb 0e                	jmp    3cc0 <__libc_start_main+0x3e>
    3cb2:	8b 83 fc ff ff ff    	mov    -0x4(%ebx),%eax
    3cb8:	8d 93 df f2 ff ff    	lea    -0xd21(%ebx),%edx
    3cbe:	89 10                	mov    %edx,(%eax)
    3cc0:	8b 45 0c             	mov    0xc(%ebp),%eax
    3cc3:	83 c0 01             	add    $0x1,%eax
    3cc6:	8d 14 85 00 00 00 00 	lea    0x0(,%eax,4),%edx
    3ccd:	8b 45 10             	mov    0x10(%ebp),%eax
    3cd0:	01 c2                	add    %eax,%edx
    3cd2:	8b 83 f8 ff ff ff    	mov    -0x8(%ebx),%eax
    3cd8:	89 10                	mov    %edx,(%eax)
    3cda:	83 7d 14 00          	cmpl   $0x0,0x14(%ebp)
    3cde:	74 05                	je     3ce5 <__libc_start_main+0x63>
    3ce0:	8b 45 14             	mov    0x14(%ebp),%eax
    3ce3:	ff d0                	call   *%eax
    3ce5:	e8 f6 00 00 00       	call   3de0 <__libc_csu_init@plt>
    3cea:	8b 45 08             	mov    0x8(%ebp),%eax
    3ced:	8b 93 f8 ff ff ff    	mov    -0x8(%ebx),%edx
    3cf3:	8b 12                	mov    (%edx),%edx
    3cf5:	83 ec 04             	sub    $0x4,%esp
    3cf8:	52                   	push   %edx
    3cf9:	ff 75 10             	push   0x10(%ebp)
    3cfc:	ff 75 0c             	push   0xc(%ebp)
    3cff:	ff d0                	call   *%eax
    3d01:	83 c4 10             	add    $0x10,%esp
    3d04:	89 45 f4             	mov    %eax,-0xc(%ebp)
    3d07:	83 ec 0c             	sub    $0xc,%esp
    3d0a:	ff 75 f4             	push   -0xc(%ebp)
    3d0d:	e8 0e 01 00 00       	call   3e20 <exit@plt>
    3d12:	83 c4 10             	add    $0x10,%esp
    3d15:	8b 45 f4             	mov    -0xc(%ebp),%eax
    3d18:	8b 5d fc             	mov    -0x4(%ebp),%ebx
    3d1b:	c9                   	leave
    3d1c:	c3                   	ret

Disassembly of section .plt:

00003d20 <putchar@plt-0x10>:
    3d20:	ff b3 04 00 00 00    	push   0x4(%ebx)
    3d26:	ff a3 08 00 00 00    	jmp    *0x8(%ebx)
    3d2c:	00 00                	add    %al,(%eax)
	...

00003d30 <putchar@plt>:
    3d30:	ff a3 0c 00 00 00    	jmp    *0xc(%ebx)
    3d36:	68 00 00 00 00       	push   $0x0
    3d3b:	e9 e0 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003d40 <strcpy@plt>:
    3d40:	ff a3 10 00 00 00    	jmp    *0x10(%ebx)
    3d46:	68 08 00 00 00       	push   $0x8
    3d4b:	e9 d0 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003d50 <printf@plt>:
    3d50:	ff a3 14 00 00 00    	jmp    *0x14(%ebx)
    3d56:	68 10 00 00 00       	push   $0x10
    3d5b:	e9 c0 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003d60 <syscall@plt>:
    3d60:	ff a3 18 00 00 00    	jmp    *0x18(%ebx)
    3d66:	68 18 00 00 00       	push   $0x18
    3d6b:	e9 b0 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003d70 <execve@plt>:
    3d70:	ff a3 1c 00 00 00    	jmp    *0x1c(%ebx)
    3d76:	68 20 00 00 00       	push   $0x20
    3d7b:	e9 a0 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003d80 <memcpy@plt>:
    3d80:	ff a3 20 00 00 00    	jmp    *0x20(%ebx)
    3d86:	68 28 00 00 00       	push   $0x28
    3d8b:	e9 90 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003d90 <malloc@plt>:
    3d90:	ff a3 24 00 00 00    	jmp    *0x24(%ebx)
    3d96:	68 30 00 00 00       	push   $0x30
    3d9b:	e9 80 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003da0 <strncmp@plt>:
    3da0:	ff a3 28 00 00 00    	jmp    *0x28(%ebx)
    3da6:	68 38 00 00 00       	push   $0x38
    3dab:	e9 70 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003db0 <pow@plt>:
    3db0:	ff a3 2c 00 00 00    	jmp    *0x2c(%ebx)
    3db6:	68 40 00 00 00       	push   $0x40
    3dbb:	e9 60 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003dc0 <funlockfile@plt>:
    3dc0:	ff a3 30 00 00 00    	jmp    *0x30(%ebx)
    3dc6:	68 48 00 00 00       	push   $0x48
    3dcb:	e9 50 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003dd0 <memcmp@plt>:
    3dd0:	ff a3 34 00 00 00    	jmp    *0x34(%ebx)
    3dd6:	68 50 00 00 00       	push   $0x50
    3ddb:	e9 40 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003de0 <__libc_csu_init@plt>:
    3de0:	ff a3 38 00 00 00    	jmp    *0x38(%ebx)
    3de6:	68 58 00 00 00       	push   $0x58
    3deb:	e9 30 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003df0 <getchar@plt>:
    3df0:	ff a3 3c 00 00 00    	jmp    *0x3c(%ebx)
    3df6:	68 60 00 00 00       	push   $0x60
    3dfb:	e9 20 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003e00 <flockfile@plt>:
    3e00:	ff a3 40 00 00 00    	jmp    *0x40(%ebx)
    3e06:	68 68 00 00 00       	push   $0x68
    3e0b:	e9 10 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003e10 <__errno_location@plt>:
    3e10:	ff a3 44 00 00 00    	jmp    *0x44(%ebx)
    3e16:	68 70 00 00 00       	push   $0x70
    3e1b:	e9 00 ff ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003e20 <exit@plt>:
    3e20:	ff a3 48 00 00 00    	jmp    *0x48(%ebx)
    3e26:	68 78 00 00 00       	push   $0x78
    3e2b:	e9 f0 fe ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003e30 <strlen@plt>:
    3e30:	ff a3 4c 00 00 00    	jmp    *0x4c(%ebx)
    3e36:	68 80 00 00 00       	push   $0x80
    3e3b:	e9 e0 fe ff ff       	jmp    3d20 <__libc_start_main+0x9e>

00003e40 <free@plt>:
    3e40:	ff a3 50 00 00 00    	jmp    *0x50(%ebx)
    3e46:	68 88 00 00 00       	push   $0x88
    3e4b:	e9 d0 fe ff ff       	jmp    3d20 <__libc_start_main+0x9e>

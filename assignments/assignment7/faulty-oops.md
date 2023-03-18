Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x96000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=00000000420e1000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 96000045 [#1] SMP
Modules linked in: hello(O) faulty(O) scull(O)
CPU: 0 PID: 151 Comm: sh Tainted: G           O      5.15.18 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x14/0x20 [faulty]
lr : vfs_write+0xa8/0x2a0
sp : ffffffc008d13d80
x29: ffffffc008d13d80 x28: ffffff80020ea640 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040001000 x22: 000000000000000c x21: 00000055948f0a00
x20: 00000055948f0a00 x19: ffffff8002053600 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc0006f7000 x3 : ffffffc008d13df0
x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x14/0x20 [faulty]
 ksys_write+0x68/0x100
 __arm64_sys_write+0x20/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0x100
 do_el0_svc+0x44/0xb0
 el0_svc+0x28/0x80
 el0t_64_sync_handler+0xa4/0x130
 el0t_64_sync+0x1a0/0x1a4
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 191938470c5445f5 ]---

<b> objectdump: </b><br> 
0000000000000000 <faulty_write>:<br>
   0:	d503245f 	bti	c<br>
   4:	d2800001 	mov	x1, #0x0                   	// #0<br>
   8:	d2800000 	mov	x0, #0x0                   	// #0<br>
   c:	d503233f 	paciasp<br>
  10:	d50323bf 	autiasp<br>
  14:	b900003f 	str	wzr, [x1]<br>
  18:	d65f03c0 	ret<br>
  1c:	d503201f 	nop<br>
line 4: d2800001 mov x1, #0x0 ; assigns 0x0 to x1<br>
line 14: b900003f str wzr, [x1] ; attempts to access at content at dereference by x1 - ( dereferencing a NULL pointer)</br>

<b>Explanation for qemu reboot:</b><br>
Based on the call trace, it appears that the issue occurred during the execution of faulty_write. <br>
Upon disassembling the faulty.ko file, it was observed that the value 0 was being written to a memory address of 0x10, which is an invalid location. <br>
As a result, there is a risk of overwriting random memory and causing problems. This could lead to the qemu rebooting to resolve the error.<br>
</br>

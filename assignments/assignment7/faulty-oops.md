# Faulty Kernel Module Error  

## How did the error occur  

Run 'echo “hello_world” > /dev/faulty' from the command line of your running qemu image  

## Kernel Output  

``` 
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000  
Mem abort info:  
  ESR = 0x96000046  
  EC = 0x25: DABT (current EL), IL = 32 bits  
  SET = 0, FnV = 0  
  EA = 0, S1PTW = 0  
Data abort info:  
  ISV = 0, ISS = 0x00000046  
  CM = 0, WnR = 1  
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000042048000  
[0000000000000000] pgd=0000000042060003, p4d=0000000042060003, pud=0000000042060003, pmd=0000000000000000  
Internal error: Oops: 96000046 [#1] SMP  
Modules linked in: hello(O) scull(O) faulty(O)  
CPU: 0 PID: 152 Comm: sh Tainted: G           O      5.10.7 #1  
Hardware name: linux,dummy-virt (DT)  
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO BTYPE=--)  
pc : faulty_write+0x10/0x20 [faulty]  
lr : vfs_write+0xc0/0x290  
sp : ffffffc010c53db0  
x29: ffffffc010c53db0 x28: ffffff8001ff6400  
x27: 0000000000000000 x26: 0000000000000000  
x25: 0000000000000000 x24: 0000000000000000  
x23: 0000000000000000 x22: ffffffc010c53e30  
x21: 00000000004c9940 x20: ffffff8001f17f00  
x19: 0000000000000012 x18: 0000000000000000  
x17: 0000000000000000 x16: 0000000000000000  
x15: 0000000000000000 x14: 0000000000000000  
x13: 0000000000000000 x12: 0000000000000000  
x11: 0000000000000000 x10: 0000000000000000  
x9 : 0000000000000000 x8 : 0000000000000000  
x7 : 0000000000000000 x6 : 0000000000000000  
x5 : ffffff800201d7b8 x4 : ffffffc008670000  
x3 : ffffffc010c53e30 x2 : 0000000000000012  
x1 : 0000000000000000 x0 : 0000000000000000  
Call trace:  
 faulty_write+0x10/0x20 [faulty]  
 ksys_write+0x6c/0x100  
 __arm64_sys_write+0x1c/0x30  
 el0_svc_common.constprop.0+0x9c/0x1c0  
 do_el0_svc+0x70/0x90  
 el0_svc+0x14/0x20  
 el0_sync_handler+0xb0/0xc0  
 el0_sync+0x174/0x180  
Code: d2800001 d2800000 d503233f d50323bf (b900003f)   
---[ end trace 06d76f8526f855a9 ]---  
```

## Analysis of the kernel output  

An oops message is usually the outcome of deferencing a NULL pointer. All addresses used by the processor are virtual addresses and  
mapped to the physical addresses through a complex structure of page table. The paging mechanism fails to point to do this mapping when we  
dereference an invalid pointer (NULL in this case). The error has occured in the faulty_write function which can be found in misc-modules/faulty.c  

``` 
ssize_t faulty_write (struct file *filp, const char __user *buf, size_t count, loff_t *pos)  
{  
	/* make a simple fault by dereferencing a NULL pointer */  
	*(int *)0 = 0;  
	return 0;  
}  
```
The oops message displays the processor status at the time of the fault including contents of CPU registers. After displaying the message,  
the process is then killed.  

The first line `Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000` tells us that we were trying to deference  
a NULL pointer which is not permitted.  

`Internal error: Oops: 96000046 [#1] SMP`  
This is the error code value and each bit has its own significance. [#1] means that the oops error occured once  

`CPU: 0` indicates the CPU on which the error occured  
`Tainted: G` indicates that a proprietary module was loaded  

```
pc : faulty_write+0x10/0x20 [faulty]  
lr : vfs_write+0xc0/0x290  
sp : ffffffc010c53db0  
```
The above lines show us the values of the CPU registers including program counter, link register and stack pointer. The values of other general purpose  
registers can be seen below that.  
`pc : faulty_write+0x10/0x20 [faulty]`  
From the program counter we can see that the error occured in the faulty_write function inside the faulty.c file. 0x10 is the offset from faulty_write  
and 0x20 is the length.  

The call trace shows the list of functions being called just before the oops occured.  

`Code: d2800001 d2800000 d503233f d50323bf (b900003f)` is the hexdump of the section of machine code that was being run at the time the oops occured  

## References

- [Understanding a Kernel Oops](https://www.opensourceforu.com/2011/01/understanding-a-kernel-oops/#:~:text=An%20%E2%80%9COops%E2%80%9D%20is%20what%20the,of%20when%20the%20fault%20occurred.)  
- Book: Linux Device Drivers - Chapter 4 - Debugging Techniques  



#include <defs.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <kdebug.h>
#include <picirq.h>
#include <trap.h>
#include <clock.h>
#include <intr.h>
#include <pmm.h>
#include <vmm.h>
#include <proc.h>
#include <thumips_tlb.h>
#include <sched.h>

void setup_exception_vector()
{
  //for QEMU sim
  extern unsigned char __exception_vector, __exception_vector_end;
  memcpy((unsigned int*)0xBFC00000, &__exception_vector,
      &__exception_vector_end - &__exception_vector);
}

void test_usb()
{
#define HCCHIPID_MASK       0xff00
#define HCCHIPID_MAGIC      0x3600
#define HCCHIPID    0x27
    volatile unsigned int *ISP1362ADDR = 0xbc020004, *ISP1362DATA = 0xbc020000;
    unsigned int chipid;
    *ISP1362ADDR = HCCHIPID;
    chipid = *ISP1362DATA;

    kprintf("%s: Read chip ID %04x\n", __func__, chipid);
    if ((chipid & HCCHIPID_MASK) != HCCHIPID_MAGIC) {
        kprintf("%s: Invalid chip ID %04x\n", __func__, chipid);
        return;
    }
    kprintf("######## USB Chip OK ########\n");
}

void __noreturn
kern_init(void) {
    //setup_exception_vector();
    tlb_invalidate_all();
    clockOnTheWall_init();
    pic_init();                 // init interrupt controller
    cons_init();                // init the console
    clock_init();               // init clock interrupt

    check_initrd();

    const char *message = "(THU.CST) os is loading ...\n\n";
    kprintf(message);

    print_kerninfo();

#if 0
    kprintf("EX\n");
    __asm__ volatile("syscall");
    kprintf("EX RET\n");
#endif

    //test_usb();

    pmm_init();                 // init physical memory management

    vmm_init();                 // init virtual memory management
    sched_init();
    proc_init();                // init process table

    ide_init();
    fs_init();

    intr_enable();              // enable irq interrupt
    //*(int*)(0x00124) = 0x432;
    //asm volatile("divu $1, $1, $1");
    cpu_idle();
}


#include <thumips.h>
#include <trap.h>
#include <stdio.h>
#include <picirq.h>
#include <sched.h>
#include <asm/mipsregs.h>
#include <clock.h>

volatile size_t ticks;

#ifdef MACH_FPGA
static unsigned int initClockOnTheWall = 0;
#endif

#define TIMER0_INTERVAL  1000000 

static void reload_timer()
{
  uint32_t counter = read_c0_count();
  counter += TIMER0_INTERVAL;
  write_c0_compare(counter);
}

int clock_int_handler(void * data)
{
  ticks++;
//  if( (ticks & 0x1F) == 0)
//    cons_putc('A');
  run_timer_list();  
  reload_timer(); 
  return 0;
}

void
clock_init(void) {
  reload_timer(); 
  pic_enable(TIMER0_IRQ);
  kprintf("++setup timer interrupts\n");
}

void clockOnTheWall_init(void){
#ifdef MACH_QEMU
#elif defined MACH_FPGA
  initClockOnTheWall = getClockOnTheWall();
#else
#  warning please define MACH_QEMU or FPGA  
#endif
}

unsigned int getClockOnTheWall(void){
#ifdef MACH_QEMU
     return (unsigned int)ticks;
#elif defined MACH_FPGA
    return (unsigned int)*(volatile unsigned int*)0xbfd00500 - initClockOnTheWall;
#else
#  warning please define MACH_QEMU or FPGA 
    return 0;
#endif
}
#include <defs.h>
#include <mmu.h>
#include <sem.h>
#include <ide.h>
#include <inode.h>
#include <kmalloc.h>
#include <dev.h>
#include <vfs.h>
#include <iobuf.h>
#include <error.h>
#include <assert.h>
#include <stdio.h>
#include <kdebug.h>
#include "dev_flash.h"

#ifdef MACH_QEMU
static unsigned short emu_flash[FLASH_BLK_COUNT][FLASH_BLK_SIZE/2];
#endif

static semaphore_t flash_sem;

static void
lock_flash(void) {
    down(&(flash_sem));
}

static void
unlock_flash(void) {
    up(&(flash_sem));
}

static void flash_write_half(uint32_t flash_addr, uint32_t data)
{
    *((volatile uint32_t*)FLASH_BASE + flash_addr) = data;
}

static uint32_t flash_read_half(uint32_t flash_addr)
{
    return *((volatile uint32_t*)FLASH_BASE + flash_addr);
}

static void flash_wait_ready(void)
{
    do{
        flash_write_half(0, 0x70);
    }while(!(flash_read_half(0) & 0x80));
}

static void
flash_read_bytes_nolock(uint32_t addr, uint8_t* buffer, uint32_t size) {
    uint32_t i, j;
    kprintf("flash_read_bytes_nolock: %x\n", addr);

#ifndef MACH_QEMU
    flash_write_half(0, 0xff); //enter read mode
#endif

    if(addr & 1){
        j = addr/2;
#ifdef MACH_QEMU
        uint32_t data = ((unsigned short*)emu_flash)[j];
#else
        uint32_t data = flash_read_half(j);
#endif
        buffer[0] = (data>>8)&0xff;
        addr++;
        buffer++;
        size--;
    }
    j = addr/2;
    for (i = 0; i < size/2; ++i, ++j)
    {
#ifdef MACH_QEMU
        uint32_t data = ((unsigned short*)emu_flash)[j];
#else
        uint32_t data = flash_read_half(j);
#endif
        buffer[i*2] = data&0xff;
        buffer[i*2+1] = (data>>8)&0xff;
    }
    if(size & 1){
        j = (addr+size-1)/2;
#ifdef MACH_QEMU
        uint32_t data = ((unsigned short*)emu_flash)[j];
#else
        uint32_t data = flash_read_half(j);
#endif
        buffer[size-1] = data&0xff;
    }
}

static void
flash_write_bytes_nolock(uint32_t addr, uint8_t* buffer, uint32_t size) {
    uint32_t i, j;

    kprintf("flash_write_bytes_nolock: %x\n", addr);

    if(addr & 1){
        j = addr/2;
#ifdef MACH_QEMU
        uint32_t data = ((unsigned short*)emu_flash)[j];
#else
        uint32_t data = flash_read_half(j);
#endif
        data = data&0xff | ((uint32_t)buffer[0] << 8);
#ifdef MACH_QEMU
        ((unsigned short*)emu_flash)[j] &= data;
#else
        flash_write_half(j, 0x40); //word program
        flash_write_half(j, data);
        flash_wait_ready();
#endif
        addr++;
        buffer++;
        size--;
    }
    j = addr/2;
    for (i = 0; i < size/2; ++i, ++j)
    {
#ifdef MACH_QEMU
        ((unsigned short*)emu_flash)[j] &=
            ((uint32_t)buffer[i*2+1]<<8)|buffer[i*2];
#else
        flash_write_half(j, 0x40); //word program
        flash_write_half(j, ((uint32_t)buffer[i*2+1]<<8)|buffer[i*2]);
        flash_wait_ready();
#endif
    }
    if(size & 1){
        j = (addr+size-1)/2;
#ifdef MACH_QEMU
        uint32_t data = ((unsigned short*)emu_flash)[j];
#else
        uint32_t data = flash_read_half(j);
#endif
        data = data&0xff00 | (buffer[size-1]);
#ifdef MACH_QEMU
        ((unsigned short*)emu_flash)[j] &= data;
#else
        flash_write_half(j, 0x40); //word program
        flash_write_half(j, data);
        flash_wait_ready();
#endif
    }
}

static void
flash_erase_blk_nolock(uint32_t blkno)
{
    kprintf("flash_erase_blk_nolock: %d\n", blkno);
#ifdef MACH_QEMU
    uint32_t i;
    for (i = 0; i < FLASH_BLK_SIZE/2; ++i)
    {
        emu_flash[blkno][i] = 0xffff;
    }
    return;
#endif

    //erase block
    flash_write_half(blkno, 0x20);
    flash_write_half(blkno, 0xD0);
    flash_wait_ready();
}

static uint8_t flash_probe()
{
    uint32_t code;
    flash_write_half(0, 0x90);
    code = flash_read_half(0);
    kprintf("Manufacture Code %x\n", code);
    flash_write_half(0, 0x90);
    kprintf("Device Code %x\n", flash_read_half(1));
    return (code == 0x89);
}

static int
flash_open(struct device *dev, uint32_t open_flags) {
    return 0;
}

static int
flash_close(struct device *dev) {
    return 0;
}

static int
flash_io(struct device *dev, struct iobuf *iob, bool write) {
    off_t offset = iob->io_offset;
    size_t resid = iob->io_resid;

    /* don't allow I/O past the end of flash */
    if ((offset + resid)/FLASH_BLK_SIZE > dev->d_blocks) {
        return -E_INVAL;
    }

    /* read/write nothing ? */
    if (resid == 0) {
        return 0;
    }

    size_t copied;
    lock_flash();
    if(write)
        flash_write_bytes_nolock(offset, iob->io_base, resid);
    else
        flash_read_bytes_nolock(offset, iob->io_base, resid);
    unlock_flash();
    return 0;
}

static int
flash_ioctl(struct device *dev, int op, void *data) {
    lock_flash();
    if(op == FLASH_IOCTL_ERASE) {
        struct flash_ioctl_erase *p = (struct flash_ioctl_erase*)data;
        int i;
        for (i = 0; i < p->nblk; ++i)
            flash_erase_blk_nolock(p->blk + i);
        unlock_flash();
        return 0;
    }
    unlock_flash();
    return -E_UNIMP;
}

static void
flash_device_init(struct device *dev) {
    dev->d_blocks = FLASH_BLK_COUNT;
    dev->d_blocksize = FLASH_BLK_SIZE;
    dev->d_open = flash_open;
    dev->d_close = flash_close;
    dev->d_io = flash_io;
    dev->d_ioctl = flash_ioctl;
    sem_init(&(flash_sem), 1);
}

void dev_init_flash()
{
#ifndef MACH_QEMU
    uint8_t flash_exist = flash_probe();
#else
    uint8_t flash_exist = 1;
#endif
    if(!flash_exist){
        kprintf("flash device not exist!\n");
        return;
    }

    struct inode *node;
    if ((node = dev_create_inode()) == NULL) {
        panic("flash: dev_create_node.\n");
    }
    flash_device_init(vop_info(node, device));

    int ret;
    if ((ret = vfs_add_dev("flash", node, 1)) != 0) {
        panic("flash: vfs_add_dev: %e.\n", ret);
    }
}

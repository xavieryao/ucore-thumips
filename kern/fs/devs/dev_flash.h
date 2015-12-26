#ifndef DEV_FLASH_HH
#define DEV_FLASH_HH


/* 64K words per block, 2 bytes per word */
#define FLASH_BLK_SIZE (64*1024*2)
#define FLASH_BLK_COUNT 64
#define FLASH_TOTAL    (8*1024*1024)
#define FLASH_BASE ((volatile uint32_t *)0xbe000000)

#define FLASH_IOCTL_ERASE 0x55
struct flash_ioctl_erase
{
    int blk;
    int nblk;
};

#endif
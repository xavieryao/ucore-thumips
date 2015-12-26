#include "spiffs.h"
#include "spiffs_inode.h"
#include <iobuf.h>
#include <dev.h>
#include <error.h>
#include <assert.h>
#include <vfs.h>
#include <inode.h>

static struct device *disk_dev;

static u8_t spiffs_work_buf[SPIFFS_CFG_LOG_PAGE_SZ() * 2];
static u8_t spiffs_fds[32 * 4];
static u8_t spiffs_cache_buf[(SPIFFS_CFG_LOG_PAGE_SZ() + 32) * 4];

static s32_t spiffs_ucore_read(u32_t addr, u32_t size, u8_t *dst)
{
    kprintf("spiffs_ucore_read addr=%x size=%d\n", addr, size);
    struct iobuf __iob, *iob = iobuf_init(&__iob, dst, size, addr);
    dop_io(disk_dev, iob, 0);
    return SPIFFS_OK;
}

static s32_t spiffs_ucore_write(u32_t addr, u32_t size, u8_t *src)
{
    kprintf("spiffs_ucore_write addr=%x size=%d\n", addr, size);
    struct iobuf __iob, *iob = iobuf_init(&__iob, src, size, addr);
    dop_io(disk_dev, iob, 1);
    return SPIFFS_OK;
}

static s32_t spiffs_ucore_erase(u32_t addr, u32_t size)
{
    assert(addr%FLASH_BLK_SIZE == 0);
    assert(size%FLASH_BLK_SIZE == 0);
    struct flash_ioctl_erase p = {addr/FLASH_BLK_SIZE, size/FLASH_BLK_SIZE};
    kprintf("spiffs_ucore_erase addr=%x size=%d\n", addr, size);
    dop_ioctl(disk_dev, FLASH_IOCTL_ERASE, &p);
    return SPIFFS_OK;
}

static int spiffs_sync(struct fs *fs)
{
    return 0;
}
static struct inode *spiffs_get_root(struct fs *fs)
{
    struct inode *node = NULL;
    spiffs_stat stat = {.type = SPIFFS_TYPE_DIR, .name="/"};
    spiffs_load_inode(fs, &node, stat);
    kprintf("root node = %x\n", node);
    return node;
}
static int spiffs_unmount(struct fs *fs)
{
    SPIFFS_unmount(fsop_info(fs, spiffs));
    return 0;
}
static void spiffs_cleanup(struct fs *fs)
{

}

static int spiffs_do_mount(struct device *dev, struct fs **fs_store)
{
    /* allocate fs structure */
    struct fs *fs;
    if ((fs = alloc_fs(spiffs)) == NULL) {
        return -E_NO_MEM;
    }
    spiffs *spi = fsop_info(fs, spiffs);

    spiffs_config cfg;
    // cfg.phys_size = 2 * 1024 * 1024; // use all spi flash
    // cfg.phys_addr = 0; // start spiffs at start of spi flash
    // cfg.phys_erase_block = 65536; // according to datasheet
    // cfg.log_block_size = 65536; // let us not complicate things
    // cfg.log_page_size = LOG_PAGE_SIZE; // as we said

    cfg.hal_read_f = spiffs_ucore_read;
    cfg.hal_write_f = spiffs_ucore_write;
    cfg.hal_erase_f = spiffs_ucore_erase;

    dop_open(dev, 0);
    disk_dev = dev;
    int res = SPIFFS_mount(spi,
                           &cfg,
                           spiffs_work_buf,
                           spiffs_fds,
                           sizeof(spiffs_fds),
                           spiffs_cache_buf,
                           sizeof(spiffs_cache_buf),
                           0);
    kprintf("mount res: %d\n", res);
    if(res < 0){
        kprintf("formating flash...");
        SPIFFS_format(spi);
        res = SPIFFS_mount(spi,
                           &cfg,
                           spiffs_work_buf,
                           spiffs_fds,
                           sizeof(spiffs_fds),
                           spiffs_cache_buf,
                           sizeof(spiffs_cache_buf),
                           0);
        kprintf("mount-2 res: %d\n", res);
        if(res < 0)
            return res;
        spiffs_stat stat;
        int fd = SPIFFS_open(spi, "/sh", SPIFFS_CREAT|SPIFFS_RDWR,0);
        SPIFFS_write(spi, fd, "hello", 4);
        kprintf("create fd: %d\n", fd);
        SPIFFS_fstat(spi, fd, &stat);
        kprintf("size=%d\n", stat.size);
        SPIFFS_close(spi, fd);
    }

    fs->fs_sync = spiffs_sync;
    fs->fs_get_root = spiffs_get_root;
    fs->fs_unmount = spiffs_unmount;
    fs->fs_cleanup = spiffs_cleanup;
    *fs_store = fs;

    return 0;
}

void spiffs_ucore_init(void)
{
    int ret;
    if ((ret =  vfs_mount("flash", spiffs_do_mount)) != 0) {
        panic("failed: spiffs_do_mount: %e.\n", ret);
    }
}

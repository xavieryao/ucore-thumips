#include "spiffs.h"
#include "spiffs_inode.h"
#include <iobuf.h>
#include <dev.h>
#include <error.h>
#include <assert.h>
#include <vfs.h>
#include <string.h>
#include <inode.h>
#include <stat.h>


static int spiffs_ucore_open(struct inode *node, uint32_t open_flags)
{
    kprintf(__func__);kprintf("node=%x\n", node);
    int res;
    struct spiffs_inode *fin = vop_info(node, spiffs_inode);
    spiffs *spi = fsop_info(vop_fs(node), spiffs);
    if(fin->stat.type == SPIFFS_TYPE_DIR){
        return NULL == SPIFFS_opendir(spi, fin->stat.name, &fin->dir) ? -E_NOENT : 0;
    }else{
        spiffs_flags flags = 0;
        if(open_flags & O_RDWR)
            flags = SPIFFS_RDWR;
        else if(open_flags & O_WRONLY)
            flags = SPIFFS_WRONLY;
        else
            flags = SPIFFS_RDONLY;
        if(open_flags & O_APPEND)
            flags |= SPIFFS_APPEND;
        res = SPIFFS_open(spi, fin->stat.name, flags, 0);
        kprintf("SPIFFS_open=%d\n", res);
        if(res >= 0){
            fin->fd = res;
            return 0;
        }else{
            return -E_NOENT;
        }
    }
}
static int spiffs_ucore_close(struct inode *node)
{
    kprintf(__func__);kprintf("node=%x\n", node);
    struct spiffs_inode *fin = vop_info(node, spiffs_inode);
    spiffs *spi = fsop_info(vop_fs(node), spiffs);
    if(fin->stat.type == SPIFFS_TYPE_DIR){
        return SPIFFS_closedir(&fin->dir) == 0 ? 0 : -E_INVAL;
    }else{
        int t = fin->fd;
        fin->fd = -1;
        return SPIFFS_close(spi, t) == 0 ? 0 : -E_INVAL;
    }
}
static int spiffs_ucore_read(struct inode *node, struct iobuf *iob)
{
    kprintf(__func__);kprintf("node=%x\n", node);
    struct spiffs_inode *fin = vop_info(node, spiffs_inode);
    spiffs *spi = fsop_info(vop_fs(node), spiffs);
    kprintf("fd=%d io_offset=%d\n",fin->fd, iob->io_offset);
    int res = SPIFFS_lseek(spi, fin->fd, iob->io_offset, SPIFFS_SEEK_SET);
    kprintf("lseek=%d\n",res);
    // if(res < 0)
    //     return -E_SEEK;
    res = SPIFFS_read(spi, fin->fd, iob->io_base, iob->io_resid);
    kprintf("read=%d\n",res);
    if(res < 0)
        return -E_BUSY;
    if(res > 0){
        iob->io_base += res;
        iob->io_offset += res;
        iob->io_resid -= res;
    }
    return res;
}
static int spiffs_ucore_write(struct inode *node, struct iobuf *iob)
{
    kprintf(__func__);kprintf("node=%x\n", node);
    struct spiffs_inode *fin = vop_info(node, spiffs_inode);
    spiffs *spi = fsop_info(vop_fs(node), spiffs);
    kprintf("fd=%d io_offset=%d\n",fin->fd, iob->io_offset);
    int res = SPIFFS_lseek(spi, fin->fd, iob->io_offset, SPIFFS_SEEK_SET);
    kprintf("lseek=%d\n",res);
    // if(res < 0)
    //     return -E_SEEK;
    res = SPIFFS_write(spi, fin->fd, iob->io_base, iob->io_resid);
    kprintf("write=%d\n",res);
    if(res < 0)
        return -E_BUSY;
    if(res > 0){
        iob->io_base += res;
        iob->io_offset += res;
        iob->io_resid -= res;
    }
    return res;
}
static int spiffs_ucore_gettype(struct inode *node, uint32_t *type_store)
{
    kprintf(__func__);kprintf("node=%x\n", node);
    struct spiffs_inode *fin = vop_info(node, spiffs_inode);
    kprintf("type=%d\n", fin->stat.type);
    switch(fin->stat.type){
        case SPIFFS_TYPE_FILE:
            *type_store = S_IFREG;
            return 0;
        case SPIFFS_TYPE_DIR:
            *type_store = S_IFDIR;
            return 0;
        case SPIFFS_TYPE_SOFT_LINK:
            *type_store = S_IFLNK;
            return 0;
        default:
            panic("Unknown file type %d\n", fin->stat.type);
    }
    return -E_INVAL;
}
static int spiffs_ucore_fstat(struct inode *node, struct stat *st)
{
    kprintf(__func__);kprintf("node=%x\n", node);
    struct spiffs_inode *fin = vop_info(node, spiffs_inode);
    spiffs_ucore_gettype(node, &st->st_mode);
    st->st_size = fin->stat.size;
    st->st_nlinks = 1;
    st->st_blocks = 0;
    kprintf("mode=%d size=%d\n", st->st_mode, st->st_size);
    return 0;
}
static int spiffs_ucore_fsync(struct inode *node)
{
    kprintf(__func__);kprintf("node=%x\n", node);
    struct spiffs_inode *fin = vop_info(node, spiffs_inode);
    spiffs *spi = fsop_info(vop_fs(node), spiffs);
    return (fin->fd!=-1 && SPIFFS_fflush(spi, fin->fd) == 0) ? 0 : -E_INVAL;
}
static int spiffs_ucore_namefile(struct inode *node, struct iobuf *iob)
{
    kprintf(__func__);kprintf("node=%x\n", node);

}
static int spiffs_ucore_getdirentry(struct inode *node, struct iobuf *iob)
{
    kprintf(__func__);kprintf("node=%x\n", node);

}
static int spiffs_ucore_reclaim(struct inode *node)
{
    kprintf(__func__);kprintf("node=%x\n", node);
    vop_kill(node);
    return 0;
}
static int spiffs_ucore_tryseek(struct inode *node, off_t pos)
{
    kprintf(__func__);kprintf("node=%x\n", node);
    struct spiffs_inode *fin = vop_info(node, spiffs_inode);
    // spiffs *spi = fsop_info(vop_fs(node), spiffs);
    // int res = SPIFFS_lseek(spi, fin->fd, pos, SPIFFS_SEEK_SET);
    return pos > fin->stat.size ? -E_SEEK : 0;
}
static int spiffs_ucore_truncate(struct inode *node, off_t len)
{
    kprintf(__func__);kprintf("node=%x\n", node);

    return -E_UNIMP;
}
static int spiffs_ucore_create(struct inode *node, const char *name, bool excl, struct inode **node_store)
{
    kprintf(__func__);kprintf("node=%x\n", node);
    return -E_UNIMP;
}
static int spiffs_ucore_lookup(struct inode *node, char *path, struct inode **node_store)
{
    u8_t name[SPIFFS_OBJ_NAME_LEN];
    int res;
    spiffs_stat stat;
    kprintf(__func__);kprintf("node=%x\n", node);
    struct spiffs_inode *fin = vop_info(node, spiffs_inode);
    spiffs *spi = fsop_info(vop_fs(node), spiffs);
    size_t length = strlen(fin->stat.name);

    if(length + strlen(path) + 2 >= SPIFFS_OBJ_NAME_LEN)
        return -E_NOENT;
    memcpy(name, fin->stat.name, length);
    if(name[length-1] != '/'){
        name[length] = '/';
        length++;
    }
    strcpy(name+length, path);

    kprintf("%s\n", name);
    res = SPIFFS_stat(spi, name, &stat);
    kprintf("SPIFFS_stat=%d\n", res);
    if(res < 0)
        return -E_NOENT;
    res = spiffs_load_inode(vop_fs(node), node_store, stat);
    return res;
}
static const struct inode_ops spiffs_node_fileops = {
    .vop_magic                      = VOP_MAGIC,
    .vop_open                       = spiffs_ucore_open,
    .vop_close                      = spiffs_ucore_close,
    .vop_read                       = spiffs_ucore_read,
    .vop_write                      = spiffs_ucore_write,
    .vop_fstat                      = spiffs_ucore_fstat,
    .vop_fsync                      = spiffs_ucore_fsync,
    .vop_reclaim                    = spiffs_ucore_reclaim,
    .vop_gettype                    = spiffs_ucore_gettype,
    .vop_tryseek                    = spiffs_ucore_tryseek,
    .vop_truncate                   = spiffs_ucore_truncate,
    .vop_lookup                     = spiffs_ucore_lookup,
    .vop_namefile                   = spiffs_ucore_namefile,
    .vop_getdirentry                = spiffs_ucore_getdirentry
};

int spiffs_load_inode(struct fs* fs, struct inode **node_store, spiffs_stat stat)
{
    struct inode *node;
    int res;
    if ((node = alloc_inode(spiffs_inode)) != NULL) {
        vop_init(node, &spiffs_node_fileops, fs);
        struct spiffs_inode *sin = vop_info(node, spiffs_inode);
        sin->fd = -1;
        sin->stat = stat;
        // res = SPIFFS_stat(fsop_info(fs, spiffs), name, &sin->stat);
        // kprintf("SPIFFS_stat=%d\n", res);
        *node_store = node;
        return 0;
    }
    return -E_NO_MEM;
}

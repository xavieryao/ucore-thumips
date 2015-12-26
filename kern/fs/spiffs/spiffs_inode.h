#ifndef SPIFFS_INODE_H__
#define SPIFFS_INODE_H__

#include <spiffs.h>
#include <vfs.h>

struct spiffs_inode{
    spiffs_file fd;
    spiffs_stat stat;
    spiffs_DIR dir;
};

int spiffs_load_inode(struct fs* fs, struct inode **node_store, spiffs_stat stat);

#endif

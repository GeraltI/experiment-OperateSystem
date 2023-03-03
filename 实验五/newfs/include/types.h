
#include <stdbool.h>
#ifndef _TYPES_H_
#define _TYPES_H_
#define UINT8_BITS               8

#define NEWFS_MAGIC_NUM           0x20011005
#define NEWFS_MAX_FILE_NAME       128
#define NEWFS_SUPER_BLOCKS        1       // super超级块包含的逻辑块数量
#define NEWFS_MAP_INODE_BLOCKS    1       // inode位图包含的逻辑块数量
#define NEWFS_MAP_DATA_BLOCKS     1       // data位图包含的逻辑块数量
#define NEWFS_INODE_PER_FILE      1       // 每个inode最多对应的file文件数量
#define NEWFS_DATA_PER_FILE       8       // 每个file文件最多包含的数据块数量
#define NEWFS_BLOCK_SIZE          1024    // 逻辑块大小
#define NEWFS_SUPER_OFS           0       // 超级块起始位置
#define NEWFS_MAP_INODE_OFS       1024    // inode位图起始位置 0 + 1024   
#define NEWFS_MAP_DATA_OFS        2048    // data位图起始位置 1024 + 1 * 1024 
#define NEWFS_INODE_OFS           3072    // inode起始位置 2048 + 1 * 1024  
#define NEWFS_INODE_SIZE          32      // 每个inode的大小 每个块存1024 / 32 = 32个INODE 一共需要NEW_ROUND_UP(3968 * 32, 1024) / 1024 = 124块存取INODE
#define NEWFS_INODE_NUM           3968    // inode数量 ((4096 - 1 - 1 - 1) * 1024 / (1024 + 32) = 3968
#define NEWFS_DATA_OFS            130048  // data起始位置 3072 + 124 * 1024
#define NEWFS_DATA_SIZE           1024    // 每个数据块大小
#define NEWFS_DATA_NUM            3968    // 数据块数量 3968

#define NEWFS_ERROR_NONE          0
#define NEWFS_ERROR_ACCESS        EACCES
#define NEWFS_ERROR_SEEK          ESPIPE     
#define NEWFS_ERROR_ISDIR         EISDIR
#define NEWFS_ERROR_NOSPACE       ENOSPC
#define NEWFS_ERROR_EXISTS        EEXIST
#define NEWFS_ERROR_NOTFOUND      ENOENT
#define NEWFS_ERROR_UNSUPPORTED   ENXIO
#define NEWFS_ERROR_IO            EIO     /* Error Input/Output */
#define NEWFS_ERROR_INVAL         EINVAL  /* Invalid Args */

#define NEWFS_IOBLOCK_SZ()              (newfs_super.sz_io) // IO块大小
#define NEWFS_DISK_SZ()                 (newfs_super.sz_disk) // 磁盘容量大小
#define NEWFS_DRIVER()                  (newfs_super.driver_fd) // 磁盘号

#define NEWFS_ROUND_DOWN(value, round)    (value % round == 0 ? value : (value / round) * round) // 向下取整计算对应的逻辑块号
#define NEWFS_ROUND_UP(value, round)      (value % round == 0 ? value : (value / round + 1) * round) // 向上取整计算对应的逻辑块号
#define NEWFS_ASSIGN_FNAME(pnewfs_dentry, _fname)\
                                        memcpy(pnewfs_dentry->fname, _fname, strlen(_fname)) 
#define NEWFS_BLKS_SZ()                 (NEWFS_ROUND_UP(NEWFS_BLOCK_SIZE, NEWFS_IOBLOCK_SZ())) // 逻辑块大小                      
#define NEWFS_INO_OFS(ino)              (NEWFS_INODE_OFS + ino * NEWFS_INODE_SIZE) // 对应的inode位置
#define NEWFS_DA_OFS(data)              (NEWFS_DATA_OFS + data * NEWFS_DATA_PER_FILE * NEWFS_DATA_SIZE) // 对应的data位置
#define NEWFS_IS_DIR(pinode)            (pinode->dentry->ftype == NEWFS_DIR) // 是否是dir文件
#define NEWFS_IS_REG(pinode)            (pinode->dentry->ftype == NEWFS_FILE) // 是否是file文件
#define NEWFS_IS_SYM_LINK(pinode)       (pinode->dentry->ftype == NEWFS_SYM_LINK) // 是否是symlink文件

struct newfs_dentry;
struct newfs_inode;
struct newfs_super;

#define NEWFS_DBG(fmt, ...) do { printf("NEWFS_DBG: " fmt, ##__VA_ARGS__); } while(0) 

typedef enum file_type {
    NEWFS_FILE,           // 普通文件
    NEWFS_DIR,            // 目录文件
    NEWFS_SYM_LINK        // 链接文件
} NEWFS_FILE_TYPE;

struct custom_options {
	const char*        device;
};

struct newfs_super_d { 
    /**
    用于识别文件系统。
    比如说，如果实现的文件系统幻数为 0x20011005
    那么如果读到的幻数不等于 0x20011005
    则表示当前磁盘中无系统，系统损坏，或者是其他不识别的文件系统。
    */
    uint32_t            magic_num;                  // 幻数
    int                 sz_disk;                    //磁盘大小
    int                 sz_io;                      //物理磁盘块大小
    int                 driver_fd;                  

    /**
    最多支持的文件数。
    为简单起见，我们允许该文件系统具有文件数上限。
    */
    int                 max_ino;                    // 最多支持的inode数
    int                 max_data;                    // 最多支持的data数

    /**
    inode位图占用的块数。
    每个块的大小等于磁盘设备的IO大小（利用ddriver的ioctl获取）。
    */
    int                 map_inode_blks;             // inode位图占用的块数

    /**
    inode位图在磁盘上的偏移。
    通过map_inode_offset和map_inode_blks字段，便可读出整个inode位图。
    */
    int                 map_inode_offset;           // inode位图在磁盘上的偏移

    int                 map_data_blks;              // data位图占用的块数
    int                 map_data_offset;            // data位图在磁盘上的偏移
    int                 inode_offset;               // inode在磁盘上的偏移
    int                 data_offset;                // data在磁盘上的偏移

    int                 sz_usage;
};

struct newfs_inode_d {  // 32B
    int                 ino;                        // 在inode位图中的下标
    int                 size;                       // 文件已占用空间
    int                 link;                       // 链接数
    NEWFS_FILE_TYPE     ftype;                      // 文件类型（目录类型、普通文件类型）
    int                 dir_cnt;                    // 如果是目录类型文件，下面有几个目录项
    uint8_t*            data;                    // 数据块指针（可固定分配）
};

struct newfs_dentry_d { // 
    char                fname[NEWFS_MAX_FILE_NAME];       // 指向的ino文件名
    NEWFS_FILE_TYPE     ftype;                      // 指向的ino文件类型
    int                 ino;                        // 指向的ino号
    int                 valid;                      // 该目录项是否有效
};

struct newfs_super {   

    int                 sz_io;
    int                 sz_disk;
    int                 sz_usage;

    int                 driver_fd;
    int                 max_ino;                    // 最多支持的文件数
    int                 max_data;                   // 最多支持的数据数

    uint8_t*            map_inode;                  // inode位图
    int                 map_inode_blks;
    int                 map_inode_offset;
    uint8_t*            map_data;                   // data位图
    int                 map_data_blks;
    int                 map_data_offset;
    int                 inode_offset;               // inode在磁盘上的偏移
    int                 data_offset;                // data在磁盘上的偏移
    bool                is_mounted;

    struct newfs_dentry*root_dentry;                // 根目录dentry
};

struct newfs_inode {   
    int                 ino;                        // 在inode位图中的下标
    int                 size;                       // 文件已占用空间
    int                 dir_cnt;                    // 目录项数量
    struct newfs_dentry*dentry;                     // 指向该inode的dentry
    struct newfs_dentry*dentrys;                    // 所有目录项  
    uint8_t*            data;                    // 数据块指针
};

struct newfs_dentry {   
    char               fname[NEWFS_MAX_FILE_NAME];        // 指向的inode文件名
    NEWFS_FILE_TYPE    ftype;                       // 指向的inode文件类型
    struct newfs_dentry*parent;                     /* 父亲Inode的dentry */
    struct newfs_dentry*brother;                    /* 兄弟 */
    int                ino;                         // 指向的inode号
    struct newfs_inode*inode;                       /* 指向inode */
    int                valid;                       // 该目录项是否有效
};

#endif /* _TYPES_H_ */
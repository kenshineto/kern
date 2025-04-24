#include <stdint.h>
#include <stdbool.h>

#include <string.h>

// various errors with fat32
enum
{
    FAT_ERR_NONE  = 0,
    FAT_ERR_NOFAT = -1,
    FAT_ERR_BROKEN  = -2,
    FAT_ERR_IO    = -3,
    FAT_ERR_PARAM = -4,
    FAT_ERR_PATH   = -5,
    // also can tell if files don't exist.
    FAT_ERR_EOF   = -6,
    //
    FAT_ERR_DENIED = -7,
    FAT_ERR_FULL   = -8,
};

enum
{
  FAT_ATTR_NONE     = 0x00,
  FAT_ATTR_RO       = 0x01,
  FAT_ATTR_HIDDEN   = 0x02,
  FAT_ATTR_SYS      = 0x04,
  FAT_ATTR_LABEL    = 0x08,
  FAT_ATTR_DIR      = 0x10,
  FAT_ATTR_ARCHIVE  = 0x20,
  FAT_ATTR_LFN      = 0x0f,
};

enum
{
  FAT_WRITE      = 0x01, // Open file for writing
  FAT_READ       = 0x02, // Open file for reading
  FAT_APPEND     = 0x04, // Set file offset to the end of the file
  FAT_TRUNC      = 0x08, // Truncate the file after opening
  FAT_CREATE     = 0x10, // Create the file if it do not exist

  FAT_ACCESSED   = 0x20, // do not use (internal)
  FAT_MODIFIED   = 0x40, // do not use (internal)
  FAT_FILE_DIRTY = 0x80, // do not use (internal)
};

enum
{
  FAT_SEEK_START,
  FAT_SEEK_CURR,
  FAT_SEEK_END,
};


typedef struct
{
  bool (*read)(uint8_t* buf, uint32_t sect);
  bool (*write)(const uint8_t* buf, uint32_t sect);
} DiskOps;

// time checker if I can be bothered to implement it.
typedef struct
{
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint8_t day;
  uint8_t month;
  uint16_t year;
} Timestamp;

typedef struct Fat
{
  struct Fat* next;
  DiskOps ops;
  uint32_t clust_msk;
  uint32_t clust_cnt;
  uint32_t info_sect;
  uint32_t fat_sect[2];
  uint32_t data_sect;  
  uint32_t root_clust;
  uint32_t last_used;
  uint32_t free_cnt;
  uint32_t sect;
  uint8_t buf[512];
  uint8_t flags;
  uint8_t clust_shift;
  uint8_t name_len;
  char name[32];
} Fat;

typedef struct
{
  //Timestamp created;
  //Timestamp modified;
  uint32_t size;
  uint8_t attr;
  char name[255];
  uint8_t name_len;
} DirInfo;

typedef struct
{
  Fat* fat;
  uint32_t sclust;
  uint32_t clust;
  uint32_t sect;
  uint16_t idx;
} Dir;

// file structure
typedef struct
{
  Fat* fat;
  uint32_t dir_sect;
  uint32_t sclust;
  uint32_t clust;
  uint32_t sect;
  uint32_t size;
  uint32_t offset;
  uint16_t dir_idx;
  uint8_t attr;
  uint8_t flags;
  uint8_t buf[512];
} File;


typedef struct __attribute__((packed))
{
  uint8_t jump[3];
  char name[8];
  uint16_t bytes_per_sect;
  uint8_t sect_per_clust;
  uint16_t res_sect_cnt;
  uint8_t fat_cnt;
  uint16_t root_ent_cnt;
  uint16_t sect_cnt_16;
  uint8_t media;
  uint16_t sect_per_fat_16;
  uint16_t sect_per_track;
  uint16_t head_cnt;
  uint32_t hidden_sect_cnt;
  uint32_t sect_cnt_32;
  uint32_t sect_per_fat_32;
  uint16_t ext_flags;
  uint8_t minor;
  uint8_t major;
  uint32_t root_cluster;
  uint16_t info_sect;
  uint16_t copy_bpb_sector;
  uint8_t reserved_0[12];
  uint8_t drive_num;
  uint8_t reserved_1;
  uint8_t boot_sig;
  uint32_t volume_id;
  char volume_label[11];
  char fs_type[8];
  uint8_t reserved_2[420];
  uint8_t sign[2];
} Bpb;

typedef struct __attribute__((packed))
{
  uint32_t head_sig;
  uint8_t reserved_0[480];
  uint32_t struct_sig;
  uint32_t free_cnt;
  uint32_t next_free;
  uint8_t reserved_1[12];
  uint32_t tail_sig;
} FsInfo;

typedef struct __attribute__((packed))
{
  uint8_t name[11];
  uint8_t attr;
  uint8_t reserved;
  uint8_t tenth;
  uint16_t cre_time;
  uint16_t cre_date;
  uint16_t acc_date;
  uint16_t clust_hi;
  uint16_t mod_time;
  uint16_t mod_date;
  uint16_t clust_lo;
  uint32_t size;
} Sfn;

typedef union __attribute__((packed))
{
  uint8_t raw[32];
  struct
  {
    uint8_t seq;
    uint8_t name0[10];
    uint8_t attr;
    uint8_t type;
    uint8_t crc;
    uint8_t name1[12];
    uint16_t clust;
    uint8_t name2[4];
  };
} Lfn;

typedef struct
{
  uint32_t sect;
  uint16_t idx;
} Loc;


static Fat* g_fat_list;
static uint8_t g_buf[512];
static uint16_t g_len;
static uint8_t g_crc;

static int sync_buf(Fat* fat) {
    // checks whether the flag is active. If not, we 
    if(fat->flags & 0x01) {
        // if write fails for whatever reason
        if(!fat->ops.write(fat->buf, fat->sect)) {
            return FAT_ERR_IO;
        }
    }
}
static int update_buf(Fat* fat, uint32_t sect) {
    // if fat->sect already is sect, we are good
    if(fat->sect != sect) {

    }
}
// attempts to sync the filesystem to fat32. Returns error code assuming one happens.
static int sync_fs(Fat* fat)
{
    int error = sync_buf(fat);
    if (error) {
        return error;
    }
    FsInfo* info = (FsInfo*)fat->buf;
    fat->flags |= 

}

static int get_fat(Fat* fat, uint32_t clust, uint32_t* out_val, uint8_t* out_flags) {

}


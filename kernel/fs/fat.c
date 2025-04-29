#include <string.h>
//#include "fat.h"
#include <stdint.h>
#include <stdbool.h>
#include "comus/fs.h"

// Note that the structures came from strawberryhacker


#define FLAG_MIRROR (1 << 7)
#define FLAG_SECOND (0x01)

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
} fat_header;

typedef struct __attribute__((packed))
{
    uint32_t head_sig;
    uint8_t reserved_0[480];
    uint32_t struct_sig;
    uint32_t free_cnt;
    uint32_t next_free;
    uint8_t reserved_1[12];
    uint32_t tail_sig;
} file_system_info;




struct fat 
{
    //uint32_t placeholder;
    uint32_t fat_sect[2];
    uint8_t clust_shift;
    uint8_t data_sect;

};

static uint32_t sect_to_clust(struct fat* fat, uint32_t sect)
{
  return ((sect - fat->data_sect) >> fat->clust_shift) + 2;
}

//------------------------------------------------------------------------------
static uint32_t clust_to_sect(struct fat* fat, uint32_t clust)
{
  return ((clust - 2) << fat->clust_shift) + fat->data_sect;
}


int fat32_init(struct disk* disk, struct fat* fat) {
    fat_header hdr;
    file_system_info info;
    disk_read(disk, 0, sizeof(fat_header), &hdr);
    disk_read(disk, hdr.info_sect * 512, sizeof(file_system_info), &info);
    bool mirror = (hdr.ext_flags & FLAG_MIRROR) != 0;
    bool use_first = (hdr.ext_flags & FLAG_MIRROR) == 0;
    uint32_t fat0 = hdr.res_sect_cnt;
    uint32_t fat1 = hdr.res_sect_cnt + hdr.sect_per_fat_32;
    fat->fat_sect[0] = use_first ? fat0 : fat1;
    fat->fat_sect[1] = mirror ? (use_first ? fat1: fat0) : 0;
    fat->clust_shift = __builtin_ctz(hdr.sect_per_clust);
    fat->data_sect = hdr.res_sect_cnt + hdr.fat_cnt * hdr.sect_per_fat_32;

    
    // TODO: Read info into fat   

}




int create_directory() {
    return;
}

int create_file() {
    return;
}



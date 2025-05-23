#include <lib.h>
#include <comus/fs.h>
#include <comus/fs/tar.h>
#include <comus/mboot.h>
#include <comus/error.h>

struct disk fs_disks[N_DISKS];
struct file_system fs_loaded_file_systems[N_DISKS];

static void load_disks(void)
{
	size_t idx = 0;

	// check for initrd
	size_t rd_len;
	void *rd = mboot_get_initrd(&rd_len);
	if (rd != NULL) {
		assert(idx < N_DISKS, "Too many disks, limit is: %d", N_DISKS);
		fs_disks[idx] = (struct disk){
			.d_present = 1,
			.d_id = idx,
			.d_type = DISK_TYPE_RAMDISK,
			.rd.start = rd,
			.rd.len = rd_len,
		};
		idx++;
	}

	// check for ide/ata devices
	struct ide_devicelist ide_list = ide_devices_enumerate();
	for (size_t i = 0; i < ide_list.num_devices; i++) {
		assert(idx < N_DISKS, "Too many disks, limit is: %d", N_DISKS);
		fs_disks[idx] = (struct disk){
			.d_present = 1,
			.d_id = idx,
			.d_type = DISK_TYPE_ATA,
			.ide = ide_list.devices[i],
		};
		idx++;
	}

	INFO("loaded %zu disks", idx);
}

static void load_fs(struct disk *disk)
{
	struct file_system *fs;

	fs = &fs_loaded_file_systems[disk->d_id];
	fs->fs_disk = disk;
	fs->fs_id = disk->d_id;
	fs->fs_present = 1;

	// try tarfs
	if (tar_mount(fs) == SUCCESS)
		return;

	fs->fs_present = 0;
	WARN("failed to load fs on disk %u", disk->d_id);
}

void fs_init(void)
{
	// zero structures
	memsetv(fs_disks, 0, sizeof(fs_disks));
	memsetv(fs_loaded_file_systems, 0, sizeof(fs_loaded_file_systems));

	// load disks
	load_disks();

	// load fs's on disks
	for (size_t i = 0; i < N_DISKS; i++)
		if (fs_disks[i].d_present)
			load_fs(&fs_disks[i]);
}

struct disk *fs_get_root_disk(void)
{
	// NOTE: currently im just getting the first disk
	// found, is this fine?

	for (int i = 0; i < N_DISKS; i++) {
		struct disk *disk = &fs_disks[i];
		if (disk->d_present)
			return disk;
	}

	return NULL;
}

struct file_system *fs_get_root_file_system(void)
{
	// NOTE: currently im just getting the first file system
	// found, is this fine?

	for (int i = 0; i < N_DISKS; i++) {
		struct file_system *fs = &fs_loaded_file_systems[i];
		if (fs->fs_present)
			return fs;
	}

	return NULL;
}

int fs_find_file_abs(struct file_system *fs, char *abs_path, struct file *res)
{
	(void)fs;
	(void)abs_path;
	(void)res;

	panic("fs_find_file_abs NOT YET IMPLEMENTED");
}

int fs_find_file_rel(struct file *rel, char *rel_path, struct file *res)
{
	(void)rel;
	(void)rel_path;
	(void)res;

	panic("fs_find_file_rel NOT YET IMPLEMENTED");
}

static int disk_read_rd(struct disk *disk, size_t offset, size_t len,
						uint8_t *buffer)
{
	if (offset >= disk->rd.len) {
		WARN("attempted to read past length of ramdisk");
		return -E_BAD_PARAM;
	}

	if (disk->rd.len - offset < len)
		len = disk->rd.len - offset;

	memcpy(buffer, disk->rd.start + offset, len);
	return len;
}

static int disk_read_ata(struct disk *disk, size_t offset, size_t len,
						 uint8_t *buffer)
{
	static size_t atabuf_len = 0;
	static uint16_t *atabuf = NULL;

	uint32_t err = offset % ATA_SECT_SIZE;
	uint32_t numsects = (len + err + ATA_SECT_SIZE - 1) / ATA_SECT_SIZE;
	int ret;

	if (atabuf == NULL || atabuf_len < numsects * ATA_SECT_SIZE) {
		if ((atabuf = krealloc(atabuf, numsects * ATA_SECT_SIZE)) == NULL)
			return -E_NO_MEMORY;
		atabuf_len = numsects * ATA_SECT_SIZE;
	}

	// read sectors
	if ((ret = ide_device_read_sectors(disk->ide, numsects,
									   offset / ATA_SECT_SIZE, atabuf)))
		return ret;

	// copy over to buffer
	memcpy(buffer, (char *)atabuf + err, len);

	return len;
}

int disk_read(struct disk *disk, size_t offset, size_t len, void *buffer)
{
	int ret = 0;

	switch (disk->d_type) {
	case DISK_TYPE_RAMDISK:
		ret = disk_read_rd(disk, offset, len, buffer);
		break;
	case DISK_TYPE_ATA:
		ret = disk_read_ata(disk, offset, len, buffer);
		break;
	default:
		ERROR("attempted to read from disk with invalid type: %d\n",
			  disk->d_type);
		ret = -E_BAD_PARAM;
	}

	return ret;
}

static int disk_write_rd(struct disk *disk, size_t offset, size_t len,
						 uint8_t *buffer)
{
	if (offset >= disk->rd.len) {
		WARN("attempted to write past length of ramdisk");
		return -E_BAD_PARAM;
	}

	if (disk->rd.len - offset < len)
		len = disk->rd.len - offset;

	memcpy(disk->rd.start + offset, buffer, len);
	return len;
}

static int disk_write_ata(struct disk *disk, size_t offset, size_t len,
						  uint8_t *buffer)
{
	static size_t atabuf_len = 0;
	static uint16_t *atabuf = NULL;

	uint32_t numsects = (len + ATA_SECT_SIZE - 1) / ATA_SECT_SIZE;
	uint32_t err = offset % ATA_SECT_SIZE;
	int ret;

	if (atabuf == NULL || atabuf_len < numsects * ATA_SECT_SIZE) {
		if ((atabuf = krealloc(atabuf, numsects * ATA_SECT_SIZE)) == NULL)
			return -E_NO_MEMORY;
		atabuf_len = numsects * ATA_SECT_SIZE;
	}

	// read sectors what will be overwritten
	if ((ret = ide_device_read_sectors(disk->ide, numsects,
									   offset / ATA_SECT_SIZE, atabuf)))
		return ret;

	// copy custom data over
	memcpy((char *)atabuf + err, buffer, len);

	// write back sectors
	if ((ret = ide_device_write_sectors(disk->ide, numsects,
										offset / ATA_SECT_SIZE, atabuf)))
		return ret;

	return len;
}

int disk_write(struct disk *disk, size_t offset, size_t len, void *buffer)
{
	int ret = 0;

	switch (disk->d_type) {
	case DISK_TYPE_RAMDISK:
		ret = disk_write_rd(disk, offset, len, buffer);
		break;
	case DISK_TYPE_ATA:
		ret = disk_write_ata(disk, offset, len, buffer);
		break;
	default:
		ERROR("attempted to write to disk with invalid type: %d\n",
			  disk->d_type);
		ret = -E_BAD_PARAM;
	}

	return ret;
}

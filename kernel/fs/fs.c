#include <lib.h>
#include <comus/fs.h>

struct disk fs_disks[MAX_DISKS];
struct file_system fs_loaded_file_systems[MAX_DISKS];

void fs_init(void)
{
	// zero structures
	memsetv(fs_disks, 0, sizeof(fs_disks));
	memsetv(fs_loaded_file_systems, 0, sizeof(fs_loaded_file_systems));

	// TODO: go though ide and/or sata drivers to load all disks into `fs_disks` structures

	// TODO: go though each disk and attempt to load a file system
}

struct disk *fs_get_root_disk(void)
{
	// NOTE: currently im just getting the first disk
	// found, is this fine?

	for (int i = 0; i < MAX_DISKS; i++) {
		struct disk *disk = &fs_disks[i];
		if (disk->present)
			return disk;
	}

	return NULL;
}

struct file_system *fs_get_root_file_system(void)
{
	// NOTE: currently im just getting the first file system
	// found, is this fine?

	for (int i = 0; i < MAX_DISKS; i++) {
		struct file_system *fs = &fs_loaded_file_systems[i];
		if (fs->present)
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

int disk_read(struct disk *disk, size_t offset, size_t len, uint8_t *buffer)
{
	(void)disk;
	(void)offset;
	(void)len;
	(void)buffer;

	panic("disk_read NOT YET IMPLEMENTED");
}

int disk_write(struct disk *disk, size_t offset, size_t len, uint8_t *buffer)
{
	(void)disk;
	(void)offset;
	(void)len;
	(void)buffer;

	panic("disk_write NOT YET IMPLEMENTED");
}

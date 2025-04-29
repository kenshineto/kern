/**
 * @file fs.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Filesystem Structures
 */

#ifndef FS_H_
#define FS_H_

#include <stddef.h>
#include <comus/limits.h>
#include <comus/drivers/ata.h>

enum disk_type {
	DISK_TYPE_ATA,
	DISK_TYPE_RAMDISK,
};

struct disk {
	/// set to 1 in array to state that fs is defined
	/// system use only
	int present;
	/// index into disks array
	/// system use only
	int id;
	/// disk type
	enum disk_type type;
	/// internal disk device
	union {
		struct {
			char *start;
			size_t len;
		} rd;
		ide_device_t ide;
	};
};

/**
 * read data from a disk into a buffer
 *
 * @param disk - the disk to read from
 * @param offset - the offset into the disk to read
 * @param len - the length of the data to read into `buffer`
 * @param buffer - the buffer to save data into
 * @returns number of bytes read on success, negative fs error code in failure
 */
int disk_read(struct disk *disk, size_t offset, size_t len, void *buffer);

/**
 * write data from a disk into a buffer
 *
 * @param disk - the disk to write from
 * @param offset - the offset into the disk to write
 * @param len - the length of the data to write into `buffer`
 * @param buffer - the buffer to read from
 * @returns number of bytes written on success, negative fs error code in failure
 */
int disk_write(struct disk *disk, size_t offset, size_t len, void *buffer);

enum file_type {
	// regular file
	F_REG = 0,
	// directory
	F_DIR = 1,
};

struct dirent {
	int d_id;
	unsigned long d_offset;
	unsigned short d_namelen;
	char d_name[N_FILE_NAME];
};

struct stat {
	/// file id
	int s_id;
	/// file type
	enum file_type s_type;
	/// file length
	unsigned long s_length;
};

struct file {
	/// file id
	int f_id;
	/// read from the file
	int (*read)(struct file *, char *, size_t);
	/// write into the file
	int (*write)(struct file *, char *, size_t);
	/// seeks the file
	int (*seek)(struct file *, long);
	/// get directory entry at index
	int (*ents)(struct file *, struct dirent *, size_t);
};

struct file_system {
	/// set to 1 in fs array to state that fs is defined
	/// system use only
	int fs_present;
	/// index into the loaded filesystems array
	/// system use only
	int fs_id;
	/// mount point of this filesystem
	/// system use only
	char fs_mount[N_FILE_NAME];
	/// the disk this filesystem is hooked up to
	struct disk disk;
	/// opens a file
	int (*open)(const char *, struct file **);
	/// closes a file
	void (*close)(struct file *);
	/// stats a file
	int (*stat)(const char *, struct stat *);
};

// list of all disks on the system
extern struct disk fs_disks[N_DISKS];

// list of all loaded file systems for each disk
extern struct file_system fs_loaded_file_systems[N_DISKS];

/**
 * Initalize file system structures
 */
void fs_init(void);

/**
 * Gets the root disk
 *
 * @returns NULL on failure
 */
struct disk *fs_get_root_disk(void);

/**
 * Gets the root file system
 *
 * @returns NULL on failure
 */
struct file_system *fs_get_root_file_system(void);

#endif /* fs.h */

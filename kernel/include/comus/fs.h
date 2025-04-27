/**
 * @file fs.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Filesystem Structures
 */

#ifndef FS_H_
#define FS_H_

#include <stdint.h>
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
	// symbolic link
	F_SYM = 2,
};

/// TODO: file name queue or storage area???
/// hash map?!? (performance !!! :) )

struct file {
	/// name of the file
	char name[N_FILE_NAME];
	/// parent directory of the file
	struct file_s *parent;
	/// type of the file
	enum file_type type;
	/// the filesystem of this file
	struct file_system *fsys;
};

/// vtable for filesystem functions
/// NOTE: feel free to change this as needed
/// its just stuff that i thought may be good
struct file_system {
	/// set to 1 in fs array to state that fs is defined
	/// system use only
	int present;
	/// index into the loaded filesystems array
	/// system use only
	int id;
	/// the disk this filesystem is hooked up to
	struct disk disk;
	/// get root file in file file_system
	/// @param fs - the file system
	/// @returns the root file or NULL on failure
	struct file *(*fs_get_root_file)(struct file_system *fs);
	/// rename a file
	/// @param fs - the file system
	/// @param file - the file to rename
	/// @param name - the new file name
	/// @returns 0 on success, or an negative fs error code on failure
	int (*fs_rename_file)(struct file_system *fs, struct file *file,
						  char *name);
	/// get length of file
	/// @param fs - the file system
	/// @param file - the file to get the length of
	/// @param length - the pointer to save the length to
	/// @return 0 on success, or an negative fs error code on failure
	int (*fs_get_file_length)(struct file_system *fs, struct file *file,
							  uint64_t *length);
	/// get created date of file
	/// @param fs - the file system
	/// @param file - the file to get the date created
	/// @param created - the pointer to save the created date to
	/// @param modified - the pointer to save the modified date to
	/// @param accessed - the pointer to save the accessed date to
	/// @return 0 on success, or an negative fs error code on failure
	int (*fs_get_file_dates)(struct file_system *fs, struct file *file,
							 uint64_t *created, uint64_t *modified,
							 uint64_t *accessed);
	/// delete a file in the file system
	/// @param fs - the file system
	/// @param file - the file to delete
	/// @returns 0 on success, or an negative fs error code on failure
	int (*fs_delete_file)(struct file_system *fs, struct file *file);
	/// create a file with a given name and type
	/// @param fs - the file system
	/// @param res - the new file structure to save the new file into
	/// @param parent - the parent (directory) of the file to create
	/// @param type - the type of file to create
	/// @returns 0 on success, or an negative fs error code on failure
	int (*fs_new_file)(struct file_system *fs, struct file *res,
					   struct file *parent, enum file_type type);
	/// get files in a directory
	/// @param fs - the file system
	/// @param dir - the directory to search into
	/// @param start - the directory entry to start at
	/// @param len - the max number of entrys to save starting at `start`
	/// @param res - the list of structures to save into
	/// @returns number of entries read, or an negative fs error code on failure
	int (*fs_get_dir_ents)(struct file_system *fs, struct file *dir,
						   size_t start, size_t len, struct file res[]);
	/// read from a file
	/// @param fs - the file system
	/// @param file - the file to read from
	/// @param offset - the offset of the file to read into
	/// @param length - the length of the file to read starting at `offset`
	/// @param buffer - the buffer to save the data into
	/// @returns number of bytes read, or an negative fs error code on failure
	int (*fs_read_file)(struct file_system *fs, struct file *file,
						size_t offset, size_t length, uint8_t *buffer);
	/// write into a file
	/// @param fs - the file system
	/// @param file - the file to write to
	/// @param offset - the offset of the file to write into
	/// @param length - the length of the data to write
	/// @param buffer - the buffer the data to write is stored in
	/// @returns number of bytes written, or an negative fs error code on failure
	int (*fs_write_file)(struct file_system *fs, struct file *file,
						 size_t offset, size_t length, uint8_t *buffer);
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

/**
 * Find a file in the given file system, traversing the path
 * Always use this function to find files globally
 *
 * @param fs - the file system to search
 * @param name - the absolute path of the file to look for
 * @param res - where to store the file structure
 * @returns 0 on success, or an negative fs error code on failure
 */
int fs_find_file_abs(struct file_system *fs, char *abs_path, struct file *res);

/**
 * Find a file in the given file system, traversing the path, relative to
 * another file.
 * Always use this function to find files globally
 *
 * @param rel - the relative file to search from
 * @param name - the absolute path of the file to look for
 * @param res - where to store the file structure
 * @returns 0 on success, or an negative fs error code on failure
 */
int fs_find_file_rel(struct file *rel, char *rel_path, struct file *res);

// NOTE: fell free to add more functions if needed :)

#endif /* fs.h */

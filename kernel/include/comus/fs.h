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
#include <limits.h>

struct disk {
	/// set to 1 in array to state that fs is defined
	/// system use only
	int present;
	/// index into disks array
	/// system use only
	int id;
	/// TODO: pci? inb/outb?
	/// we need a structure to access disks and how to define them
	/// IDE / SATA / .... ???
	/// we should probable create ide_disk, or sata_disk, and so on
	/// then convert this part to a union with a tag specifier
	/// we then need drivers for ide and/or sata, ide is easier
};

enum file_type {
	// regular file
	F_REG = 0,
	// directory
	F_DIR = 1,
	// symbolic link
	F_SYM = 2,
};

struct file {
	/// name of the file
	char name[MAX_FILE_NAME_LEN];
	/// parent directory of the file
	struct file_s *parent;
	/// type of the file
	enum file_type type;
	/// times
	uint64_t created;
	uint64_t modified;
	uint64_t accessed;
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
	/// @returns the root file or NULL on failure
	struct file *(*fs_get_root_file)(void);
	/// update file on disk with provided file
	/// @param file - the file to update
	/// @returns 0 on success, or an negative fs error code on failure
	/// used for updating created, modified, access, and file name
	int (*fs_update_file)(struct file *file);
	/// delete a file in the file system
	/// @param file - the file to delete
	/// @returns 0 on success, or an negative fs error code on failure
	int (*fs_delete_file)(struct file *file);
	/// create a file with a given name and type
	/// @param res - the new file structure to save the new file into
	/// @param parent - the parent (directory) of the file to create
	/// @param type - the type of file to create
	/// @returns 0 on success, or an negative fs error code on failure
	int (*fs_new_file)(struct file **res, struct file *parent, enum file_type type);
	/// get files in a directory
	/// @param dir - the directory to search into
	/// @param start - the directory entry to start at
	/// @param len - the max number of entrys to save starting at `start`
	/// @param res - the list of structures to save into
	/// @returns number of entries read, or an negative fs error code on failure
	int (*fs_get_dir_ents)(struct file *dir, size_t start, size_t len, struct file **res);
	/// read from a file
	/// @param file - the file to read from
	/// @param offset - the offset of the file to read into
	/// @param length - the length of the file to read starting at `offset`
	/// @param buffer - the buffer to save the data into
	/// @returns number of bytes read, or an negative fs error code on failure
	int (*fs_read_file)(struct file *file, size_t offset, size_t length, uint8_t *buffer);
	/// write into a file (REPLACE ALL DATA)
	/// @param file - the file to write to
	/// @param length - the length of the data to write
	/// @param buffer - the buffer the data to write is stored in
	/// @returns number of bytes written, or an negative fs error code on failure
	int (*fs_write_file)(struct file *file, size_t length, uint8_t *buffer);
	/// append into a file (write data at end)
	/// @param file - the file to append to
	/// @param length - the length of the data to append
	/// @param buffer - the buffer the data to append is stored in
	/// @returns number of bytes written, or an negative fs error code on failure
	int (*fs_append_file)(struct file *file, size_t length, uint8_t *buffer);
	/// close a file in the filesystem
	/// @param file - the file to close
	/// @returns 0 on success, or an negative fs error code on failure
	int (*fs_close_file)(struct file *file);
	///
	/// !! PRIVATE FILE SYSTEM DATA MAY CHOOSE TO GO HERE !!
	///
};

// list of all disks on the system
extern struct disk fs_disks[MAX_DISKS];

// list of all loaded file systems for each disk
extern struct file_system fs_loaded_file_systems[MAX_DISKS];

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
 */
struct file *fs_find_file_abs(struct file_system *fs, char *abs_path);

/**
 * Find a file in the given file system, traversing the path, relative to
 * another file.
 * Always use this function to find files globally
 *
 * @param rel - the relative file to search from
 * @param name - the absolute path of the file to look for
 */
struct file *fs_find_file_rel(struct file *rel, char *rel_path);

// NOTE: fell free to add more functions if needed :)

#endif /* fs.h */

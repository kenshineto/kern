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
	int d_present;
	/// disk id
	int d_id;
	/// disk type
	enum disk_type d_type;
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

/// file type
enum file_type {
	/// regular file
	F_REG = 0,
	/// directory
	F_DIR = 1,
};

/// directory entry
struct dirent {
	/// file offset into directory
	unsigned long d_offset;
	/// file name len
	unsigned short d_namelen;
	/// file name
	char d_name[N_FILE_NAME];
};

/// file statistics
struct stat {
	/// file type
	enum file_type s_type;
	/// file length
	unsigned long s_length;
};

/// seek whence
enum {
	/// seek from start of file
	SEEK_SET = 0,
	/// seek from current position
	SEEK_CUR = 1,
	/// seek from end of file
	SEEK_END = 2,
};

/// generic file pointer type. file system open function
/// must extend this struct, allocate it on the heap,
/// and return it as a generic file pointer.
///
/// # Functions
///
/// read - read bytes from a opened file
/// write - write bytes to a opened file
/// seek - seek the open file
/// close - close an opened file (free pointer and other structures)
///
/// # Example FS Open
///
/// ```
/// struct example_file {
///     struct file file;
///     size_t offset; // custom
///     size_t sector; // data
/// };
///
/// int example_open(struct file_system *fs, const char *fullpath,
///         struct file **out)
/// {
///     struct example_file *file;
///
///     // do some checks here to get file info at full path
///     // header = example_locate(fs, fullpath); ??
///
///     file = kalloc(sizeof(struct example_file));
///     file->f_type = F_REG;
///     file->read = example_read;
///     file->write = example_write;
///     file->seek = example_seek;
///     file->ents = example_ents;
///     file->close = example_close; // frees pointer from kalloc
///     *out = (struct file *) *file;
///     return 0; // success
/// }
/// ```
///
struct file {
	/// file type
	enum file_type f_type;
	/// read from the file
	int (*read)(struct file *file, char *buffer, size_t nbytes);
	/// write into the file
	int (*write)(struct file *file, const char *buffer, size_t nbytes);
	/// seeks the file
	int (*seek)(struct file *file, long int offset, int whence);
	/// get directory entry at index
	int (*ents)(struct file *file, struct dirent *dirent, size_t dir_index);
	/// closes a file
	void (*close)(struct file *file);
};

/// open flags
enum {
	O_CREATE = 0x01,
	O_RDONLY = 0x02,
	O_WRONLY = 0x04,
	O_APPEND = 0x08,
	O_RDWR = 0x10,
};

/// file system vtable, used for opening
/// and stating files. filesystem mount functions must
/// set fs_name, fs_disk, open, and stat.
///
/// # Variables
///
/// fs_present - if this filesystem is present in `fs_loaded_file_systems`.
///            - this is set in fs.c. do not touch.
///
/// fs_id - the unique if for this filesystem.
///       - this is set in fs.c. do not touch.
///
/// fs_disk - the disk of this filesyste.
///         - this is set in fs.c. do not touch.
///
/// fs_name - the custom name for this filesystem. mount function is to set
///         - this.
///
/// # Functions
///
/// open - open a file at a given fullpath, returning the file pointer
///      - in out on success. see `struct file`
///
/// stat - get statistics on a file at fillpath
///
/// # Example FS Mount
///
/// // mount fs on disk in fs, present, id, & disk are already set
/// int example_mount(struct file_system *fs)
/// {
///     // do some checks to make sure fs on disk is valid
///     if (failure) {
///         return 1;
///     }
///
///     fs->fs_name = "examplefs";
///     fs->open = example_open; // see `struct file`
///     fs->stat = example_stat;
///     INFO("loaded examplefs on disk %u", fs->fs_disk->d_id);
///     return 0;
/// }
///
struct file_system {
	/// set to 1 in fs array to state that fs is defined
	int fs_present;
	/// filesystem id
	int fs_id;
	/// the disk this filesystem is hooked up to
	struct disk *fs_disk;
	/// filesystem name
	const char *fs_name;
	/// opens a file
	int (*open)(struct file_system *fs, const char *fullpath, int flags,
				struct file **out);
	/// stats a file
	int (*stat)(struct file_system *fs, const char *fullpath,
				struct stat *file);
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

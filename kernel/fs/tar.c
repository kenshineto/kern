#include <comus/fs.h>
#include <lib.h>
#include <comus/tar.h>

// the placements of these values mimics their placement in standard UStar
struct tar_header {
	char name[100]; // 0-99
	char mode[8]; // 100-107
	char ownerUID[8]; // 108-115
	char groupUID[8]; // 116-123
	char fileSize[12]; // 124-135
	char lastMod[12]; // 136-147
	char checksum[8]; // 148-155
	char type_flag; // 156
	char linked_name[100]; // 157-256
	char magic[6]; // 257-262
	char version[2]; // 263-264
	char username[32]; // 265-296
	char groupname[32]; // 297-328
	char majorNum[8]; // 329-336
	char minorNum[8]; // 337-344
	char prefix[155]; // 345-499
	char notUsed[12]; // 500-511
};
#define TAR_SIZE 512
#define TMAGIC "ustar" /* ustar and a null */
#define TMAGLEN 6
#define TVERSION "00" /* 00 and no null */
#define TVERSLEN 2
#define ERROR_TAR 1
#define NOERROR_TAR 0

#define REGTYPE '0' /* regular file */
#define DIRTYPE '5' /* directory */

struct tar_file {
	struct file file;
	struct file_system *fs;
	size_t len;
	size_t offset;
	size_t sect;
};
/// @brief reads from the disk into tar_header
/// @param disk the disk used with disk_read
/// @param sect the sector to read (used for disk_read offset)
/// @param hdr the tar_header to read into. read_tar_header is essentially used to output this.
/// @return NOERROR_TAR or ERROR_TAR
int read_tar_header(struct disk *disk, uint32_t sect, struct tar_header *hdr)
{
	// makes sure it reads enough bytes.
	if (disk_read(disk, sect * TAR_SIZE, TAR_SIZE, hdr) < TAR_SIZE) {
		return ERROR_TAR;
	}
	// makes sure magic and version are correct for USTar.
	if (memcmp(hdr->magic, TMAGIC, TMAGLEN) != 0 ||
		memcmp(hdr->version, TVERSION, TVERSLEN) != 0) {
		return ERROR_TAR;
	}
	return NOERROR_TAR;
}

/// @brief reads from the file, and puts it into buffer
/// @param f the file to be read
/// @param buffer the buffer that the content of the file is read into
/// @param len length of the buffer, which caps what can be read if the file has more data.
/// @return size of what was read.
int tar_read(struct file *f, void *buffer, size_t len)
{
	struct tar_file *tf = (struct tar_file *)f;
	long size = MIN((tf->len - tf->offset), len);
	if (tf->file.f_type != F_REG || size < 1) {
		return ERROR_TAR;
	}
	size = disk_read(tf->fs->fs_disk, (tf->sect + 1) * TAR_SIZE + tf->offset,
					 size, buffer);
	tf->offset += size;
	return size;
}
/// @brief used to locate files
/// @param fs the file system being used
/// @param filepath the path to the file
/// @param sect the sector to begin at, defaults to 0 if NULL.
/// @param sect_return the sector to be outputted
/// @param outHeader tar_header output for file found.
/// @return NOERROR_TAR or ERROR_TAR
int find_file(struct file_system *fs, const char *filepath, size_t *sect,
			  size_t *sect_return, struct tar_header *outHeader)
{
	struct tar_header hdr;
	size_t curr_sect;
	if (sect == NULL) {
		curr_sect = 0;
	} else {
		curr_sect = *sect;
	}
	while (1 == 1) {
		// if read_tar_header errors
		if (read_tar_header(fs->fs_disk, curr_sect, &hdr) != 0) {
			return ERROR_TAR;
		}
		if (memcmp(hdr.name, filepath, strlen(filepath) + 1) != 0) {
			// didn't find it.
			curr_sect +=
				((strtoull(hdr.fileSize, NULL, 8) + TAR_SIZE - 1) / TAR_SIZE) +
				1;
			continue;
		} else {
			// found it
			*outHeader = hdr;
			*sect_return = curr_sect;
			if (sect != NULL) {
				sect += curr_sect;
			}
			return NOERROR_TAR;
		}
	}
	return ERROR_TAR;
}

// similar to find_file, but with a partition
int find_file_redux(struct file_system *fs, const char *filepath, size_t *sect,
					size_t *sect_return, struct tar_header *outHeader)
{
	struct tar_header hdr;
	size_t curr_sect;
	if (sect == NULL) {
		curr_sect = 0;
	} else {
		curr_sect = *sect;
	}
	while (1 == 1) {
		if (read_tar_header(fs->fs_disk, curr_sect, &hdr) != 0) {
			return ERROR_TAR;
		}
		if (memcmp(hdr.name, filepath,
				   MIN(strlen(filepath), strlen(hdr.name))) != 0) {
			// didn't find it.

			curr_sect +=
				((strtoull(hdr.fileSize, NULL, 8) + TAR_SIZE - 1) / TAR_SIZE) +
				1;
			continue;
		} else {
			// found it
			*outHeader = hdr;
			*sect_return = curr_sect;
			if (sect != NULL) {
				sect += curr_sect +
						((strtoull(hdr.fileSize, NULL, 8) + TAR_SIZE - 1) /
						 TAR_SIZE) +
						1;
			}
			return NOERROR_TAR;
		}
	}
	return ERROR_TAR; // it should never actually reach here.
}
/// @brief closes the file
/// @param f the file to close, and free memory from.
void tar_close(struct file *f)
{
	kfree(f);
}

/// @brief does seek on the tar file
/// @param f the file to perform seek
/// @param offsetAdd what to add/adjust to the offset
/// @param theSeek what kind of seek we are doing
/// @return the new offset, or -1 on error (we can't use 1 since that is a possible offset change).
int tar_seek(struct file *f, long int offsetAdd, int theSeek)
{
	struct tar_file *tf = (struct tar_file *)f;
	if (theSeek == SEEK_SET) {
		tf->offset = offsetAdd;
		return tf->offset;
	} else if (theSeek == SEEK_CUR) {
		tf->offset = tf->offset + offsetAdd;
		return tf->offset;
	} else if (theSeek == SEEK_END) {
		tf->offset = tf->len + offsetAdd;
		return tf->offset;
	} else {
		return -1; // if seek has a bad value for some reason.
	}
}

/// @brief placeholder write function (tar doesn't do this)
/// @param f the file to write to (in theory)
/// @param buffer the buffer to write from (in theory)
/// @param len the length of the buffer to be written into (in theory)
/// @return ERROR_TAR or NOERROR_TAR (in this case always ERROR_TAR).
int tar_write(struct file *f, const void *buffer, size_t len)
{
	// tar doesn't do write lol. This is just here so there is something to send to write
	(void)f;
	(void)buffer;
	(void)len;
	return ERROR_TAR;
}

/// @brief gets the directory entry for the given file
/// @param f the file
/// @param ent the directory entry is put into here
/// @param entry where idx needs to reach
/// @return NOERROR_TAR or ERROR_TAR
int tar_ents(struct file *f, struct dirent *ent, size_t entry)
{
	struct tar_file *tf;
	struct tar_header dir, hdr;
	size_t sect;
	size_t sect_off = 0;
	size_t idx = 0;
	tf = (struct tar_file *)f;
	sect = 0;
	if (tf->file.f_type != F_DIR)
		return ERROR_TAR;
	if (read_tar_header(tf->fs->fs_disk, sect, &dir)) {
		return ERROR_TAR;
	}
	while (1) {
		if (find_file_redux(tf->fs, dir.name, &sect_off, &sect, &hdr))
			return ERROR_TAR;

		if (idx != entry) {
			idx++;
			continue;
		}
		ent->d_offset = entry;
		ent->d_namelen = strlen(hdr.name);
		memcpy(ent->d_name, hdr.name, ent->d_namelen + 1);
		return NOERROR_TAR;
	}

	return ERROR_TAR;
}

/// @brief opens up the file at the full file path, and prepares it for further action
/// @param fs the file system being used
/// @param fullpath the full file path to the file that needs to be opened
/// @param flags in this case, just used to check whether the file is set to read only, to make sure it is correct
/// @param out the file, ready to be acted upon, with the other functions loaded onto it.
/// @return NOERROR_TAR or ERROR_TAR
int tar_open(struct file_system *fs, const char *fullpath, int flags,
			 struct file **out)
{
	struct tar_header hdr;
	struct tar_file *newFile;
	size_t sect_result;
	find_file(fs, fullpath, NULL, &sect_result, &hdr);
	if (flags != O_RDONLY) {
		return ERROR_TAR;
	}
	newFile =
		kalloc(sizeof(struct tar_file)); // allocate memory to the new file.
	// sets the values for the opened file.
	newFile->file.f_type = F_REG;
	newFile->fs = fs;
	newFile->file.read = tar_read;
	newFile->file.close = tar_close;
	newFile->file.write = tar_write; // doesn't actually work;
	newFile->file.ents = tar_ents;
	newFile->file.seek = tar_seek;
	newFile->offset = 0;
	newFile->len = strtoull(hdr.fileSize, NULL, 8);
	newFile->sect = sect_result;

	*out = (struct file *)newFile;
	return NOERROR_TAR;
}

/// @brief gets the stats for the file found at fullpath, putting them in out
/// @param fs the file system being used
/// @param fullpath the full filepath of the file
/// @param out the stats for the file found
/// @return NOERROR_TAR OR ERROR_TAR
int tar_stat(struct file_system *fs, const char *fullpath, struct stat *out)
{
	struct tar_header hdr;
	size_t sect_result;
	find_file(fs, fullpath, NULL, &sect_result, &hdr);
	out->s_length = strtoull(hdr.fileSize, NULL, 8);
	if (hdr.type_flag == REGTYPE) {
		out->s_type = F_REG;
	} else if (hdr.type_flag == DIRTYPE) {
		out->s_type = F_DIR;
	} else {
		// shouldn't reach here, but we have to account for if it does.
		return ERROR_TAR;
	}
	return NOERROR_TAR;
}

/// @brief use tar for the filesystem.
/// @param fs the file system being used, and loading tar into it.
/// @return NOERROR_TAR or ERROR_TAR
int tar_mount(struct file_system *fs)
{
	struct tar_header hdr;
	// reads into hdr
	if (read_tar_header(fs->fs_disk, 0, &hdr) == 0) {
		fs->fs_name = "tar";
		fs->open = tar_open;
		fs->stat = tar_stat;
		fs->fs_present = true;
		return NOERROR_TAR;
	}
	return ERROR_TAR;
}

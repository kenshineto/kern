#include "lib/kio.h"
#include <comus/fs.h>
#include <lib.h>

struct tar_hdr {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char unused[12];
};

struct tar_file {
	struct file file;
	struct file_system *fs;
	size_t len;
	size_t offset;
	size_t sect;
};

#define TAR_SECT_SIZE 512

#define TMAGIC "ustar"
#define TMAGLEN 6
#define TVERSION "00"
#define TVERSLEN 2

#define REGTYPE '0'
#define DIRTYPE '5'

static int read_tar_hdr(struct disk *disk, uint32_t sect, struct tar_hdr *hdr)
{
	if (disk_read(disk, sect * TAR_SECT_SIZE, TAR_SECT_SIZE, hdr) <
		TAR_SECT_SIZE)
		return 1;

	// check magic
	if (memcmp(hdr->magic, TMAGIC, TMAGLEN) != 0)
		return 1;

	// check version
	if (memcmp(hdr->version, TVERSION, TVERSLEN) != 0)
		return 1;

	return 0;
}

static int tar_to_fs_type(char typeflag)
{
	switch (typeflag) {
	case REGTYPE:
		return F_REG;
	case DIRTYPE:
		return F_DIR;
	default:
		return -1;
	}
}

static int tar_locate(struct file_system *fs, const char *path,
					  struct tar_hdr *out_hdr, size_t *out_sect,
					  size_t *in_sect, bool partial)
{
	struct tar_hdr hdr;
	size_t sect = 0;

	if (in_sect != NULL)
		sect = *in_sect;

	while (1) {
		size_t filesize, sects;
		int cmp;

		if (read_tar_hdr(fs->fs_disk, sect, &hdr))
			return 1;

		filesize = strtoull(hdr.size, NULL, 8);
		sects = (filesize + TAR_SECT_SIZE - 1) / TAR_SECT_SIZE;

		if (partial) {
			size_t len = MIN(strlen(path), strlen(hdr.name));
			cmp = memcmp(hdr.name, path, len);
		} else {
			cmp = memcmp(hdr.name, path, strlen(path) + 1);
		}

		if (cmp) {
			// not our file, goto next
			sect += sects + 1;
			continue;
		}

		// we found our file!
		*out_hdr = hdr;
		*out_sect = sect;
		if (in_sect != NULL)
			*in_sect = sect + sects + 1;
		return 0;
	}

	return 1;
}

int tar_read(struct file *in, char *buf, size_t len)
{
	struct tar_file *file = (struct tar_file *)in;
	size_t max_bytes = file->len - file->offset;
	long bytes = MIN(max_bytes, len);

	if (file->file.f_type != F_REG)
		return 1;

	if (bytes < 1)
		return 0;

	bytes = disk_read(file->fs->fs_disk,
					  (file->sect + 1) * TAR_SECT_SIZE + file->offset, bytes,
					  buf);
	if (bytes < 0)
		return bytes; // return err code

	file->offset += bytes;
	return bytes;
}

int tar_write(struct file *in, const char *buf, size_t len)
{
	(void)in;
	(void)buf;
	(void)len;

	// cannot write to tar balls
	return -1;
}

int tar_seek(struct file *in, long int off, int whence)
{
	struct tar_file *file = (struct tar_file *)in;
	switch (whence) {
	case SEEK_SET:
		file->offset = off;
		return file->offset;
	case SEEK_CUR:
		file->offset += off;
		return file->offset;
	case SEEK_END:
		file->offset = file->len + off;
		return file->offset;
	default:
		return -1;
	}
}

int tar_ents(struct file *in, struct dirent *ent, size_t entry)
{
	struct tar_file *file;
	struct tar_hdr dir, hdr;
	size_t sect;
	size_t sect_off = 0;
	size_t idx = 0;

	file = (struct tar_file *)in;
	sect = 0;

	if (file->file.f_type != F_DIR)
		return -1;

	if (read_tar_hdr(file->fs->fs_disk, sect, &dir))
		return 1;

	while (1) {
		if (tar_locate(file->fs, dir.name, &hdr, &sect, &sect_off, true))
			return 1;

		if (idx != entry) {
			idx++;
			continue;
		}

		ent->d_offset = entry;
		ent->d_namelen = strlen(hdr.name);
		memcpy(ent->d_name, hdr.name, ent->d_namelen + 1);
		return 0;
	}

	return 1;
}

void tar_close(struct file *file)
{
	kfree(file);
}

int tar_open(struct file_system *fs, const char *path, int flags,
			 struct file **out)
{
	struct tar_file *file;
	struct tar_hdr hdr;
	size_t sect;

	// cannot create or write files
	if (flags != O_RDONLY)
		return 1;

	if (tar_locate(fs, path, &hdr, &sect, NULL, false))
		return 1;

	file = kalloc(sizeof(struct tar_file));
	if (file == NULL)
		return 1;

	file->file.f_type = tar_to_fs_type(hdr.typeflag);
	file->file.read = tar_read;
	file->file.write = tar_write;
	file->file.seek = tar_seek;
	file->file.ents = tar_ents;
	file->file.close = tar_close;
	file->fs = fs;
	file->len = strtoull(hdr.size, NULL, 8);
	file->offset = 0;
	file->sect = sect;
	*out = (struct file *)file;

	return 0;
}

int tar_stat(struct file_system *fs, const char *path, struct stat *stat)
{
	struct tar_hdr hdr;
	size_t sect;

	if (tar_locate(fs, path, &hdr, &sect, NULL, false))
		return 1;

	stat->s_length = strtoull(hdr.size, NULL, 8);
	stat->s_type = tar_to_fs_type(hdr.typeflag);

	return 0;
}

int tar_mount(struct file_system *fs)
{
	struct tar_hdr hdr;

	// if first tar hdr is valid, assume valid tarball
	if (read_tar_hdr(fs->fs_disk, 0, &hdr))
		return 1;

	fs->fs_present = true;
	fs->fs_name = "tar";
	fs->open = tar_open;
	fs->stat = tar_stat;

	INFO("loaded tarfs on disk %d", fs->fs_disk->d_id);
	return 0;
}

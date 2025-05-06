/**
 * @file tar.h
 *
 * Tarball
 */

#ifndef TAR_FS_H_
#define TAR_FS_H_

#include <comus/fs.h>

/**
 * Attempts to mount tar filesystem on disk
 * @returns 0 on success
 */
int tar_mount(struct file_system *fs);

#endif /* fs.h */

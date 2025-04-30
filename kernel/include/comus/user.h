/**
 * @file user.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Userland functions
 */

#ifndef USER_H_
#define USER_H_

#include <comus/procs.h>
#include <comus/fs.h>

/**
 * Load a user elf program from a file into a pcb
 */
int user_load(struct pcb *pcb, struct file *file);

/**
 * Clone a user process. Used for fork().
 */
struct pcb *user_clone(struct pcb *pcb);

/**
 * Clean up all loaded userland data from a pcb
 */
void user_cleanup(struct pcb *pcb);

#endif /* user.h */

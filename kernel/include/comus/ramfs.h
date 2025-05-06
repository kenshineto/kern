#include <comus/fs.h>
#ifndef RAMFS_H_
#define RAMFS_H_
#define MAX_FILES 32
#define MAX_FILE_SIZE 4096

struct file {
    char name[32];
    size_t size;
    char *data;
} file;

struct file allTheFiles[MAX_FILES];
int numberOfFiles = 0;

#endif
/*
typedef struct ramfs_file {
    char name[32];
    int size;
    char *data;
} file;

typedef struct ramfs_directory {
    char name[32];
    file *files[MAX_FILES];
    directory *directories[MAX_FILES];
    int file_count;
    int directory_count;
} directory;

typedef struct ramfs_root {
    directory *root;
} root;

//struct file allTheFiles[MAX_FILES];
int numberOfFiles = 0;


#endif*/
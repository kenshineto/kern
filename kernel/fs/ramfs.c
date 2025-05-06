/*#include <comus/fs.h>
#include <lib.h>
//#include <comus/tar.h>
//#include <string.h>
#include <comus/ramfs.h>

#define NOERROR 0
#define ERROR 1

int ramfs_check_exists(const char *name) {
    for(int i = 0; i < numberOfFiles; i++) {
        if(memcmp(allTheFiles[i].name, name, strlen(allTheFiles[i].name)) == 0) {
            // found it
            return i;
        }
    }
    // doesn't exist
    return -1;
}
int ramfs_create(const char *name) {
    if(ramfs_check_exists(name) != -1) {
        // already exists
        return ERROR;
    }
    struct file *newFile;
    *newFile->name = name;
    newFile->size = 0;
    newFile->data = kalloc(MAX_FILE_SIZE);
    newFile = &allTheFiles[numberOfFiles];

}

int ramfs_delete(const char *name) {
    int i = ramfs_check_exists(name);
    if(i >= 0) {
        kfree(allTheFiles[i].data);
        for(int j = i; j < numberOfFiles; j++) {
            allTheFiles[j] = allTheFiles[j+1];
            numberOfFiles -= 1; 
        }
        return NOERROR;

    }
    return ERROR;
}

int ramfs_read(const char *name, char *buffer, size_t len) {
    int i = ramfs_check_exists(name);
    if(i >= 0) {
        memcpy(buffer, allTheFiles[i].data, len);
        return NOERROR;
    }
    return ERROR;
}


int ramfs_write(const char *name, char *buffer) {
    int i = ramfs_check_exists(name);
    if(i >= 0) {
        memcpy(buffer)
    }
}

// here we return the index of the file as well.
/*int ramfs_find_file(root *r, const char *fullpath, const char *name, file *out, directory *outDirectory) {
    directory *location = r->root;
    if(ramfs_find_directory(r, fullpath, name, location) == NOERROR) {
        for(int i = 0; i < location->file_count; i++) {
            if(strcmp(location->files[i]->name, name) == 0) {
                outDirectory = location;
                out = location->files[i];
                return i;
            }
        }

    }
    return -1;
}
int ramfs_find_directory(root *r, const char *fullpath, const char *name, directory *out) {
    directory *location = r->root;
    char *path = fullpath;
    char *tempPath = strtok(tempPath, "/");
    while(tempPath != NULL) {
        bool wasItFound = false;

        for(int i = 0; i < location->directory_count; i++) {
            if(strcmp(location->directories[i]->name, tempPath) == 0) {
                // yay we found it;
                location = location->directories[i];
                wasItFound = true;
                break;
                
            }
        }
        if(!wasItFound) {
            return ERROR;
        }
        tempPath = strtok(NULL, "/");
        
    }
    out = location;
    return NOERROR;

}

int ramfs_create_file(root *r, const char *fullpath, const char *name) {
    // trace through the path
    // struct ramfs_directory *location = r->root;
    directory *location = r->root;

    if(ramfs_find_directory(r, fullpath, name, location) == NOERROR) {
        if(location->file_count >= MAX_FILES) {
            return ERROR;
        }
        file *newFile = malloc(sizeof(file));
        strcpy(newFile->name, name);
        newFile->size = 0;
        newFile->data = malloc(MAX_FILE_SIZE);
        location->files[location->file_count] = newFile;
        location->file_count += 1;

        return NOERROR;

    }
    return ERROR;
}

int ramfs_delete_file(root *r, const char *fullpath, const char *name) {
    directory *location;
    if(ramfs_find_directory(r, fullpath, name, location) == NOERROR) {
        file *find_file;
        int file_index = ramfs_find_file(r, fullpath, name, find_file, location) >= 0;
        if(file_index >= 0) {
            free(location->files[file_index]->data);
            free(location->files[file_index]);
            for(int i = file_index + 1; i < location->file_count; i++) {
                location->files[i-1] = location->files[i];
                //memcpy(location->files[i-1], location->files[i], location->files[i]->size);
                //free(location->files[i]);
            }
            location->file_count -= 1;
            return NOERROR;
        }
        return ERROR;
        
    }
    return ERROR;
}

int ramfs_create_directory() {

}

int ramfs_delete_directory() {
}

int ramfs_write() {
    
}


root *ramfs_init() {
    root *r = malloc(sizeof(root));
    r->root = malloc(sizeof(directory));
    strcpy(r->root->name, "/");
    r->root->file_count = 0;
    r->root->directory_count = 0;
    return r;
}

*/

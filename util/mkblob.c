/**
** @file	mkblob.c
**
** @author	Warren R. Carithers
**
** Create a binary blob from a collection of ELF files.
*/
#define	_DEFAULT_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>

/*
** Blob file organization
**
** The file begins with a four-byte magic number and a four-byte integer
** indicating the number of ELF files contained in the blob. This is
** followed by an array of 32-byte file table entries, and then the contents
** of the ELF files in the order they appear in the program file table.
**
**		Bytes        Contents
**		-----        ----------------------------
**		0 - 3        File magic number ("BLB\0")
**      4 - 7        Number of ELF files in blob ("n")
**      8 - n*32+8   Program file table
**		n*32+9 - ?   ELF file contents
**
** Each program file table entry contains the following information:
**
**		name         File name (up to 19 characters long)
**		offset       Byte offset to the ELF header for this file
**		size         Size of this ELF file, in bytes
**		flags        Flags related to this file
*/

// blob header
typedef struct header_s {
	char magic[4];
	uint32_t num;
} header_t;

// length of the file name field
#define NAMELEN      20

// program descriptor
typedef struct prog_s {
	char name[NAMELEN];  // truncated name (15 chars)
	uint32_t offset;     // offset from the beginning of the blob
	uint32_t size;       // size of this ELF module
	uint32_t flags;      // miscellaneous flags
} prog_t;

// modules must be written as multiples of eight bytes
#define FL_ROUNDUP     0x00000001

// mask for mod 8 checking
#define FSIZE_MASK     0x00000007

// program list entry
typedef struct node_s {
	prog_t *data;
	char *fullname;
	struct node_s *next;
} node_t;

node_t *progs, *last_prog;   // list pointers
uint32_t n_progs;            // number of files being copied
uint32_t offset;             // current file area offset

/**
** Name:	process
**
** Do the initial processing for an ELF file
**
** @param name  The name of the file
*/
void process( const char *name ) {
	struct stat info;

	// check the name length
	if( strlen(name) >= NAMELEN ) {
		fprintf( stderr, "%s: name exceeds length limit (%d)\n",
				name, NAMELEN-1 );
		return;
	}

	// does it exist?
	if( stat(name,&info) < 0 ) {
		perror( name );
		return;
	}

	// is it a regular file?
	if( !S_ISREG(info.st_mode) ) {
		fprintf( stderr, "%s: not a regular file\n", name );
		return;
	}

	// open it and check the file header
	int fd = open( name, O_RDONLY );
	if( fd < 0 ) {
		perror( name );
		return;
	}

	// read and check the ELF header
	Elf32_Ehdr hdr;
	int n = read( fd, &hdr, sizeof(Elf32_Ehdr) );
	close( fd );

	if( n != sizeof(Elf32_Ehdr) ) {
		fprintf( stderr, "%s: header read was short - only %d\n", name, n );
		return;
	}

	if( hdr.e_ident[EI_MAG0] != ELFMAG0 ||
		hdr.e_ident[EI_MAG1] != ELFMAG1 ||
		hdr.e_ident[EI_MAG2] != ELFMAG2 ||
		hdr.e_ident[EI_MAG3] != ELFMAG3   ) {
		fprintf( stderr, "%s: bad ELF magic number\n", name );
		return;
	}

	// ok, it's a valid ELF file - create the prog list entry
	prog_t *new = calloc( 1, sizeof(prog_t) );
	if( new == NULL ) {
		fprintf( stderr, "%s: calloc prog returned NULL\n", name );
		return;
	}

	node_t *node = calloc( 1, sizeof(node_t) );
	if( node == NULL ) {
		free( new );
		fprintf( stderr, "%s: calloc node returned NULL\n", name );
		return;
	}

	node->data = new;
	node->fullname = strdup( name );

	// copy in the name

	// only want the last component
	const char *slash = strrchr( name, '/' );
	if( slash == NULL ) {
		// only the file name
		slash = name;
	} else {
		// skip the slash
		++slash;
	}

	strncpy( new->name, slash, sizeof(new->name)-1 );
	new->offset = offset;
	new->size = info.st_size;

	// bump our counters
	++n_progs;
	offset += info.st_size;

	// make sure it's a multiple of eight bytes long
	if( (info.st_size & FSIZE_MASK) != 0 ) {
		// nope, so we must round it up when we write it out
		new->flags |= FL_ROUNDUP;
		// increases the offset to the next file
		offset += 8 - (info.st_size & FSIZE_MASK);
	}
	
	// add to the list
	if( progs == NULL ) {
		// first entry
		progs = node;
	} else {
		// add to the end
		if( last_prog == NULL ) {
			fprintf( stderr, "%s: progs ! NULL, last_prog is NULL\n", name );
			free( new );
			free( node->fullname );
			free( node );
			return;
		}
		last_prog->next = node;
	}
	last_prog = node;
}

/**
** Name:	copy
**
** Copy the contents of a program list entry into the blob
**
** @param ofd   The output FILE* to be written
** @param prog  Pointer to the program list entry for the file
*/
void copy( FILE *ofd, node_t *node ) {

	prog_t *prog = node->data;

	// open it so we can copy it
	int fd = open( node->fullname, O_RDONLY );
	if( fd < 0 ) {
		perror( node->fullname );
		return;
	}

	uint8_t buf[512];

	// copy it block-by-block
	do {
		int n = read( fd, buf, 512);
		// no bytes --> we're done
		if( n < 1 ) {
			break;
		}
		// copy it, and verify the copy count
		int k = fwrite( buf, 1, n, ofd );
		if( k != n ) {
			fprintf( stderr, "%s: write of %d returned %d\n", 
					prog->name, n, k );
		}
	} while( 1 );

	printf( "%s: copied %d", prog->name, prog->size );

	// do we need to round up?
	if( (prog->flags & FL_ROUNDUP) != 0 ) {

		// we'll fill with NUL bytes
		uint64_t filler = 0;

		// how many filler bytes do we need?
		int nbytes = 8 - (prog->size & FSIZE_MASK);

		// do it, and check the transfer count to be sure
		int n = fwrite( &filler, 1, nbytes, ofd );
		if( n != nbytes ) {
			fprintf( stderr, "%s: fill write of %d returned %d\n",
					prog->name, nbytes, n );
		}

		// report that we added some filler bytes
		printf( "(+%d)", n );
	}
	puts( " bytes" );

	// all done!
	close( fd );
}

int main( int argc, char *argv[] ) {

	// construct program list
	for( int i = 1; i < argc; ++i ) {
		process( argv[i] );
	}

	if( n_progs < 1 ) {
		fputs( "Nothing to do... exiting.", stderr );
		exit( 0 );
	}

	// create the output file
	FILE *ofd;
	ofd = fopen( "user.img", "wb" );
	if( ofd == NULL ) {
		perror( "user.img" );
		exit( 1 );
	}

	printf( "Processing %d ELF files\n", n_progs );

	// we need to adjust the offset values so they are relative to the
	// start of the blob, not relative to the start of the file area.
	// do this by adding the sum of the file header and program entries
	// to each offset field.
	
	uint32_t hlen = sizeof(header_t) + n_progs * sizeof(prog_t);
	node_t *curr = progs;
	while( curr != NULL ) {
		curr->data->offset += hlen;
		curr = curr->next;
	}

	// write out the blob header
	header_t hdr = { "BLB", n_progs };
	if( fwrite(&hdr,sizeof(header_t),1,ofd) != 1 ) {
		perror( "blob header" );
		fclose( ofd );
		exit( 1 );
	}

	// next, the program entries
	curr = progs;
	while( curr != NULL ) {
		if( fwrite(curr->data,sizeof(prog_t),1,ofd) != 1 ) {
			perror( "blob prog entry write" );
			fclose( ofd );
			exit( 1 );
		}
		curr = curr->next;
	}

	// finally, copy the files
	curr = progs;
	while( curr != NULL ) {
		prog_t *prog = curr->data;
		copy( ofd, curr );
		node_t *tmp = curr;
		curr = curr->next;
		free( tmp->data );
		free( tmp->fullname );
		free( tmp );
	}

	fclose( ofd );

	return 0;
}

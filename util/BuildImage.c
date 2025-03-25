/**
** SCCS ID:	@(#)BuildImage.c	2.2	1/16/25
**
** @file	BuildImage.c
**
** @author	K. Reek
** @author	Jon Coles
** @author	Warren R. Carithers
** @author	Garrett C. Smith
**
** Modify the bootstrap image to include the information
** on the programs to be loaded, and produce the file
** that contains the concatenation of these programs.
**
*/

#define	_POSIX_C_SOURCE	200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define	TRUE	1
#define	FALSE	0

#define	DRIVE_FLOPPY	0x00
#define	DRIVE_USB		0x80

#define	SECT_SIZE	512

char *progname;				/* invocation name of this program */
char *bootstrap_filename;	/* path of file holding bootstrap program */
char *output_filename;		/* path of disk image file */
FILE *out;					/* output stream for disk image file */
short drive = DRIVE_USB;	/* boot drive */

/*
** Array into which program information will be stored, starting at the
** end and moving back toward the front.  The array is the same size as
** a sector, which is guaranteed to be larger than the maximum possible
** space available for this stuff in the bootstrap image.  Thus, the
** bootstrap image itself (and the amount of space available on the
** device) are the only limiting factors on how many program sections
** can be loaded.
*/

#define	N_INFO	( SECT_SIZE / sizeof( short ) )

short info[ N_INFO ];
int n_info = N_INFO;

/**
** quit with an appropriate message
**
** @param msg   NULL, or a message to be printed to stderr
** @param call_perror  non-zero if perror() should be used; else,
**                     fprintf() will be used
**
** does not return
*/
void quit( char *msg, int call_perror ) {
	if( msg != NULL ){
		// preserve the error code in case we need it
		int err_num = errno;
		fprintf( stderr, "%s: ", progname );
		errno = err_num;
		if( call_perror ){
			perror( msg );
		} else {
			fprintf( stderr, "%s\n", msg );
		}
	}
	if( output_filename != NULL ){
		unlink( output_filename );
	}
	exit( EXIT_FAILURE );
	// NOTREACHED
}

const char	usage_error_msg[] =
  "\nUsage: %s [ -d drive ] -b bootfile -o outfile { progfile loadpt } ...\n\n"
  "\t'drive' is either 'floppy' or 'usb' (default 'usb')\n\n"
  "\tThere must be at least one program file and load point.\n\n"
  "\tLoad points may be specified either as 32-bit quantities in hex,\n"
  "\tdecimal or octal (e.g. 0x10c00, 68608, 0206000 are all equivalent),\n"
  "\tor as an explicit segment:offset pair whose digits are always\n"
  "\tinterpreted as hexadecimal values (e.g. 10c0:0000, 1000:0c00 are\n"
  "\tboth equivalent to the previous examples).\n\n";

/**
** print a usage message and then call quit()
**
** does not return
*/
void usage_error( void ){
	fprintf( stderr, usage_error_msg, progname );
	quit( NULL, FALSE );
	// NOTREACHED
}

/**
** copy the contents of a binary file into the output file, padding the
** last sector with NUL bytes
**
** @param in   open FILE to be read
** @return the number of sectors copied from the file
*/
int copy_file( FILE *in ){
	int n_sectors = 0;
	char buf[ SECT_SIZE ];
	int n_bytes;
	int i;

	/*
	** Copy the file to the output, being careful that the
	** last sector is padded with null bytes out to the
	** sector size.
	*/
	n_sectors = 0;
	while( (n_bytes = fread( buf, 1, sizeof( buf ), in )) > 0 ){
		// pad this sector out to block size
		if( n_bytes < sizeof( buf ) ){
			int i;

			for( i = n_bytes; i < sizeof( buf ); i += 1 ){
				buf[ i ] = '\0';
			}
		}
		if( fwrite( buf, 1, sizeof( buf ), out ) != sizeof( buf ) ){
			quit( "Write failed or was wrong size", FALSE );
		}
		n_sectors += 1;
	}
	return n_sectors;
}

/**
** process a file whose contents should be at a specific'
** address in memory when the program is loaded
**
** @param name  path to the file to be copied
** @param addr  string containing the load address
*/
void process_file( char *name, char *addr ){
	long address;
	short segment, offset;
	int n_bytes;

	/*
	** Open the input file.
	*/
	FILE *in = fopen( name, "rb" );
	if( in == NULL ){
		quit( name, TRUE );
	}

	/*
	** Copy the file to the output, being careful that the
	** last block is padded with null bytes.
	*/
	int n_sectors = copy_file( in );
	fclose( in );

	/*
	** Decode the address they gave us. We'll accept two forms:
	** "nnnn:nnnn" for a segment:offset value (assumed to be hex),
	** "nnnnnnn" for a decimal, hex, or octal value
	*/
	int valid_address = FALSE;
	char *cp = strchr( addr, ':' );
	if( cp != NULL ){
		// must be in nnnn:nnnn form exactly
		if( strlen( addr ) == 9 && cp == addr + 4 ){
			char *ep1, *ep2;
			int a1, a2;

			segment = strtol( addr, &ep1, 16 );
			offset = strtol( addr + 5, &ep2, 16 );
			address = ( segment << 4 ) + offset;
			valid_address = *ep1 == '\0' && *ep2 == '\0';
		} else {
			fprintf( stderr, "Bad address format - '%s'\n", addr );
			quit( NULL, FALSE );
		}
	} else {
		// just a number, possibly hex or octal
		char *ep;

		address = strtol( addr, &ep, 0 );
		segment = (short)( address >> 4 );
		offset = (short)( address & 0xf );
		valid_address = *ep == '\0' && address <= 0x0009ffff;
	}

	if( !valid_address ){
		fprintf( stderr, "%s: Invalid address: %s\n", progname, addr );
		quit( NULL, FALSE );
	}

	/*
	** Make sure the program will fit!
	*/
	if( address + n_sectors * SECT_SIZE > 0x0009ffff ){
		fprintf( stderr, "Program %s too large to start at 0x%08x\n",
		    name, (unsigned int) address );
		quit( NULL, FALSE );
	}

	if( n_info < 3 ){
		quit( "Too many programs!", FALSE );
	}


	/*
	** Looks good: report and store the information.
	*/
	fprintf( stderr, "  %s: %d sectors, loaded at 0x%x\n",
	    name, n_sectors, (unsigned int) address );

	info[ --n_info ] = n_sectors;
	info[ --n_info ] = segment;
	info[ --n_info ] = offset;
}

/*
** Global variables set by getopt()
*/

extern int optind, optopt;
extern char *optarg;

/**
** process the command-line arguments
**
** @param ac  the count of entries in av
** @param av  the argument vector
*/
void process_args( int ac, char **av ) {
	int c;
	
	while( (c=getopt(ac,av,":d:o:b:")) != EOF ) {

		switch( c ) {

		case ':':	/* missing arg value */
			fprintf( stderr, "missing operand after -%c\n", optopt );
			/* FALL THROUGH */

		case '?':	/* error */
			usage_error();
			/* NOTREACHED */

		case 'b':	/* -b bootstrap_file */
			bootstrap_filename = optarg;
			break;

		case 'd':	/* -d drive */
			switch( *optarg ) {
			case 'f':	drive = DRIVE_FLOPPY; break;
			case 'u':	drive = DRIVE_USB; break;
			default:	usage_error();
			}
			break;

		case 'o':	/* -o output_file */
			output_filename = optarg;
			break;
		
		default:
			usage_error();
		
		}
	
	}

	if( !bootstrap_filename ) {
		fprintf( stderr, "%s: no bootstrap file specified\n", progname );
		exit( 2 );
	}

	if( !output_filename ) {
		fprintf( stderr, "%s: no disk image file specified\n", progname );
		exit( 2 );
	}

	/*
	** Must have at least two remaining arguments (file to load, 
	** address at which it should be loaded), and must have an
	** even number of remaining arguments.
	*/
	int remain = ac - optind;
	if( remain < 2 || (remain & 1) != 0 ) {
		usage_error();
	}

}

/**
** build a bootable image file from one or more binary files
**
** usage:
**   BuildImage [ -d drive ] -b bootfile -o outfile { binfile1 loadpt1 } ... ]
**
** @param ac   command-line argument count
** @param av   command-line argument vector
** @return EXIT_SUCCESS or EXIT_FAILURE
*/
int main( int ac, char **av ) {
	FILE	*bootimage;
	int	bootimage_size;
	int	n_bytes, n_words;
	short	existing_data[ N_INFO ];
	int	i;
	
	/*
	** Save the program name for error messages
	*/
	progname = strrchr( av[ 0 ], '/' );
	if( progname != NULL ){
		progname++;
	} else {
		progname = av[ 0 ];
	}

	/*
	** Process arguments
	*/
	process_args( ac, av );

	/*
	** Open the output file
	*/

	out = fopen( output_filename, "wb+" );
	if( out == NULL ){
		quit( output_filename, TRUE );
	}

	/*
	** Open the bootstrap file and copy it to the output image.
	*/
	bootimage = fopen( bootstrap_filename, "rb" );
	if( bootimage == NULL ){
		quit( bootstrap_filename, TRUE );
	}

	/*
	** Remember the size of the bootstrap for later, as we
	** need to patch some things into it
	*/
	int n_sectors = copy_file( bootimage );
	fclose( bootimage );

	bootimage_size = n_sectors * SECT_SIZE;
	fprintf( stderr, "  %s: %d sectors\n", bootstrap_filename, n_sectors );

	/*
	** Process the programs one by one
	*/
	ac -= optind;
	av += optind;
	while( ac >= 2 ){
		process_file( av[ 0 ], av[ 1 ] );
		ac -= 2; av += 2;
	}

	/*
	** Check for oddball leftover argument
	*/
	if( ac > 0 ){
		usage_error();
	}

	/*
	** Seek to where the array of module data  must begin and read
	** what's already there.
	*/
	n_words = ( N_INFO - n_info );
	n_bytes = n_words * sizeof( info[ 0 ] );
	fseek( out, bootimage_size - n_bytes, SEEK_SET );
	if( fread( existing_data, sizeof(info[0]), n_words, out ) != n_words ){
		quit( "Read from boot image failed or was too short", FALSE );
	}

	/*
	** If that space is non-zero, we have a problem
	*/
	for( i = 0; i < n_words; i += 1 ){
		if( existing_data[ i ] != 0 ){
			quit( "Too many programs to load!", FALSE );
		}
	}

	/*
	** We know that we're only overwriting zeros at the end of
	** the bootstrap image, so it is ok to go ahead and do it.
	*/
	fseek( out, bootimage_size - n_bytes, SEEK_SET );
	if( fwrite( info + n_info, sizeof( info[ 0 ] ), n_words, out ) != n_words ){
		quit( "Write to boot image failed or was too short", FALSE );
	}

	/*
	** Write the drive index to the image.
	*/
	fseek( out, 508, SEEK_SET );
	fwrite( (void *)&drive, sizeof(drive), 1, out );

	fclose( out );

	return EXIT_SUCCESS;

}

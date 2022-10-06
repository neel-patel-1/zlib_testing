#include <zlib.h>
#include <stdlib.h>
#include <stdio.h>

#define BUF_SIZE 4096
#define OUTPUT_BUF 8192

# ifdef DO_DEBUG
# define DEBUG(fmt_str, ...)                                  \
   do {                                                       \
       fprintf(stderr,"[%s:%d:%s()]" fmt_str,				  \
		__FILE__, __LINE__, __func__, ##__VA_ARGS__);         \
       fflush(stderr);                               		  \
   } while (0) 
# else
# define DEBUG(fmt_str, ...) (void)0
# endif

#define ERROR(fmt_str, ...)                                  \
  do {                                                       \
       fprintf(stderr,"[ERROR %s:%d:%s()]" fmt_str,			 \
		__FILE__, __LINE__, __func__, ##__VA_ARGS__);        \
      fflush(stderr);                               	     \
	  exit(1);												 \
  } while (0) 


/*
 * fills len for callee with file len
 * buf points to buffer filled with corpus
 * data
 *
 * returns length of file
 */
long file_to_buf(const char * f_name, Bytef * buf, long * len){
	FILE *fp = fopen(f_name, "r");
	len = (long *)malloc(sizeof(long));
	buf = NULL;
	if (fp == NULL){
		ERROR("Corpus file could not be opened\n");
	}
	if ( fseek(fp, 0L, SEEK_END) == 0){
		*len = ftell(fp); /* initialize len*/
		if (*len == -1){ ERROR("Determining File Length Failed\n");}
		DEBUG("Opening corpus of file size: %.4fMB\n", (float)*len/1024/1024);
		buf = (Bytef *)malloc(sizeof(Bytef) * (*len + 1));
		//source = mmap(NULL, memory_size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
		if ( fseek(fp, 0L, SEEK_SET) != 0 ){ERROR("Could not go back to beginning of file\n");}
		size_t newLen = fread(buf, sizeof(char), *len, fp);
		DEBUG("Allocated %ld bytes and found %ld when reading\n",*len, newLen);
		if (ferror( fp ) != 0){
			fputs("Error reading file", stderr);
		}else{
			buf[*len] = '\0';
		}
	}
	fclose(fp);
	return *len;
}

/* reuse the same output buffer for compressed data -- keeping track of the total compressed size 
 * with the total_out member variable */
double comp(char * file, int gr, int ratio){
	long file_len, out_size;
	int ret = 0;
	double c_rat;

	/* zstream init */
	z_stream * z;
	z = (z_stream *)malloc(sizeof(z_stream));
	z->zalloc=Z_NULL;
	z->zfree=Z_NULL;
	deflateInit(z, ratio);

	Bytef * buf, * out; /* buffers of granularity size */
	int n_bufs, l_buf_sz=0, sub; /* number of buffers, size of data in last buf, extr
									padding bytes not emmitted by hardware */

	/* get input file into long buffer */
	file_len = file_to_buf( file, buf, &file_len);
	out_size = file_len * 2;
	out = (Bytef *)malloc(sizeof(Bytef) * out_size);

	/* get num bufs */
	n_bufs = file_len / gr;
	DEBUG("Split %lu file into %d buffers of size %d\n", file_len, n_bufs, gr);
	if( (l_buf_sz = (file_len % gr)) != 0 ){
		n_bufs ++;
	}
	/* padding bytes to subtract */
	sub = n_bufs * 5;

	/* all gr-sized bufs */
	int i, off=0;
	for ( i=0; i < (n_bufs-1); i++ ){ /* compress all bufs besides last */

		/* we reuse the same buffer/pointer since only measuiring compression ratio */
		z->avail_in=gr;
		z->next_in=(buf+off);
		z->avail_out=out_size;
		z->next_out=out;

		while ( z->avail_in != 0 ){
			ret = deflate(z, Z_SYNC_FLUSH);
			if ( ret != Z_OK ){
				DEBUG("Total compressed bytes on iter %d -- %lu\n", i, z->total_out);
				ERROR( "Bad return from compressor:%d \n", ret);
			}
			if ( z-> avail_out == 0){
				z->avail_out=out_size;
				z->next_out=out;
				DEBUG("Total compressed bytes on iter %d -- %lu\n", i, z->total_out);
			}
		}

		DEBUG("Total compressed bytes on iter %d -- %lu\n", i, z->total_out);
	}

	/* last (< gr content) buf */
	if ( (z->avail_in = l_buf_sz) != 0 ){
		z->avail_out=(gr) * 2;
		z->next_in=buf;
		z->next_out=out;
		while ( z->avail_in > 0 ){
			ret = deflate(z, Z_FINISH);
			if ( ! ( ret == Z_OK  || ret == Z_STREAM_END) ){
				ERROR( "Bad return from compressor: %d\n", ret);
			}
			if ( z-> avail_out == 0){
				ERROR( "Out of destination buffer space\n" );
			}
		}
		DEBUG("Total compressed bytes on iter %d -- %lu\n", i, z->total_out);
	}
	else{
		ret = deflate(z, Z_FINISH);
		if ( ret != Z_STREAM_END){
			ERROR( "Output space not sufficient for Z_FINISH\n");
		}
	}

	c_rat = (float)file_len/(z->total_out-sub);
	printf("Compressor_Level:%d Granularity:%d  Compression_Ratio:%.5f\n",ratio, gr, c_rat);

	return c_rat;

}


int main(int argc, char ** argv ){
	int c;
	opterr = 0;

	int gr = 4096;
	int ratio = 6;
	int sweep = 0;

	char * file;

	while ((c = getopt (argc, argv, "sf:g:")) != -1)
	switch (c)
	{
		case 'g':
			gr = atoi(optarg);
			break;
		case 'c':
			gr = atoi(optarg);
			break;
		case 'f':
			file = optarg;
			DEBUG("%s\n", file);
			break;
		case 'r':
			ratio = atoi(optarg);
			DEBUG("%d\n", ratio);
			break;
		case 's':
			sweep=1;
			break;
		case '?':
			fprintf (stderr, "Unknown option character `\\x%x'.\n",optopt);
			return 1;
		default:
			abort ();
	}
	DEBUG("Calling comp\n");
	if( sweep == 1 ){
	for ( int j=64; j<=4096; j+=64 ){
		for ( int i=0; i<10; i++){
				comp(file,j,i);
			}
		}
		return 0;
	}
	
	comp(file, gr,ratio);
	return 0;
}

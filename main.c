#include <zlib.h>
#include <stdlib.h>
#include <stdio.h>

#define BUF_SIZE 4096
#define OUTPUT_BUF 8192

int comp_64b(int gr){
	char * buf, * out;
	int ret;
	z_stream * z;
	z = (z_stream *)malloc(sizeof(z_stream));
	buf = (char *)malloc(BUF_SIZE);
	out = (char *)malloc(OUTPUT_BUF);
	z->avail_in=BUF_SIZE;
	z->avail_out=OUTPUT_BUF;
	z->next_in=buf;
	z->zalloc=Z_NULL;
	z->zfree=Z_NULL;
	deflateInit(z, 6);	
	//deflate(z);
	while( z->avail_in > 0 ){
		ret = deflate( z , Z_SYNC_FLUSH);
		if ( ret == Z_STREAM_END ){
			break;
		}
		if ( z->avail_out < BUF_SIZE )
			z->next_in=buf+z->avail_out;
		/*
		switch( ret ){
			Z_OK:
				break;
			default:
				break;
		}
		*/
	}

	printf("64 Byte Compression Ratio: %lu\n", z->total_out);

}


int main(){
	comp_64b(2);
}

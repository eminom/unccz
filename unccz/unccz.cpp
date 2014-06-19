
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"

#define CC_HOST_IS_BIG_ENDIAN (bool)(*(unsigned short *)"\0\xff" < 0x100) 
#define CC_SWAP32(i)  ((i & 0x000000ff) << 24 | (i & 0x0000ff00) << 8 | (i & 0x00ff0000) >> 8 | (i & 0xff000000) >> 24)
#define CC_SWAP16(i)  ((i & 0x00ff) << 8 | (i &0xff00) >> 8)   
#define CC_SWAP_INT32_LITTLE_TO_HOST(i) ((CC_HOST_IS_BIG_ENDIAN == true)? CC_SWAP32(i) : (i) )
#define CC_SWAP_INT16_LITTLE_TO_HOST(i) ((CC_HOST_IS_BIG_ENDIAN == true)? CC_SWAP16(i) : (i) )
#define CC_SWAP_INT32_BIG_TO_HOST(i)    ((CC_HOST_IS_BIG_ENDIAN == true)? (i) : CC_SWAP32(i) )
#define CC_SWAP_INT16_BIG_TO_HOST(i)    ((CC_HOST_IS_BIG_ENDIAN == true)? (i):  CC_SWAP16(i) )

struct CCZHeader {
	unsigned char   sig[4];             // signature. Should be 'CCZ!' 4 bytes
	unsigned short  compression_type;   // should 0
	unsigned short  version;            // should be 2 (although version type==1 is also supported)
	unsigned int    reserved;           // Reserved for users.
	unsigned int    len;                // size of the uncompressed file
};

enum {
	CCZ_COMPRESSION_ZLIB,               // zlib format.
	CCZ_COMPRESSION_BZIP2,              // bzip2 format (not supported yet)
	CCZ_COMPRESSION_GZIP,               // gzip format (not supported yet)
	CCZ_COMPRESSION_NONE,               // plain (not supported yet)
};

unsigned char* readData(const char *path, unsigned int *length)
{
	if (FILE *fin = fopen(path, "rb"))
	{
        fseek(fin, 0, SEEK_END);
        int totLength = ftell(fin);
        fseek(fin, 0, SEEK_SET);

		unsigned char *buffer = new unsigned char[totLength];
		memset(buffer, 0, totLength * sizeof(unsigned char));

		int left = totLength;
		int written = 0;
		while(left > 0)
		{
			int itemCount = fread(buffer + written, sizeof(unsigned char), left, fin );
			if(itemCount <= 0 ){
				break;
			}
			written += itemCount;
			left -= itemCount;
		}
		*length = written;
		return buffer;
	}
	return NULL;
}

int ccInflateCCZFile(const char *path, unsigned char **out)
{
    // load file into memory
    unsigned int fileLen = 0;
    unsigned char* compressed = readData(path, &fileLen);
    
    if(NULL == compressed || 0 == fileLen)
    {
        printf("cocos2d: Error loading CCZ compressed file\n");
        return -1;
    }
    
    struct CCZHeader *header = (struct CCZHeader*) compressed;
    
    // verify header
    if( header->sig[0] == 'C' && header->sig[1] == 'C' && header->sig[2] == 'Z' && header->sig[3] == '!' )
    {
        // verify header version
        unsigned int version = CC_SWAP_INT16_BIG_TO_HOST( header->version );
        if( version > 2 )
        {
            printf("cocos2d: Unsupported CCZ header format\n");
            delete [] compressed;
            return -1;
        }
        
        // verify compression format
        if( CC_SWAP_INT16_BIG_TO_HOST(header->compression_type) != CCZ_COMPRESSION_ZLIB )
        {
            printf("cocos2d: CCZ Unsupported compression method\n");
            delete [] compressed;
            return -1;
        }
    }
    else if( header->sig[0] == 'C' && header->sig[1] == 'C' && header->sig[2] == 'Z' && header->sig[3] == 'p' )
    {
		printf("failed to to so\n");
		abort();
    }
    else
    {
        printf("cocos2d: Invalid CCZ file");
		abort();
    }
    
    unsigned int len = CC_SWAP_INT32_BIG_TO_HOST(header->len);
    *out = (unsigned char*)malloc(len);
    if(! *out )
    {
        printf("cocos2d: CCZ: Failed to allocate memory for texture");
        delete [] compressed;
        return -1;
    }
    
    unsigned long destlen = len;
    unsigned long source = (unsigned long) compressed + sizeof(*header);
    int ret = uncompress(*out, &destlen, (Bytef*)source, fileLen - sizeof(*header) );
    
    delete [] compressed;
    
    if( ret != Z_OK )
    {
        printf("cocos2d: CCZ: Failed to uncompress data");
        free( *out );
        *out = NULL;
        return -1;
    }
    
    return len;
}

void writeToFile(const char *path, unsigned char *buffer, int length)
{
	if (FILE *fout = fopen(path, "wb"))
	{
		int left = length;
		int written = 0;
		while(left > 0)
		{
			int thisTerm = fwrite(buffer + written, sizeof(unsigned char), left, fout);
			if(thisTerm <= 0){
				break;
			}
			left -= thisTerm;
			written += thisTerm;
		}
		fclose(fout);
	}
}

int main()
{
	unsigned char *buffer = NULL;
	int length = ccInflateCCZFile("k.pvr.ccz", &buffer);
	writeToFile("k.pvr", buffer, length);
}
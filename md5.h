#ifndef _MD5_H_
#define _MD5_H_
#include <string.h>
#include <stdlib.h>
typedef struct  
{  
    unsigned int count[2];  
    unsigned int state[4];  
    unsigned char buffer[64];     
}MD5_CTX;  


#define E(x,y,z) ((x & y) | (~x & z))  
#define G(x,y,z) ((x & z) | (y & ~z))  
#define H(x,y,z) (x^y^z)  
#define I(x,y,z) (y ^ (x | ~z))  
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))  
#define FF(a,b,c,d,x,s,ac) \
{ \
    a += E(b,c,d) + x + ac; \
        a = ROTATE_LEFT(a,s); \
        a += b; \
}
#define GG(a,b,c,d,x,s,ac) \
{ \
    a += G(b,c,d) + x + ac; \
        a = ROTATE_LEFT(a,s); \
        a += b; \
}
#define HH(a,b,c,d,x,s,ac) \
{ \
    a += H(b,c,d) + x + ac; \
        a = ROTATE_LEFT(a,s); \
        a += b; \
}  
#define II(a,b,c,d,x,s,ac) \
{ \
    a += I(b,c,d) + x + ac; \
        a = ROTATE_LEFT(a,s); \
        a += b; \
}
extern void MD5Init(MD5_CTX *context);  
extern void MD5Update(MD5_CTX *context,unsigned char *input,unsigned int inputlen);  
extern void MD5Final(MD5_CTX *context,unsigned char digest[16]);  
extern void MD5Transform(unsigned int state[4],unsigned char block[64]);  
extern void MD5Encode(unsigned char *output,unsigned int *input,unsigned int len);  
extern void MD5Decode(unsigned int *output,unsigned char *input,unsigned int len);  
extern int create_md5(unsigned char *need_encrypt_data,int size,unsigned char *pdecrypt,char* key);
;


#endif

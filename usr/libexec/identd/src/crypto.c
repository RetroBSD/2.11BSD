/*
** crypto.c                                  Crypto extension to pidentd.
**
** This file is in the public domain. -- Planar 1994.02.21
**
** Crypto routines.
*/

#ifdef INCLUDE_CRYPT

#ifdef NeXT31
#  include <libc.h>
#endif

#include <stdio.h>

#include <sys/types.h>
#include <netinet/in.h>

#ifndef HPUX7
#  include <arpa/inet.h>
#endif

#include <sys/stat.h>
#include <sys/time.h>

#include <des.h>

typedef unsigned short int_16;
typedef unsigned int   int_32;

struct info
{
  int_32 checksum;
  int_16 random;
  int_16 uid;
  int_32 date;
  int_32 ip_local;
  int_32 ip_remote;
  int_16 port_local;
  int_16 port_remote;
};

typedef union data
{
  struct info   fields;
  int_32        longs[6];
  unsigned char chars[24];
} data;

static char result[33];
des_key_schedule sched;

char to_asc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char to_bin[] =
{
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x3e, 0x80, 0x80, 0x80, 0x3f,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
    0x17, 0x18, 0x19, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
    0x31, 0x32, 0x33, 0x80, 0x80, 0x80, 0x80, 0x80,
};


void init_encryption(key_text)
    char *key_text;
{
    des_cblock key_bin;

    des_string_to_key(key_text, &key_bin);
    des_set_key(&key_bin, sched);
}

char *make_packet(uid, laddr, lport, faddr, fport)
    int uid;
    struct in_addr *laddr, *faddr;
    int lport, fport;
{
    struct timeval tv;
    union data r;
    int res, i, j;
    
    res = gettimeofday(&tv, NULL);
    srand(tv.tv_usec);
    
    r.fields.random = rand();
    r.fields.uid = htons(uid);
    if (res == 0)
    {
	r.fields.date = htonl(tv.tv_sec);
    }
    else
    {
	r.fields.date = htonl(0);
    }
    
    r.fields.ip_local = laddr->s_addr;
    r.fields.ip_remote = faddr->s_addr;
    r.fields.port_local = htons(lport);
    r.fields.port_remote = htons(fport);

    r.fields.checksum = 0;
    for (i = 1; i < 6; i++)
    {
	r.longs[0] ^= r.longs[i];
    }

    des_ecb_encrypt((des_cblock *)&(r.longs[0]), (des_cblock *)&(r.longs[0]),
		    sched, DES_ENCRYPT);
    r.longs[2] ^= r.longs[0];
    r.longs[3] ^= r.longs[1];
    
    des_ecb_encrypt((des_cblock *)&(r.longs[2]), (des_cblock *)&(r.longs[2]),
		    sched, DES_ENCRYPT);
    r.longs[4] ^= r.longs[2];
    r.longs[5] ^= r.longs[3];
    
    des_ecb_encrypt((des_cblock *)&(r.longs[4]), (des_cblock *)&(r.longs[4]),
		    sched, DES_ENCRYPT);

    for (i = 0, j = 0; i < 24; i+=3, j+=4)
    {
	result[j  ] = to_asc[63 & (r.chars[i  ] >> 2)];
	result[j+1] = to_asc[63 & ((r.chars[i  ] << 4) + (r.chars[i+1] >> 4))];
	result[j+2] = to_asc[63 & ((r.chars[i+1] << 2) + (r.chars[i+2] >> 6))];
	result[j+3] = to_asc[63 & (r.chars[i+2])];
    }
    result[32] = '\0';

    return result;
}

#define MAX_KEYS 1024

static des_cblock keys[MAX_KEYS];
static int num_keys;

void init_decryption(key_file)
    FILE *key_file;
{
    char buf[1024];

    num_keys = 0;
    
    while (fgets(buf, 1024, key_file) != NULL && num_keys < MAX_KEYS)
    {
	des_string_to_key(buf, &keys[num_keys++]);
    }
    
    des_set_key (&keys [0], sched);
}


static char readable[75];

char *decrypt_packet(packet)
    char *packet;
{
    union data r;
    int i, j, k;
    time_t date_in_sec;
    char *date_in_ascii;

    for (k = -1; k < num_keys; k++)
    {
	if (k >= 0)
	    des_set_key(&keys [k], sched);
	
	for(i = 0, j = 0; i < 24; i+=3, j+=4)
	{
	    r.chars[i  ] = (to_bin[packet[j  ]]<<2)+(to_bin[packet[j+1]]>>4);
	    r.chars[i+1] = (to_bin[packet[j+1]]<<4)+(to_bin[packet[j+2]]>>2);
	    r.chars[i+2] = (to_bin[packet[j+2]] << 6) + (to_bin[packet[j+3]]);
	}
    
	des_ecb_encrypt((des_cblock *)&(r.longs[4]),
			(des_cblock *)&(r.longs[4]),
			sched, DES_DECRYPT);
	r.longs[4] ^= r.longs[2];
	r.longs[5] ^= r.longs[3];
	
	des_ecb_encrypt((des_cblock *)&(r.longs[2]),
			(des_cblock *)&(r.longs[2]),
			sched, DES_DECRYPT);
	
	r.longs[2] ^= r.longs[0];
	r.longs[3] ^= r.longs[1]; 
	des_ecb_encrypt((des_cblock *)&(r.longs[0]),
			(des_cblock *)&(r.longs[0]),
			sched, DES_DECRYPT);

	for (i = 1; i < 6; i++)
	{
	    r.longs[0] ^= r.longs[i];
	}
	
	if (r.fields.checksum == 0)
	    goto GoodKey;
    }
    return NULL;

  GoodKey:
    date_in_sec = ntohl(r.fields.date);
    date_in_ascii = ctime(&date_in_sec);
    
    sprintf(readable, "%24.24s %u %u.%u.%u.%u %u %u.%u.%u.%u %u",
	    date_in_ascii, ntohs(r.fields.uid),
	    ((unsigned char *) &r.fields.ip_local)[0],
	    ((unsigned char *) &r.fields.ip_local)[1],
	    ((unsigned char *) &r.fields.ip_local)[2],
	    ((unsigned char *) &r.fields.ip_local)[3],
	    (unsigned) ntohs(r.fields.port_local),
	    ((unsigned char *) &r.fields.ip_remote)[0],
	    ((unsigned char *) &r.fields.ip_remote)[1],
	    ((unsigned char *) &r.fields.ip_remote)[2],
	    ((unsigned char *) &r.fields.ip_remote)[3],
	    (unsigned) ntohs(r.fields.port_remote));
    
    return readable;
}

#else

#ifndef NeXT31
int dummy = 0;   /* Just here to make some compilers shut up :-) */
#endif

#endif

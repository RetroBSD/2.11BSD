/*
** idecrypt.c                                     Crypto extension to pidentd.
**
** This file is in the public domain. -- Planar 1994.02.22
**
** Decryption utility.
*/

#ifdef INCLUDE_CRYPT

#include <stdio.h>
#include "paths.h"
#include "crypto.h"

char is_base_64 [] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
};


void decrypt_file (f)
    FILE *f;
{
    int c;
    int i;
    char buf[32];
    char *cleartext;

    while (1)
    {
	c = getc(f);
	
      Same:
	if (c == EOF)
	    return;
	
	if (c != '[')
	{
	    putchar(c);
	    continue;
	}
	
	for (i = 0; i < 32; i++)
	{
	    c = getc(f);
	    if (c == EOF)
		break;
	    if (!is_base_64[c])
		break;
	    buf [i] = c;
	}
	
	if (i == 32)
	    c = getc(f);
	
	if (i < 32 || c != ']')
	{
	    putchar('[');
	    fwrite(buf, 1, i, stdout);
	    goto Same;
	}
	
	cleartext = decrypt_packet(buf);
	if (cleartext == NULL)
	{
	    putchar('[');
	    fwrite(buf, 1, 32, stdout);
	    putchar(']');
	}
	else
	{
	    printf("%s", cleartext);
	}
    }
}

main (argc, argv)
     int argc;
     char **argv;
{
    int i;
    FILE *f;
    FILE *key_file;
    
    key_file = fopen(PATH_DESKEY, "r");
    if (key_file == NULL)
    {
	fprintf(stderr, "idecrypt: cannot open key file: ");
	perror(PATH_DESKEY);
	exit (3);
    }
    
    init_decryption(key_file);
    close(key_file);
    
    if (argc < 2)
    {
	decrypt_file(stdin);
    }
    else
    {
	for (i = 1; i < argc; i++)
	{
	    if (!strcmp(argv [i], "-"))
	    {
		decrypt_file(stdin);
		continue;
	    }
	    
	    f = fopen(argv [i], "r");
	    if (f == NULL)
	    {
		perror(argv [i]);
		continue;
	    }
	    
	    decrypt_file(f);
	    fclose(f);
	}
    }
    
    exit(0);
}

#else /* no INCLUDE_CRYPT */

#include <stdio.h>

int main ()
{
    fprintf(stderr, "idecrypt: compiled without encryption\n");
    exit(2);
}

#endif

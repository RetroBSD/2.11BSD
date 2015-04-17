/* Crypto extension to pidentd.
   This file is in the public domain. -- Planar 1994.02.22

   Description of the crypto routines.
*/

extern void init_encryption ();
extern char *make_packet ();
extern void init_decryption ();
extern char *decrypt_packet ();

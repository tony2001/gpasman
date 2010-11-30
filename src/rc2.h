/* Header file for rc2 implementation by Matthew Palmer <mjp16@uow.edu.au> */

/* Externally worked functions */
void rc2_expandkey(char [], int, int);
void rc2_encrypt(unsigned short *);
void rc2_decrypt(unsigned short *);

/* The internals */
void _rc2_mix(unsigned short *);
void _rc2_mash(unsigned short *);
void _rc2_rmix(unsigned short *);
void _rc2_rmash(unsigned short *);
int _rc2_pow(int, int);
unsigned short _rc2_ror(unsigned short, int);
unsigned short _rc2_rol(unsigned short, int);

/* End rc2.h */


#ifndef CRC32_H
#define CRC32_H

#define UPDC32(octet, crc) (crc_32_tab[((crc)\
			^ (octet)) & 0xff] ^ ((crc) >> 8))

unsigned int crc32buffer(const unsigned char *buffer, const unsigned int len, unsigned int oldcrc32);
unsigned int crc32simple(void *buf, unsigned int size);

#endif /* CRC32_H */

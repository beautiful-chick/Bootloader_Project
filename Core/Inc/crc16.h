/*
 *==========================================================================
 *
 *      16 bit CRC with polynomial x^16+x^12+x^5+1
 *
 *==========================================================================
 */

#ifndef CRC16_H_
#define CRC16_H_

#include <stdint.h>

extern uint16_t crc16_checksum(unsigned char *buf, int len);

#endif /* CRC16_H_ */

/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _BITVECTOR_H
#define _BITVECTOR_H

#define BITVECTOR_NIBBLE_SIZE 8

void BitVector_SetBit(uint8* vector, int* offset);
void BitVector_ClearBit(uint8* vector, int* offset);
void BitVector_WriteNibblet(uint8* vector, int nibble, int* offset);

// Original readers. Kept for compatibility with existing GGPO code.
int BitVector_ReadBit(uint8* vector, int* offset);
int BitVector_ReadNibblet(uint8* vector, int* offset);

// Bounds-checked readers.
// bit_count represents the total number of valid bits in the buffer.
// On failure, offset and output values remain unchanged.
bool BitVector_TryReadBit(const uint8* vector, int bit_count, int* offset, int* value);
bool BitVector_TryReadNibblet(const uint8* vector, int bit_count, int* offset, int* value);

#endif // _BITVECTOR_H

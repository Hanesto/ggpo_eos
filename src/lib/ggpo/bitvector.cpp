/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "types.h"
#include "bitvector.h"

void
BitVector_SetBit(uint8* vector, int* offset)
{
   vector[(*offset) / 8] |= (1 << ((*offset) % 8));
   *offset += 1;
}

void
BitVector_ClearBit(uint8* vector, int* offset)
{
   vector[(*offset) / 8] &= ~(1 << ((*offset) % 8));
   *offset += 1;
}

void
BitVector_WriteNibblet(uint8* vector, int nibble, int* offset)
{
   ASSERT(nibble < (1 << BITVECTOR_NIBBLE_SIZE));

   for (int i = 0; i < BITVECTOR_NIBBLE_SIZE; i++)
   {
      if (nibble & (1 << i)){ BitVector_SetBit(vector, offset); }
      else{ BitVector_ClearBit(vector, offset); }
   }
}

// Original unbounded reader.
// Retained for compatibility with existing GGPO code.
int
BitVector_ReadBit(uint8* vector, int* offset)
{
   int retval = !!(vector[(*offset) / 8] & (1 << ((*offset) % 8)));
   *offset += 1;

   return retval;
}

// Original unbounded reader.
// Retained for compatibility with existing GGPO code.
int
BitVector_ReadNibblet(uint8* vector, int* offset)
{
   int nibblet = 0;

   for (int i = 0; i < BITVECTOR_NIBBLE_SIZE; i++)
   {
      nibblet |= (BitVector_ReadBit(vector, offset) << i);
   }

   return nibblet;
}

// Bounds-checked single-bit reader.
bool
BitVector_TryReadBit(const uint8* vector, int bit_count, int* offset, int* value)
{
    if (!vector || !offset || !value){ return false;}
    if (bit_count < 0 || *offset < 0 || *offset >= bit_count){ return false; }

    const int bit_index = *offset;

    *value = !!(vector[bit_index / 8] & (1 << (bit_index % 8)));
    *offset = bit_index + 1;

    return true;
}

// Bounds-checked 8-bit reader.
bool
BitVector_TryReadNibblet(const uint8* vector, int bit_count, int* offset, int* value)
{
   if (!vector || !offset || !value){ return false; }
   if (bit_count < 0 || *offset < 0){ return false; }

   // A complete nibble must be available.
   if (*offset > bit_count - BITVECTOR_NIBBLE_SIZE){ return false; }

   int temporary_offset = *offset;
   int nibble = 0;

   for (int i = 0; i < BITVECTOR_NIBBLE_SIZE; i++)
   {
      int bit = 0;

      if (!BitVector_TryReadBit(vector, bit_count, &temporary_offset, &bit))
      {
         return false;
      }

      nibble |= bit << i;
   }

   // Commit only after all eight bits were read successfully.
   *offset = temporary_offset;
   *value = nibble;

   return true;
}

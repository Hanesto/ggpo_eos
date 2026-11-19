/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _STATIC_BUFFER_H
#define _STATIC_BUFFER_H

#include <types.h>

template<class T, int N> class StaticBuffer
{
public:
   StaticBuffer<T, N>() :
      _size(0) {
   } 

   T& operator[](int i) {
      ASSERT(i >= 0 && i < _size);
      return _elements[i];
   }

   void push_back(const T &t) {
      ASSERT(_size != (N-1));
      _elements[_size++] = t;
   }

   void remove(int index)
   {
      ASSERT(index >= 0 && index < _size);

      for (int i = index; i < _size - 1; i++)
      {
         _elements[i] = _elements[i + 1];
      }

      _size--;
   }

   int size() {
      return _size;
   }


protected:
   T        _elements[N];
   int      _size;
};

#endif

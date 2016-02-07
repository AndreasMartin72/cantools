/*  mdfsg.c --  access MDF signal groups
    Copyright (C) 2012-2015 Andreas Heitmann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>
#include <endian.h>
#include <byteswap.h>
#include "mdfsg.h"
#include "mdfmodel.h"


/* convert signal to double value */
double
mdf_signal_convert(const uint8_t *const data_int_ptr,
                   const mdf_t *const mdf,
                   const cn_block_t *const cn_block)
{
  /* decode */
  uint8_t number_bits  = cn_block->number_bits;
  uint8_t bit_offset   = cn_block->first_bit % 8;    /* LSB */
  uint8_t number_bytes = ( bit_offset + number_bits + 7 ) / 8;
  uint8_t group_bytes  = number_bytes <= 2 ? number_bytes : ( number_bytes <= 4 ? 4 : 8 );
  uint16_t default_byte_order_big_endian = id_block_get(mdf)->byte_order; /* 0==Intel, other=Motorola*/
  cc_block_t *cc_block = cc_block_get(mdf, cn_block->link_conversion_formula);
  double converted_double;
  int64_t data_int64;
  double data_ieee754;
  const int sdt = cn_block->signal_data_type;
  const int cn_is_big_endian =     (sdt == sdt_unsigned_int_big_endian)
                                || (sdt == sdt_signed_int_big_endian)
                                || (sdt == sdt_ieee754_float_big_endian)
                                || (sdt == sdt_ieee754_double_big_endian)
                                || (default_byte_order_big_endian &&
                                   (   (sdt == sdt_unsigned_int_default)
                                    || (sdt == sdt_signed_int_default)
                                    || (sdt == sdt_ieee754_float_default)
                                    || (sdt == sdt_ieee754_double_default)));
  /* swap words if channel endianess differs from machine endianess */
#ifdef WORDS_BIGENDIAN
  const int swap = !cn_is_big_endian;
#else
  const int swap = cn_is_big_endian;
#endif

  uint8_t buffer[8];
  const uint8_t* data_ptr = data_int_ptr;

  if(cn_is_big_endian)
  {
    for( int i = 0; i < number_bytes; i++ )
    {
      buffer[group_bytes-i-1] = data_ptr[number_bytes-i-1];
    }
  }
  else
  {
    for( int i = 0; i < number_bytes; i++ )
    {
      buffer[i] = data_ptr[i];
    }
  }

  if(swap)
  {
    for( int i = 0; i < group_bytes/2; i++ )
    {
        buffer[i] = buffer[group_bytes-i-1];
    }
  }
          
  data_ptr = buffer;

  /* extract data */
  switch(sdt) 
  {
    case sdt_unsigned_int_default:
    case sdt_unsigned_int_big_endian:
    case sdt_unsigned_int_little_endian:
      if(number_bytes == 1) 
      {
        data_int64 = ((*(uint8_t*)data_ptr) >> bit_offset);
        data_int64 &= (1u << number_bits) - 1u;
      } 
      else if(number_bytes == 2) 
      {
        data_int64  = ((*(uint16_t*)data_ptr) >> bit_offset);
        data_int64 &= (1u << number_bits) - 1u;
      } 
      else if(number_bytes == 4) 
      {
        data_int64  = ((*(uint32_t*)data_ptr) >> bit_offset);
        data_int64 &= (1u << number_bits) - 1u;
      } 
      else {
        assert(bit_offset + number_bits <= 64);
        data_int64 = (*(uint64_t*)data_ptr) >> bit_offset;

        if(number_bits < 64) 
        {
          data_int64 &= ((uint64_t)1 << number_bits) - 1u;
        }
      }
      break;
    case sdt_signed_int_default:
    case sdt_signed_int_big_endian:
    case sdt_signed_int_little_endian:
      assert(number_bits >= 2);
      if(number_bytes == 1) 
      {
        uint8_t data_u8;

        data_u8 = *(uint8_t*)data_ptr;
        data_u8 <<= 8 - number_bits - bit_offset;
        data_int64 = (int64_t)((*(int8_t*)&data_u8) >> (8-number_bits));
      } 
      else if(number_bytes == 2) 
      {
        uint16_t data_u16;

        data_u16 = *(uint16_t*)data_ptr;
        data_u16 <<= 16 - number_bits - bit_offset;
        data_int64 = (int64_t)((*(int16_t*)&data_u16) >> (16-number_bits));
      } 
      else if(number_bytes == 4) 
      {
        uint32_t data_u32;

        data_u32 = *(uint32_t*)data_ptr;
        data_u32 <<= 32 - number_bits - bit_offset;
        data_int64 = (int64_t)((*(int32_t*)&data_u32) >> (32-number_bits));
      } 
      else 
      {
        uint64_t data_u64;

        assert(bit_offset + number_bits <= 64);

        data_u64 = *(uint64_t*)data_ptr;
        data_u64 <<= 64 - number_bits - bit_offset;
        data_int64 = (int64_t)((*(int64_t*)&data_u64) >> (64-number_bits));
      }
      break;
    case sdt_ieee754_float_default:
    case sdt_ieee754_float_big_endian:
    case sdt_ieee754_float_little_endian:
      {
        uint32_t data_u32 = *(uint32_t*)data_ptr;
        data_ieee754 = *(float *)&data_u32;
        assert( number_bits = 32 && bit_offset == 0 );
      }
      break;
    case sdt_ieee754_double_default:
    case sdt_ieee754_double_big_endian:
    case sdt_ieee754_double_little_endian:
      {
        uint64_t data_u64 = *(uint64_t*)data_ptr;
        data_ieee754 = *(double *)&data_u64;
        assert( number_bits = 64 && bit_offset == 0 );
      }
      break;
    case sdt_string: /* string */
      data_int64 = 0;
      break;
    default:
	  mdf_fprintf(stderr,"signal_data_type %hu not implemented\n",
	              (unsigned short)sdt);
      exit(EXIT_FAILURE);
  }

  /* convert data */
  if(cc_block) {
    switch(cc_block->conversion_type) {
    case 0:
      switch(sdt) {
      case sdt_unsigned_int_default:
      case sdt_unsigned_int_big_endian:
      case sdt_string:
        converted_double = ((uint64_t)data_int64) * cc_block->supplement.linear.p2
          +cc_block->supplement.linear.p1;
        break;
      case sdt_signed_int_default:
      case sdt_signed_int_big_endian:
        converted_double = data_int64 * cc_block->supplement.linear.p2
          +cc_block->supplement.linear.p1;
        break;
      case sdt_ieee754_float_default:
      case sdt_ieee754_float_big_endian:
      case sdt_ieee754_float_little_endian:
      case sdt_ieee754_double_default:
      case sdt_ieee754_double_big_endian:
      case sdt_ieee754_double_little_endian:
        converted_double = data_ieee754 * cc_block->supplement.linear.p2
                         + cc_block->supplement.linear.p1;
        break;
      default:
        assert(1);
      }
      break;
    case 11: /* text table lookup. use raw value for now */
      converted_double = (double)data_int64;
      break;
    case 12:
      converted_double = (double)data_int64;
      break;
    case 65535: /* 65535 = 1:1 conversion formula (Int = Phys) */
      converted_double = (double)data_int64;
      break;
    default:
      converted_double = 0.0;
      mdf_fprintf(stderr,"conversion %hu not implemented\n",
                  (unsigned short)cc_block->conversion_type);
      exit(EXIT_FAILURE);
    }
  } else {
    switch(sdt) {
      case sdt_unsigned_int_default:
      case sdt_unsigned_int_big_endian:
      case sdt_string:
        converted_double = (double)(uint64_t)data_int64;
        break;
      case sdt_signed_int_default:
      case sdt_signed_int_big_endian:
        converted_double = (double)data_int64;
        break;
    case sdt_ieee754_float_default:
    case sdt_ieee754_double_default:
    case sdt_ieee754_float_big_endian:
      converted_double = data_ieee754;
      break;
    default:
      assert(1);
    }
  }
  return converted_double;
}


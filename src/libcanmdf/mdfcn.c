/*  mdfcn.c -- process channel block
    Copyright (C) 2012-2017 Andreas Heitmann

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

#include "cantools_config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "mdfcn.h"
#include "mdfsg.h"

void
mdfProcessChannel(const mdf_t *const mdf,
                  const cg_block_t *const cg_block,
                  const cn_block_t *const cn_block,
                  const filter_t *const filter,
                  const double *const time,
                  const double *const value,
                  mdfSignalCb_t const mdfSignalCb,
                  const void *const cbData)
{
  uint32_t can_id, can_channel;
  ce_block_t *ce_block;
  char *message;
  char *signal_name = cn_get_long_name(mdf, cn_block);
  double *timeValue;
  const size_t nbytes = sizeof(double)*cg_block->number_of_records;

  if(cg_block->number_of_records > 0) {
    timeValue = (double *)mdf_malloc(
         2 * (size_t)cg_block->number_of_records * sizeof(double));
    memcpy(timeValue, time,  nbytes);
    memcpy(&timeValue[cg_block->number_of_records], value, nbytes);
  } else {
    timeValue = NULL;
  }

  /* message info */
  ce_block = ce_block_get(mdf, cn_block->link_extensions);
  ce_get_message_info(ce_block, &message, &can_id, &can_channel);
  
  if(mdf->verbose_level >= 2) {
    mdf_printf("name=%s, records = %lu, ID=0x%lx, can_ch=%lu, message=%s\n",
           signal_name,
           (unsigned long)cg_block->number_of_records,
           (unsigned long)can_id, 
           (unsigned long)can_channel,
           message);
  }

  /* Time series data for signal prepared. Invoke callback function */
  mdfSignalCb(mdf, can_channel, cg_block->number_of_records, 
              cn_block->channel_type, message, signal_name,
              timeValue, filter, cbData);

  mdf_free(message);
  mdf_free(signal_name);
  mdf_free(timeValue);
}

void
mdfProcessChannelsSorted(const mdf_t *const mdf,
                         const filter_t *const filter,
                         link_t link,
                         uint32_t number_of_records,
                         uint16_t number_record_ids,
                         uint16_t record_size,
                         uint8_t *data_base,
                         mdfSignalCb_t const mdfSignalCb,
                         const void *const cbData)
{
  const cn_block_t *cn_block;
  int icn;
  uint32_t can_id, can_channel;
  uint8_t first_record_id, second_record_id;
  uint32_t ir;
  double *timeValue;
  double *targetArray;
  size_t dims[2];
  sint32_t i_channel_type;
  uint16_t version_number = id_block_get(mdf)->version_number;
  uint16_t id_block_standard_flags = id_block_get(mdf)->standard_flags;

  dims[0] = number_of_records;
  dims[1] = 2;

  if(id_block_standard_flags &
     id_block_standard_flags_update_of_record_counters_required) {
    number_of_records = 0;
  }

  if(number_of_records > 0) {
    timeValue = (double *)mdf_malloc(sizeof(double)*2*number_of_records);
  } else {
    timeValue = NULL;
  }

  /*
   * skip channels with 0 number of records
   */
  if(number_of_records > 0) {
    for(i_channel_type = 1; i_channel_type >=0; i_channel_type --) {
      /* 0: data, 1: time. Start with time to reuse it for signal data
         of all channels of the channel group */ 
      for( cn_block = cn_block_get(mdf, link)                        , icn=0;
           cn_block;
           cn_block = cn_block_get(mdf, cn_block->link_next_cn_block), icn++) {
        uint8_t *data;
        ce_block_t *ce_block;
        char* message;
        char *signal_name = cn_get_long_name(mdf, cn_block);

        /* match channel type. start with time channel per outer loop */
        if(cn_block->channel_type != (uint32_t)i_channel_type) 
        {
          mdf_free( signal_name );
          continue;
        }

        /* target array */
        targetArray = (i_channel_type == 1)
          ?&timeValue[0]
          :&timeValue[number_of_records];

        /* message info */
        ce_block = ce_block_get(mdf, cn_block->link_extensions);
        ce_get_message_info(ce_block, &message, &can_id, &can_channel);

        /* loop over records */
        data = data_base;

        if(number_record_ids >= 1) {
          first_record_id = *data++;
        }
        for(ir=0; ir<number_of_records; ir++) {
          const uint8_t *data_int_ptr;
          uint32_t offset = cn_block->first_bit/8;

          if(version_number >= 300) {
            offset += cn_block->additional_byte_offset;
          }
          data_int_ptr = &data[offset];

          /* convert and store value */
          *targetArray++ = mdf_signal_convert(data_int_ptr, mdf, cn_block);

          /* next data record */
          data += record_size;
        } /* for all records */

        if(number_record_ids == 2) {
          second_record_id = *data++;
        }
      
        /* Time series data for signal prepared. Invoke callback function */
        mdfSignalCb(mdf, can_channel,
                    number_of_records, 
                    cn_block->channel_type,
                    message,
                    signal_name,
                    timeValue,
                    filter,
                    cbData);

        mdf_free(message);
        mdf_free(signal_name);
      }
    }
  }
  
  if(timeValue != NULL) {
    mdf_free(timeValue);
  }
}

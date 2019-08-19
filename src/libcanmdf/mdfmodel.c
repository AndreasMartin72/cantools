/*  mdfmodel.c --  MDF model access functions
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

#include <string.h>
#include "mdfmodel.h"

inline bool check_corrupt(const mdf_t *const mdf, const link_t lnk, size_t obj_size)
{
  if( !lnk || !mdf )
  {
    return false;
  }

  if( lnk > mdf->size-obj_size )
  {
    const_cast<mdf_t*>(mdf)->corrupt = 1;
    return false;
  }

  return true;
}

id_block_t *
id_block_get(const mdf_t *const mdf)
{
  return (id_block_t *)mdf->base;
}

hd_block_t *
hd_block_get(const mdf_t *const mdf)
{
  return (hd_block_t *)(mdf->base+64);
}

dg_block_t *
dg_block_get(const mdf_t *const mdf, const link_t lnk)
{
  if(!check_corrupt(mdf, lnk, sizeof(dg_block_t))) return NULL;
  return (dg_block_t *)(mdf->base+lnk);
}

cg_block_t *
cg_block_get(const mdf_t *const mdf, const link_t lnk)
{
  if(!check_corrupt(mdf, lnk, sizeof(cg_block_t))) return NULL;
  return (cg_block_t *)(mdf->base+lnk);
}

cn_block_t *
cn_block_get(const mdf_t *const mdf, const link_t lnk)
{
  if(!check_corrupt(mdf, lnk, sizeof(cn_block_t))) return NULL;
  return (cn_block_t *)(mdf->base+lnk);
}

ce_block_t *
ce_block_get(const mdf_t *const mdf, const link_t lnk)
{
  if(!check_corrupt(mdf, lnk, sizeof(ce_block_t))) return NULL;
  return (ce_block_t *)(mdf->base+lnk);
}

tx_block_t *
tx_block_get(const mdf_t *const mdf, const link_t lnk)
{
  if(!check_corrupt(mdf, lnk, sizeof(tx_block_t))) return NULL;
  return (tx_block_t *)(mdf->base + lnk);
}

const char *
tx_block_get_text(const mdf_t *const mdf, link_t lnk)
{
  if(!check_corrupt(mdf, lnk, sizeof(tx_block_t))) return NULL;
  return &((tx_block_t *)(mdf->base + lnk))->text1;
}

pr_block_t *
pr_block_get(const mdf_t *const mdf, const link_t lnk)
{
  if(!check_corrupt(mdf, lnk, sizeof(pr_block_t))) return NULL;
  return (pr_block_t *)(mdf->base + lnk);
}

dr_block_t *
dr_block_get(const mdf_t *const mdf, const link_t lnk)
{
  if(!check_corrupt(mdf, lnk, sizeof(dr_block_t))) return NULL;
  return (dr_block_t *)(mdf->base + lnk);
}

cc_block_t *
cc_block_get(const mdf_t *const mdf, const link_t lnk)
{
  if(!check_corrupt(mdf, lnk, sizeof(cc_block_t))) return NULL;
  return (cc_block_t *)(mdf->base + lnk);
}

char *
cn_get_long_name(const mdf_t *const mdf, const cn_block_t *const cn_block)
{
  uint16_t version_number = id_block_get(mdf)->version_number;
  const char *asam_name;
  char *name;

  if(version_number >= 212) {
    asam_name = tx_block_get_text(mdf, cn_block->link_asam_mcd_name);
  } else {
    asam_name = NULL;
  }
  if(asam_name != NULL) {
    name = mdf_strdup(asam_name);
  } else {
    name = mdf_strndup((char*)cn_block->signal_name,sizeof(cn_block->signal_name));
  }
  return name;
}

char *
ce_get_message_name(const ce_block_t *const ce_block)
{
  char *message;
  
  /* message info */
  if(ce_block != NULL) {
    switch(ce_block->extension_type) {
    case 19:
      message = mdf_strndup(
        (const char *)ce_block->supplement.vector_can.message_name,
        sizeof(ce_block->supplement.vector_can.message_name));
      break;
    case 2:
      message = mdf_strndup(
        (const char *)ce_block->supplement.dim.description,
        sizeof(ce_block->supplement.dim.description));
      break;
    default:
      message = mdf_strdup("(undef)");
      break;
    }
  } else {
    message = mdf_strdup("(undef)");
  }
  return message;
}

void
ce_get_message_info(const ce_block_t *const ce_block,
                    char **const message_name_ptr,
                    uint32_t *const can_id_ptr,
                    uint32_t *const can_channel_ptr)
{
  if(ce_block != NULL) {
    switch(ce_block->extension_type) {
    case 19:
      *can_id_ptr      = ce_block->supplement.vector_can.can_id;
      *can_channel_ptr = ce_block->supplement.vector_can.can_channel;
      break;
    case 2:
    default:
      *can_id_ptr      = 0;
      *can_channel_ptr = 0;
    }
    *message_name_ptr = ce_get_message_name(ce_block);
  } else {
    *can_id_ptr        = 0;
    *can_channel_ptr   = 0;
    *message_name_ptr  = mdf_strdup("(undef)");
  }
} 

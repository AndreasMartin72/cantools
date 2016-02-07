/*  mdffile.c --  handle MDF file connection
    Copyright (C) 2012-2014 Andreas Heitmann

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

#include "config.h"

#ifndef __GNUC__
  #include <io.h>
#else
  #define _open open
#endif


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "mdfmodel.h"
#include "mdffile.h"

const mdf_t *
mdf_attach(const char *filename, int verbose_level)
{
  MDF_CREATE(mdf_t, mdf);

  if( mdf )
  {
    mdf->verbose_level = verbose_level;
    mdf->corrupt = 0;

    /* open input file */
    mdf->fd = _open(filename, O_RDONLY);
    if(mdf->fd != -1) {
      struct stat sb;

      fstat(mdf->fd, &sb);
      mdf->size = sb.st_size;
      mdf->base = (uint8_t*)mmap(NULL, mdf->size, PROT_READ,
                                 MAP_PRIVATE, mdf->fd, (off_t)0);
      if(mdf->base == MAP_FAILED) {
        mdf_fprintf(stderr, "mdf_attach(): can't mmap MDF file %s\n",filename);
        MDF_FREE(mdf);
        mdf = NULL;
      }
    } else {
      mdf_fprintf(stderr, "mdf_attach(): can't open MDF file %s\n",filename);
      MDF_FREE(mdf);
      mdf = NULL;
    }
  }

  return mdf;
}

void
mdf_detach(mdf_t ** mdf)
{
  if( mdf && *mdf)
  {
    munmap((*mdf)->base, (*mdf)->size);
    close((*mdf)->fd);
    MDF_FREE(*mdf);
  }
}

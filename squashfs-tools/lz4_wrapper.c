/*
 * Copyright (c) 2010 LG Electronics
 * Chan Jeong <chan.jeong@lge.com>
 *
 * All modifications Copyright (c) 2010
 * Phillip Lougher <phillip@lougher.demon.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * lzo_wrapper.c
 */

#include <stdlib.h>
#include <string.h>

#include "lz4/lz4.h"

#include "squashfs_fs.h"
#include "compressor.h"

static int squashfs_lz4_options(char *argv[], int argc)
{
	return 0;
}

static int squashfs_lz4_init(void **strm, int block_size, int flags)
{
	return 0;
}


static int squashfs_lz4_compress(void *strm, void *d, void *s, int size, int block_size,
		int *error)
{
	unsigned int outlen;

	outlen = LZ4_compress(s, d, size);
	if(outlen <= 0)
		goto failed;
	if(outlen >= size)
		/*
		 * Output buffer overflow. Return out of buffer space
		 */
		return 0;

	return outlen;

failed:
	/*
	 * All other errors return failure, with the compressor
	 * specific error code in *error
	 */
	*error = 0;
	return -1;
}


static int squashfs_lz4_uncompress(void *d, void *s, int size, int block_size, int *error)
{
	int res;

	res = LZ4_decompress_safe(s, d, size, block_size);
	if(res <= 0){
		*error = res;
		return -1;
	}

	*error = 0;
	return res;
}

struct compressor lz4_comp_ops = {
	.init = squashfs_lz4_init,
	.compress = squashfs_lz4_compress,
	.uncompress = squashfs_lz4_uncompress,
	.options = squashfs_lz4_options,
	.usage = NULL,
	.id = LZ4_COMPRESSION,
	.name = "lz4",
	.supported = 1
};


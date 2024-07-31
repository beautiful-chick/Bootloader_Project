/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

	Based on ringbuffer.c by Patrick Prasse (patrick.prasse@gmx.net). Code
	has been modified by Telenor and Gemalto.

 */


#include <string.h>
#include <assert.h>
#include "ringbuf.h"

void rb_init (struct ring_buffer *rb, uint8_t* buf, int size)
{
    memset (rb, 0, sizeof (struct ring_buffer));
    rb->rd_pointer = 0;
    rb->wr_pointer = 0;
    rb->buffer= buf;
    memset(buf, 0, size);
    rb->size = size;
}

int rb_data_size (struct ring_buffer *rb)
{
    return ((rb->wr_pointer - rb->rd_pointer) & (rb->size-1));
}

int rb_free_size (struct ring_buffer *rb)
{
    return (rb->size - 1 - rb_data_size(rb));
}

int rb_write (struct ring_buffer *rb, uint8_t * buf, int len)
{
    int total;
    int i;

    /* total = len = min(space, len) */
    total = rb_free_size(rb);
    if(len > total)
        len = total;
    else
        total = len;

    i = rb->wr_pointer;
    if(i + len > rb->size)
    {
        memcpy(rb->buffer + i, buf, rb->size - i);
        buf += rb->size - i;
        len -= rb->size - i;
        i = 0;
    }
    memcpy(rb->buffer + i, buf, len);
    rb->wr_pointer = i + len;
    return total;
}

int rb_read (struct ring_buffer *rb, uint8_t * buf, int max)
{
    int total;
    int i;
    /* total = len = min(used, len) */
    total = rb_data_size(rb);

    if(max > total)
        max = total;
    else
        total = max;

    i = rb->rd_pointer;
    if(i + max > rb->size)
    {
        memcpy(buf, rb->buffer + i, rb->size - i);
        buf += rb->size - i;
        max -= rb->size - i;
        i = 0;
    }
    memcpy(buf, rb->buffer + i, max);
    rb->rd_pointer = i + max;
    return total;
}

void rb_clear (struct ring_buffer *rb)
{
    memset(rb->buffer,0,rb->size);
    rb->rd_pointer=0;
    rb->wr_pointer=0;
}

uint8_t rb_peek(struct ring_buffer* rb, int index)
{
	assert(index < rb_data_size(rb));

	return rb->buffer[((rb->rd_pointer + index) % rb->size)];
}

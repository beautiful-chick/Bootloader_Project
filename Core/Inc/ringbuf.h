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

	Based on ringbuffer.h by Patrick Prasse (patrick.prasse@gmx.net). Code
	has been modified by Telenor and Gemalto.

 */

#ifndef __RINGBUF_H_
#define __RINGBUF_H_

#include <sys/types.h>
#include <stdint.h>

struct ring_buffer {
	uint8_t *buffer;
    int wr_pointer;
    int rd_pointer;
    int size;
};

/* Initial the ring buffer */
void rb_init (struct ring_buffer *ring, uint8_t* buff, int size) ;

/*  Description: Write $len bytes data in $buf into ring buffer $rb 
 * Return Value: The actual written into ring buffer data size, if ring buffer 
 * left space size small than $len, then only part of the data be written into.
 */
int rb_write (struct ring_buffer *rb, uint8_t * buf, int len) ;

/* Get ring buffer left free size  */
int rb_free_size (struct ring_buffer *rb);

/* Read $max bytes data from ring buffer $rb to $buf */
int rb_read (struct ring_buffer *rb, uint8_t * buf, int max);

/* Read a specify $index byte data in ring buffer $rb  */
uint8_t rb_peek(struct ring_buffer* rb, int index);

/* Get data size in the ring buffer  */
int rb_data_size (struct ring_buffer *);

/* Clear the ring buffer data  */
void rb_clear (struct ring_buffer *rb) ;

#endif /* __RINGBUF_H_ */

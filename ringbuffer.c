/*
 * ResCoin - Multiple type Resource Scheduling of VMs in pursuit of fairness 
 * Copyright (c) 2014, Coperd <lhcwhu@gmail.com>
 * All rights reserved.
 *
 * Author: Coperd <lhcwhu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <errno.h>
#include "ringbuffer.h"

void rb_init(struct ringbuffer *rb, int size)
{
    rb->size = size + 1;
    rb->start = 0;
    rb->end = 0;
    rb->buff = (elemtype *)malloc(sizeof(elemtype) * rb->size);
}

void rb_free(struct ringbuffer *rb)
{
    free(rb->buff);
}

int rb_if_full(struct ringbuffer *rb)
{
    return (rb->end + 1) % rb->size == rb->start;
}

int rb_is_empty(struct ringbuffer *rb)
{
    return rb->end == rb->start;
}

void rb_write(struct ringbuffer *rb, elemtype *val)
{
    rb->buff[rb->end] = *val;
    rb->end = (rb->end + 1) % rb->size;
    if (rb->end == rb->start)
        /*overwrite */
        rb->start = (rb->start + 1) % rb->size;
}

void rb_read(struct ringbuffer *rb, elemtype *val)
{
    *val = rb->buff[rb->start];
    rb->start = (rb->start + 1) % rb->size;
}

void rb_read_last(struct ringbuffer *rb, elemtype *val)
{
    int pos = (rb->end - 1 + rb->size) % rb->size;
    *val = rb->buff[pos];
}

void rb_read_ith(struct ringbuffer *rb, elemtype *val)
{
    if ((rb->end - rb->start) < k)
       exit;
    int pos = (rb->end - k + rb->size) % rb->size;
    *val = rb->buff[pos];
}

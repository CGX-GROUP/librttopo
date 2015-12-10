/**********************************************************************
 * $Id: bytebuffer.c 11218 2013-03-28 13:32:44Z robe $
 *
 * rttopo - topology library
 * Copyright 2015 Nicklas Avén <nicklas.aven@jordogskog.no>
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * The name of the author may not be used to endorse or promote
 * products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 **********************************************************************/


#include "librtgeom_internal.h"
#include "bytebuffer.h"

/**
* Allocate a new bytebuffer_t. Use bytebuffer_destroy to free.
*/
bytebuffer_t* 
bytebuffer_create(RTCTX *ctx)
{
	RTDEBUG(2,"Entered bytebuffer_create");
	return bytebuffer_create_with_size(ctx, BYTEBUFFER_STARTSIZE);
}

/**
* Allocate a new bytebuffer_t. Use bytebuffer_destroy to free.
*/
bytebuffer_t* 
bytebuffer_create_with_size(RTCTX *ctx, size_t size)
{
	RTDEBUGF(2,"Entered bytebuffer_create_with_size %d", size);
	bytebuffer_t *s;

	s = rtalloc(ctx, sizeof(bytebuffer_t));
	s->buf_start = rtalloc(ctx, size);
	s->readcursor = s->writecursor = s->buf_start;
	s->capacity = size;
	memset(s->buf_start,0,size);
	RTDEBUGF(4,"We create a buffer on %p of %d bytes", s->buf_start, size);
	return s;
}

/**
* Allocate just the internal buffer of an existing bytebuffer_t
* struct. Useful for allocating short-lived bytebuffers off the stack.
*/
void 
bytebuffer_init_with_size(RTCTX *ctx, bytebuffer_t *b, size_t size)
{
	b->buf_start = rtalloc(ctx, size);
	b->readcursor = b->writecursor = b->buf_start;
	b->capacity = size;
	memset(b->buf_start, 0, size);
}

/**
* Free the bytebuffer_t and all memory managed within it.
*/
void 
bytebuffer_destroy(RTCTX *ctx, bytebuffer_t *s)
{
	RTDEBUG(2,"Entered bytebuffer_destroy");
	RTDEBUGF(4,"The buffer has used %d bytes",bytebuffer_getlength(ctx, s));
	
	if ( s->buf_start ) 
	{
		RTDEBUGF(4,"let's free buf_start %p",s->buf_start);
		rtfree(ctx, s->buf_start);
		RTDEBUG(4,"buf_start is freed");
	}
	if ( s ) 
	{
		rtfree(ctx, s);		
		RTDEBUG(4,"bytebuffer_t is freed");
	}
	return;
}

/**
* Set the read cursor to the beginning
*/
void 
bytebuffer_reset_reading(RTCTX *ctx, bytebuffer_t *s)
{
	s->readcursor = s->buf_start;
}

/**
* Reset the bytebuffer_t. Useful for starting a fresh string
* without the expense of freeing and re-allocating a new
* bytebuffer_t.
*/
void 
bytebuffer_clear(RTCTX *ctx, bytebuffer_t *s)
{
	s->readcursor = s->writecursor = s->buf_start;
}

/**
* If necessary, expand the bytebuffer_t internal buffer to accomodate the
* specified additional size.
*/
static inline void 
bytebuffer_makeroom(RTCTX *ctx, bytebuffer_t *s, size_t size_to_add)
{
	RTDEBUGF(2,"Entered bytebuffer_makeroom with space need of %d", size_to_add);
	size_t current_write_size = (s->writecursor - s->buf_start);
	size_t capacity = s->capacity;
	size_t required_size = current_write_size + size_to_add;

	RTDEBUGF(2,"capacity = %d and required size = %d",capacity ,required_size);
	while (capacity < required_size)
		capacity *= 2;

	if ( capacity > s->capacity )
	{
		RTDEBUGF(4,"We need to realloc more memory. New capacity is %d", capacity);
		s->buf_start = rtrealloc(ctx, s->buf_start, capacity);
		s->capacity = capacity;
		s->writecursor = s->buf_start + current_write_size;
		s->readcursor = s->buf_start + (s->readcursor - s->buf_start);
	}
	return;
}

/**
* Writes a uint8_t value to the buffer
*/
void 
bytebuffer_append_byte(RTCTX *ctx, bytebuffer_t *s, const uint8_t val)
{	
	RTDEBUGF(2,"Entered bytebuffer_append_byte with value %d", val);	
	bytebuffer_makeroom(ctx, s, 1);
	*(s->writecursor)=val;
	s->writecursor += 1;
	return;
}


/**
* Writes a uint8_t value to the buffer
*/
void 
bytebuffer_append_bulk(RTCTX *ctx, bytebuffer_t *s, void * start, size_t size)
{	
	RTDEBUGF(2,"bytebuffer_append_bulk with size %d",size);	
	bytebuffer_makeroom(ctx, s, size);
	memcpy(s->writecursor, start, size);
	s->writecursor += size;
	return;
}

/**
* Writes a uint8_t value to the buffer
*/
void 
bytebuffer_append_bytebuffer(RTCTX *ctx, bytebuffer_t *write_to,bytebuffer_t *write_from )
{	
	RTDEBUG(2,"bytebuffer_append_bytebuffer");	
	size_t size = bytebuffer_getlength(ctx, write_from);
	bytebuffer_makeroom(ctx, write_to, size);
	memcpy(write_to->writecursor, write_from->buf_start, size);
	write_to->writecursor += size;
	return;
}


/**
* Writes a signed varInt to the buffer
*/
void 
bytebuffer_append_varint(RTCTX *ctx, bytebuffer_t *b, const int64_t val)
{	
	size_t size;
	bytebuffer_makeroom(ctx, b, 8);
	size = varint_s64_encode_buf(ctx, val, b->writecursor);
	b->writecursor += size;
	return;
}

/**
* Writes a unsigned varInt to the buffer
*/
void 
bytebuffer_append_uvarint(RTCTX *ctx, bytebuffer_t *b, const uint64_t val)
{	
	size_t size;
	bytebuffer_makeroom(ctx, b, 8);
	size = varint_u64_encode_buf(ctx, val, b->writecursor);
	b->writecursor += size;
	return;
}


/*
* Writes Integer to the buffer
*/
void
bytebuffer_append_int(RTCTX *ctx, bytebuffer_t *buf, const int val, int swap)
{
	RTDEBUGF(2,"Entered bytebuffer_append_int with value %d, swap = %d", val, swap);	
	
	RTDEBUGF(4,"buf_start = %p and write_cursor=%p", buf->buf_start,buf->writecursor);
	char *iptr = (char*)(&val);
	int i = 0;

	if ( sizeof(int) != RTWKB_INT_SIZE )
	{
		rterror(ctx, "Machine int size is not %d bytes!", RTWKB_INT_SIZE);
	}
	
	bytebuffer_makeroom(ctx, buf, RTWKB_INT_SIZE);
	/* Machine/request arch mismatch, so flip byte order */
	if ( swap)
	{
		RTDEBUG(4,"Ok, let's do the swaping thing");	
		for ( i = 0; i < RTWKB_INT_SIZE; i++ )
		{
			*(buf->writecursor) = iptr[RTWKB_INT_SIZE - 1 - i];
			buf->writecursor += 1;
		}
	}
	/* If machine arch and requested arch match, don't flip byte order */
	else
	{
		RTDEBUG(4,"Ok, let's do the memcopying thing");		
		memcpy(buf->writecursor, iptr, RTWKB_INT_SIZE);
		buf->writecursor += RTWKB_INT_SIZE;
	}
	
	RTDEBUGF(4,"buf_start = %p and write_cursor=%p", buf->buf_start,buf->writecursor);
	return;

}





/**
* Writes a float64 to the buffer
*/
void
bytebuffer_append_double(RTCTX *ctx, bytebuffer_t *buf, const double val, int swap)
{
	RTDEBUGF(2,"Entered bytebuffer_append_double with value %lf swap = %d", val, swap);	
	
	RTDEBUGF(4,"buf_start = %p and write_cursor=%p", buf->buf_start,buf->writecursor);
	char *dptr = (char*)(&val);
	int i = 0;

	if ( sizeof(double) != RTWKB_DOUBLE_SIZE )
	{
		rterror(ctx, "Machine double size is not %d bytes!", RTWKB_DOUBLE_SIZE);
	}

	bytebuffer_makeroom(ctx, buf, RTWKB_DOUBLE_SIZE);
	
	/* Machine/request arch mismatch, so flip byte order */
	if ( swap )
	{
		RTDEBUG(4,"Ok, let's do the swapping thing");		
		for ( i = 0; i < RTWKB_DOUBLE_SIZE; i++ )
		{
			*(buf->writecursor) = dptr[RTWKB_DOUBLE_SIZE - 1 - i];
			buf->writecursor += 1;
		}
	}
	/* If machine arch and requested arch match, don't flip byte order */
	else
	{
		RTDEBUG(4,"Ok, let's do the memcopying thing");			
		memcpy(buf->writecursor, dptr, RTWKB_DOUBLE_SIZE);
		buf->writecursor += RTWKB_DOUBLE_SIZE;
	}
	
	RTDEBUG(4,"Return from bytebuffer_append_double");		
	return;

}

/**
* Reads a signed varInt from the buffer
*/
int64_t 
bytebuffer_read_varint(RTCTX *ctx, bytebuffer_t *b)
{
	size_t size;
	int64_t val = varint_s64_decode(ctx, b->readcursor, b->buf_start + b->capacity, &size);
	b->readcursor += size;
	return val;
}

/**
* Reads a unsigned varInt from the buffer
*/
uint64_t 
bytebuffer_read_uvarint(RTCTX *ctx, bytebuffer_t *b)
{	
	size_t size;
	uint64_t val = varint_u64_decode(ctx, b->readcursor, b->buf_start + b->capacity, &size);
	b->readcursor += size;
	return val;
}

/**
* Returns the length of the current buffer
*/
size_t 
bytebuffer_getlength(RTCTX *ctx, bytebuffer_t *s)
{
	return (size_t) (s->writecursor - s->buf_start);
}


/**
* Returns a new bytebuffer were both ingoing bytebuffers is merged.
* Caller is responsible for freeing both incoming bytefyffers and resulting bytebuffer
*/
bytebuffer_t*
bytebuffer_merge(RTCTX *ctx, bytebuffer_t **buff_array, int nbuffers)
{
	size_t total_size = 0, current_size, acc_size = 0;
	int i;
	for ( i = 0; i < nbuffers; i++ )
	{
		total_size += bytebuffer_getlength(ctx, buff_array[i]);
	}
		
	bytebuffer_t *res = bytebuffer_create_with_size(ctx, total_size);
	for ( i = 0; i < nbuffers; i++)
	{
		current_size = bytebuffer_getlength(ctx, buff_array[i]);
		memcpy(res->buf_start+acc_size, buff_array[i]->buf_start, current_size);
		acc_size += current_size;
	}
	res->writecursor = res->buf_start + total_size;
	res->readcursor = res->buf_start;
	return res;
}



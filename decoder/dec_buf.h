/*
 * bio.h
 *
 *  Created on: 2014年10月22日
 *      Author: lily
 */

#ifndef _DEC_BUF_H
#define _DEC_BUF_H


typedef struct dec_buf_st{
	u_char *heap;
	u_char *head, *tail;
	size_t capacity;
	size_t size;
}*dec_buf_t;


dec_buf_t    dec_buf_new(size_t);
void         dec_buf_free(dec_buf_t b);
int          dec_buf_extend(dec_buf_t b, size_t grow);
int         dec_buf_append(dec_buf_t b, const void* src, size_t len);
int         dec_buf_consume(dec_buf_t b, size_t use);
int         dec_buf_init(dec_buf_t);

#endif /* BIO_H_ */

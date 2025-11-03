#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dec_buf.h"


//buf 新的数据从tail写入，释放时从head读取
dec_buf_t dec_buf_new(size_t size)
{
	dec_buf_t b =(dec_buf_t)calloc(1, sizeof(struct dec_buf_st));
	b->capacity = size;
	b->tail = b->head = b->heap = (u_char*)calloc(1,size);
	return b;
}

int dec_buf_init(dec_buf_t b)
{
	b->tail = b->head = b->heap;
	b->size =0;
	memset(b->heap,0,b->capacity);
	return 0;
}

void dec_buf_free(dec_buf_t b){
	if(b->heap)
		free(b->heap);
	free(b);
}


int dec_buf_extend(dec_buf_t b, size_t grow){
	size_t tailTOend =b->capacity- (b->tail - b->heap)-1; 
	size_t newsize;
	if(tailTOend < grow){
		memmove(b->heap, b->head, b->size);
		b->head = b->heap;
		b->tail = b->head + b->size;
		memset(b->tail,0,(b->capacity - b->size));
	}
	newsize =  b->size + grow;
	if(newsize > b->capacity){
		printf(">>>dec_buf realloc %d %d\r\n",grow,b->size);//正常情况下可以实现缓存循环利用
#if 0		
		char* tmp = realloc(b->heap, newsize);
		b->heap = tmp;
		b->head = b->heap;
		b->tail = b->head + b->size;
		b->capacity = newsize;
		memset(b->tail,0,(b->capacity - b->size));
#endif
		memset(b->heap,0,b->capacity);
		b->tail= b->heap;
		b->head = b->heap;
		b->size = 0;
		

	return 1;
	}
	
	return 0;
}
int dec_buf_append(dec_buf_t b, const void* src, size_t len)
{
	int ret = dec_buf_extend(b, len);
	memcpy(b->tail, src, len);
	b->tail += len;
	b->size += len;
	return ret;
}

int dec_buf_consume(dec_buf_t b, size_t use)
{
	b->size -= use;
	if(b->size <= 0){
		b->head = b->tail = b->heap;
		b->size = 0;
	} else {
		b->head += use;
	}
	return 0;
}

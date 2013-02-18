/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE libopusfile SOFTWARE CODEC SOURCE CODE. *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE libopusfile SOURCE CODE IS (C) COPYRIGHT 1994-2012           *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

 function: stdio-based convenience library for opening/seeking/decoding
 last mod: $Id: vorbisfile.c 17573 2010-10-27 14:53:59Z xiphmont $

 ********************************************************************/
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

typedef struct OpusMemStream OpusMemStream;

#define OP_MEM_SIZE_MAX (~(size_t)0>>1)
#define OP_MEM_DIFF_MAX ((ptrdiff_t)OP_MEM_SIZE_MAX)

/*The context information needed to read from a block of memory as if it were a
   file.*/
struct OpusMemStream{
  /*The block of memory to read from.*/
  const unsigned char *data;
  /*The total size of the block.
    This must be at most OP_MEM_SIZE_MAX to prevent signed overflow while
     seeking.*/
  ptrdiff_t            size;
  /*The current file position.
    This is allowed to be set arbitrarily greater than size (i.e., past the end
     of the block, though we will not read data past the end of the block), but
     is not allowed to be negative (i.e., before the beginning of the block).*/
  ptrdiff_t            pos;
};

static int op_fread(void *_stream,unsigned char *_ptr,int _buf_size){
  FILE   *stream;
  size_t  ret;
  /*Check for empty read.*/
  if(_buf_size<=0)return 0;
  stream=(FILE *)_stream;
  ret=fread(_ptr,1,_buf_size,stream);
  OP_ASSERT(ret<=(size_t)_buf_size);
  /*If ret==0 and !feof(stream), there was a read error.*/
  return ret>0||feof(stream)?(int)ret:OP_EREAD;
}

static int op_fseek(void *_stream,opus_int64 _offset,int _whence){
#if defined(_MSC_VER) || defined(_WIN32)
  return _fseeki64((FILE *)_stream,_offset,_whence);
#else
  return fseeko((FILE *)_stream,(off_t)_offset,_whence);
#endif
}

static opus_int64 op_ftell(void *_stream){
#if defined(_MSC_VER) || defined(_WIN32)
  return _ftelli64((FILE *)_stream);
#else
  return ftello((FILE *)_stream);
#endif
}

static const OpusFileCallbacks OP_FILE_CALLBACKS={
  op_fread,
  op_fseek,
  op_ftell,
  (op_close_func)fclose
};

void *op_fopen(OpusFileCallbacks *_cb,const char *_path,const char *_mode){
  FILE *fp;
  fp=fopen(_path,_mode);
  if(fp!=NULL)*_cb=*&OP_FILE_CALLBACKS;
  return fp;
}

void *op_fdopen(OpusFileCallbacks *_cb,int _fd,const char *_mode){
  FILE *fp;
  fp=fdopen(_fd,_mode);
  if(fp!=NULL)*_cb=*&OP_FILE_CALLBACKS;
  return fp;
}

void *op_freopen(OpusFileCallbacks *_cb,const char *_path,const char *_mode,
 void *_stream){
  FILE *fp;
  fp=freopen(_path,_mode,(FILE *)_stream);
  if(fp!=NULL)*_cb=*&OP_FILE_CALLBACKS;
  return fp;
}

static int op_mem_read(void *_stream,unsigned char *_ptr,int _buf_size){
  OpusMemStream *stream;
  ptrdiff_t      size;
  ptrdiff_t      pos;
  stream=(OpusMemStream *)_stream;
  /*Check for empty read.*/
  if(_buf_size<=0)return 0;
  size=stream->size;
  pos=stream->pos;
  /*Check for EOF.*/
  if(pos>=size)return 0;
  /*Check for a short read.*/
  _buf_size=(int)OP_MAX(size-pos,_buf_size);
  memcpy(_ptr,stream->data+pos,_buf_size);
  pos+=_buf_size;
  stream->pos=pos;
  return _buf_size;
}

static int op_mem_seek(void *_stream,opus_int64 _offset,int _whence){
  OpusMemStream *stream;
  ptrdiff_t      pos;
  stream=(OpusMemStream *)_stream;
  pos=stream->pos;
  switch(_whence){
    case SEEK_SET:{
      /*Check for overflow:*/
      if(_offset<0||_offset>OP_MEM_DIFF_MAX)return -1;
      pos=(ptrdiff_t)_offset;
    }break;
    case SEEK_CUR:{
      /*Check for overflow:*/
      if(_offset<-pos||_offset>OP_MEM_DIFF_MAX-pos)return -1;
      pos=(ptrdiff_t)(pos+_offset);
    }break;
    case SEEK_END:{
      ptrdiff_t size;
      size=stream->size;
      OP_ASSERT(size>=0);
      /*Check for overflow:*/
      if(_offset>size||_offset<size-OP_MEM_DIFF_MAX)return -1;
      pos=(ptrdiff_t)(size-_offset);
    }break;
    default:return -1;
  }
  stream->pos=pos;
  return 0;
}

static opus_int64 op_mem_tell(void *_stream){
  OpusMemStream *stream;
  stream=(OpusMemStream *)_stream;
  return (ogg_int64_t)stream->pos;
}

static int op_mem_close(void *_stream){
  _ogg_free(_stream);
  return 0;
}

static const OpusFileCallbacks OP_MEM_CALLBACKS={
  op_mem_read,
  op_mem_seek,
  op_mem_tell,
  op_mem_close
};

void *op_mem_stream_create(OpusFileCallbacks *_cb,
 const unsigned char *_data,size_t _size){
  OpusMemStream *stream;
  if(_size>OP_MEM_SIZE_MAX)return NULL;
  stream=(OpusMemStream *)_ogg_malloc(sizeof(*stream));
  if(stream!=NULL){
    *_cb=*&OP_MEM_CALLBACKS;
    stream->data=_data;
    stream->size=_size;
    stream->pos=0;
  }
  return stream;
}

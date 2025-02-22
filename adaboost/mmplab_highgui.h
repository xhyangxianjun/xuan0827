/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef __HIGHGUI_H_
#define __HIGHGUI_H_

#include "highgui.h"
#include "cxmisc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

#ifndef WIN32
#include "cvconfig.h"
#else
void  FillBitmapInfo( BITMAPINFO* bmi, int width, int height, int bpp, int origin );
#endif

/* Errors */
#define HG_OK          0 /* Don't bet on it! */
#define HG_BADNAME    -1 /* Bad window or file name */
#define HG_INITFAILED -2 /* Can't initialize HigHGUI. Possibly, can't find vlgrfmts.dll */
#define HG_WCFAILED   -3 /* Can't create a window */
#define HG_NULLPTR    -4 /* The null pointer where it should not appear */
#define HG_BADPARAM   -5 

#define CV_WINDOW_MAGIC_VAL     0x00420042
#define CV_TRACKBAR_MAGIC_VAL   0x00420043

/***************************** CvCapture structure ******************************/

#define CV_CAPTURE_BASE_API_COUNT 6

typedef void         (CV_CDECL* CvCaptureCloseFunc)         ( CvCapture* capture );
typedef int          (CV_CDECL* CvCaptureGrabFrameFunc)     ( CvCapture* capture );
typedef IplImage   * (CV_CDECL* CvCaptureRetrieveFrameFunc) ( CvCapture* capture );
typedef double       (CV_CDECL* CvCaptureGetPropertyFunc)   ( CvCapture* capture, int id );
typedef int          (CV_CDECL* CvCaptureSetPropertyFunc)   ( CvCapture* capture,
                                                              int id, double value );
typedef const char * (CV_CDECL* CvCaptureGetDescriptionFunc)( CvCapture* capture );

typedef struct CvCaptureVTable
{
    int                           count;
    CvCaptureCloseFunc            close;
    CvCaptureGrabFrameFunc        grab_frame;
    CvCaptureRetrieveFrameFunc    retrieve_frame;
    CvCaptureGetPropertyFunc      get_property;
    CvCaptureSetPropertyFunc      set_property;
    CvCaptureGetDescriptionFunc   get_description;
}
CvCaptureVTable;

typedef struct CvCapture
{
    CvCaptureVTable* vtable;
}
CvCapture;


#ifdef WIN32
#define HAVE_VFW 1

/* uncomment to enable OpenEXR codec (will not compile under MSVC6) */ 
//#define HAVE_ILMIMF 1

/* uncomment to enable CMUCamera1394 fireware camera module */
//#define HAVE_CMU1394 1
#endif


#if defined (HAVE_CAMV4L) || defined (HAVE_CAMV4L2)
CvCapture * cvCaptureFromCAM_V4L( int index );
#endif

#ifdef HAVE_DC1394
CvCapture * cvCaptureFromCAM_DC1394( int index );
#endif

#ifdef HAVE_MIL
CvCapture* cvCaptureFromCAM_MIL( int index );
#endif

#ifdef HAVE_CMU1394
CvCapture * cvCaptureFromCAM_CMU( int index );
#endif

#ifdef HAVE_TYZX
CV_IMPL CvCapture * cvCaptureFromCAM_TYZX( int index );
#endif

#ifdef WIN32
CvCapture* cvCaptureFromCAM_VFW( int index );
CvCapture* cvCaptureFromFile_VFW( const char* filename );
#endif

#ifdef HAVE_XINE
CvCapture* cvCaptureFromFile_XINE   (const char* filename);
#endif

#ifdef HAVE_FFMPEG
CvCapture* cvCaptureFromFile_FFMPEG (const char* filename);
#endif

#ifdef HAVE_QUICKTIME
CvCapture * cvCaptureFromFile_QT (const char  * filename);
CvCapture * cvCaptureFromCAM_QT  (const int     index);
#endif

#endif /* __HIGHGUI_H_ */

#ifndef _BITSTRM_H_
#define _BITSTRM_H_

#include <stdio.h>
#include <setjmp.h>

#if _MSC_VER >= 1200
    #pragma warning( disable: 4711 4324 )
#endif

#define  RBS_THROW_EOS    -123  /* <end of stream> exception code */
#define  RBS_THROW_FORB   -124  /* <forrbidden huffman code> exception code */
#define  RBS_HUFF_FORB    2047  /* forrbidden huffman code "value" */

typedef unsigned char uchar;
typedef unsigned long ulong;

// class RBaseStream - base class for other reading streams.
class RBaseStream
{
public:
    //methods
    RBaseStream();
    virtual ~RBaseStream();
    
    virtual bool  Open( const char* filename );
    virtual void  Close();
    void          SetBlockSize( int block_size, int unGetsize = 4 );
    bool          IsOpened();
    void          SetPos( int pos );
    int           GetPos();
    void          Skip( int bytes );
    jmp_buf&      JmpBuf();
    
protected:
    
    jmp_buf m_jmp_buf;
    uchar*  m_start;
    uchar*  m_end;
    uchar*  m_current;
    FILE*   m_file;
    int     m_unGetsize;
    int     m_block_size;
    int     m_block_pos;
    bool    m_jmp_set;
    bool    m_is_opened;

    virtual void  ReadBlock();
    virtual void  Release();
    virtual void  Allocate();
};


// class RLByteStream - uchar-oriented stream.
// l in prefix means that the least significant uchar of a multi-uchar value goes first
class RLByteStream : public RBaseStream
{
public:
    virtual ~RLByteStream();
    
    int     GetByte();
    void    GetBytes( void* buffer, int count, int* readed = 0 );
    int     GetWord();
    int     GetDWord(); 
};

// class RMBitStream - uchar-oriented stream.
// m in prefix means that the most significant uchar of a multi-uchar value go first
class RMByteStream : public RLByteStream
{
public:
    virtual ~RMByteStream();

    int     GetWord();
    int     GetDWord(); 
};

// class RLBitStream - bit-oriented stream.
// l in prefix means that the least significant bit of a multi-bit value goes first
class RLBitStream : public RBaseStream
{
public:
    virtual ~RLBitStream();
    
    void    SetPos( int pos );
    int     GetPos();
    int     Get( int bits );
    int     Show( int bits );
    int     GetHuff( const short* table );
    void    Move( int shift );
    void    Skip( int bytes );
        
protected:
    int     m_bit_idx;
    virtual void  ReadBlock();
};

// class RMBitStream - bit-oriented stream.
// m in prefix means that the most significant bit of a multi-bit value goes first
class RMBitStream : public RLBitStream
{
public:
    virtual ~RMBitStream();
    
    void    SetPos( int pos );
    int     GetPos();
    int     Get( int bits );
    int     Show( int bits );
    int     GetHuff( const short* table );
    void    Move( int shift );
    void    Skip( int bytes );

protected:
    virtual void  ReadBlock();
};


// WBaseStream - base class for output streams
class WBaseStream
{
public:
    //methods
    WBaseStream();
    virtual ~WBaseStream();
    
    virtual bool  Open( const char* filename );
    virtual void  Close();
    void          SetBlockSize( int block_size );
    bool          IsOpened();
    int           GetPos();
    
protected:
    
    uchar*  m_start;
    uchar*  m_end;
    uchar*  m_current;
    int     m_block_size;
    int     m_block_pos;
    FILE*   m_file;
    bool    m_is_opened;
    
    virtual void  WriteBlock();
    virtual void  Release();
    virtual void  Allocate();
};


// class WLByteStream - uchar-oriented stream.
// l in prefix means that the least significant uchar of a multi-byte value goes first
class WLByteStream : public WBaseStream
{
public:
    virtual ~WLByteStream();

    void    PutByte( int val );
    void    PutBytes( const void* buffer, int count );
    void    PutWord( int val );
    void    PutDWord( int val ); 
};


// class WLByteStream - uchar-oriented stream.
// m in prefix means that the least significant uchar of a multi-byte value goes last
class WMByteStream : public WLByteStream
{
public:
    virtual ~WMByteStream();

    void    PutWord( int val );
    void    PutDWord( int val ); 
};


// class WLBitStream - bit-oriented stream.
// l in prefix means that the least significant bit of a multi-bit value goes first
class WLBitStream : public WBaseStream
{
public:
    virtual ~WLBitStream();
    
    int     GetPos();
    void    Put( int val, int bits );
    void    PutHuff( int val, const int* table );
        
protected:
    int     m_bit_idx;
    int     m_val;
    virtual void  WriteBlock();
};


// class WMBitStream - bit-oriented stream.
// l in prefix means that the least significant bit of a multi-bit value goes first
class WMBitStream : public WBaseStream
{
public:
    WMBitStream();
    virtual ~WMBitStream();
    
    bool    Open( const char* filename );
    void    Close();
    virtual void  Flush();

    int     GetPos();
    void    Put( int val, int bits );
    void    PutHuff( int val, const ulong* table );
        
protected:
    int     m_bit_idx;
    ulong   m_pad_val;
    ulong   m_val;
    virtual void  WriteBlock();
    void    ResetBuffer();
};



#define BSWAP(v)    (((v)<<24)|(((v)&0xff00)<<8)| \
                    (((v)>>8)&0xff00)|((unsigned)(v)>>24))

int* bsCreateSourceHuffmanTable( const uchar* src, int* dst, 
                                 int max_bits, int first_bits );
bool bsCreateDecodeHuffmanTable( const int* src, short* dst, int max_size );
bool bsCreateEncodeHuffmanTable( const int* src, ulong* dst, int max_size );

void bsBSwapBlock( uchar *start, uchar *end );
bool bsIsBigEndian( void );

extern const ulong bs_bit_mask[];

#endif/*_BITSTRM_H_*/

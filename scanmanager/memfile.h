/*
   Memory File IO abstraction
   Adapted from CxImage library
*/

#ifndef MEMFILE_H__
#define MEMFILE_H__

#include <stdint.h>

class CxMemFile
{
protected:
   bool	alloc(uint32_t nBytes);
   void	free();

   uint8_t  *m_pBuffer;
   uint32_t  m_Size;
   bool      m_bFreeOnClose;
   int32_t   m_Position;     // current position
   int32_t   m_Edge;         // buffer size
   bool      m_bEOF;

public:
   CxMemFile(uint8_t* pBuffer = nullptr, uint32_t size = 0);
   virtual ~CxMemFile();

   bool open();
   uint8_t *getBuffer(bool bDetachBuffer = true);

   virtual bool     close();
   virtual size_t   read(void *buffer, size_t size, size_t count);
   virtual size_t   write(const void *buffer, size_t size, size_t count);
   virtual bool     seek(int32_t offset, int32_t origin);
   virtual int32_t  tell();
   virtual int32_t  size();
   virtual bool     flush();
   virtual bool     eof();
   virtual int32_t  error();
   virtual bool     putc(uint8_t c);
   virtual int32_t  getc();
   virtual char    *gets(char *string, int32_t n);
   virtual int32_t  scanf(const char *format, void* output);
};

#endif

// EOF


/*
   Memory File IO abstraction
   Adapted from CxImage library
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "memfile.h"

//
// Constructor
//
CxMemFile::CxMemFile(uint8_t *pBuffer, uint32_t size)
{
   m_pBuffer = pBuffer;
   m_Position = 0;
   m_Size = m_Edge = size;
   m_bFreeOnClose = (pBuffer == nullptr);
   m_bEOF = false;
}

//
// Destructor
//
CxMemFile::~CxMemFile()
{
   close();
}

//
// Close the memory file
//
bool CxMemFile::close()
{
   if(m_pBuffer && m_bFreeOnClose)
   {
      ::free(m_pBuffer);
      m_pBuffer = nullptr;
      m_Size = 0;
   }

   return true;
}

//
// Open the memory file by creating a new allocated buffer.
//
bool CxMemFile::open()
{
   if(m_pBuffer)
      return false;	// Can't re-open without closing first

   m_Position = m_Size = m_Edge = 0;
   m_pBuffer = static_cast<uint8_t *>(malloc(1));
   m_bFreeOnClose = true;

   return (m_pBuffer != nullptr);
}

//
// Get the backing memory buffer. If bDetachBuffer is true, then
// this object will not free the buffer any longer when it is closed
// or destroyed.
//
uint8_t* CxMemFile::getBuffer(bool bDetachBuffer)
{
   //can only detach, avoid inadvertantly attaching to
   // memory that may not be ours [Jason De Arte]
   if(bDetachBuffer)
      m_bFreeOnClose = false;

   return m_pBuffer;
}

//
// Read data from the buffer
//
size_t CxMemFile::read(void *buffer, size_t size, size_t count)
{
   if(buffer == nullptr) 
      return 0;

   if(m_pBuffer == nullptr)
      return 0;
   
   if(m_Position >= int32_t(m_Size))
   {
      m_bEOF = true;
      return 0;
   }

   int32_t nCount = int32_t(count*size);
   if(nCount == 0)
      return 0;

   int32_t nRead;
   if(m_Position + nCount > int32_t(m_Size))
   {
      m_bEOF = true;
      nRead = (m_Size - m_Position);
   }
   else
      nRead = nCount;

   memcpy(buffer, m_pBuffer + m_Position, nRead);
   m_Position += nRead;

   return size_t(nRead / size);
}

//
// Write to the buffer
//
size_t CxMemFile::write(const void *buffer, size_t size, size_t count)
{
   m_bEOF = false;
   if(m_pBuffer == nullptr)
      return 0;
   if(buffer == nullptr)
      return 0;

   int32_t nCount = int32_t(count * size);
   if(nCount == 0)
      return 0;

   if(m_Position + nCount > m_Edge)
   {
      if(!alloc(m_Position + nCount))
         return false;
   }

   memcpy(m_pBuffer + m_Position, buffer, nCount);

   m_Position += nCount;

   if(m_Position > int32_t(m_Size))
      m_Size = m_Position;

   return count;
}

//
// Seek to a position in the buffer.
//
bool CxMemFile::seek(int32_t offset, int32_t origin)
{
   m_bEOF = false;
   if(m_pBuffer == nullptr)
      return false;
   int32_t lNewPos = m_Position;

   if(origin == SEEK_SET)
      lNewPos = offset;
   else if(origin == SEEK_CUR)
      lNewPos += offset;
   else if(origin == SEEK_END)
      lNewPos = m_Size + offset;
   else 
      return false;

   if(lNewPos < 0)
      lNewPos = 0;

   m_Position = lNewPos;
   return true;
}

//
// Get the current offset into the buffer.
//
int32_t CxMemFile::tell()
{
   if(m_pBuffer == nullptr)
      return -1;

   return m_Position;
}

//
// Get the size of the buffer in bytes.
//
int32_t CxMemFile::size()
{
   if(m_pBuffer == nullptr)
      return -1;

   return m_Size;
}

//
// Flush the buffer (this is a no-op).
//
bool CxMemFile::flush()
{
   if(m_pBuffer == nullptr)
      return false;

   return true;
}

//
// Check if at EOF
//
bool CxMemFile::eof()
{
   if(m_pBuffer == nullptr)
      return true;

   return m_bEOF;
}

//
// Check if in error state.
//
int32_t CxMemFile::error()
{
   if(m_pBuffer == nullptr) 
      return -1;

   return (m_Position > int32_t(m_Size));
}

//
// Add a character to the end of the buffer.
//
bool CxMemFile::putc(uint8_t c)
{
   m_bEOF = false;
   if(m_pBuffer == nullptr)
      return false;

   if(m_Position >= m_Edge)
   {
      if(!alloc(m_Position + 1))
         return false;
   }

   m_pBuffer[m_Position++] = c;

   if(m_Position > int32_t(m_Size))
      m_Size = m_Position;

   return true;
}

//
// Get a character from the buffer.
//
int32_t CxMemFile::getc()
{
   if(m_pBuffer == nullptr || m_Position >= int32_t(m_Size))
   {
      m_bEOF = true;
      return EOF;
   }

   return *(uint8_t *)((uint8_t *)m_pBuffer + m_Position++);
}

//
// Get a string from the buffer.
//
char *CxMemFile::gets(char *string, int32_t n)
{
   n--;
   int32_t c, i = 0;

   while(i < n)
   {
      c = getc();
      if(c == EOF)
         return nullptr;
      string[i++] = char(c);
      if(c == '\n')
         break;
   }

   string[i] = '\0';
   return string;
}

//
// scanf - unimplemented
//
int32_t CxMemFile::scanf(const char *format, void* output)
{
   return 0;
}

//
// Reallocate the internal backing buffer.
//
bool CxMemFile::alloc(uint32_t dwNewLen)
{
   if(dwNewLen > uint32_t(m_Edge))
   {
      // find new buffer size
      uint32_t dwNewBufferSize = uint32_t(((dwNewLen >> 16) + 1) << 16);

      // allocate new buffer
      if(m_pBuffer == nullptr) 
         m_pBuffer = static_cast<uint8_t *>(malloc(dwNewBufferSize));
      else
         m_pBuffer = static_cast<uint8_t *>(realloc(m_pBuffer, dwNewBufferSize));

      // I own this buffer now (caller knows nothing about it)
      m_bFreeOnClose = true;

      m_Edge = dwNewBufferSize;
   }

   return (m_pBuffer != nullptr);
}

//
// Free the internal buffer
//
void CxMemFile::free()
{
   close();
}

// EOF


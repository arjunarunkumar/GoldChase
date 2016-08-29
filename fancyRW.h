#ifndef fancyRW_h
#define fancyRW_h

#include<iostream>
#include <errno.h>

template<typename T>
int READ(int fd, T* obj_ptr, int count)
{
  char* addr=(char*)obj_ptr;
  //loop. Read repeatedly until count bytes are read in
  int bytesRead = 0;
  int totalBytesRead = 0;
  while (count > 0)
  {
	bytesRead = read(fd, addr + totalBytesRead, count);
    //std::cout << "bytes read = " << bytesRead << std::endl;
    if (bytesRead == -1)
    {
      if (errno != EINTR)
      {
        return -1;
      }
    }
	else
	{
		totalBytesRead += bytesRead;
		count -= totalBytesRead;
	}
  }
  return totalBytesRead;
}

template<typename T>
int WRITE(int fd, T* obj_ptr, int count)
{
  char* addr=(char*)obj_ptr;
  //loop. Write repeatedly until count bytes are written out
  int bytesWritten = 0;
  int totalBytesWritten = 0;
  while (count > 0)
  {
    //std::cout << "bytes written = " << bytesWritten << std::endl;
	bytesWritten = write(fd, addr + totalBytesWritten, count);
    if (bytesWritten == -1)
    {
      if (errno != EINTR)
      {
        return -1;
      }
    }
	else
	{
		totalBytesWritten += bytesWritten;
		count -= totalBytesWritten;
	}
  }
  return totalBytesWritten;
}
#endif

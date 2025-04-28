#ifndef _FILMMASTER2000_H_
#define _FILMMASTER2000_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

typedef struct Video {
  int64_t frameCount;
  unsigned char channelCount;
  unsigned char height;
  unsigned char width;
  unsigned char *data;
} Video;

FILE *safeOpen(const char *filename, const char *mode);

bool decodeVideo(const char *filename, Video *video);
bool encodeVideo(const char *filename, const Video *video);
void reverseVideo(Video *video);
void swapChannel(Video *video, unsigned char ch1, unsigned char ch2);
void clipChannel(Video *video, unsigned char ch, unsigned char minVal,
                 unsigned char maxVal);
void scaleChannel(Video *video, unsigned char ch, float scaleFactor);
void freeVideoMemory(Video *video);
void *safeMalloc(size_t size);

extern int prioritizeSpeed, prioritizeMemory;

#endif 

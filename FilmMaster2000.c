#include "FilmMaster2000.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
  Video *video;
  size_t frameStart;
  size_t frameCount;
  struct WorkerArgs {
    enum { SWAP, CLIP, SCALE } operation;
    union {
      struct {
        unsigned char ch1;
        unsigned char ch2;
      } swap;
      struct {
        unsigned char ch;
        unsigned char minVal;
        unsigned char maxVal;
      } clip;
      struct {
        unsigned char ch;
        float scaleFactor;
      } scale;
    };
  } args;
} FrameWorker;

int prioritizeMemory, prioritizeSpeed;

void *worker(void *pthargs) {
  FrameWorker *args = (FrameWorker *)pthargs;

  //   printf("worker %ld started, [%lu -- %lu]\n", pthread_self(),
  //   args->frameStart,
  //          args->frameCount);

  size_t channelSize = (size_t)args->video->height * args->video->width;
  size_t frameSize = channelSize * args->video->channelCount;

  for (size_t i = 0; i < args->frameCount; i++) {
    unsigned char *frame =
        args->video->data + frameSize * (i + args->frameStart);
    // unsigned char *frameEnd = frame + frameSize;
    // frame points to current Frame (including n Channels)

    if (args->args.operation == SWAP) {
      // swap channels [ch1] and [ch2] inside the frame
      unsigned char tmpChannel[channelSize];
      unsigned char *pch1 = frame + channelSize * args->args.swap.ch1;
      unsigned char *pch2 = frame + channelSize * args->args.swap.ch2;
      memcpy(tmpChannel, pch1, channelSize);
      memcpy(pch1, pch2, channelSize);
      memcpy(pch2, tmpChannel, channelSize);
    } else if (args->args.operation == CLIP) {
      // clip ch channel to minVal & maxVal
      unsigned char *pch = frame + channelSize * args->args.clip.ch;
      for (size_t d = 0; d < channelSize; d++) {
        if (pch[d] < args->args.clip.minVal) {
          pch[d] = args->args.clip.minVal;
        } else if (pch[d] > args->args.clip.maxVal) {
          pch[d] = args->args.clip.maxVal;
        }
      }
    } else if (args->args.operation == SCALE) {
      unsigned char *pch = frame + channelSize * args->args.scale.ch;
      for (size_t d = 0; d < channelSize; d++) {
        unsigned char *val = &pch[d];
        int scaled = (int)(*val * args->args.scale.scaleFactor);
        *val = (unsigned char)(scaled > 255 ? 255 : (scaled < 0 ? 0 : scaled));
      }
    }
  }

  return NULL;
}

void *safeMalloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    fprintf(stderr, "Error: Memory allocation failed for size %zu.\n", size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

bool decodeVideo(const char *filename, Video *video) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Error: Cannot open input file '%s'.\n", filename);
    return false;
  }

  if (fread(&video->frameCount, sizeof(uint64_t), 1, file) != 1 ||
      fread(&video->channelCount, sizeof(unsigned char), 1, file) != 1 ||
      fread(&video->height, sizeof(unsigned char), 1, file) != 1 ||
      fread(&video->width, sizeof(unsigned char), 1, file) != 1) {
    fprintf(stderr, "Error: Failed to read video metadata.\n");
    fclose(file);
    return false;
  }

  size_t dataSize = (size_t)video->frameCount * video->channelCount *
                    video->height * video->width;
  video->data = malloc(dataSize);
  if (!video->data) {
    fprintf(stderr, "Error: Memory allocation failed.\n");
    fclose(file);
    return false;
  }

  if (fread(video->data, 1, dataSize, file) != dataSize) {
    fprintf(stderr, "Error: Failed to read video data.\n");
    free(video->data);
    fclose(file);
    return false;
  }

  fclose(file);
  return true;
}

bool encodeVideo(const char *filename, const Video *video) {
  FILE *file = fopen(filename, "wb");
  if (!file) {
    fprintf(stderr, "Error: Cannot open output file '%s'.\n", filename);
    return false;
  }

  if (fwrite(&video->frameCount, sizeof(uint64_t), 1, file) != 1 ||
      fwrite(&video->channelCount, sizeof(unsigned char), 1, file) != 1 ||
      fwrite(&video->height, sizeof(unsigned char), 1, file) != 1 ||
      fwrite(&video->width, sizeof(unsigned char), 1, file) != 1) {
    fprintf(stderr, "Error: Failed to write video metadata.\n");
    fclose(file);
    return false;
  }

  size_t dataSize = (size_t)video->frameCount * video->channelCount *
                    video->height * video->width;
  if (fwrite(video->data, 1, dataSize, file) != dataSize) {
    fprintf(stderr, "Error: Failed to write video data.\n");
    fclose(file);
    return false;
  }

  fclose(file);
  return true;
}

void reverseVideo(Video *video) {
  size_t frameSize =
      ((size_t)video->channelCount) * video->height * video->width;

  if (prioritizeSpeed) {
    // block based memory swap
    unsigned char *temp = safeMalloc(frameSize);
    for (int64_t i = 0; i < video->frameCount / 2; i++) {
      unsigned char *startFrame = video->data + i * frameSize;
      unsigned char *endFrame =
          video->data + (video->frameCount - 1 - i) * frameSize;

      memcpy(temp, startFrame, frameSize);
      memcpy(startFrame, endFrame, frameSize);
      memcpy(endFrame, temp, frameSize);
    }
    free(temp);
  } else if (prioritizeMemory) {
    for (int64_t i = 0; i < video->frameCount / 2; i++) {
      unsigned char *startFrame = video->data + i * frameSize;
      unsigned char *endFrame =
          video->data + (video->frameCount - 1 - i) * frameSize;

      // do in-place memory swap
      for (size_t j = 0; j < frameSize; j++) {
        char tmp = startFrame[j];
        startFrame[j] = endFrame[j];
        endFrame[j] = tmp;
      }
    }
  } else {
    unsigned char *frameBuffer1 = safeMalloc(frameSize);
    unsigned char *frameBuffer2 = safeMalloc(frameSize);

    for (int64_t i = 0, j = video->frameCount - 1; i < j; i++, j--) {
      memcpy(frameBuffer1, video->data + i * frameSize, frameSize);
      memcpy(frameBuffer2, video->data + j * frameSize, frameSize);

      memcpy(video->data + i * frameSize, frameBuffer2, frameSize);
      memcpy(video->data + j * frameSize, frameBuffer1, frameSize);
    }

    free(frameBuffer1);
    free(frameBuffer2);
  }
}

static void startWorkers(Video *video, struct WorkerArgs *args) {
  FrameWorker workers[4];
  pthread_t pids[4];

  memset(workers, 0, sizeof(workers));
  memset(pids, 0, sizeof(pids));

  int nframes = video->frameCount / 4;
  for (int i = 0; i < 4; i++) {
    workers[i].video = video;
    workers[i].args = *args;
    workers[i].frameStart = nframes * i;
    workers[i].frameCount = nframes;
  }

  // The last worker is responsible for the remaining frames
  workers[3].frameCount = video->frameCount - nframes * 3;

  // Start 4 worker threads
  for (int i = 0; i < 4; i++) {
    pthread_create(&pids[i], NULL, worker, &workers[i]);
  }

  // Wait for joining
  for (int i = 0; i < 4; i++) {
    pthread_join(pids[i], NULL);
  }
}

void swapChannel(Video *video, unsigned char ch1, unsigned char ch2) {
  if (ch1 >= video->channelCount || ch2 >= video->channelCount) {
    fprintf(stderr, "Error: Invalid channel indices.\n");
    return;
  }

  if (prioritizeSpeed) {
    struct WorkerArgs args = {.operation = SWAP,
                              .swap = {.ch1 = ch1, .ch2 = ch2}};
    startWorkers(video, &args);
  } else if (prioritizeMemory) {
    size_t channelSize = (size_t)video->height * video->width;
    size_t frameSize =
        (size_t)video->height * video->width * video->channelCount;

    for (size_t f = 0; f < (size_t)video->frameCount; f++) {
      unsigned char *frameData = video->data + f * frameSize;

      for (size_t i = 0; i < channelSize; i++) {
        unsigned char temp = frameData[i + ch1 * channelSize];
        frameData[i + channelSize * ch1] = frameData[i + channelSize * ch2];
        frameData[i + channelSize * ch2] = temp;
      }
    }
  } else {
    size_t channelSize = (size_t)video->height * video->width;
    size_t frameSize =
        (size_t)video->height * video->width * video->channelCount;

    unsigned char *frameData = safeMalloc(frameSize);

    for (size_t f = 0; f < (size_t)video->frameCount; f++) {
      memcpy(frameData, video->data + f * frameSize, frameSize);

      for (size_t i = 0; i < channelSize; i++) {
        unsigned char temp = frameData[i + ch1 * channelSize];
        frameData[i + channelSize * ch1] = frameData[i + channelSize * ch2];
        frameData[i + channelSize * ch2] = temp;
      }

      memcpy(video->data + f * frameSize, frameData, frameSize);
    }

    free(frameData);
  }

  printf("Channel swap operation completed successfully.\n");
}

void clipChannel(Video *video, unsigned char ch, unsigned char minVal,
                 unsigned char maxVal) {
  if (ch >= video->channelCount) {
    fprintf(stderr, "Error: Invalid channel index.\n");
    return;
  }

  if (prioritizeSpeed) {
    struct WorkerArgs args = {
        .operation = CLIP,
        .clip = {.ch = ch, .minVal = minVal, .maxVal = maxVal}};
    startWorkers(video, &args);
  } else if (prioritizeMemory) {
    size_t channelSize = video->height * video->width;
    for (size_t f = 0; f < (size_t)video->frameCount; f++) {
      unsigned char *channel =
          video->data + (f * video->channelCount + ch) * channelSize;
      // the memcpy-src should be: video->data + f * channelSize
      for (size_t i = 0; i < channelSize; i++) {
        if (channel[i] < minVal) {
          channel[i] = minVal;
        } else if (channel[i] > maxVal) {
          channel[i] = maxVal;
        }
      }
    }
  } else {
    size_t channelSize = video->height * video->width;
    unsigned char *frameData = safeMalloc(channelSize);
    for (size_t f = 0; f < (size_t)video->frameCount; f++) {
      unsigned char *channel =
          video->data + (f * video->channelCount + ch) * channelSize;
      memcpy(frameData, channel, channelSize);
      // the memcpy-src should be: video->data + f * channelSize
      for (size_t i = 0; i < channelSize; i++) {
        if (frameData[i] < minVal) {
          frameData[i] = minVal;
        } else if (frameData[i] > maxVal) {
          frameData[i] = maxVal;
        }
      }
      memcpy(channel, frameData, channelSize);
    }
    free(frameData);
  }
}

void scaleChannel(Video *video, unsigned char ch, float scaleFactor) {
  if (ch >= video->channelCount) {
    fprintf(stderr, "Error: Invalid channel index.\n");
    return;
  }

  if (prioritizeSpeed) {
    struct WorkerArgs args = {.operation = SCALE,
                              .scale = {.ch = ch, .scaleFactor = scaleFactor}};
    startWorkers(video, &args);
  } else {
    size_t channelSize = video->height * video->width;
    for (size_t frame = 0; frame < (size_t)video->frameCount; frame++) {
      for (size_t i = 0; i < channelSize; i++) {
        unsigned char *val =
            &video->data[frame * video->channelCount * channelSize +
                         ch * channelSize + i];
        // first offset #frame frames: FrameSize is channelCount * channelSize,
        //                  so offset  frame * channelCount * channelSize
        // Then offset the given ch:   + ch * channelSize
        // Then offset the i-th pixel: + i
        int scaled = (int)(*val * scaleFactor);
        *val = (unsigned char)(scaled > 255 ? 255 : (scaled < 0 ? 0 : scaled));
      }
    }
  }
}

void freeVideoMemory(Video *video) {
  if (video && video->data) {
    free(video->data);
    video->data = NULL;
  }
}

#include "FilmMaster2000.h"
#include <stdlib.h>
#include <string.h>

void printUsage(const char *progName) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s <input.bin> <output.bin> [-S|-M] reverse\n", progName);
  fprintf(stderr, "  %s <input.bin> <output.bin> [-S|-M] swap_channel X,Y\n",
          progName);
  fprintf(stderr,
          "  %s <input.bin> <output.bin> [-S|-M] clip_channel C [min,max]\n",
          progName);
  fprintf(stderr,
          "  %s <input.bin> <output.bin> [-S|-M] scale_channel C factor\n",
          progName);
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    printUsage(argv[0]);
    return 1;
  }

  const char *inputFile = argv[1];
  const char *outputFile = argv[2];
  const char *command = argv[3];
  int parameter = 4;

  Video video;
  memset(&video, 0, sizeof(video));
  if (!decodeVideo(inputFile, &video)) {
    return 1;
  }

  if (strcmp(command, "-S") == 0) {
    prioritizeSpeed = true;
    command = argv[4];
    parameter = 5;
  } else if (strcmp(command, "-M") == 0) {
    prioritizeMemory = true;
    command = argv[4];
    parameter = 5;
  }

  if (strcmp(command, "reverse") == 0) {
    reverseVideo(&video);
  } else if (strcmp(command, "swap_channel") == 0 && argc >= 5) {
    unsigned char ch1, ch2;
    sscanf(argv[parameter], "%hhu,%hhu", &ch1, &ch2);
    swapChannel(&video, ch1, ch2);
  } else if (strcmp(command, "clip_channel") == 0 && argc >= 6) {
    unsigned char ch, minVal, maxVal;
    sscanf(argv[parameter], "%hhu", &ch);
    sscanf(argv[parameter + 1], "[%hhu,%hhu]", &minVal, &maxVal);
    clipChannel(&video, ch, minVal, maxVal);
  } else if (strcmp(command, "scale_channel") == 0 && argc >= 6) {
    unsigned char ch;
    float factor;
    sscanf(argv[parameter], "%hhu", &ch);
    factor = atof(argv[parameter + 1]);
    scaleChannel(&video, ch, factor);
  } else {
    printUsage(argv[0]);
    freeVideoMemory(&video);
    return 1;
  }

  if (!encodeVideo(outputFile, &video)) {
    freeVideoMemory(&video);
    return 1;
  }

  freeVideoMemory(&video);
  printf("Operation '%s' completed successfully.\n", command);
  return 0;
}

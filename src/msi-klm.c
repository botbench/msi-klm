/*
 ============================================================================
 Name        : msi-klm.c
 Author      : Xander Soldaat
 Version     : 0.1
 Copyright   : Copyright 2014 Xander Soldaat
 Description : MSI Keyboard LED manager - based on commands found in
               https://github.com/wearefractal/msi-keyboard
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hidapi.h>
#include <stdint.h>
#include <string.h>

#define REGION_LEFT     0
#define REGION_MIDDLE   1
#define REGION_RIGHT    2
#define REGIONS         3

typedef struct
{
  uint8_t changed;
  uint8_t region;
  uint8_t colour;
  uint8_t intensity;
} tRegionInfo, *tRegionInfoPtr;

int getRegionInfo(char *colour_data, tRegionInfoPtr regionPtr)
{
  char *colour = NULL;
  char *intensity = NULL;
  char *delim = ",:";

  // The default values;
  regionPtr->colour = 0;      // off
  regionPtr->intensity = 0;   // high

  if (colour_data != NULL)
  {
    colour = strtok(colour_data, delim);
    intensity = strtok(NULL, delim);
  }
  else
    return 1;

  /*
   * off : 0
   * "red": 1,
   *   "orange": 2,
   * "yellow": 3,
   * "green": 4,
   * "sky": 5,
   * "blue": 6,
   * "purple": 7,
   * "white": 8
   */
  if (colour != NULL)
  {
    switch(colour[0])
    {
      case '0':
        regionPtr->colour = 0;
        break;
      case 'r':
        regionPtr->colour = 1;
        break;
      case 'o':
        regionPtr->colour = 2;
        break;
      case 'y':
        regionPtr->colour = 3;
        break;
      case 'g':
        regionPtr->colour = 4;
        break;
      case 's':
        regionPtr->colour = 5;
        break;
      case 'b':
        regionPtr->colour = 6;
        break;
      case 'p':
        regionPtr->colour = 7;
        break;
      case 'w':
        regionPtr->colour = 8;
        break;
      default:
        regionPtr->colour = 8;
        break;
    }
  }

  /*
   * "light": 3,
   * "low": 0,
   * "med": 1,
   * "high": 2
  */

  if (intensity != NULL)
  {
    switch(intensity[0])
    {
      case 'l':
        if (strcmp(intensity, "light") == 0)
          regionPtr->intensity = 3;
        else if (strcmp(intensity, "low") == 0)
          regionPtr->intensity = 0;
        break;
      case 'm':
        regionPtr->intensity = 1;
        break;
      default:
        regionPtr->intensity = 2;
        break;
    }
  }
  return 0;
}


void cleanupExit(hid_device *handle, int8_t retval)
{
  hid_close(handle);

  /* Free static HIDAPI objects. */
  hid_exit();

  exit(retval);
}


int setMode(hid_device *handle, uint8_t mode)
{
  uint8_t report[32];

  memset(report, 0, 32);

  report[0] = 1;
  report[1] = 2;
  report[2] = 65;   // commit
  report[3] = mode; // set hardware mode
  report[4] = 0;
  report[5] = 0;
  report[6] = 0;
  report[7] = 236; // EOR

  return hid_send_feature_report(handle, report, 17);
}


int setRegion(hid_device *handle, tRegionInfoPtr regionPtr)
{
  uint8_t report[32];

  memset(report, 0, 32);

  // header
  report[0] = 1;
  report[1] = 2;
  report[2] = 66; // set
  report[3] = regionPtr->region;
  report[4] = regionPtr->colour;
  report[5] = regionPtr->intensity;
  report[6] = 0;
  report[7] = 236;  // EOR (end of request)

  return hid_send_feature_report(handle, report, 17);
}


int main(int argc, char* argv[])
{
  hid_device *handle;

  struct hid_device;

  char *colour_data = NULL;

  int c;
  int i;

  uint8_t mode = 0;

  tRegionInfo regions[3];

  // We shouldn't update anything that hasn't been changed
  regions[REGION_LEFT].changed = 0;
  regions[REGION_MIDDLE].changed = 0;
  regions[REGION_RIGHT].changed = 0;

  // The regions are 1 indexed, not 0
  regions[REGION_LEFT].region = 1;
  regions[REGION_MIDDLE].region = 2;
  regions[REGION_RIGHT].region = 3;

  // Open the device using the VID, PID,
  handle = hid_open(0x1770, 0xff00, NULL);

  // Looks like we couldn't open a handle, oh well.
  if (!handle) {
    printf("unable to open device\n");
    return 1;
  }

  opterr = 0;

  while ((c = getopt (argc, argv, "l:m:r:ngbdw")) != -1)
  {
   switch (c)
   {
     case 'l':
       colour_data = optarg;
       if (getRegionInfo(colour_data, &regions[REGION_LEFT]) < 0)
       {
         printf("Colour info incorrect\n");
         cleanupExit(handle, EXIT_FAILURE);
       }
       regions[REGION_LEFT].changed = 1;
       break;

     case 'm':
       colour_data = optarg;
       if (getRegionInfo(colour_data, &regions[REGION_MIDDLE]) < 0)
       {
         printf("Colour info incorrect\n");
         cleanupExit(handle, EXIT_FAILURE);
       }
       regions[REGION_MIDDLE].changed = 1;
       break;

     case 'r':
       colour_data = optarg;
       if (getRegionInfo(colour_data, &regions[REGION_RIGHT]) < 0)
       {
         printf("Colour info incorrect\n");
         cleanupExit(handle, EXIT_FAILURE);
       }
       regions[REGION_RIGHT].changed = 1;
       break;

     case 'n':
       if (mode == 0)
         mode = 1;
       break;

     case 'g':
       if (mode == 0)
         mode = 2;
       break;

     case 'b':
       if (mode == 0)
         mode = 3;
       break;

     case 'd':
       if (mode == 0)
         mode = 4;
       break;

     case 'w':
       if (mode == 0)
         mode = 5;
       break;

     default:
       printf("Bad argument\n");
       cleanupExit(handle, EXIT_FAILURE);
       break;
   }  // end switch
  } // end while

  // check the various regions to see if they have been updated
  for (i = 0; i < REGIONS; i++)
  {
    if (regions[i].changed == 1)
    {
      if (setRegion(handle, &regions[i]) < 0)
      {
        printf("Unable to set region, exiting\n");
        cleanupExit(handle, EXIT_FAILURE);
      }
    }
  }

  // If mode wasn't set, set it to normal
  if (mode == 0)
    mode = 1;

  if (setMode(handle, mode) < 0)
  {
    printf("Unable to set mode, exiting\n");
    cleanupExit(handle, EXIT_FAILURE);
  }

  hid_close(handle);

  /* Free static HIDAPI objects. */
  hid_exit();

  return EXIT_SUCCESS;
}

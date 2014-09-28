#ifndef MOTION_ESTIMATION_HEADER
#define MOTION_ESTIMATION_HEADER

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#if defined(OMP_ON_MAIN)
#include <omp.h>
#elif defined(OMP_ON_ABS)
#include <omp.h>
#elif defined(OMP_ON_DIA)
#include <omp.h>
#else
#define BLEACH
#endif

#define CHILD_NUM  4

typedef enum tagBOOLEAN {

  FALSE,
  TRUE

} boolean;

typedef enum tagSEARCHMETHOD {

  NONE_SEARCH,
  ABSOLUTE_SEARCH,
  DIAMOND_SEARCH,
  HEXAGON_SEARCH

} SearchMethod;

typedef struct tagYUVcolorFoemat {

  unsigned char y;
  char u;
  char v;

} YUVcolor;

typedef struct tagBITMAPFILEHEADER {

  unsigned short byType;
  unsigned long  bfSize;
  unsigned short bfReserved1;
  unsigned short bfReserved2;
  unsigned long  bfOffBits;

} BITMAPFILEHEADER;

typedef struct tagBITMAPCOREHEADER {

  unsigned long  bcSize;
  short          bcWidth;
  short          bcHeight;
  unsigned short bcPlanes;
  unsigned short bcBitCount;

} BITMAPCOREHEADER;


typedef struct tagBITMAPINFOHEADER {

  unsigned long  biSize;
  long           biWidth;
  long           biHeight;
  unsigned short biPlanes;
  unsigned short biBitCount;
  unsigned long  biCopmression;
  unsigned long  biSizeImage;
  long           biXPixPerMeter;
  long           biYPixPerMeter;
  unsigned long  biClrUsed;
  unsigned long  biCirImportant;

} BITMAPINFOHEADER;

typedef enum tagMACROBLOCKSIZE { /* width x height */

  SIXTYFOUR,            //  0
  SIXTYFOUR_THIRTYTWO,  //  1
  THIRTYTWO_SIXTYFOUR,  //  2
  THIRTYTWO,            //  3
  THIRTYTWO_SIXTEEN,    //  4
  SIXTEEN_THIRTYTWO,    //  5
  SIXTEEN,              //  6
  SIXTEEN_EIGHT,        //  7
  EIGHT_SIXTEEN,        //  8
  EIGHT,                //  9
  EIGHT_FOUR,           // 10
  FOUR_EIGHT,           // 11
  FOUR,                 // 12
  SIXTYFOUR_SIXTEEN,    // 13
  SIXTYFOUR_FORTYEIGHT, // 14
  SIXTEEN_SIXTYFOUR,    // 15
  FORTYEIGHT_SIXTYFOUR, // 16
  THIRTYTWO_EIGHT,      // 17
  THIRTYTWO_TWENTYFOUR, // 18
  EIGHT_THIRTYTWO,      // 19
  TWENTYFOUR_THIRTYTWO, // 20
  SIXTEEN_FOUR,         // 21
  SIXTEEN_TWELVE,       // 22
  FOUR_SIXTEEN,         // 23
  TWELVE_SIXTEEN        // 24

} MBS;

typedef enum tagCUPATTERN {
  _2Nx2N,
  _2NxnU,
  _2NxnD,
  _nLx2N,
  _nRx2N,
  _2NxN,
  _Nx2N,
  _NxN,
  _NONE
} CUPattern;

typedef struct tagPOSITION {

  short x;
  short y;

} Position;

typedef struct tagCTU {

  boolean split_cu_flag;
  MBS size;
  Position MV;
  struct tagCTU *child[CHILD_NUM];
  YUVcolor *picture;

} CTU;

typedef struct tagINPUTPICTURESTATUS {

  int width;
  int height;

  int numberOfCuX;
  int numberOfCuY;

  FILE *inputFile;
  long picStartP;

  BITMAPFILEHEADER *bfh;
  BITMAPCOREHEADER *bch;
  BITMAPINFOHEADER *bih;

  boolean isBitmapCore;

  YUVcolor *picture;
  CTU *ctu;

} InPicStatus;

typedef enum tagENCODEMODE {

  NONE_ENC,
  MPEG,
  AVC,
  HEVC

} EncMode;

typedef struct tagPARAMETER {

  boolean isCommandErrorDetected;
  boolean isDebugMode;
  EncMode encodeMode;
  MBS minCUSize;
  MBS CUtype;
  int CUSize;
  FILE *DebugFp;
  SearchMethod method;
  int searchRange;
  double threshold;
  int picWidth;
  int picHeight;
  boolean isWindows;

  int minThread;
  int maxThread;
  int listValid;
  int repeatNum;
  boolean isProfile;
  boolean isAMP;

  int inp[2];
  int out;

} PARAM;

extern PARAM param;
extern FILE *YUV;
extern FILE *YUV1;
extern int counter[26];
extern int CuSizeDefine[25][2];
extern char DebugCTUSize[25][21];
extern unsigned char ValueForEachPUSize[25];
extern unsigned long long TotalRefPicNum;

/* bitmapFileHeader.c */
boolean getPicture    (InPicStatus *ips);
boolean freeAllPicmem (InPicStatus *ips);

/* picture.c */
void changeRGBtoYUV   (InPicStatus *ips);
void WriteMVOnTheFile (CTU *ctu, FILE *outFile);
void calcMotionVector (InPicStatus *ips, FILE *outFile);

/* setCTU.c */
void SetCTU     (InPicStatus *ips, FILE *outFile);
void FreeCtuMem (InPicStatus *ips);

/* Command.c */
boolean SetParamsFromCommand (int argc, char *argv[]);

/* profile.c */
void WriteOutProfiles (InPicStatus *ips, char *argv[], double *listTrial);

#endif // MOTION_ESTIMATION_HEADER

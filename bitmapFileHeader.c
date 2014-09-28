#include "motionEstimation.h"

#define INFO_HEADER 54

static boolean readBitmapFileHeader (InPicStatus *ips);
static boolean readBitmapCoreHeader (InPicStatus *ips);
static boolean readBitmapInfoHeader (InPicStatus *ips);
static boolean createYUVPicture     (InPicStatus *ips);

boolean getPicture (InPicStatus *ips)
{

  if (! readBitmapFileHeader (ips))
      return FALSE;

  if (ips->bfh->bfOffBits == INFO_HEADER)
  {

    ips->isBitmapCore = FALSE;

    if (! readBitmapInfoHeader (ips))
      return FALSE;

    ips->width = ips->bih->biWidth;
    ips->height = ips->bih->biHeight;
    ips->numberOfCuX = ips->width / param.CUSize;
    ips->numberOfCuY = ips->height / param.CUSize;

    if ((ips->width % param.CUSize != 0) || (ips->height % param.CUSize != 0))
    {

      fprintf (stderr, "[ME]erro : Unsupported picture size.\n");
      return FALSE;

    }

    if ((ips->bih->biBitCount != 32) && (ips->bih->biBitCount != 24))
    {

      fprintf (stderr, "[ME]erro : Unsupported color type, aborting.\n");
      return FALSE;

    }

    if (ips->bih->biCopmression != 0)
    {

      fprintf (stderr, "[ME]erro : Unsupported color palet type, aborting.\n");
      return FALSE;

    }

    if (! createYUVPicture (ips))
      return FALSE;

    return TRUE;

  } else {

    ips->isBitmapCore = TRUE;

    if (! readBitmapCoreHeader (ips))
      return FALSE;
    else
      return TRUE;

  }

}

static boolean readBitmapFileHeader (InPicStatus *ips)
{

  if ((ips->bfh = (BITMAPFILEHEADER *) malloc (sizeof (BITMAPFILEHEADER))) == NULL)
    return FALSE;

  fread (&ips->bfh->byType,      2, 1, ips->inputFile); // sizeof (unsigned short)
  fread (&ips->bfh->bfSize,      4, 1, ips->inputFile); // sizeof (unsigned  long)
  fread (&ips->bfh->bfReserved1, 2, 1, ips->inputFile); // sizeof (unsigned short)
  fread (&ips->bfh->bfReserved2, 2, 1, ips->inputFile); // sizeof (unsigned short)
  fread (&ips->bfh->bfOffBits,   4, 1, ips->inputFile); // sizeof (unsigned  long)

  if (param.isWindows)
    ips->bfh->bfOffBits = INFO_HEADER;

  return TRUE;

}

static boolean readBitmapCoreHeader (InPicStatus *ips)
{

  if ((ips->bch = (BITMAPCOREHEADER *) malloc (sizeof (BITMAPCOREHEADER))) == NULL)
    return FALSE;

  fread (&ips->bch->bcSize,     4, 1, ips->inputFile); // sizeof (unsgined  long)
  fread (&ips->bch->bcWidth,    2, 1, ips->inputFile); // sizeof (         short)
  fread (&ips->bch->bcHeight,   2, 1, ips->inputFile); // sizeof (         short)
  fread (&ips->bch->bcPlanes,   2, 1, ips->inputFile); // sizeof (unsigned short)
  fread (&ips->bch->bcBitCount, 2, 1, ips->inputFile); // sizeof (unsigned short)

  return TRUE;

}

static boolean readBitmapInfoHeader (InPicStatus *ips)
{

  if ((ips->bih = (BITMAPINFOHEADER *) malloc (sizeof (BITMAPINFOHEADER))) == NULL)
    return FALSE;

  fread (&ips->bih->biSize,          4, 1, ips->inputFile); // sizeof (unsigned  long)
  fread (&ips->bih->biWidth,         4, 1, ips->inputFile); // sizeof (          long)
  fread (&ips->bih->biHeight,        4, 1, ips->inputFile); // sizeof (          long)
  fread (&ips->bih->biPlanes,        2, 1, ips->inputFile); // sizeof (unsigned short)
  fread (&ips->bih->biBitCount,      2, 1, ips->inputFile); // sizeof (unsigned short)
  fread (&ips->bih->biCopmression,   4, 1, ips->inputFile); // sizeof (unsigned  long)
  fread (&ips->bih->biSizeImage,     4, 1, ips->inputFile); // sizeof (unsigned  long)
  fread (&ips->bih->biXPixPerMeter,  4, 1, ips->inputFile); // sizeof (          long)
  fread (&ips->bih->biYPixPerMeter,  4, 1, ips->inputFile); // sizeof (          long)
  fread (&ips->bih->biClrUsed,       4, 1, ips->inputFile); // sizeof (unsigned  long)
  fread (&ips->bih->biCirImportant,  4, 1, ips->inputFile); // sizeof (unsigned  long)

  if (param.picWidth != 0)
    ips->bih->biWidth = (long) param.picWidth;

  if (param.picHeight != 0)
    ips->bih->biHeight = (long) param.picHeight;

  if (param.isWindows)
    ips->bih->biCopmression = (unsigned long) 24;

  return TRUE;

}

static boolean createYUVPicture (InPicStatus *ips)
{

  if ((ips->picture = (YUVcolor *) malloc (sizeof (YUVcolor) * ips->height * ips->width)) == NULL)
    return FALSE;

  return TRUE;

}

boolean freeAllPicmem (InPicStatus *ips)
{

  free (ips->bfh);

  if (ips->isBitmapCore)
    free (ips->bch);
  else
    free (ips->bih);

  free (ips->picture);

  return TRUE;

}

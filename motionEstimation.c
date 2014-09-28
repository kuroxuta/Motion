#include "motionEstimation.h"

PARAM param;

static double getETime ()
{

  struct timeval tv;

  gettimeofday (&tv, NULL);

  return (double) tv.tv_sec + (double) tv.tv_usec * 1e-6;

};

static FILE *openInputFile (char *filename)
{

  FILE *fp;

  if ((fp = fopen (filename, "rb")) == NULL)
    return NULL;

  return fp;

}

static FILE *openOutputFile (char *filename)
{

  FILE *fps;

  if ((fps = fopen (filename, "wb")) == NULL)
    return NULL;

  if (param.isDebugMode)
  {
    if ((param.DebugFp = fopen ("out.debug", "w")) == NULL)
      param.isDebugMode = FALSE;
    else
      param.isDebugMode = TRUE;

  }

  return fps;

}

static void writeMVfileHeader (FILE *fps, InPicStatus *ips)
{

  char fType[2];
  unsigned short x = (unsigned short) ips[0].width;
  unsigned short y = (unsigned short) ips[0].height;
  unsigned short cuSize = (unsigned short) param.CUtype;
  unsigned short range  = (unsigned short) param.searchRange;

  fType[0] = 'M';
  fType[1] = 'V';

  fwrite (&fType,  sizeof (fType),  1, fps);
  fwrite (&x,      sizeof (x),      1, fps);
  fwrite (&y,      sizeof (y),      1, fps);
  fwrite (&cuSize, sizeof (cuSize), 1, fps);
  fwrite (&range,  sizeof (range),  1, fps);

}

static boolean destroyProgram (InPicStatus *ips, FILE *outFp)
{

  //FreeCtuMem (&ips[0]);
  freeAllPicmem (&ips[0]);
  FreeCtuMem (&ips[1]);
  fclose (ips[0].inputFile);
  fclose (ips[1].inputFile);

  fclose (outFp);

  if (param.isDebugMode)
    fclose (param.DebugFp);

  return TRUE;

}
int main (int argc, char *argv[])
{

  FILE *outFile;
  InPicStatus ips[2];
  int i, j;
  double start, end;
  double *listTrial;

  if (! SetParamsFromCommand (argc, argv))
    return 0;
  else {

    if (((ips[0].inputFile = openInputFile (argv[param.inp[0]])) == NULL) ||
	((ips[1].inputFile = openInputFile (argv[param.inp[1]])) == NULL))
    {

      fprintf (stderr, "[ME]erro : failed to open input files.\n");
      return FALSE;

    }

    if ((outFile = openOutputFile (argv[param.out])) == NULL)
    {

      fprintf (stderr, "[ME]erro : failed to open output file.\n");
      return FALSE;

    }

  }

  for (i = 0; i < 2; i++)
  {

    if (! getPicture (&ips[i]))
      return (int) destroyProgram (ips, outFile);

    fprintf (stderr, "[ME]info : file name ... %s\n", argv[param.inp[i]]);
    fprintf (stderr, "[ME]info : picture No.%d size --> %d x %d\n", i + 1, ips[i].width, ips[i].height);

    changeRGBtoYUV (&ips[i]);
    if (i == 1)
      SetCTU (&ips[i], outFile);

  }

  // Write file header.
  writeMVfileHeader (outFile, ips);
  for (i = 0; i < 14; i++)
    counter[i] = 0;

  listTrial = (double *) malloc (sizeof (double) * param.listValid * param.repeatNum);

  for (i = 0; i < param.repeatNum; i++)
  {

#ifndef BLEACH
    for (j = param.minThread; j < param.maxThread + 1; j++)
    {

      omp_set_num_threads (j);
      fprintf (stderr, "[ME]info : current thread num ... %d.\n", j);
#endif

      start = getETime ();
      calcMotionVector (ips, outFile);
      end = getETime ();

      fprintf (stderr, "[ME]info : %.10fsec          \n", end - start);

#ifndef BLEACH
      listTrial[(j - param.minThread) * param.repeatNum + i] = end - start;
    }
#else
    listTrial[i] = end - start;
#endif

  }

  fprintf (stderr, "[ME]info : this mode accessed %llu pics.\n", TotalRefPicNum);

  // Write Date.
  for (i = 0; i < ips[0].numberOfCuX * ips[0].numberOfCuY; i++)
    WriteMVOnTheFile (&ips[1].ctu[i], outFile);

#if defined(DEBUG)
  fprintf (stderr, "\n[ME]debu : prediction unit usage.\n");
  for (i = 0; i < 25; i++)
    fprintf (stderr, "[ME]debu : [%20s] ===> %8d (%6.3f %%)\n", DebugCTUSize[i], counter[i], (double) 100 * counter[i] / counter[25]);
  fprintf (stderr, "[ME]debu : %27s %8d (100.000%%)\n", "TOTAL", counter[25]);
#endif

  if (param.isProfile)
    WriteOutProfiles (ips, argv, listTrial);

  return (int) destroyProgram (ips, outFile);

}

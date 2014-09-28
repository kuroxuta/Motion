#include "motionEstimation.h"

static void        InitAllParams ();
static boolean    SetDebugModeOn (char *str);
static boolean      SetCodecMode (char *str);
static boolean   SetSearchMethod (char *str);
static boolean    SetSearchRange (char *argv[], int *pos);
static boolean      SetThreshold (char *argv[], int *pos);
static boolean        SetCTUSize (char *argv[], int *pos);
static boolean    SetPictureSize (char *argv[], int *pos);
static boolean  GetInputFilename (char *argv[], int *pos);
static boolean GetOutputFilename (char *argv[], int *pos);
static boolean      SetMinThread (char *argv[], int *pos);
static boolean      SetMaxThread (char *argv[], int *pos);
static boolean      GetRepeatNum (char *argv[], int *pos);
static boolean        IsPorfiled (char *str);
static boolean             IsAMP (char *str);
static boolean     IsWindowsType (char *str);
static void       FillUpCommands ();
static void         ShowCommands ();

boolean SetParamsFromCommand (int argc, char *argv[])
{

  int i;

  InitAllParams ();

  for (i = 1; i < argc; i++)
  {

    if (SetDebugModeOn         (argv[i]));
    else if (SetCodecMode      (argv[i]));
    else if (SetSearchMethod   (argv[i]));
    else if (IsPorfiled        (argv[i]));
    else if (IsAMP             (argv[i]));
    else if (IsWindowsType     (argv[i]));
    else if (SetSearchRange    (argv, &i));
    else if (SetThreshold      (argv, &i));
    else if (GetInputFilename  (argv, &i));
    else if (GetOutputFilename (argv, &i));
    else if (SetCTUSize        (argv, &i));
    else if (SetPictureSize    (argv, &i));
    else if (SetMinThread      (argv, &i));
    else if (SetMaxThread      (argv, &i));
    else if (GetRepeatNum      (argv, &i));

    else
      param.isCommandErrorDetected = TRUE;

    if (param.isCommandErrorDetected)
    {

      fprintf (stderr, "[ME]erro : invalid command is detected.\n");
      return FALSE;

    }

  }

  FillUpCommands ();
  ShowCommands ();

  return TRUE;

}

static void InitAllParams ()
{

  param.isCommandErrorDetected = FALSE;
  param.isDebugMode = FALSE;
  param.encodeMode = NONE_ENC;
  param.minCUSize = 0;
  param.CUSize = 0;
  param.DebugFp = NULL;
  param.method = NONE_SEARCH;
  param.searchRange = 0;
  param.threshold = 0.0;

  param.minThread = 0;
  param.maxThread = 0;
  param.listValid = 1;
  param.repeatNum = 0;
  param.isProfile = FALSE;
  param.isAMP     = FALSE;

  param.picWidth  = 0;
  param.picHeight = 0;
  param.isWindows = FALSE;

  param.inp[0] = 0;
  param.inp[1] = 0;
  param.out    = 0;

}

static boolean SetDebugModeOn (char *str)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (str, "--debug") == 0)
  {

    param.isDebugMode = TRUE;
    return TRUE;

  }

  return FALSE;

}

static boolean SetCodecMode (char *str)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (str, "--mpeg") == 0)
  {

    if (param.encodeMode != NONE_ENC)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : codec mode is set twice.\n");
      return FALSE;

    } else {

      param.encodeMode = MPEG;
      return TRUE;

    }

  } else if (strcmp (str, "--avc") == 0) {

    if (param.encodeMode != NONE_ENC)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : codec mode is set twice.\n");
      return FALSE;

    } else {

      param.encodeMode = AVC;
      return TRUE;

    }

  } else if (strcmp (str, "--hevc") == 0) {

    if (param.encodeMode != NONE_ENC)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : codec mode is set twice.\n");
      return FALSE;

    } else {

      param.encodeMode = HEVC;
      return TRUE;

    }

  }

  return FALSE;

}

static boolean SetSearchMethod (char *str)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (str, "--abs") == 0)
  {

    if (param.method != NONE_SEARCH)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : search method is set twice.\n");
      return FALSE;

    } else {

      param.method = ABSOLUTE_SEARCH;
      return TRUE;

    }

  } else if (strcmp (str, "--dia") == 0) {

    if (param.method != NONE_SEARCH)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : search method is set twice.\n");
      return FALSE;

    } else {

      param.method = DIAMOND_SEARCH;
      return TRUE;

    }

  } else if (strcmp (str, "--hex") == 0) {

    if (param.method != NONE_SEARCH)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : search method is set twice.\n");
      return FALSE;

    } else {

      param.method = HEXAGON_SEARCH;
      return TRUE;

    }

  }

  return FALSE;

}

static boolean SetSearchRange (char *argv[], int *pos)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (argv[*pos], "--range") == 0)
  {

    if (param.searchRange != 0)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : search range is defined twice.\n");
      return FALSE;

    } else {

      *pos += 1;

      if ((param.searchRange = atoi (argv[*pos])) == 0)
      {

	fprintf (stderr, "[ME]erro : invalid search range.\n");
	param.isCommandErrorDetected = TRUE;
	return FALSE;

      } else {

	return TRUE;

      }

    }

  }

  return FALSE;

}

static boolean SetThreshold (char *argv[], int *pos)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (argv[*pos], "--thre") == 0)
  {

    if (param.threshold != 0.0)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : threshold is defined twice.\n");
      return FALSE;

    } else {

      *pos += 1;

      if ((param.threshold = atof (argv[*pos])) == 0.0)
      {

	fprintf (stderr, "[ME]erro : invalid threshold.\n");
	param.isCommandErrorDetected = TRUE;
	return FALSE;

      } else {

	return TRUE;

      }

    }

  }

  return FALSE;

}
static boolean SetCTUSize (char *argv[], int *pos)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (argv[*pos], "--CTU") == 0)
  {

    if (param.CUSize != 0)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : ctu size is defined twice.\n");
      return FALSE;

    } else {

      *pos += 1;

      if ((param.CUSize = atoi (argv[*pos])) == 0)
      {

	fprintf (stderr, "[ME]erro : invalid ctu size.\n");
	param.isCommandErrorDetected = TRUE;
	return FALSE;

      } else {

	return TRUE;

      }

    }

  }

  return FALSE;

}

static boolean SetPictureSize (char *argv[], int *pos)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (argv[*pos], "--psize") == 0)
  {

    if (param.picWidth != 0)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : picture size is defined twice.\n");
      return FALSE;

    } else {

      *pos += 1;

      scanf ("%dx%d", &param.picWidth, &param.picHeight);

      if ((param.picWidth == 0) || (param.picHeight == 0))
      {

	fprintf (stderr, "[ME]erro : invalid picture size.\n");
	param.isCommandErrorDetected = TRUE;

      } else {

	return TRUE;

      }

    }

  }

  return FALSE;

}

static boolean GetInputFilename (char *argv[], int *pos)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (argv[*pos], "--input") == 0)
  {

    if ((param.inp[0] != 0) || (param.inp[1] != 0))
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : input files were already defined.\n");
      return FALSE;

    } else {

      *pos += 1;
      param.inp[0] = *pos;
      *pos += 1;
      param.inp[1] = *pos;
      return TRUE;

    }

  }

  return FALSE;

}

static boolean GetOutputFilename (char *argv[], int *pos)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (argv[*pos], "--output") == 0)
  {

    if (param.out != 0)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : output file was already defined.\n");
      return FALSE;

    } else {

      *pos += 1;
      param.out = *pos;
      return TRUE;

    }

  }

  return FALSE;

}

static boolean SetMinThread (char *argv[], int *pos)
{

  if (param.isCommandErrorDetected)
    return FALSE;

#ifdef BLEACH
  if (strcmp (argv[*pos], "--min-thread") == 0)
  {
    fprintf (stderr, "[ME]warn : --min--thread command can be used when the program has been compiled with OpenMP.\n");
    fprintf (stderr, "[ME]warn : insert \"CFLAGS=-DOMP_ON_MAIN or -DOMP_ON_ABS or both\" when you complie it with make.\n");
  }
  return TRUE;
#endif

  if (strcmp (argv[*pos], "--min-thread") == 0)
  {

    if (param.minThread != 0)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : minimum thread was already defined.\n");
      return FALSE;

    } else {

      *pos += 1;

      if ((param.minThread = atoi (argv[*pos])) == 0)
      {

	fprintf (stderr, "[ME]erro : invalid min thread.\n");
	param.isCommandErrorDetected = TRUE;
	return FALSE;

      } else {

	return TRUE;

      }

    }

  }

  return FALSE;

}

static boolean SetMaxThread (char *argv[], int *pos)
{

  if (param.isCommandErrorDetected)
    return FALSE;

#ifdef BLEACH
  if (strcmp (argv[*pos], "--max-thread") == 0)
  {
    fprintf (stderr, "[ME]warn : --max--thread command can be used when the program has been compiled with OpenMP.\n");
    fprintf (stderr, "[ME]warn : insert \"CFLAGS=-DOMP_ON_MAIN or -DOMP_ON_ABS or both\" when you complie it with make.\n");
  }
  return TRUE;
#endif

  if (strcmp (argv[*pos], "--max-thread") == 0)
  {

    if (param.maxThread != 0)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : maximum thread was already defined.\n");
      return FALSE;

    } else {

      *pos += 1;

      if ((param.maxThread = atoi (argv[*pos])) == 0)
      {

	fprintf (stderr, "[ME]erro : invalid max thread.\n");
	param.isCommandErrorDetected = TRUE;
	return FALSE;

      } else {

	return TRUE;

      }

    }

  }

  return FALSE;

}

static boolean GetRepeatNum (char *argv[], int *pos)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (argv[*pos], "--repeat") == 0)
  {

    if (param.repeatNum != 0)
    {

      param.isCommandErrorDetected = TRUE;
      fprintf (stderr, "[ME]erro : repeat time was already defined.\n");
      return FALSE;

    } else {

      *pos += 1;

      if ((param.repeatNum = atoi (argv[*pos])) == 0)
      {

	param.isCommandErrorDetected = TRUE;
	fprintf (stderr, "[ME]erro : invalid repeat time.\n");
	return FALSE;

      } else {

	return TRUE;

      }

    }

  }

  return FALSE;

}

static boolean IsPorfiled (char *str)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (str, "--profile") == 0)
  {

    param.isProfile = TRUE;
    return TRUE;

  }

  return FALSE;

}

static boolean IsAMP (char *str)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (str, "--AMP") == 0)
  {

    param.isAMP = TRUE;
    return TRUE;

  }

  return FALSE;

}

static boolean IsWindowsType (char *str)
{

  if (param.isCommandErrorDetected)
    return FALSE;

  if (strcmp (str, "--win") == 0)
  {

    param.isWindows = TRUE;
    return TRUE;

  }

  return FALSE;

}

static void FillUpCommands ()
{

  int n;

  if (param.method == NONE_SEARCH)
    param.method = ABSOLUTE_SEARCH;

  if (param.encodeMode == NONE_ENC)
    param.encodeMode = HEVC;

  if (param.searchRange == 0)
    param.searchRange = 5;

  if (param.threshold == 0.0)
    param.threshold = 16.0;

  if (param.repeatNum <= 0)
    param.repeatNum = 1;

  if (param.repeatNum > 10)
    param.repeatNum = 10;

#ifndef BLEACH
  if (param.minThread <= 0)
    param.minThread = omp_get_max_threads ();

  if (param.maxThread <= 0)
    param.maxThread = omp_get_max_threads ();

  if (param.minThread > omp_get_max_threads ())
    param.minThread = omp_get_max_threads ();

  if (param.maxThread > omp_get_max_threads ())
    param.maxThread = omp_get_max_threads ();

  if (param.maxThread < param.minThread)
  {
    n = param.maxThread;
    param.maxThread = param.minThread;
    param.minThread = n;
  }

  param.listValid = param.maxThread - param.minThread + 1;
#endif

}

static void ShowCommands ()
{

  if (param.isDebugMode)
  {

    fprintf (stderr, "[ME]info : debug mode is on.\n");

  }

  if (param.isProfile)
  {

    fprintf (stderr, "[ME]info : profile mode is on.\n");

  }

  if (param.isProfile)
  {

    fprintf (stderr, "[ME]info : AMP mpde is on.\n");

  }

  if (param.encodeMode == MPEG)
  {

    fprintf (stderr, "[ME]info : MPEG mode.\n");
    param.minCUSize = SIXTEEN;
    if (param.CUSize != 16)
      param.CUSize = 16;
    param.CUtype = SIXTEEN;

  } else if (param.encodeMode == AVC) {

    fprintf (stderr, "[ME]info : AVC mode.\n");
    param.minCUSize = FOUR;
    if (param.CUSize != 16)
      param.CUSize = 16;
    param.CUtype = SIXTEEN;

  } else {

    fprintf (stderr, "[ME]info : HEVC mode.\n");
    param.minCUSize = FOUR;

    switch (param.CUSize)
    {
    case 16:
      param.CUtype = SIXTEEN;
      break;
    case 32:
      param.CUtype = THIRTYTWO;
      break;
    case 64:
      param.CUtype = SIXTYFOUR;
      break;
    default:
      param.CUSize = 64;
      param.CUtype = SIXTYFOUR;
      break;

    }

  }

  if (param.method == ABSOLUTE_SEARCH)
  {

    fprintf (stderr, "[ME]info : absolute search  ... range %d.\n", param.searchRange);

  } else {

    if (param.method == DIAMOND_SEARCH)
    {

      fprintf (stderr, "[ME]info : diamond search   ... range");

    } else {

      fprintf (stderr, "[ME]info : hexagonal Search ... range");

    }

    fprintf (stderr, " %d.\n", param.searchRange);

  }

  fprintf (stderr, "[ME]info : threshold        ... %f.\n", param.threshold);
  fprintf (stderr, "[ME]info : ctu size         ... %d.\n", param.CUSize);
  fprintf (stderr, "[ME]info : minimum ctu size ... %s.\n", DebugCTUSize[param.minCUSize]);
  fprintf (stderr, "[NE]info : repeat time      ... %d.\n", param.repeatNum);

#ifndef BLEACH
  fprintf (stderr, "[ME]info : OpenMP is used.\n");
  fprintf (stderr, "[ME]info : this program runs on multi threads (maximum = %d).\n", omp_get_max_threads ());
  fprintf (stderr, "[ME]info : minimum threads  ... %d.\n", param.minThread);
  fprintf (stderr, "[ME]info : maximum threads  ... %d.\n", param.maxThread);
#endif

}

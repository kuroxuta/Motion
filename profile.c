#include "motionEstimation.h"

static void WriteOutResources  (FILE *fp, InPicStatus *ips, char *argv[]);
static void WriteOutParameters (FILE *fp, InPicStatus *ips, char *argv[]);
static void WriteOutTrialList  (FILE *fp, double *listTrial);

void WriteOutProfiles (InPicStatus *ips, char *argv[], double *listTrial)
{

  FILE *fp;

  fp = fopen ("ME.prof", "w");

  WriteOutResources  (fp, ips, argv);
  WriteOutParameters (fp, ips, argv);
  WriteOutTrialList  (fp, listTrial);

  fclose (fp);

}

static void WriteOutResources (FILE *fp, InPicStatus *ips, char *argv[])
{

  fprintf (fp, "+----------------------------------------------------------------\n");
  fprintf (fp, "| Motion Estimation log file.\n");
  fprintf (fp, "+----------------------------------------------------------------\n");
  fprintf (fp, "|\n");
  fprintf (fp, "+-- Input Source.\n");
  fprintf (fp, "|     |\n");
  fprintf (fp, "|     +-- 1st file.\n");
  fprintf (fp, "|     |    |\n");
  fprintf (fp, "|     |    +-- size     : %d x %d\n", ips[0].width, ips[0].height);
  fprintf (fp, "|     |    |\n");
  fprintf (fp, "|     |    +-- filename : %s\n", argv[param.inp[0]]);
  fprintf (fp, "|     |\n");
  fprintf (fp, "|     +-- 2nd file.\n");
  fprintf (fp, "|          |\n");
  fprintf (fp, "|          +-- size     : %d x %d\n", ips[1].width, ips[1].height);
  fprintf (fp, "|          |\n");
  fprintf (fp, "|          +-- filename : %s\n", argv[param.inp[1]]);
  fprintf (fp, "|\n");
  fprintf (fp, "+-- Output Source.\n");
  fprintf (fp, "|     |\n");
  fprintf (fp, "|     +-- Motion vectors.\n");
  fprintf (fp, "|     |    |\n");
  fprintf (fp, "|     |    +-- filename : %s\n", argv[param.out]);
  fprintf (fp, "|     |\n");
  fprintf (fp, "|     +-- Debug info.\n");
  fprintf (fp, "|          |\n");
  fprintf (fp, "|          +-- filename : ");

  if (param.isDebugMode)
    fprintf (fp, "out.debug\n");
  else
    fprintf (fp, "NO CREATED\n");

  fprintf (fp, "|\n");
  fprintf (fp, "+----------------------------------------------------------------\n");

}

static void WriteOutParameters (FILE *fp, InPicStatus *ips, char *argv[])
{

  char EncName[4][5] = {"", "MPEG", "AVC", "HEVC"};
  char AlgName[4][18] = {"", "Absolute search", "Diamond serach", "Hexagonal search"};
  char BlocName[13][8] = {
    "64 x 64", "64 x 32", "32 x 64", "32 x 32", "32 x 16", "16 x 32",
    "16 x 16", "16 x  8", " 8 x 16", " 8 x  8", " 8 x  4", " 4 x  8", " 4 x  4"
  };
  char OMPName[2][4] = {"OFF", "ON"};
  char Parallel[4][23] = {"", "OMP_ON_MAIN", "OMP_ON_ABS", "OMP_ON_MAIN OMP_ON_ABS"};

#ifdef BLEACH
  int ompname = 0;
  int para = 0;
#else
  int ompname = 1;
  int para = 0;
#endif

#if defined(OMP_ON_MAIN)
  para += 1;
#elif defined(OMP_ON_ABS)
  para += 2;
#endif


  fprintf (fp, "| Parameters.\n");
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "|      Codec type | %43s |\n", EncName[param.encodeMode]);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "|       Algorithm | %43s |\n", AlgName[param.method]);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "|    Search range | %43d |\n", param.searchRange);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "|       Threshold | %43.6f |\n", param.threshold);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "|        CTU Size | %43s |\n", BlocName[param.CUtype]);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "| Minimum PU size | %43s |\n", BlocName[param.minCUSize]);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "|     Trial count | %43d |\n", param.repeatNum);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "|          OpenMP | %43s |\n", OMPName[ompname]);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "|     Parallelize | %43s |\n", Parallel[para]);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "| Maximum Threads | %43d |\n", param.minThread);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");
  fprintf (fp, "| Minimum Threads | %43d |\n", param.maxThread);
  fprintf (fp, "+-----------------+---------------------------------------------+\n");

}

static void WriteOutTrialList (FILE *fp, double *listTrial)
{

  int i, j;
  double ave;
  FILE *csv;

  csv = fopen ("ME.svc", "w");

  fprintf (fp, "\n\n\n\n");

  fprintf (fp, "+------------+");
  for (i = 0; i < param.repeatNum; i++)
  {
    fprintf (fp, "+------------");
  }
  fprintf (fp, "++------------");
  fprintf (fp, "+\n");

  fprintf (fp, "| thread num ||");
  for (i = 0; i < param.repeatNum; i++)
  {
    fprintf (fp, " %5d time |", i + 1);
  }
  fprintf (fp, "|    average |\n");

  fprintf (fp, "+============+");
  for (i = 0; i < param.repeatNum; i++)
  {
    fprintf (fp, "+============");
  }
  fprintf (fp, "++============" );
  fprintf (fp, "+\n");

  for (i = 0; i < param.listValid; i++)
  {

    ave = 0.0;
    fprintf (fp, "| %3d thread ||", i + param.minThread);
    fprintf (csv, "%d,", i + param.minThread);
    for (j = 0; j < param.repeatNum; j++)
    {
      fprintf (fp, " %10.6f |", listTrial[i * param.repeatNum + j]);
      ave += listTrial[i * param.repeatNum + j];
    }
    fprintf (fp, "| %10.6f ", ave / param.repeatNum);
    fprintf (csv, "%10.6f\n", ave / param.repeatNum);
    fprintf (fp, "|\n");

    fprintf (fp, "+------------+");
    for (j = 0; j < param.repeatNum; j++)
    {
      fprintf (fp, "+------------");
    }
    fprintf (fp, "++------------");
    fprintf (fp, "+\n");

  }

  fclose (csv);

}

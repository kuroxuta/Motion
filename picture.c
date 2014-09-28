#include "motionEstimation.h"

#define DIAMOND_SEARCH_RANGE 4
#define HEXAGON_SEARCH_RANGE 6

static void NextPUTree      (InPicStatus *ips, CTU *ctu, int cCUX, int cCUY, FILE *outFile, MBS size);
static void something       (InPicStatus *ips, CTU *ctu, int cCUX, int cCUY, FILE *outFile);
static  int calcSADinTwoCUs (InPicStatus *ips, YUVcolor *base, YUVcolor *src, int sx, int sy, int xDis, int yDis);
static void AbsoluteSearch  (InPicStatus *ips, YUVcolor *base, Position *vec, int xDis, int yDis);
static void DiamondSearch   (InPicStatus *ips, YUVcolor *base, Position *vec, int xDis, int yDis);
static void HexagonalSearch (InPicStatus *ips, YUVcolor *vase, Position *vec, int xDis, int yDis);

unsigned long long TotalRefPicNum = 0;

int counter[26] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned char ValueForEachPUSize[25] = {
  255,
  127,
  127,
   63,
   31,
   31,
   15,
    7,
    7,
    3,
    1,
    1,
    0,
  175,
   79,
   79,
  175,
   37,
   25,
   25,
   37,
    9,
    5,
    5,
    9

};

static int  Offsets[25][4][2] =
{
  {{ 0,  0}, { 0,  0}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 0, 32}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, {32,  0}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, {32,  0}, { 0, 32}, {32, 32}},
  {{ 0,  0}, { 0, 16}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, {16,  0}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, {16,  0}, { 0, 16}, {16, 16}},
  {{ 0,  0}, { 0,  8}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 8,  0}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 8,  0}, { 0,  8}, { 8,  8}},
  {{ 0,  0}, { 0,  4}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 4,  0}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 4,  0}, { 0,  4}, { 4,  4}},
  {{ 0,  0}, { 0, 16}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 0, 48}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, {16,  0}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, {48,  0}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 0,  8}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 0, 24}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 8,  0}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, {24,  0}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 0,  4}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 0, 12}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, { 4,  0}, { 0,  0}, { 0,  0}},
  {{ 0,  0}, {12,  0}, { 0,  0}, { 0,  0}}
};

char DebugCTUSize[25][21] = {
  "SIXTYFOUR",
  "SIXTYFOUR_THIRTYTWO",
  "THIRTYTWO_SIXTYFOUR",
  "THIRTYTWO",
  "THIRTYTWO_SIXTEEN",
  "SIXTEEN_THIRTYTWO",
  "SIXTEEN",
  "SIXTEEN_EIGHT",
  "EIGHT_SIXTEEN",
  "EIGHT",
  "EIGHT_FOUR",
  "FOUR_EIGHT",
  "FOUR",
  "SIXTYFOUR_SIXTEEN",
  "SIXTYFOUR_FORTYEIGHT",
  "SIXTEEN_SIXTYFOUR",
  "FORTYEIGHT_SIXTYFOUR",
  "THIRTYTWO_EIGHT",
  "THIRTYTWO_TWENTYFOUR",
  "EIGHT_THIRTYTWO",
  "TWENTYFOUR_THIRTYTWO",
  "SIXTEEN_FOUR",
  "SIXTEEN_TWELVE",
  "FOUR_SIXTEEN",
  "TWELVE_SIXTEEN"
};

void changeRGBtoYUV (InPicStatus *ips)
{
  int getBufByte = ips->bih->biBitCount == 24 ? 3 : 4;
  int i, j, n;
  unsigned char *str;

  if ((str = (unsigned char *) malloc (sizeof (unsigned char) * getBufByte)) == NULL)
    return;

  for (i = ips->height - 1; -1 < i; i--)
  {

    for (j = 0; j < ips->width; j++)
    {

      fread (str, getBufByte, 1, ips->inputFile);

      n = i * ips->width + j;
      ips->picture[n].y = (unsigned char) ( ((double) str[2] * 0.2126) + ((double) str[1] * 0.7152) + ((double) str[0] * 0.0722));
      ips->picture[n].u = (         char) (-((double) str[2] * 0.1146) - ((double) str[1] * 0.3854) + ((double) str[0] * 0.5000));
      ips->picture[n].v = (         char) ( ((double) str[2] * 0.5000) - ((double) str[1] * 0.4542) - ((double) str[0] * 0.0458));

    }

  }

  free (str);

}

void calcMotionVector (InPicStatus *ips, FILE *outFile)
{

  int i, j, h;

  h = 0;

#if defined(OMP_ON_MAIN)
#pragma omp parallel for private(j)
#endif

  for (i = 0; i < ips[0].numberOfCuY; i++)
  {

    for (j = 0; j < ips[0].numberOfCuX; j++)
    {

#if defined(OMP_ON_MAIN)
	h = i * ips[0].numberOfCuX + j;
#endif
	NextPUTree (ips, &ips[1].ctu[h], j * param.CUSize, i * param.CUSize, outFile, ips[1].ctu[h].size);

#if defined(DEBUG)
      fprintf (stderr, "[ME]info : processing ... %d per\r", h * 100 / (ips[0].numberOfCuX * ips[0].numberOfCuY));
#endif

#ifndef OMP_ON_MAIN
      h++;
#endif

    }

  }

}

void WriteMVOnTheFile (CTU *ctu, FILE *outFile)
{

  int i;

  if (ctu->split_cu_flag == FALSE)
  {

    putc (ValueForEachPUSize[ctu->size], outFile);
    putc ((char) ctu->size, outFile);
    fwrite (&ctu->MV, sizeof (Position), 1, outFile);

    counter[ctu->size]++;
    counter[25]++;

  } else {

    for (i = 0; i < CHILD_NUM; i++)
    {

      if (ctu->child[i] == NULL)
	break;

      WriteMVOnTheFile (ctu->child[i], outFile);

    }

  }

}

static void MergeCUsInSameCU (CTU *ctu, MBS size)
{

  if (ctu->child[2] == NULL)
  {

    if ((ctu->child[0]->MV.x != ctu->child[1]->MV.x) || (ctu->child[0]->MV.y != ctu->child[1]->MV.y))
      return;

    if ((!ctu->child[0]->split_cu_flag) && (!ctu->child[1]->split_cu_flag))
    {

      /* Merge _Nx2N or _2NxN type. */
      ctu->split_cu_flag = FALSE;
      ctu->size = size;
      ctu->MV.x = ctu->child[0]->MV.x;
      ctu->MV.y = ctu->child[0]->MV.y;

    }

  } else {

    if ((!ctu->child[0]->split_cu_flag) && (!ctu->child[1]->split_cu_flag) &&
	(!ctu->child[2]->split_cu_flag) && (!ctu->child[3]->split_cu_flag))
    {

      if ((ctu->child[0]->MV.x == ctu->child[1]->MV.x) && (ctu->child[0]->MV.y == ctu->child[1]->MV.y))
      {

	if ((ctu->child[2]->MV.x == ctu->child[3]->MV.x) && (ctu->child[2]->MV.y == ctu->child[3]->MV.y))
	{

	  if ((ctu->child[0]->MV.x == ctu->child[2]->MV.x) && (ctu->child[0]->MV.y == ctu->child[2]->MV.y))
	  {

	    ctu->split_cu_flag = FALSE;
	    ctu->size = size;
	    ctu->MV.x = ctu->child[0]->MV.x;
	    ctu->MV.y = ctu->child[0]->MV.y;

	  } else {

	    ctu->size = size;
	    ctu->child[0]->size = (MBS) ((int) size + 1); // SIXTEEN_EIGHT
	    ctu->child[1]->size = (MBS) ((int) size + 1); // SIXTEEN_EIGHT
	    ctu->child[1]->MV.x = ctu->child[2]->MV.x;
	    ctu->child[1]->MV.x = ctu->child[2]->MV.y;
	    free (ctu->child[2]);
	    free (ctu->child[3]);
	    ctu->child[2] = ctu->child[3] = NULL;

	  }

	}

      } else if ((ctu->child[0]->MV.x == ctu->child[2]->MV.x) && (ctu->child[0]->MV.y == ctu->child[2]->MV.y)) {

	if ((ctu->child[1]->MV.x == ctu->child[3]->MV.x) && (ctu->child[1]->MV.y == ctu->child[3]->MV.y))
	{

	  ctu->size = size;
	  ctu->child[0]->size = (MBS) ((int) size + 2); // EIGHT_SIXTEEN
	  ctu->child[1]->size = (MBS) ((int) size + 2); // EIGHT_SIXTEEN
	  free (ctu->child[2]);
	  free (ctu->child[3]);
	  ctu->child[2] = ctu->child[3] = NULL;

	}

      }

    }

  }

}

static void NextPUTree (InPicStatus *ips, CTU *ctu, int cCUX, int cCUY, FILE *outFile, MBS size)
{

  int i;

  if (ctu->split_cu_flag == FALSE)
  {

    something (ips, ctu, cCUX, cCUY, outFile);

  } else {

    for (i = 0; i < CHILD_NUM; i++)
    {

      if (ctu->child[i] == NULL)
	break;

      if ((i == 1) && ((13 <= ctu->child[i]->size) && (ctu->child[i]->size <= 24)))
      {

	NextPUTree (ips, ctu->child[i], cCUX + Offsets[ctu->child[0]->size][i][0], cCUY + Offsets[ctu->child[0]->size][i][1], outFile, ctu->child[i]->size);

      } else {

	NextPUTree (ips, ctu->child[i], cCUX + Offsets[ctu->child[i]->size][i][0], cCUY + Offsets[ctu->child[i]->size][i][1], outFile, ctu->child[i]->size);

      }

    }

    //MergeCUsInSameCU (ctu, size);

  }

}

static void something (InPicStatus *ips, CTU *ctu, int cCUX, int cCUY, FILE *outFile)
{

  int xDis, yDis;
  YUVcolor base[param.CUSize * param.CUSize];

  ctu->MV.x = cCUX;
  ctu->MV.y = cCUY;


#if defined(DEBUG)
  if (param.isDebugMode)
    fprintf (param.DebugFp, "ctu size [%s]\r\n", DebugCTUSize[ctu->size]);
#endif

  xDis = CuSizeDefine[ctu->size][0];
  yDis = CuSizeDefine[ctu->size][1];
  memcpy (base, ctu->picture, sizeof (YUVcolor) * xDis * yDis);

  switch (param.method)
  {
  case ABSOLUTE_SEARCH:
    AbsoluteSearch (ips, base, &ctu->MV, xDis, yDis);
    break;
  case DIAMOND_SEARCH:
    DiamondSearch (ips, base, &ctu->MV, xDis, yDis);
    break;
  case HEXAGON_SEARCH:
    HexagonalSearch (ips, base, &ctu->MV, xDis, yDis);
    break;
  default:
    break;
  }

#if defined(DEBUG)
  if (param.isDebugMode)
    fprintf (param.DebugFp, "vec ==> [%4d, %4d]\r\n", ctu->MV.x, ctu->MV.y);
#endif

}

static void AbsoluteSearch (InPicStatus *ips, YUVcolor *base, Position *vec, int xDis, int yDis)
{

  int j, xs, ys, xf, yf, m;
  int cx = vec->x, cy = vec->y;
  int sad;
  int sadMin = 0x7fffffff;

  ys = vec->y - param.searchRange;
  xs = vec->x - param.searchRange;
  yf = vec->y + param.searchRange + yDis + 1;
  xf = vec->x + param.searchRange + xDis + 1;

  if (ys < 0) ys = 0;
  m = ys;
  if (xs < 0) xs = 0;

  if (yf > ips[1].height)
    yf = ips[1].height - yDis + 1;
  else
    yf = vec->y + param.searchRange + 1;

  if (xf > ips[1].width)
    xf = ips[1].width  - xDis + 1;
  else
    xf = vec->x + param.searchRange + 1;

#if defined(DEBUG)
  if (param.isDebugMode)
    fprintf (param.DebugFp, "Search range ... (%2d - %2d, %2d - %2d) [%2d, %2d] ::: %2d - %2d\r\n", xs, ys, xf - 1, yf - 1, vec->x, vec->y, xDis, yDis);
#endif

#if defined(OMP_ON_ABS)
#pragma omp parallel for private(j)
#endif

  for (ys = m; ys < yf; ys++)
  {

    for (j = xs; j < xf; j++)
    {

      if ((sad = calcSADinTwoCUs (ips, base, ips[0].picture, j, ys, xDis, yDis)) < sadMin)
      {

	sadMin = sad;
	vec->x = j;
	vec->y = ys;

      } else if (sad == sadMin) {

	if ((j == cx) && (ys == cy))
	{

	  vec->x = j;
	  vec->y = ys;

	}

      }

    }

  }

  vec->x -= cx;
  vec->y -= cy;

}

static void DiamondSearch (InPicStatus *ips, YUVcolor *base, Position *vec, int xDis, int yDis)
{

  int j, h, xs, ys, xf, yf;
  int cx = vec->x, cy = vec->y;
  int bx = cx, by = cy;
  int px, py;
  Position searchVector[DIAMOND_SEARCH_RANGE] = {{0, -1}, {-1, 0}, {1, 0}, {0, 1}};
  Position doneList[200];
  int point = 0;
  int sad;
  int sadMin = 0x7fffffff;
  boolean isEnd = FALSE;

  ys = vec->y - param.searchRange;
  xs = vec->x - param.searchRange;
  yf = vec->y + param.searchRange + yDis + 1;
  xf = vec->x + param.searchRange + xDis + 1;

  if (ys < 0) ys = 0;
  if (xs < 0) xs = 0;

  if (yf > ips[1].height)
    yf = ips[1].height - yDis + 1;
  else
    yf = vec->y + param.searchRange + 1;

  if (xf > ips[1].width)
    xf = ips[1].width - xDis + 1;
  else
    xf = vec->x + param.searchRange + 1;

#if defined(DEBUG)
  if (param.isDebugMode)
    fprintf (param.DebugFp, "Search range ... (%2d - %2d, %2d - %2d) [%2d, %2d] ::: %2d - %2d\r\n", xs, ys, xf - 1, yf - 1, vec->x, vec->y, xDis, yDis);
#endif

  if ((sad = calcSADinTwoCUs (ips, base, ips[0].picture, vec->x, vec->y, xDis, yDis)) < sadMin)
  {

    sadMin = sad;
    doneList[point].x = vec->x;
    doneList[point].y = vec->y;
    point++;

  }

  while (1)
  {

#if defined(OMP_ON_DIA)
#pragma omp parallel for
#endif
    for (j = 0; j < DIAMOND_SEARCH_RANGE; j++)
    {

      px = cx + searchVector[j].x;
      py = cy + searchVector[j].y;

      if ((xs <= px) && (px < xf) && (ys <= py) && (py < yf))
      {

	for (h = 0; h < point; h++)
	  if ((doneList[h].x == px) && (doneList[h].y == py))
	    break;

	if (h == point)
	{

	  if ((sad = calcSADinTwoCUs (ips, base, ips[0].picture, px, py, xDis, yDis)) < sadMin)
	  {

	    sadMin = sad;
	    vec->x = px;
	    vec->y = py;

	  }

	  doneList[point].x = px;
	  doneList[point].y = py;
	  point++;

	}

      } else {

	isEnd = TRUE;

      }

    }

    if (((vec->x == cx) && (vec->y == cy)) || (isEnd == TRUE))
      break;
    else
    {

      cx = vec->x;
      cy = vec->y;

    }

  }

  vec->x -= bx;
  vec->y -= by;

}

static void HexagonalSearch (InPicStatus *ips, YUVcolor *base, Position *vec, int xDis, int yDis)
{

  int j, h, xs, ys, xf, yf;
  int cx = vec->x, cy = vec->y;
  int bx = cx, by = cy;
  int px, py;
  Position searchHexVector[HEXAGON_SEARCH_RANGE] = {{-1, -2}, {-1, 2}, {-2, 0}, {2, 0}, {-1, 2}, {1, 2}};
  Position doneList[200];
  int point = 0;
  int sad;
  int sadMin = 0x7fffffff;
  boolean isEnd = FALSE;

  ys = vec->y - param.searchRange;
  xs = vec->x - param.searchRange;
  yf = vec->y + param.searchRange + yDis + 1;
  xf = vec->x + param.searchRange + xDis + 1;

  if (ys < 0) ys = 0;
  if (xs < 0) xs = 0;

  if (yf > ips[1].height)
    yf = ips[1].height - yDis + 1;
  else
    yf -= yDis;

  if (xf > ips[1].width)
    xf = ips[1].width - xDis + 1;
  else
    xf -= xDis;

#if defined(DEBUG)
  if (param.isDebugMode)
    fprintf (param.DebugFp, "Search range ... (%2d - %2d, %2d - %2d) [%2d, %2d] ::: %2d - %2d\r\n", xs, ys, xf - 1, yf - 1, vec->x, vec->y, xDis, yDis);
#endif

  if ((sad = calcSADinTwoCUs (ips, base, ips[0].picture, vec->x, vec->y, xDis, yDis)) < sadMin)
  {

    sadMin = sad;
    doneList[point].x = vec->x;
    doneList[point].y = vec->y;
    point++;

  }

  while (1)
  {

    for (j = 0; j < HEXAGON_SEARCH_RANGE; j++)
    {

      px = cx + searchHexVector[j].x;
      py = cy + searchHexVector[j].y;

      if ((xs <= px) && (px < xf) && (ys <= py) && (py < yf))
      {

	for (h = 0; h < point; h++)
	  if ((doneList[h].x == px) && (doneList[h].y == py))
	    break;

	if (h == point)
	{

	  if ((sad = calcSADinTwoCUs (ips, base, ips[0].picture,px, py, xDis, yDis)) < sadMin)
	  {

	    sadMin = sad;
	    vec->x = px;
	    vec->y = py;

	  }

	  doneList[point].x = px;
	  doneList[point].y = py;
	  point ++;

	}

      } else {

	isEnd = TRUE;

      }

    }

    if (((vec->x == cx) && (vec->y == cy)) || (isEnd == TRUE))
      break;
    else
    {

      cx = vec->x;
      cy = vec->y;

    }

  }

  vec->x -= bx;
  vec->y -= by;

}

static int calcSADinTwoCUs (InPicStatus *ips, YUVcolor *base, YUVcolor *src, int sx, int sy, int xDis, int yDis)
{

  int sad = 0;
  int i = sy + yDis, j = sx + xDis, k, h, n, m = sy;

#if defined(DEBUG)
  if (param.isDebugMode)
    fprintf (param.DebugFp, "SAD (%4d, %4d)", sx, sy);
#endif

  h = 0;
  for (sy = m; sy < i; sy++)
  {

    n = sy * ips[0].width;

    for (k = sx; k < j; k++)
    {

      sad += abs (((int) src[n + k].y) - ((int) base[h].y));
      sad += abs (((int) src[n + k].u) - ((int) base[h].u));
      sad += abs (((int) src[n + k].v) - ((int) base[h].v));

#if defined(DEBUG)
      TotalRefPicNum++;
#endif
      h++;

    }

  }

#if defined(DEBUG)
  if (param.isDebugMode)
    fprintf (param.DebugFp, " = %d\r\n", sad);
#endif

  return sad;

}

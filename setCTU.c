#include "motionEstimation.h"

static void            CalcCTUAsMatched (InPicStatus *ips, CTU *next, int posX, int posY, MBS size);
static double                       RMS (InPicStatus *ips, int xSize, int ySize, int cX, int cY);
static boolean           IsAdjustedType (double can1, double can2, double border, double thre);
static void             PushMatchedType (boolean *typeList, double *RMSs, MBS size, int _2Nx2N_);
static CUPattern SelectMatchedCUPattern (boolean *typeList, double *RMSs);

#define MAX_ARG_2(arg1, arg2) ((arg1) < (arg2) ? (arg2) : (arg1))

int CuSizeDefine[25][2] = {
  {64, 64},
  {64, 32},
  {32, 64},
  {32, 32},
  {32, 16},
  {16, 32},
  {16, 16},
  {16,  8},
  { 8, 16},
  { 8,  8},
  { 8,  4},
  { 4,  8},
  { 4,  4},
  {64, 16},
  {64, 48},
  {16, 64},
  {48, 64},
  {32,  8},
  {32, 24},
  { 8, 32},
  {24, 32},
  {16,  4},
  {16, 12},
  { 4, 16},
  {12, 16}
};

/* This matrix must be listed in the order under below.
 * NxN -> Nx2N -> 2NxN -> 2Nx2N
 */
static MBS mbsOrder[57] = {
  FOUR,    FOUR,    FOUR,    FOUR,
  FOUR_EIGHT,           FOUR_EIGHT,
  EIGHT_FOUR,           EIGHT_FOUR,
  EIGHT,   EIGHT,   EIGHT,   EIGHT,

  EIGHT_SIXTEEN,        EIGHT_SIXTEEN,
  SIXTEEN_EIGHT,        SIXTEEN_EIGHT,
  SIXTEEN_FOUR,         SIXTEEN_TWELVE,
  SIXTEEN_TWELVE,       SIXTEEN_FOUR,
  FOUR_SIXTEEN,         TWELVE_SIXTEEN,
  TWELVE_SIXTEEN,       FOUR_SIXTEEN,
  SIXTEEN, SIXTEEN, SIXTEEN, SIXTEEN,

  SIXTEEN_THIRTYTWO,    SIXTEEN_THIRTYTWO,
  THIRTYTWO_SIXTEEN,    THIRTYTWO_SIXTEEN,
  THIRTYTWO_EIGHT,      THIRTYTWO_TWENTYFOUR,
  THIRTYTWO_TWENTYFOUR, THIRTYTWO_EIGHT,
  EIGHT_THIRTYTWO,      TWENTYFOUR_THIRTYTWO,
  TWENTYFOUR_THIRTYTWO, EIGHT_THIRTYTWO,
  THIRTYTWO, THIRTYTWO, THIRTYTWO, THIRTYTWO,

  THIRTYTWO_SIXTYFOUR,  THIRTYTWO_SIXTYFOUR,
  SIXTYFOUR_THIRTYTWO,  SIXTYFOUR_THIRTYTWO,
  SIXTYFOUR_SIXTEEN,    SIXTYFOUR_FORTYEIGHT,
  SIXTYFOUR_FORTYEIGHT, SIXTYFOUR_SIXTEEN,
  SIXTEEN_SIXTYFOUR,    FORTYEIGHT_SIXTYFOUR,
  FORTYEIGHT_SIXTYFOUR, SIXTEEN_SIXTYFOUR,
  SIXTYFOUR
};

/* This matrix must be listed in the order under below.
 * NxN -> Nx2N -> 2NxN -> 2Nx2N
 */
static int mbsPos[17][2] = {
  { 0,  0},
  {32,  0},
  { 0, 32},
  {32, 32},
  { 0,  0},
  {32,  0},
  { 0,  0},
  { 0, 32},
  { 0,  0},
  { 0, 16},
  { 0,  0},
  { 0, 48},
  { 0,  0},
  {16,  0},
  { 0,  0},
  {48,  0},
  { 0,  0}
};

void SetCTU (InPicStatus *ips, FILE *outFile)
{

  int i, j, h, k, xSlice, ySlice;
  int l, m, p, s;
  int size = sizeof (YUVcolor) * param.CUSize * param.CUSize;

  xSlice = ips->width  / param.CUSize;
  ySlice = ips->height / param.CUSize;

  ips->picStartP = ftell (ips->inputFile);

  if ((ips->ctu = (CTU *) malloc (sizeof (CTU) * xSlice * ySlice)) == NULL)
  {

    fprintf (stderr, "[ME]erro : failed to get ctuTop on the memory.\n");
    return;

  }

  h = 0;
  for (i = 0; i < ySlice; i++)
  {

    for (j = 0; j < xSlice; j++)
    {

      for (k = 0; k < CHILD_NUM; k++)
	ips->ctu[h].child[k] = NULL;

      ips->ctu[h].picture = (YUVcolor *) malloc (size);
      for (l = 0; l < param.CUSize; l++)
      {

	for (m = 0; m < param.CUSize; m++)
	{

	  p = l * param.CUSize + m;
	  s = (i * param.CUSize + l) * ips->width + j * param.CUSize + m;
	  ips->ctu[h].picture[p].y = ips->picture[s].y;
	  ips->ctu[h].picture[p].u = ips->picture[s].u;
	  ips->ctu[h].picture[p].v = ips->picture[s].v;

	}

      }

      CalcCTUAsMatched (ips, &ips->ctu[h], j * param.CUSize, i * param.CUSize, param.CUtype);
      h++;

    }

  }

  freeAllPicmem (ips);

  return;

}

static void CalcCTUAsMatched (InPicStatus *ips, CTU *next, int posX, int posY, MBS size)
{

  double RMSs[17];
  boolean typeList[8] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
  int start, shift, pixel, pixels[2];
  int typeOfAMP = 0;
  int h, i, j, k, l, m, lim;
  CUPattern pat;

  if (size == SIXTYFOUR)
    start = 40, shift = 0;
  else if (size == THIRTYTWO)
    start = 24, shift = 1;
  else if (size == SIXTEEN)
    start =  8, shift = 2;
  else if (size == EIGHT)
    start =  0, shift = 3;

  if (size == param.minCUSize)
  {

    pat = _2Nx2N;

  } else {

    if (size == EIGHT)
      lim = 9;
    else
      lim = 17;

    for (i = 0; i < lim; i++)
    {

      RMSs[i] = RMS (ips, CuSizeDefine[mbsOrder[i + start]][0], CuSizeDefine[mbsOrder[i + start]][1],
		     posX + (mbsPos[i][0] >> shift), posY + (mbsPos[i][1] >> shift));

    }

    PushMatchedType (typeList, RMSs, size, lim - 1);
    pat = SelectMatchedCUPattern (typeList, RMSs);

  }

  switch (pat)
  {

  case _NxN:
    next->split_cu_flag = TRUE;
    next->size = size;

    for (i = 0; i < CHILD_NUM; i++)
    {

      next->child[i] = (CTU *) malloc (sizeof (CTU));
      for (j = 0; j < CHILD_NUM; j++)
	next->child[i]->child[j] = NULL;
      pixel = CuSizeDefine[mbsOrder[start]][0] * CuSizeDefine[mbsOrder[start]][1];
      next->child[i]->picture = (YUVcolor *) malloc (sizeof (YUVcolor) * pixel);

      k = 0, l = mbsPos[i][0] >> shift, m = mbsPos[i][1] >> shift;
      for (j = 0; j < CuSizeDefine[mbsOrder[start]][1]; j++)
      {

	for (h = 0; h < CuSizeDefine[mbsOrder[start]][0]; h++)
	{

	  next->child[i]->picture[k].y = next->picture[(j + m) * CuSizeDefine[size][0] + h + l].y;
	  next->child[i]->picture[k].u = next->picture[(j + m) * CuSizeDefine[size][0] + h + l].u;
	  next->child[i]->picture[k].v = next->picture[(j + m) * CuSizeDefine[size][0] + h + l].v;
	  k++;

	}

      }

      CalcCTUAsMatched (ips, next->child[i], posX + l, posY + m, (MBS) ((int) size + 3));

    }

    return;

  case _Nx2N:
    next->split_cu_flag = TRUE;
    next->size = size;

    for (i = 0; i < 2; i++)
    {

      next->child[i] = (CTU *) malloc (sizeof (CTU));
      next->child[i]->split_cu_flag = FALSE;
      next->child[i]->size          = mbsOrder[start + 4]; // Or [start + 5]
      pixel = CuSizeDefine[next->child[i]->size][0] * CuSizeDefine[next->child[i]->size][1];
      next->child[i]->picture       = (YUVcolor *) malloc (sizeof (YUVcolor) * pixel);

      k = 0, l = i * (CuSizeDefine[next->child[i]->size][0]);
      for (j = 0; j < CuSizeDefine[next->child[i]->size][1]; j++)
      {

	for (h = 0; h < CuSizeDefine[next->child[i]->size][0]; h++)
	{

	  next->child[i]->picture[k].y = next->picture[j * CuSizeDefine[size][0] + h + l].y;
	  next->child[i]->picture[k].u = next->picture[j * CuSizeDefine[size][0] + h + l].u;
	  next->child[i]->picture[k].v = next->picture[j * CuSizeDefine[size][0] + h + l].v;
	  k++;

	}

      }

    }

    break;

  case _2NxN:
    next->split_cu_flag = TRUE;
    next->size = size;

    for (i = 0; i < 2; i++)
    {

      next->child[i] = (CTU *) malloc (sizeof (CTU));
      next->child[i]->split_cu_flag = FALSE;
      next->child[i]->size          = mbsOrder[start + 6]; // Or [start + 7]
      pixel = CuSizeDefine[next->child[i]->size][0] * CuSizeDefine[next->child[i]->size][1];
      next->child[i]->picture       = (YUVcolor *) malloc (sizeof (YUVcolor) * pixel);
      memcpy (next->child[i]->picture, &next->picture[i * pixel], sizeof (YUVcolor) * pixel);

    }

    break;

  case _2NxnU:
    typeOfAMP += 1;
  case _2NxnD:
    typeOfAMP += 2;
    next->split_cu_flag = TRUE;
    next->size = size;

    if (typeOfAMP == 3)
      typeOfAMP = 8;
    else
      typeOfAMP = 10;

    for (i = 0; i < 2; i++)
    {

      next->child[i]                = (CTU *) malloc (sizeof (CTU));
      next->child[i]->split_cu_flag = FALSE;
      next->child[i]->size          = mbsOrder[start + typeOfAMP + i];
      pixels[i] = CuSizeDefine[next->child[i]->size][0] * CuSizeDefine[next->child[i]->size][1];
      next->child[i]->picture       = (YUVcolor *) malloc (sizeof (YUVcolor) * pixels[i]);

    }

    memcpy (next->child[0]->picture, &next->picture[0], sizeof (YUVcolor) * pixels[0]);
    memcpy (next->child[1]->picture, &next->picture[pixels[0]], sizeof (YUVcolor) * pixels[1]);

    break;

  case _nLx2N:
    typeOfAMP += 1;
  case _nRx2N:
    typeOfAMP += 2;
    next->split_cu_flag = TRUE;
    next->size = size;

    if (typeOfAMP == 3)
      typeOfAMP = 12;
    else
      typeOfAMP = 14;

    for (i = 0; i < 2; i++)
    {

      next->child[i]                = (CTU *) malloc (sizeof (CTU));
      next->child[i]->split_cu_flag = FALSE;
      next->child[i]->size          = mbsOrder[start + typeOfAMP + i];
      pixel = CuSizeDefine[next->child[i]->size][0] * CuSizeDefine[next->child[i]->size][1];
      next->child[i]->picture       = (YUVcolor *) malloc (sizeof (YUVcolor) * pixel);

      k = 0;
      l = i ? i * CuSizeDefine[next->child[0]->size][0] : 0;
      for (j = 0; j < CuSizeDefine[next->child[i]->size][1]; j++)
      {

	for (h = 0; h < CuSizeDefine[next->child[i]->size][0]; h++)
	{

	  next->child[i]->picture[k].y = next->picture[j * CuSizeDefine[size][0] + h + l].y;
	  next->child[i]->picture[k].u = next->picture[j * CuSizeDefine[size][0] + h + l].u;
	  next->child[i]->picture[k].v = next->picture[j * CuSizeDefine[size][0] + h + l].v;
	  k++;

	}

      }

    }

    break;

  case _2Nx2N:
    next->split_cu_flag = FALSE;
    next->size = size;

    break;

  default:
    break;

  }

  for (i = 0; i < CHILD_NUM; i++)
  {

    if (next->child[i] != NULL)
    {

      for (j = 0; j < CHILD_NUM; j++)
	next->child[i]->child[j] = NULL;

    } else {

      break;

    }

  }

}

static CUPattern SelectMatchedCUPattern (boolean *typeList, double *RMSs)
{

  double maxLeft, maxRight, maxPat1, maxPat2;
  CUPattern pat1 = _NONE, pat2 = _NONE;

  if (typeList[_2Nx2N])
    return _2Nx2N;
  else if (typeList[_NxN])
    return _NxN;
  else
  {

    if ((typeList[_2NxnU]) && (typeList[_2NxnD]))
    {

      maxLeft  = MAX_ARG_2 (RMSs[8], RMSs[9]);
      maxRight = MAX_ARG_2 (RMSs[10], RMSs[11]);

      if (maxLeft < maxRight)
      {
	pat1 = _2NxnU;
	maxPat1 = maxLeft;
      } else {
	pat1 = _2NxnD;
	maxPat1 = maxRight;
      }

    } else {

      if (typeList[_2NxnU])
      {
	pat1 = _2NxnU;
	maxPat1 = MAX_ARG_2 (RMSs[8], RMSs[9]);
      } else if (typeList[_2NxnD]) {
	pat1 = _2NxnD;
	maxPat1 = MAX_ARG_2 (RMSs[10], RMSs[11]);
      }

    }

    if ((typeList[_nLx2N]) && (typeList[_nRx2N]))
    {

      maxLeft  = MAX_ARG_2 (RMSs[12], RMSs[13]);
      maxRight = MAX_ARG_2 (RMSs[14], RMSs[15]);

      if (maxLeft < maxRight)
      {
	pat2 = _nLx2N;
	maxPat2 = maxLeft;
      } else {
	pat2 = _nRx2N;
	maxPat2 = maxRight;
      }

    } else {

      if (typeList[_nLx2N])
      {
	pat2 = _nLx2N;
	maxPat2 = MAX_ARG_2 (RMSs[12], RMSs[13]);
      } else if (typeList[_nRx2N]) {
	pat2 = _nRx2N;
	maxPat2 = MAX_ARG_2 (RMSs[14], RMSs[15]);
      }

    }

    if ((pat1 != _NONE) && (pat2 != _NONE))
    {

      if (maxPat1 == MAX_ARG_2 (maxPat1, maxPat2))
      {

	pat1 = pat2;
	maxPat1 = maxPat2;

      }

    } else {

      if (pat2 != _NONE)
      {

	pat1 = pat2;
	maxPat1 = maxPat2;

      }

    }	

    if ((typeList[_Nx2N]) && (typeList[_2NxN]))
    {

      // Group Left
      // N_2N
      maxLeft  = MAX_ARG_2 (RMSs[4], RMSs[5]);

      // Group Right
      // _2NxN
      maxRight = MAX_ARG_2 (RMSs[6], RMSs[7]);

      if (maxLeft < maxRight)
      {
	pat2 = _Nx2N;
	maxPat2 = maxLeft;
      } else {
	pat2 = _2NxN;
	maxPat2 = maxRight;
      }

    } else {

      if (typeList[_Nx2N])
      {
	pat2 = _Nx2N;
	maxPat2 = maxLeft;
      } else if (typeList[_2NxN]) {
	pat2 = _2NxN;
	maxPat2 = maxRight;
      } else {

	pat2 = _NONE;

      }

    }

    if ((pat1 == _NONE) && (pat2 == _NONE))
    {

      fprintf (stderr, "[ME]erro : error coused to select best CTU size.\n");
      return _NONE;

    } else {

      if (pat1 == _NONE)
      {

	return pat2;

      } else if (pat2 == _NONE) {

	return pat1;

      } else {

	if (maxPat1 == MAX_ARG_2 (maxPat1, maxPat2))
	{

	  return pat2;

	} else {

	  return pat1;

	}

      }

    }

  }

}

static void PushMatchedType (boolean *typeList, double *RMSs, MBS size,  int _2Nx2N_)
{

  boolean isAlreadMatched = FALSE;
  int AddAdjustedTypeNum = 0;

  if (((RMSs[_2Nx2N_] <= RMSs[0]) &&
       (RMSs[_2Nx2N_] <= RMSs[1]) &&
       (RMSs[_2Nx2N_] <= RMSs[2]) &&
       (RMSs[_2Nx2N_] <= RMSs[3])) ||
      ((RMSs[0] <= param.threshold) &&
       (RMSs[1] <= param.threshold) &&
       (RMSs[2] <= param.threshold) &&
       (RMSs[3] <= param.threshold)))
  {

    typeList[_2Nx2N] = TRUE;
    return;

  } else {

    if (IsAdjustedType (RMSs[0], RMSs[2], RMSs[4], param.threshold))
    {

      AddAdjustedTypeNum += 1;

      if (IsAdjustedType (RMSs[1], RMSs[3], RMSs[5], param.threshold))
      {

	isAlreadMatched = TRUE;
	typeList[_Nx2N] = TRUE;

      }

    }

    if (IsAdjustedType (RMSs[0], RMSs[1], RMSs[6], param.threshold))
    {

      AddAdjustedTypeNum += 2;

      if (IsAdjustedType (RMSs[2], RMSs[3], RMSs[7], param.threshold))
      {

	isAlreadMatched = TRUE;
	typeList[_2NxN] = TRUE;

      }

    }

    if ((size != EIGHT) && (param.isAMP == TRUE))
    {

      if (IsAdjustedType (RMSs[8], RMSs[9], RMSs[_2Nx2N_], param.threshold))
      {

	isAlreadMatched = TRUE;
	typeList[_2NxnU] = TRUE;

      }

      if (IsAdjustedType (RMSs[10], RMSs[11], RMSs[_2Nx2N_], param.threshold))
      {

	isAlreadMatched = TRUE;
	typeList[_2NxnD] = TRUE;

      }

      if (IsAdjustedType (RMSs[12], RMSs[13], RMSs[_2Nx2N_], param.threshold))
      {

	isAlreadMatched = TRUE;
	typeList[_nLx2N] = TRUE;

      }

      if (IsAdjustedType (RMSs[14], RMSs[15], RMSs[_2Nx2N_], param.threshold))
      {

	isAlreadMatched = TRUE;
	typeList[_nRx2N] = TRUE;

      }

    }

  }

  if (isAlreadMatched == FALSE)
  {

    if ((size == EIGHT) && (param.encodeMode == HEVC))
    {

      switch (AddAdjustedTypeNum)
      {

      case 0:

	if (IsAdjustedType (RMSs[1], RMSs[3], RMSs[5], param.threshold))
	  typeList[_Nx2N] = TRUE;
	else if (IsAdjustedType (RMSs[2], RMSs[3], RMSs[7], param.threshold))
	  typeList[_2NxN] = TRUE;
	else
	  typeList[_2Nx2N] = TRUE;

	break;
      case 1:
	typeList[_Nx2N] = TRUE;
	break;
      case 2:
	typeList[_2NxN] = TRUE;
	break;
      case 3:
	typeList[_Nx2N] = TRUE;
	typeList[_2NxN] = TRUE;
	break;
      default:
	break;

      }

    } else {

      typeList[_NxN] = TRUE;

    }

  }

  return;

}

static double RMS (InPicStatus *ips, int xSize, int ySize, int cX, int cY)
{

  int i, j;
  double MSE = 0.0;
  int sady, sadu, sadv, pos;
  YUVcolor base;

  pos = cY * ips->width + cX;
  base.y = ips->picture[pos].y;
  base.u = ips->picture[pos].u;
  base.v = ips->picture[pos].v;

  for (i = 0; i < ySize; i++)
  {

    for (j = 0; j < xSize; j++)
    {

      pos = (i + cY) * ips->width + (j + cX);
      sady = ips->picture[pos].y - base.y;
      sadu = ips->picture[pos].u - base.u;
      sadv = ips->picture[pos].v - base.v;
      MSE += (double) (sady * sady);
      MSE += (double) (sadu * sadu);
      MSE += (double) (sadv * sadv);

    }

  }

  return sqrt (MSE / (xSize * ySize));

}

static boolean IsAdjustedType (double can1, double can2, double border, double thre)
{

  if (((border <= can1) && (border <= can2)) ||
      ((can1   <= thre) && (can2   <= thre)) ||
      ((can1 + can2) / 2 > border))
  {

    return TRUE;

  } else {

    return FALSE;

  }

}

void FreeCtuMem (InPicStatus *ips)
{

  int i, h, n = ips->numberOfCuX * ips->numberOfCuY;

  for (i = 0; i < n; i++)
  {

    for (h = 0; h < CHILD_NUM; h++)
      if (ips->ctu[i].child[h] == NULL)
	break;
      else
	free (ips->ctu[i].child[h]);

  }

  free (ips->ctu);

}

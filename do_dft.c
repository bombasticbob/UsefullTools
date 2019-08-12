//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                     _                _   __  _                           //
//                  __| |  ___       __| | / _|| |_     ___                 //
//                 / _` | / _ \     / _` || |_ | __|   / __|                //
//                | (_| || (_) |   | (_| ||  _|| |_  _| (__                 //
//                 \__,_| \___/_____\__,_||_|   \__|(_)\___|                //
//                            |_____|                                       //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//          Copyright (c) 2013 by S.F.T. Inc. - All rights reserved         //
//  Use, copying, and distribution of this software may be licensed accor-  //
//    ding to either a GPLv2, GPLv3, MIT, or BSD license, as appropriate.   //
//                                                                          //
//     OR - if you prefer - just use/distribute it without ANY license.     //
//   But I'd like some credit for it. A favorable mention is appreciated.   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <math.h>

// build command:  gcc -o do_dft do_dft.c -lm

// approximate value of pi beyond precision of double
#define _PI_ 3.1415926535897932384626433832795028841971693993751



typedef struct _XY_
{
  double dX;
  double dY;
} XY;

typedef struct _MY_XY_
{
  XY *pData;
  int nItems; // # of items
  int nSize;  // memory block size
} MY_XY;

/////////////////////////////////////////////////////////////////////////////
// FUNCTION: dFourier
//
// on entry 'aVal' is array of XY, 'nVal' is # of entries in aVal,
// nH is # harmonic (sin,cos) coefficients to generate [excluding '0']
// and 'dA' and 'dB' are the sin and cos arrays, and 'dC' is the 'C0' value.
//
// note:  list must be sorted by X value, no duplicate X values
//
/////////////////////////////////////////////////////////////////////////////
void dFourier(XY * aVal, int nVal, int nH, double *dC, double *dA, double *dB)
{
int i1, i2;
double dX, dY, dX0, dXY;


  *dC = 0.0;

  // assume sorted list, calculate X0, XY for x = -PI to PI

  dXY = 2.0 * _PI_ / (aVal[nVal - 1].dX + (aVal[nVal - 1].dX - aVal[0].dX) / (nVal - 1));
  dX0 = -dXY * aVal[0].dX - _PI_; // derived from -_PI_ == dX0 + dXY * aVal[0].dX

  for(i1 = 0; i1 <= nH; i1++)
  {
    if(i1 < nH)
    {
      dA[i1] = dB[i1] = 0.0;
    }

    for(i2 = 0; i2 < nVal; i2++)
    {
      if(!i1)
      {
        *dC += aVal[i2].dY;

        // printf("temporary:  item %d X=%g\n", i2, (double)(dX0 + aVal[i2].dX * dXY));
      }
      else
      {
        double dXNew = i1 * (dX0 + aVal[i2].dX * dXY);

        dA[i1 - 1] += aVal[i2].dY * cos(dXNew);
        dB[i1 - 1] += aVal[i2].dY * sin(dXNew);
      }
    }
  }

  // fix up arrays and whatnot

  *dC /= nVal; //C0 must be half A[0] i.e Y = A[0] / 2 +[sum n = 1 - ?] An *cos(n * X) + Bn * sin(n * X)
  // see http : //en.wikipedia.org / wiki / Fourier_series

  for(i1 = 0; i1 < nH; i1++)
  {
    dA[i1] *= 2.0 / nVal;
    dB[i1] *= 2.0 / nVal;
  }
}


// FUNCTION:xy_comp - sort compare for 'XY' structure

int xy_comp(const void *p1, const void *p2)
{
int iRval = ((XY *) p1)->dX - ((XY *) p2)->dX;

  if(iRval > 0)
  {
    return 1;
  }

  if(iRval < 0)
  {
    return -1;
  }

  return 0;
}

// FUNCTION:get_xy_data - file input of X and Y values(space delimiter)

MY_XY get_xy_data(FILE * pIn)
{
char tbuf[512];
double dX, dY;
MY_XY xyNULL = {NULL, 0, 0}, xy = {NULL, 0, 0};


  while(fgets(tbuf, sizeof(tbuf), pIn))
  {
    if(!xy.pData ||
       xy.nItems * sizeof(xy.pData[0]) >= xy.nSize)
    {
      if(xy.nItems > 1024)
      {
        xy.nSize = (xy.nItems * 2) * sizeof(xy.pData[0]);
      }
      else
      {
        xy.nSize = 2048 * sizeof(xy.pData[0]);
      }

      if(xy.pData)
      {
        void *p1 = realloc(xy.pData, xy.nSize + 1);

        if(!p1)
        {
          free(xy.pData);
          return xyNULL;
        }

        xy.pData = (XY *) p1;
      }
      else
      {
        xy.pData = (XY *) malloc(xy.nSize);

        if(!xy.pData)
        {
          return xyNULL;
        }
      }
    }

    dX = dY = 0.0;

    sscanf(tbuf, "%lg %lg\n", &dX, &dY);

    // printf("TEMPORARY:  data point %d %g %g   %s\n", xy.nItems, dX, dY, tbuf);

    xy.pData[xy.nItems].dX = dX;
    xy.pData[xy.nItems].dY = dY;
    xy.nItems++;
  }

  // sort data by X

  qsort(xy.pData, xy.nItems, sizeof(xy.pData[0]), xy_comp);

  return xy;
}


void usage(void)
{
  fprintf(stderr,
          "USAGE:  do_dft2 -h\n"
          "        do_dft2 [input_file [input_file [...]]]\n"
          "where   'input_file' is the name of a file containing rows of X and Y values\n"
          "                     delimited by white-space and terminated with LF\n"
          " and    '-h' instructs do_dft to print this information\n"
          "        (if no file or '-h' specified, input is 'stdin')\n");
}


////////////////
//MAIN
///////////////

int main(int argc, char *argv[])
{
double dC, dA[4096], dB[4096], dErr;
int i1, i2;
FILE *pIn = stdin;
int nHarm;


  while(argc > 1)
  {
    const char *p1 = argv[1];

    if(*p1 != '-')
    {
      break;
    }

    p1++;

    if(!*p1)
    {
      usage();
      return -1;
    }
    while(*p1)
    {
      if(*p1 == 'h')
      {
        usage();
        if(p1[1] || argc > 2)
        {
          return -1;
        }
        else
        {
          return 0;
        }
      }
     //TODO:other options, like cycle count maybe ?
      else
      {
        usage();
        return -2; // unknown option
      }
    }

    argc--; // in anticipation of other options, this is the loop counter for it
    argv++;
  }

  while(argc > 1 || pIn == stdin)
  {
MY_XY xy;

    if(argc > 1)
    {
      pIn = fopen(argv[1], "r");

      if(!pIn)
      {
        argv++;
        argc--;

        continue;
      }

      printf("FILE:  %s\n", argv[1]);
      argv++;
      argc--;
    }

    xy = get_xy_data(pIn);

    fclose(pIn);
    pIn = NULL;

    if(!xy.pData || !xy.nItems)
    {
      continue;
    }

    nHarm = xy.nItems / 2 > sizeof(dA) / sizeof(dA[0])
          ? sizeof(dA) / sizeof(dA[0]) : xy.nItems / 2;

    if(xy.nItems < 2 || nHarm < 1)
    {
      continue;
    }

    dFourier(xy.pData, xy.nItems, nHarm, &dC, dA, dB);

    printf("harm #\t      magnitude\t    phase (deg)\t  offset (C0)=%g\n", dC);

    for(i1 = 0; i1 < nHarm; i1++)
    {
      printf("  %3d\t"
             // "%7g\t%7g\t"
             "%15.6f\t%15.6f\n",
             i1 + 1,
             //dA[i1],
             //dB[i1],
             sqrt(dA[i1] * dA[i1] + dB[i1] * dB[i1]),
             atan2(dB[i1], dA[i1]) * 180 / _PI_ + 180);
      // NOTE: atan result for cosine will be - 180, sin - 90
      //       because the analysis is - PI to PI
      //       adding 180 will give you 0, 90
    }

    //figure out relative error
    for(i1 = 0, dErr = 0.0; i1 < xy.nItems; i1++)
    {
      double dCheck = dC;
      double dXY = 2.0 * _PI_ / (xy.pData[xy.nItems - 1].dX + (xy.pData[xy.nItems - 1].dX - xy.pData[0].dX) / (xy.nItems - 1));
      double dX0 = -dXY * xy.pData[0].dX - _PI_; // derived from -_PI_ == dX0 + dXY * aVal[0].dX

      for(i2 = 0; i2 < nHarm; i2++)
      {
        dCheck += dA[i2] * cos((i2 + 1) * (xy.pData[i1].dX * dXY + dX0))
           + dB[i2] * sin((i2 + 1) * (xy.pData[i1].dX * dXY + dX0));
      }

      // printf("  data point %d\t%g\t%g\t%g\n", i1, xy.pData[i1].dX, xy.pData[i1].dY, dCheck);
      dErr += (dCheck - xy.pData[i1].dY) * (dCheck - xy.pData[i1].dY);
    }

    printf("formula error:  %g\n", sqrt(dErr / xy.nItems));

    if(xy.pData)
    {
      free(xy.pData);
    }
  }

  return 0;
}



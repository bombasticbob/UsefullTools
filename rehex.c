//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                              _                                           //
//                   _ __  ___ | |__    ___ __  __    ___                   //
//                  | '__|/ _ \| '_ \  / _ \\ \/ /   / __|                  //
//                  | |  |  __/| | | ||  __/ >  <  _| (__                   //
//                  |_|   \___||_| |_| \___|/_/\_\(_)\___|                  //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//          Copyright (c) 2021 by S.F.T. Inc. - All rights reserved         //
//       Use, copying, and distribution of this software are licensed       //
//     according to the GPLv2, LGPLv2, or BSD license, as appropriate.      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

// This utility is incomplete and supplied as-is.  Use it at your own risk.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

int hex_digit(char c1)
{
  if(c1 >= '0' && c1 <='9')
    return c1 - '0';
  else if(c1 >= 'a' && c1 <= 'f')
    return c1 - 'a' + 10;
  else if(c1 >= 'A' && c1 <= 'F')
    return c1 - 'A' + 10;

  return -1;
}

int hex_val(const char *pC)
{
  int i1=hex_digit(pC[0]);
  int i2=hex_digit(pC[1]);

  if(i1 < 0 || i2 < 0)
    return -1;
  else
    return i1 * 16 + i2;
}

int hex_val4(const char *pC)
{
  int i1=hex_val(pC);
  int i2=hex_val(pC + 2);

  if(i1 < 0 || i2 < 0)
    return -1;
  else
    return i1 * 256 + i2;
}

struct ihdr
{
  char colon;
  char length[2];
  char addr[4];
  char type[2]; // data is
} __attribute__((packed));


int main(int argc, char *argv[])
{
char tbuf[1024];
char outbuf[4096 * 4096 + 4];
int iRval, iC, i1, iLine, iSum, iPC, cbLen;
FILE *pIn, *pOut;
char *p1, *p2;
uint8_t *pC, *pE;
struct ihdr *pHdr;


  pOut = stdout;
  pIn = stdin;

  iRval = -1;

  if(argc >= 2)
  {
    pIn = fopen(argv[1], "r");
    if(pIn && argc > 3)
    {
      pOut = fopen(argv[2], "w");
    }
  }
  if(!pIn || !pOut)
  {
    fprintf(stderr, "Error opening in/out files, errno=%d\n", errno);
    goto the_end;
  }

  pC = (uint8_t *)outbuf;
  pE = pC + sizeof(outbuf);

  memset(pC, 0xff, sizeof(outbuf));
  memset(tbuf, 0, sizeof(tbuf));

  iLine = 0;
  iPC = -1;
  cbLen = 0;


  while(!feof(pIn) && fgets(tbuf, sizeof(tbuf), pIn))
  {
    int iLen, iAddr, iType;

    iLine++;

    // trim line
    p1 = (char *)tbuf + strlen(tbuf) - 1;
    while(p1 >= (char *)tbuf && *p1 <= ' ')
      *(p1--) = 0;

    // record type
    pHdr = (struct ihdr *)tbuf;
    iLen = hex_val(pHdr->length);
    iAddr = hex_val4(pHdr->addr);
    iType = hex_val(pHdr->type);


    if(pHdr->colon != ':')
    {
      fprintf(stderr, "bad header:  %-9.9s\n", tbuf);
      goto the_end;
    }

    if(iLen < 0 || iAddr < 0 || iType < 0)
    {
      fprintf(stderr, "line %d Bad header:  len=%d, addr=%d, type=%d\n", iLine, iLen, iAddr, iType);
      fprintf(stderr, "%-9.9s\n", tbuf);
      goto the_end;
    }

    if(iLen == 0)
    {
      if(iType == 1) // EOF
      {
        fprintf(stderr, "EOF found line %d\n", iLine);
        break;
      }
      else
      {
        fprintf(stderr, "line %d Bad header (unsupported type):  len=%d, addr=%d, type=%d\n", iLine, iLen, iAddr, iType);
        fprintf(stderr, "%-9.9s\n", tbuf);
        goto the_end;
      }
    }
    else if(iLen == 2)
    {
      if(iType == 2)
      {
        fprintf(stderr, "Extended segment addr found line %d (ignored)\n", iLine);
        goto do_next_line;
      }
      else if(iType == 4)
      {
        fprintf(stderr, "Extended linear addr found line %d (ignored)\n", iLine);
        goto do_next_line;
      }
      else if(iType == 3 || iType == 5)
      {
        fprintf(stderr, "line %d Bad header (unsupported type+length):  len=%d, addr=%d, type=%d\n", iLine, iLen, iAddr, iType);
        fprintf(stderr, "%-9.9s\n", tbuf);
        goto the_end;
      }
    }
    else if(iLen == 4)
    {
      if(iType == 3)
      {
        fprintf(stderr, "Start segment addr found line %d (ignored)\n", iLine);
        goto do_next_line;
      }
      else if(iType == 5)
      {
        fprintf(stderr, "Start linear addr found line %d (ignored)\n", iLine);
        goto do_next_line;
      }
      else if(iType == 2 || iType == 4)
      {
        fprintf(stderr, "line %d Bad header (unsupported type+length):  len=%d, addr=%d, type=%d\n", iLine, iLen, iAddr, iType);
        fprintf(stderr, "%-9.9s\n", tbuf);
        goto the_end;
      }
    }
    if(iType != 0) // error, bad type
    {
      fprintf(stderr, "line %d Bad header (unsupported type):  len=%d, addr=%d, type=%d\n", iLine, iLen, iAddr, iType);
      fprintf(stderr, "%-9.9s\n", tbuf);
      goto the_end;
    }

    if(iPC < 0)
      iPC = iAddr; // the start address, for reference later

    // convert iLen bytes and put them into the output buffer
    p1 = (char *)tbuf + sizeof(struct ihdr);
    p2 = p1 + strlen(tbuf);

    // test checksum
    iSum = iLen + (uint8_t)(iAddr >> 8) + (uint8_t)(iAddr & 0xff);

    for(i1=0; p1 < p2 && i1 < iLen && pC < pE; i1++, p1 += 2)
    {
      uint8_t b1 = hex_val(p1);

      *(pC++) = b1;
      iSum += b1;
    }

    // verify the hecksum
    iSum = (uint8_t)(-iSum);

    if(hex_val(p1) != iSum)
    {
      fprintf(stderr, "line %d checksum mismatch \"%s\":  %d (%xH)   %d (%xH)",
              iLine, p1, hex_val(p1), hex_val(p1), iSum, iSum);
    }

do_next_line:
    memset(tbuf, 0, sizeof(tbuf));
  }

  pE = pC; // the new end
  pC = (uint8_t *)outbuf;

  // now write it out all normalized

  while(pC < pE)
  {
    struct ihdr hdr;
    int iLen, iType;

    if((pE - pC) >= 32)
    {
      iLen = 32;
    }
    else
    {
      iLen = pE - pC;
    }

    iSum = (uint8_t)iLen + (uint8_t)(iPC >> 8) + (uint8_t)(iPC & 0xff);

    fprintf(pOut, ":%02X%04X00", (uint8_t)iLen, (uint16_t)iPC);

    for(i1=0; i1 < iLen; i1++, pC++)
    {
      iSum += *pC;
      fprintf(pOut, "%02X", *pC);
    }

    iPC += iLen; // update it

    iSum = (uint8_t)(-iSum);
    fprintf(pOut, "%02X\n", iSum);
  }

  fputs(":00000001FF\n", pOut);

  iRval = 0;

the_end:
  if(pOut && pOut != stdout)
    fclose(pOut);

  if(pIn && pIn != stdin)
    fclose(pIn);

  return iRval;
}

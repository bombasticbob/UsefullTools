#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


#define MAX_COLS 64
#define MAX_LINE 4096

#define DO_SUB_ARGS(X)  (X)++; if(!*(X)) { argv++; argc--; if(argc <= 1) { usage(); return 2; } (X) = argv[1]; if(*(X) == '-') { usage(); return 3; } }


int iCount = 0; // global record count

void usage(void)
{
   fprintf(stderr, "usage:  agg -o [columns] -s [columns] -m [columns] -M [columns]\n\n"
                   "  where   -o indicates the 'order' columns\n"
                   "          -s indicates the SUM columns\n"
                   "          -a indicates the AVG columns\n"
                   "          -c indicated the COUNT columns\n"
                   "          -m indicates the MIN columns\n"
                   "          -M indicates the MAX columns\n"
                   "          -x indicates the MIN STRING columns\n"
                   "          -X indicates the MAX STRING columns\n"
                   " and the column number is 1-based\n");

}

void do_columns(const char *p1, long *iArray, long iVal)
{
const char *p2;
char tbuf[256];
long iIncr = 0;


    if(!iVal)
    {
        iVal = 1;
        iIncr = 1;
    }


    while(*p1)
    {
        while(*p1 && *p1 <= ' ')
            p1++;

        p2 = p1;
        while(*p1 > ' ')
            p1++;

        if(p1 > p2)
        {
            memcpy(tbuf, p2, p1 - p2);
            tbuf[p1 - p2] = 0;
            if(tbuf[0] == '-')
            {
                iArray[atoi(tbuf + 1) - 1] = -iVal;
            }
            else
            {
                iArray[atoi(tbuf) - 1] = iVal;
            }

            if(iIncr)
                iVal++;
        }
    }
}

void split_columns(char *pLine, char **aCols)
{
char *p1;
long iCol = 0;

    p1 = pLine;
    while(*p1 && *p1 <= ' ')
        p1++;

    while(*p1)
    {
        aCols[iCol++] = p1;

        while(*p1 > ' ')
            p1++;

        if(*p1)
            *(p1++) = 0;

        while(*p1 && *p1 <= ' ')
            p1++;
    }

    while(iCol < MAX_COLS)
    {
      aCols[iCol++] = NULL;  // mark 'end of line', fill array
    }
}

void do_summary(char **pThisCols, char **pLastCols, double *pdTotals, const char **ppTotals,
                const long *iOrder, const long *iSummary)
{
long iCol;
long iBreak = 0;
int iVal;
double fVal;



    // check for control break first

    if(!pLastCols || !pThisCols)
    {
        iBreak = 1;
    }
    else
    {
        for(iCol=0; !iBreak && (pThisCols[iCol] || pLastCols[iCol]); iCol++)
        {
            if(iOrder[iCol] && !iSummary[iCol])
            {
                if(!pThisCols[iCol] || !pLastCols[iCol])  // not a match
                {
                    iBreak = 1;
                }
                else if(strcmp(pThisCols[iCol], pLastCols[iCol]))
                {
                    iBreak = 1;
                }
            }
        }        
    }

    if(!iBreak)
    {
        iCount++;

        for(iCol=0; pThisCols[iCol]; iCol++)
        {
            if(iSummary[iCol] == 3 ||
               iSummary[iCol] == -3)
            {
                if(!pThisCols[iCol])
                {
                    if(!ppTotals[iCol])
                        iVal = 0;
                    else
                        iVal = 1;
                }
                else if(!ppTotals[iCol])
                    iVal = -1;
                else
                    iVal = strcmp(pThisCols[iCol], ppTotals[iCol]);
            }
            else if(iSummary[iCol])
            {
                fVal = atof(pThisCols[iCol]);
            }

            if(iSummary[iCol] == 1)  // "sum"
            {
                pdTotals[iCol] += fVal;
            }
            else if(iSummary[iCol] == 2) // "max"
            {
                if(pdTotals[iCol] < fVal)
                  pdTotals[iCol] = fVal;
            }
            else if(iSummary[iCol] == -2) // "min"
            {
                if(pdTotals[iCol] > fVal)
                  pdTotals[iCol] = fVal;
            }
            else if(iSummary[iCol] == 3) // "max string"
            {
                if(iVal > 0)
                {
                    char *p1 = malloc(strlen(pThisCols[iCol]) + 1);
                    if(p1)
                    {
                        strcpy(p1, pThisCols[iCol]);
                    }

                    if(ppTotals[iCol])
                    {
                        free((void *)ppTotals[iCol]);
                    }

                    ppTotals[iCol] = p1;
                }
            }
            else if(iSummary[iCol] == -3) // "min string"
            {
                if(iVal < 0)
                {
                    char *p1 = pThisCols[iCol] ? malloc(strlen(pThisCols[iCol]) + 1) : NULL;
                    if(p1)
                    {
                        strcpy(p1, pThisCols[iCol]);
                    }

                    if(ppTotals[iCol])
                    {
                        free((void *)ppTotals[iCol]);
                    }

                    ppTotals[iCol] = p1;
                }
            }
            else if(iSummary[iCol] == 4) // "average"
            {
                pdTotals[iCol] += fVal;
            }
            else if(iSummary[iCol] == 5) // "count"
            {
                // count is automatic, so only deal with it in the output phase
            }
        }
        return;
    }

    // here's where we do OUTPUT

    if(pLastCols)
    {
        for(iCol=0; pLastCols[iCol] || (pThisCols && pThisCols[iCol]); iCol++)
        {
            if(iCol)
                fputs("\t", stdout);

            if(!iSummary[iCol])
            {
                if(pLastCols[iCol])
                    fputs(pLastCols[iCol], stdout);
            }
            else if(iSummary[iCol] == 3 || iSummary[iCol] == -3)
            {
                if(ppTotals[iCol])
                {
                    fputs((char *)ppTotals[iCol], stdout);

                    free((void *)ppTotals[iCol]);  // memory cleanup done here
                    ppTotals[iCol] = NULL;
                }
            }
            else if(iSummary[iCol] == 4)
            {
                if(iCount)
                {
                    fprintf(stdout, "%g", pdTotals[iCol] / iCount);
                }
                else
                {
                    fputs("~", stdout); // just in case
                }
            }
            else if(iSummary[iCol] == 5)
            {
                fprintf(stdout, "%d", iCount);
            }
            else
            {
                fprintf(stdout, "%g", pdTotals[iCol]);
            }
        }

        fputs("\n", stdout);
    }

    // zero out the 'totals' array
    for(iCol=0; (pLastCols && pLastCols[iCol]) || (pThisCols && pThisCols[iCol]); iCol++)
    {
        if(pThisCols && iSummary[iCol] &&
           iSummary[iCol] != 3 && iSummary[iCol] != -3)
        {
          fVal = atof(pThisCols[iCol]);
        }

        if(pThisCols && iSummary[iCol] == 1 ||  // "sum"
           pThisCols && iSummary[iCol] == 2 ||  // "max"
           pThisCols && iSummary[iCol] == -2 || // "min"
           pThisCols && iSummary[iCol] == 4)    // "average"
        {
            pdTotals[iCol] = fVal;
        }
        else if(pThisCols && iSummary[iCol] == 3) // "max string"
        {
            char *p1 = malloc(strlen(pThisCols[iCol]) + 1);
            if(p1)
            {
                strcpy(p1, pThisCols[iCol]);
            }

            if(ppTotals[iCol])
            {
                free((void *)ppTotals[iCol]);
            }

            ppTotals[iCol] = p1;
        }
        else if(pThisCols && iSummary[iCol] == -3) // "min string"
        {
            char *p1 = malloc(strlen(pThisCols[iCol]) + 1);

            if(p1)
            {
                strcpy(p1, pThisCols[iCol]);
            }

            if(ppTotals[iCol])
            {
                free((void *)ppTotals[iCol]);
            }

            ppTotals[iCol] = p1;
        }
        else // including 'count'
        {
            pdTotals[iCol] = 0.0;
            ppTotals[iCol] = NULL;
        }
    }

    iCount = 1; // always do this for a 'first record'
}


int main(int argc, char *argv[])
{
long iOrder[MAX_COLS];
long iSummary[MAX_COLS];
char *pCols1[MAX_COLS], *pCols2[MAX_COLS];
char lines1[MAX_LINE], lines2[MAX_LINE];
char **pLastCols, **pThisCols;
char *pLastLine, *pThisLine;
double dTotals[MAX_COLS];
const char *pTotals[MAX_COLS];
FILE *pIn = NULL;
int i1;


    memset(iOrder, 0, sizeof(iOrder));
    memset(iSummary, 0, sizeof(iSummary));
    memset(dTotals, 0, sizeof(dTotals));
    memset(pTotals, 0, sizeof(pTotals));

    while(argc > 1)
    {
        if(argv[1][0] == '-')
        {
            const char *p1 = argv[1] + 1;

            if(*p1 == 'o')
            {
                DO_SUB_ARGS(p1);
                do_columns(p1, iOrder, 0);
            }
            else if(*p1 == 's')
            {
                DO_SUB_ARGS(p1);
                do_columns(p1, iSummary, 1);
            }
            else if(*p1 == 'm')
            {
                DO_SUB_ARGS(p1);
                do_columns(p1, iSummary, -2);
            }
            else if(*p1 == 'M')
            {
                DO_SUB_ARGS(p1);
                do_columns(p1, iSummary, 2);
            }
            else if(*p1 == 'x')
            {
                DO_SUB_ARGS(p1);
                do_columns(p1, iSummary, -3);
            }
            else if(*p1 == 'X')
            {
                DO_SUB_ARGS(p1);
                do_columns(p1, iSummary, 3);
            }
            else if(*p1 == 'a')
            {
                DO_SUB_ARGS(p1);
                do_columns(p1, iSummary, 4);
            }
            else if(*p1 == 'c')
            {
                DO_SUB_ARGS(p1);
                do_columns(p1, iSummary, 5);
            }
            else
            {
                usage();
                return 1;
            }
        }
        argc--;
        argv++;
    }

    if(argc > 1)
        pIn = fopen(argv[1], "r");
    else
        pIn = stdin;

    if(!pIn)
        return 1;

    // TODO:  sort first?

    pLastLine = NULL;
    pThisLine = lines2;
    pLastCols = NULL;
    pThisCols = pCols2;

    fputs("TEMPORARY:  order columns:", stderr);

    for(i1=0; i1 < MAX_COLS; i1++)
    {
      if(iOrder[i1])
      {
        fprintf(stderr, " %d", i1 + 1);
      }
    }
    
    fputs("\nTEMPORARY:  summary columns:", stderr);

    for(i1=0; i1 < MAX_COLS; i1++)
    {
      if(iSummary[i1])
      {
        const char *pTemp;
        switch(iSummary[i1])
        {
          case 1:
            pTemp = "sum";
            break;
          case 2:
            pTemp = "max";
            break;
          case -2:
            pTemp = "min";
            break;
          case 3:
            pTemp = "max$";
            break;
          case -3:
            pTemp = "min$";
            break;
          case 4:
            pTemp = "average";
            break;
          case 5:
            pTemp = "count";
            break;
        }

        fprintf(stderr, " %s(%d)", pTemp, i1 + 1);
      }
    }

    fputs("\n=====================================================================\n\n", stderr);

    // split each line into columns, then
    // use sort columns as 'key' and sum final column

    while(!feof(pIn))
    {
        memset(pThisLine, 0, MAX_LINE);
        if(fgets(pThisLine, MAX_LINE - 1, pIn))
        {
            split_columns(pThisLine, pThisCols);

            do_summary(pThisCols, pLastCols, dTotals, pTotals, iOrder, iSummary);

            if(!pLastLine || pLastLine == lines1)
            {
                pLastLine = lines2;
                pThisLine = lines1;
                pLastCols = pCols2;
                pThisCols = pCols1;
            }
            else
            {
                pLastLine = lines1;
                pThisLine = lines2;
                pLastCols = pCols1;
                pThisCols = pCols2;
            }
        }
    }

    // final summary
    do_summary(NULL, pLastCols, dTotals, pTotals, iOrder, iSummary);

    if(pIn != stdin)
        fclose(pIn);

    return 0;
}


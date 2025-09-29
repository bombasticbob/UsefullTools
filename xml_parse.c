//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                          _                                               //
//        __  __ _ __ ___  | | _ __    __ _  _ __  ___   ___     ___        //
//        \ \/ /| '_ ` _ \ | || '_ \  / _` || '__|/ __| / _ \   / __|       //
//         >  < | | | | | || || |_) || (_| || |   \__ \|  __/ _| (__        //
//        /_/\_\|_| |_| |_||_|| .__/  \__,_||_|   |___/ \___|(_)\___|       //
//                            |_|                                           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                     Copyright (c) 2025 by S.F.T. Inc.                    //
//  Use, copying, and distribution of this software are licensed according  //
//           to the GPLv2, LGPLv2, or BSD license, as appropriate           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#include <io.h>
#include <sys/types.h>
#else // WIN32
#include <unistd.h>
#endif // WIN32
#include <string.h>
#include <memory.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>


#ifdef WIN32
#define __inline__ __inline
#define PATH_MAX _MAX_PATH
#define PRINTF_1_2
typedef _off_t off_t;
#define strncasecmp strnicmp
#else // WIN32
#define PRINTF_1_2 __attribute__ ((format(printf, 1, 2)))
#endif // WIN32


#define WBAlloc(X) malloc(X)
#define WBFree(X) free(X)
#define WBReAlloc(X,Y) realloc(X,Y)

#define CHPARSEXML_DEFAULT 0  /**< Default behavior for 'CHFindNextXMLTag()' (just look for '<') */
#define CHPARSEXML_PAREN   1  /**< For 'CHFindNextXMLTag()', stop parsing on detection of '(' or ')' */
#define CHPARSEXML_BRACKET 2  /**< For 'CHFindNextXMLTag()', stop parsing on detection of '[' or ']' */


#define WB_IF_DEBUG_LEVEL(L) if(WBCheckDebugLevel((L)))

#define WB_DEBUG_PRINT(L, ...) \
    WB_IF_DEBUG_LEVEL(L) { WBDebugPrint(__VA_ARGS__); }
#define WB_DEBUG_DUMP(L,X,Y,Z) \
    WB_IF_DEBUG_LEVEL(L) { WBDebugDump(X,Y,Z); }

#define WB_WARN_PRINT(...) WB_DEBUG_PRINT(DebugLevel_WARN, __VA_ARGS__)
#define WB_ERROR_PRINT(...) WB_DEBUG_PRINT(DebugLevel_ERROR, __VA_ARGS__)
#define WB_WARN_DUMP(X,Y,Z) WB_DEBUG_DUMP(DebugLevel_WARN, X,Y,Z)
#define WB_ERROR_DUMP(X,Y,Z) WB_DEBUG_DUMP(DebugLevel_ERROR, X,Y,Z)


typedef int WB_INT32;
typedef unsigned int WB_UINT32;
typedef long long WB_INT64;
typedef unsigned long long WB_UINT64;

#if !defined(__SIZEOF_POINTER__) // TODO find a better way to deal with pointer size if this isn't defined
#define __SIZEOF_POINTER__ 0
#endif
#if defined(__LP64__) /* TODO see what WIN32 vs WIN64 does */
typedef WB_UINT64 WB_UINTPTR;
#elif __SIZEOF_POINTER__ == 4 /* 4-byte pointer */
typedef WB_UINT32 WB_UINTPTR;
#else // assume long pointer
typedef WB_UINT64 WB_UINTPTR;
#endif // __LP64__

#ifdef WIN32
#define WB_UNLIKELY(X) (X)
#define WB_LIKELY(X) (X)
#else
// requiress compiler support of '___builtin_expect()'
#define WB_UNLIKELY(x) (__builtin_expect (!!(x), 0))
#define WB_LIKELY(x) (__builtin_expect (!!(x), 1))
#endif // WIN32


enum DebugLevel
{
  DebugLevel_None = 0,      //!< none (no debug output)
  DebugLevel_ERROR = 0,     //!< errors (output whenever debug cmopiled in)
  DebugLevel_WARN = 1,      //!< warnings (all debug levels)

                            // criteria for selecting debug output
  DebugLevel_Minimal = 1,   //!< minimal, implies warnings and important information
  DebugLevel_Light = 2,     //!< light, implies basic/summary process/flow information
  DebugLevel_Medium = 3,    //!< medium, implies process/flow tracing
  DebugLevel_Heavy = 4,     //!< heavy, implies detailed process/flow tracing
  DebugLevel_Chatty = 5,    //!< chatty, implies details about flow decisions
  DebugLevel_Verbose = 6,   //!< verbose, implies details regarding information used for decision making
  DebugLevel_Excessive = 7, //!< excessive, implies more information that you probably want
  DebugLevel_MAXIMUM = 7,   //!< maximum value for masked level
  DebugLevel_MASK = 7,      //!< mask for allowed 'level' values (none through Excessive)

  DebugSubSystem_ALL         = 0,           //!< 'ALL' is the default unless masked bits are non-zero
  DebugSubSystem_RESTRICT    = 0x80000000,  //!< only show specific subsystems (prevents zero masked value)
  DebugSubSystem_BITSHIFT    = 3,           //!< bit # for 'lowest' subsystem bit
  DebugSubSystem_MAX         = 0x40000000,  //!< reserved

  DebugSubSystem_MASK = ~7L  //!< mask for allowed 'subsystem' bits
};

typedef struct tagCHXMLEntry
{
  int iNextIndex;      ///< 0-based index for next item at this level; <= 0 for none.  0 marks "end of 'chain'"
  int iContainer;      ///< 0-based index for container; < 0 for none.  [ZERO is valid for this element]
  int iContentsIndex;  ///< 0-based first array index for 'contents' for this entry; <= 0 for none

  int nLabelOffset;    ///< BYTE offset to label (zero-byte-terminated) string (from beginning of array) for this entry; < 0 for 'no label'
  int nDataOffset;     ///< BYTE offset to data (zero-byte-terminated) string (from beginning of array) for the entry data; < 0 for 'no data'

} CHXMLEntry;


// utilities borrowed from X11Workbench Project
int WBCheckDebugLevel(WB_UINT64 dwLevel);
void WBDebugPrint(const char *pFmt, ...) PRINTF_1_2;

void WBDelay(uint32_t uiDelay);  // approximate delay for specified period (in microseconds).  may be interruptible
char *WBCopyString(const char *pSrc);
char *WBCopyStringN(const char *pSrc, unsigned int nMaxChars);
void WBCatString(char **ppDest, const char *pSrc);  // concatenate onto WBAlloc'd string
void WBCatStringN(char **ppDest, const char *pSrc, unsigned int nMaxChars);
CHXMLEntry *CHParseXML(const char *pXMLData, int cbLength);
void CHDebugDumpXML(CHXMLEntry *pEntry); // dumps XML using debug I/O functions
char *CHParseXMLTagContents(const char *pTag, int cbLength);
const char *CHFindNextXMLTag(const char *pTagContents, int cbLength, int nNestingFlags);
const char *CHFindEndOfXMLTag(const char *pTagContents, int cbLength);
const char *CHFindEndingXMLTag(const char **ppTag, int cbLength, const char **ppOpenTagEnd);
  // returns *ppTag is start of ending tag, *ppOpenTagEnd is end of open tag, returbn is pointer past ending tag
const char *CHFindEndOfXMLSection(const char *pTagContents, int cbLength, char cEndChar, int bUseQuotes);
void WBNormalizeXMLString(char *pString); // remove quotes and translate &quot; &lt; etc. to regular ASCII
size_t WBReadFileIntoBuffer(const char *szFileName, char **ppBuf);

void  DoPrintXMLContents(int argc, const char *argv[], CHXMLEntry *pXML);


WB_UINT64 iWBDebugLevel=0;

static __inline__ WB_UINT64 WBGetDebugLevel(void)
{
  return iWBDebugLevel;
}

void usage()
{
  fputs("Usage: [-h][-i file] 'xml.value.thingy' ['xml.value.thingy'[...]]\n",
        stderr);
}

int main(int argc, char *argv[])
{
int i1, i2;//, i3;
size_t cbBuf;
char *pBuf = NULL, fname[PATH_MAX * 2] = "";
CHXMLEntry *pXML;
int bTest = 0;
#ifdef WIN32
int optind;
char *optarg;
#endif // WIN32


  if(argc < 1)
  {
    usage();
    return -1;
  }

#ifdef WIN32
  i2=0;
  for(optind=1; optind < argc;)
  {
    if(!argv[optind][i2])
    {
      optind++;
      i2 = 0;
      continue;
    }

    if(!i2)
    {
      if(argv[optind][0] != '-')
        break;

      if(argv[optind][1] == '-') // double-dash
      {
        optind++;
        break;
      }

      i2++;
    }

    i1 = argv[optind][i2++];
    if(!strchr("vht", i1)) // things with no arguments
    {
      if(i1 == 'i') // required args
      {
        if(argv[optind][i2])
        {
          optarg = &argv[optind][i2];
          i2 = strlen(optarg);
        }
        else
        {
          optind++;
          if(optind < argc)
          {
            optarg = argv[optind++];
            i2 = 0;
          }
          else
            i1 = 0;
        }
      }
//      else if(i1 == 'X') // optional arg
//      {
//        if(argv[optind][i2])
//        {
//          optarg = &argv[optind][i2];
//          i2 = strlen(optarg);
//        }
//        else
//        {
//          optind++;
//          if(optind < argc)
//          {
//            optarg = argv[optind++];
//            i2 = 0;
//          }
//          else
//            optarg = NULL;
//        }
//      }
      else
      {
        i1 = 0;
      }
    }
#else // WIN32
  while((i1 = getopt(argc, argv, "hvti:")) >= 0)
  {
#endif // WIN32
    switch(i1)
    {
      case 'i':
        strncpy(fname, optarg, sizeof(fname) - 1);
        break;

      case 'v':
        iWBDebugLevel++;
        break;

      case 'h':
        usage();
        return -1;

      case 't':
        bTest = 1;
        break;

      default:
        WB_ERROR_PRINT("Illegal option: '%c'\n", i1);
        usage();
        return -2;
    }
  }

  argc -= optind;
  argv += optind;

  if(bTest)
  {
    static const char szTest[]=
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
      "<TheXML thingy='yessir' value=4>\n"
      "  TestValue\n"
      "  <test>boo</test>\n"
      "  <thing doohickey/>\n"
      "</TheXML>\n";


    pXML = CHParseXML(szTest, sizeof(szTest) - 1);
  }
  else
  {
    if(!fname[0]) // stdin
    {
      cbBuf = WBReadFileIntoBuffer(NULL, &pBuf);
    }
    else
    {
      cbBuf = WBReadFileIntoBuffer(fname, &pBuf);
    }

    if(!pBuf || cbBuf < 0)
    {
      WB_ERROR_PRINT("Error %d (%xH) reading input file '%s'\n",
                     errno, errno, fname);

      usage();

      return 1;
    }
    else if(!cbBuf)
    {
      WB_WARN_PRINT("Warning - input file is empty\n");
      return 1;
    }

    fwrite(pBuf, cbBuf, 1, stderr);

    pXML = CHParseXML(pBuf, cbBuf);
    free(pBuf);
  }

  if(!pXML || (pXML->iNextIndex <= 0 && pXML->iContentsIndex <= 0))
  {
    if(pXML)
    {
      WB_WARN_PRINT("XML content is empty - '%s' %d %d\n", fname, pXML->iNextIndex, pXML->iContentsIndex);
      free(pXML);
    }
    else
    {
      WB_ERROR_PRINT("Error parsing XML - '%s'\n", fname);
    }

    return 2;
  }

  DoPrintXMLContents(argc, (const char **)argv, pXML);

  free(pXML);

  return 0;
}



void WBDelay(uint32_t uiDelay)  // approximate delay for specified period (in microseconds).  may be interruptible
{
#ifdef WIN32
  if(uiDelay <= 1000)
    Sleep(1);
  else
    Sleep(uiDelay / 1000);
#else // WIN32
//#ifdef HAVE_NANOSLEEP
struct timespec tsp;

  if(WB_UNLIKELY(uiDelay >= 1000000L))
  {
    tsp.tv_sec = uiDelay / 1000000L;
    uiDelay = uiDelay % 1000000L; // number of microseconds converted to nanoseconds
  }
  else
  {
    tsp.tv_sec = 0; // it's assumed that this method is slightly faster
  }

  tsp.tv_sec = 0;
  tsp.tv_nsec = uiDelay * 1000;  // wait for .1 msec

  nanosleep(&tsp, NULL);
//#else  // HAVE_NANOSLEEP
//
//  usleep(uiDelay);  // 100 microsecs - a POSIX alternative to 'nanosleep'
//
//#endif // HAVE_NANOSLEEP
#endif // WIN32
}


char *WBCopyString(const char *pSrc)
{
char *pDest;
int iLen;

  if(!pSrc || !*pSrc)
  {
    pDest = WBAlloc(2);

    if(pDest)
    {
      *pDest = 0;
    }
  }
  else
  {
    iLen = strlen(pSrc);

    pDest = WBAlloc(iLen + 1);

    if(pDest)
    {
      memcpy(pDest, pSrc, iLen);
      pDest[iLen] = 0;
    }
  }

  return pDest;
}

char *WBCopyStringN(const char *pSrc, unsigned int nMaxChars)
{
char *pDest;
int iLen;
const char *p1;

  if(!pSrc || !*pSrc)
  {
    pDest = WBAlloc(2);

    if(pDest)
    {
      *pDest = 0;
    }
  }
  else
  {
    for(p1 = pSrc, iLen = 0; iLen < (int)nMaxChars && *p1; p1++, iLen++)
    { } // determine length of 'pStr' to copy

    pDest = WBAlloc(iLen + 1);

    if(pDest)
    {
      memcpy(pDest, pSrc, iLen);
      pDest[iLen] = 0;
    }
  }

  return pDest;
}


void WBCatString(char **ppDest, const char *pSrc)  // concatenate onto WBAlloc'd string
{
int iLen, iLen2;
char *p2;

  if(!ppDest || !pSrc || !*pSrc)
  {
    return;
  }

  if(*ppDest)
  {
    iLen = strlen(*ppDest);
    iLen2 = strlen(pSrc);

    p2 = *ppDest;
    *ppDest = WBReAlloc(p2, iLen + iLen2 + 1);
    if(!*ppDest)
    {
      *ppDest = p2;
      return;  // not enough memory
    }

    p2 = iLen + *ppDest;  // re-position end of string

    memcpy(p2, pSrc, iLen2);
    p2[iLen2] = 0;  // make sure last byte is zero
  }
  else
  {
    *ppDest = WBCopyString(pSrc);
  }
}

void WBCatStringN(char **ppDest, const char *pSrc, unsigned int nMaxChars)
{
int iLen, iLen2;
char *p2;
const char *p3;


  if(!ppDest || !pSrc || !*pSrc)
  {
    return;
  }

  if(*ppDest)
  {
    iLen = strlen(*ppDest);

    for(iLen2=0, p3 = pSrc; iLen2 < (int)nMaxChars && *p3; p3++, iLen2++)
    { }  // determine what the length of pSrc is up to a zero byte or 'nMaxChars', whichever is first

    p2 = *ppDest;
    *ppDest = WBReAlloc(p2, iLen + iLen2 + 1);
    if(!*ppDest)
    {
      *ppDest = p2; // restore the old pointer value
      return;  // not enough memory
    }

    p2 = iLen + *ppDest;  // re-position end of string

    memcpy(p2, pSrc, iLen2);
    p2[iLen2] = 0;  // make sure last byte is zero
  }
  else
  {
    *ppDest = WBCopyStringN(pSrc, nMaxChars);
  }
}

#define CHAR_MODE_BUFFSIZE 1048576

size_t WBReadFileIntoBuffer(const char *szFileName, char **ppBuf)
{
off_t cbLen = (off_t)-1;
size_t cbF;
int cb1, iChunk;
char *pBuf;
int iFile, bCharMode = 0;


  // if the file cannot be "seek"d I use char mode but limit to 1Mb
  // this lets me read /proc files easily

  if(!ppBuf)
  {
    return (size_t)-1;
  }

#ifndef WIN32
  if(!szFileName || !*szFileName) // use stdin
    iFile = STDIN_FILENO; // fcntl(STDIN_FILENO,  F_DUPFD, 0);  // dup stdin handle so I can close it later
  else
#endif // WIN32
    iFile = open(szFileName, O_RDONLY); // open read only (assume no locking for now)

  if(iFile < 0)
  {
    return (size_t)-1;
  }



#ifndef WIN32
  if(!szFileName || !*szFileName) // use stdin
  {
    cbLen = (off_t)CHAR_MODE_BUFFSIZE;
    bCharMode = 1;
  }
  else
#endif // WIN32
  {
    // how long is my file?

    cbLen = (unsigned long)lseek(iFile, 0, SEEK_END); // location of end of file

    if(cbLen == (off_t)-1)
    {
      *ppBuf = NULL; // make sure

      if(errno == EINVAL) // char mode file like /proc var?
      {
        cbLen = (off_t)CHAR_MODE_BUFFSIZE;
        bCharMode = 1;
      }
    }
    else
    {
      lseek(iFile, 0, SEEK_SET); // back to beginning of file
    }
  }

  *ppBuf = pBuf = WBAlloc(cbLen + 1);

  if(!pBuf)
  {
    cbLen = (off_t)-1; // to mark 'error'
  }
  else if(cbLen >= 0)
  {
    *pBuf = 0;
    cbF = cbLen;

    if(bCharMode)
      cbLen = 0;

    while(cbF > 0)
    {
      if(bCharMode)
      {
        iChunk = 16;
      }
      else
      {
        iChunk = 1048576; // 1MByte at a time
      }

      if((size_t)iChunk > cbF)
      {
        iChunk = (int)cbF;
      }

      cb1 = read(iFile, pBuf, iChunk);

      if(cb1 == -1)
      {
        if(errno == EAGAIN) // allow this
        {
          WBDelay(100);
          continue; // for now just do this
        }

        cbLen = -1;
        break;
      }
      else if(!cb1) // EOF
      {
        break;  // done
      }

      if(cb1 != iChunk) // did not read enough bytes
      {
        iChunk = cb1; // for now
      }

      cbF -= iChunk;
      pBuf += iChunk;
      *pBuf = 0;  // I allocated an extra byte for this

      if(bCharMode)
      {
        cbLen += iChunk;  // reported length in char mode
      }
    }
  }


#ifndef WIN32
  if(iFile != STDIN_FILENO)
#endif // WIN32
    close(iFile);

  return (size_t) cbLen;
}



enum
{
  Special_NONE = 0,
  Special_Question,
  Special_Comment,
  Special_CDATA,
  Special_BANG
};

static int CHGetSpecialXMLTagType(const char *pTag, const char *pEnd)
{
  if((pEnd - pTag) > 2 && (pTag[1] == '?' || pTag[1] == '!')) // special
  {
    if(pTag[1] == '?')
      return Special_Question;  // <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
    else if((pEnd - pTag) > 3 && pTag[2] == '-')
      return Special_Comment;   // <!-- comment -->
    else if((pEnd - pTag) > 9 && !strncasecmp(pTag + 2, "[CDATA[", 7))  // be strict with it
      return Special_CDATA;     // <![CDATA[something]]>
    else
      return Special_BANG;      // <!DOCTYPE html>
  }

  return Special_NONE;
}

// add an XML entry at *ppCur, incrementing *ppCur and returning a pointer to it
// re-allocates buffers as needed to fit the entry.  returns NULL if there was an error
static CHXMLEntry *InternalAddXMLEntry(const char *szTagName, // NULL or "" for embedded data; else tag name
                                       const char *szValue, // text representation of value with white space stripped from ends
                                       CHXMLEntry *pContainer, // container pointer or NULL
                                       CHXMLEntry **ppOrigin, int *pcbOrigin, CHXMLEntry **ppCur,
                                       char **ppData, int *pcbData, char **ppCurData)
{
int cb1, cbNew;
void *pV;
//char *p1;
CHXMLEntry *pC;


  if((uint8_t *)(*ppCur + 1) >= (uint8_t *)*ppOrigin + *pcbOrigin)
  {
    // reallocate
    cbNew = *pcbOrigin + 0x1000 * sizeof(CHXMLEntry);
    pV = realloc(*ppOrigin, cbNew);
    if(pV)
    {
      *ppCur = (CHXMLEntry *)pV + (*ppCur - *ppOrigin);  // relocate *ppCur
      *ppOrigin = (CHXMLEntry *)pV; // new pointer
      *pcbOrigin = cbNew;
    }
    else
    {
      WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
      return NULL; // error?
    }
  }

  if(szTagName)
    cb1 = strlen(szTagName);
  else
    cb1 = 0;

  if(szValue)
    cb1 += strlen(szValue);

  cb1 += 2;

  if((uint8_t *)*ppCurData + cb1 >= (uint8_t *)*ppData + *pcbData)
  {
    // reallocate
    cbNew = *pcbData + 0x10000; // 64k
    pV = realloc(*ppData, cbNew);
    if(pV)
    {
      *ppCurData = (char *)pV + (*ppCurData - *ppData);  // relocate *ppCurData
      *ppData = (char *)pV; // new pointer
      *pcbData = cbNew;
    }
    else
    {
      WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
      return NULL; // error?
    }
  }

//typedef struct tagCHXMLEntry
//{
//  int iNextIndex;      ///< 0-based index for next item at this level; <= 0 for none.  0 marks "end of 'chain'"
//  int iContainer;      ///< 0-based index for container; < 0 for none.  [ZERO is valid for this element]
//  int iContentsIndex;  ///< 0-based first array index for 'contents' for this entry; <= 0 for none
//
//  int nLabelOffset;    ///< BYTE offset to label (zero-byte-terminated) string (from beginning of array) for this entry; < 0 for 'no label'
//  int nDataOffset;     ///< BYTE offset to data (zero-byte-terminated) string (from beginning of array) for the entry data; < 0 for 'no data'
//
//} CHXMLEntry;

  pC = (*ppCur)++;
  pC->iNextIndex = 0; // 'end of chain' for now
  if(pContainer)
    pC->iContainer = pContainer - *ppOrigin;
  else
    pC->iContainer = -1;
  pC->iContentsIndex = 0; // indicate no contents (yet)


  if(pContainer && pContainer->iContentsIndex <= 0)
  {
    pContainer->iContentsIndex = pC - *ppOrigin;
  }
  else if(pContainer || pC > *ppOrigin) // NOT the very first one
  {
    CHXMLEntry *pS = *ppOrigin + (pContainer ? pContainer->iContentsIndex : 0);

    // walk the list of siblings, add to the end

    while(pS->iNextIndex > 0)
      pS = *ppOrigin + pS->iNextIndex;

    pS->iNextIndex = pC - *ppOrigin;
  }

  pC->nLabelOffset = -1;
  if(szTagName)
  {
    cb1 = strlen(szTagName);
    if(cb1)
    {
      pC->nLabelOffset = *ppCurData - *ppData;
      memcpy(*ppCurData, szTagName, cb1);
      (*ppCurData) += cb1;
    }
  }
  *((*ppCurData)++) = 0;  // terminating zero byte

  pC->nDataOffset = -1;
  if(szValue)
  {
    cb1 = strlen(szValue);
    pC->nDataOffset = *ppCurData - *ppData;

    if(cb1)
    {
      memcpy(*ppCurData, szValue, cb1);
      (*ppCurData) += cb1;
    }
  }
  *((*ppCurData)++) = 0;  // terminating zero byte

  WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - add %s=%s\n", __FUNCTION__, __LINE__, szTagName, szValue);

  return pC; // pointer to this element.
}

static const char *InternalParseXML(CHXMLEntry **ppOrigin, int *pcbOrigin, CHXMLEntry **ppCur,
                                    char **ppData, int *pcbData, char **ppCurData,
                                    CHXMLEntry *pContainer, // to mark iContentsIndex (NULL for top level)
                                    const char *pXMLData, const char *pXMLDataEnd)
{
const char *pC, *pE, *p1; //, *pCE, *p2;
char *p3, *p4, *p5;
CHXMLEntry *pEntry;


  // parse a section of XML, adding contents to the end of 'ppOrigin', and returning
  // a pointer to the next element on success (or NULL otherwise).  This function will
  // re-allocate '*ppOrigin' as needed, storing the max size in '*pcbOrigin'.  It can
  // also recurse to embedded sections, and then process them as needed to get the XML
  // hierarchy correct in the CHXMLEntry pointed to by 'ppOrigin'.
  //
  // A recursive call can affect *ppOrigin.  It should be explicitly re-loaded on return.

  // this isn't needed per se, but having it here can't hurt..
  if(!ppOrigin || !*ppOrigin || !pcbOrigin || !ppCur || !*ppCur ||
     !pXMLData || !pXMLDataEnd ||
     (((WB_UINTPTR)pXMLDataEnd) < ((WB_UINTPTR)pXMLData))) // warning abatement, use type cast for WB_UINTPTR
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
    return NULL; // just reject these possibilities outright and return "error"
  }

  // this function will return on error or if it finds and parses the ending tag

  pC = pXMLData;
  pE = pXMLDataEnd;

  // for each tag do the following:
  //   find start of tag
  //   find ending tag / end of tag
  //   interpret embedded values
  //   interpret data values (recurse if needed)

  while(pC && pC < pE)
  {
    const char *pC2, *pE2, *pV, *pVE;

    pC2 = CHFindNextXMLTag(pC, pE - pC, CHPARSEXML_DEFAULT);

    if(!pC2)
    {
      WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - NULL from CHFindNextXMLTag, %.*s\n", __FUNCTION__, __LINE__, (int)(pE - pC), pC);
      return NULL;
    }

    pV = pC;

    while(pV < pC2 && *pV <= ' ')
      pV++; // skip white space

    if(pV < pC2)  // a value is there
    {
      if(!pContainer) // this is a syntax error
      {
        WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
        return NULL;
      }

      pVE = pC2;
      while(pVE > pV && *(pVE - 1) <= ' ')
        pVE--;
      // TODO - handling multiple embedded un-named values?

      if(pV < pVE) // value found
      {
        void * pTemp;

        // add this as a value to the container

        p5 = WBCopyStringN(pV, pVE - pV);
        pTemp = InternalAddXMLEntry(NULL, p5, pContainer, ppOrigin, pcbOrigin, ppCur, ppData, pcbData, ppCurData);
        WBFree(p5);

        if(!pTemp)
        {
          WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
          return NULL;
        }

        pC = pC2; // keep parsing - if pC2 >= pE this will exit
        continue;
      }
    }

    if(pC2 >= pE)
    {
      pC = pC2; // the return value
      break; // done
    }

    p1 = pC2; // remember start of tag
    pC = CHFindEndingXMLTag(&pC2, pE - pC, &pE2);
      // pC = end of XML 'section'
      // pC2 = start of ending tag
      // pE2 = end of opening tag
      // self-closing tag pC == pE2, pC2 == p1

    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - tag info  %p %p %p %p\n",
                   __FUNCTION__, __LINE__, p1 - 1, pE2, pC2, pC);
    WB_DEBUG_PRINT(DebugLevel_Medium, "  %.*s\n=====\n  %.*s\n=====\n  %.*s\n=================================\n",
                   (int)(pE2 - p1 + 1), p1 - 1,
                   (int)((pC2 > pE2) ? (pC2 - pE2) : 1), (const char *)((pC2 > pE2) ? pE2 : ""),
                   (int)((pC - pC2) ? (pC - pC2) : 1), (const char *)((pC - pC2) ? pC2 : ""));

    if(p1 < (pE - 3) && p1[1] == '!' && p1[2] == '-' && p1[3] == '-') // comment
      continue; // skip the comment

    p3 = CHParseXMLTagContents(p1, pE2 - p1);   // parse contents as name\tvalue\0
    if(!p3)
    {
      WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
      return NULL;
    }

    // for each string pair in 'p3' store the value in the output array
    // the first is always the tag name, followed by a tab and no data.
    // the last string has a 0 byte as the first char,marking the end
    // the buffer needs to be WBFree'd

    p4 = p3;  // p3 points to the tag name and also is WBAlloc'd pointer
    while(*p4 && *p4 != '\t')
      p4++;
    if(*p4)
      *(p4++) = 0;
    while(*p4)
      p4++;
    if(p4[1])
      p4++;  // pointa to the first name/value pair or '\0'

    // add entry for tag
    pEntry = InternalAddXMLEntry(p3, NULL, pContainer, ppOrigin, pcbOrigin, ppCur, ppData, pcbData, ppCurData);

    if(!pEntry)
    {
      WBFree(p3);
      WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
      return NULL;
    }

    // pEntry will be the container for embedded values in tag

    while(*p4) // parse out embedded name/value pairs
    {
      p5 = p4;  // p5 is name
      while(*p4 && *p4 != '\t')
        p4++;
      if(*p4)
        *(p4++) = 0; // p4 is value

      if(!InternalAddXMLEntry(p5, p4, pEntry, ppOrigin, pcbOrigin, ppCur, ppData, pcbData, ppCurData))
      {
        WBFree(p3);
        WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
        return NULL;
      }

      while(*p4)
        p4++; // end of value

      p4++;  // point to next name, or zero byte if done
    }

    WBFree(p3);

    // recurse - this handles raw values also

    pV = pE2;  // next spot to look for a tag
    while(pV && pV < pC2) // recursively parse all contained XML (should happen only once though)
      pV = InternalParseXML(ppOrigin, pcbOrigin, ppCur, ppData, pcbData, ppCurData, pEntry, pV, pC2);

    // pC should already point at the tag
  }

  if(!pC)
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
  }

  return pC; // where I stopped searching
}



CHXMLEntry *CHParseXML(const char *pXMLData, int cbLength)
{
CHXMLEntry *pRval = NULL;
CHXMLEntry *pXE, *pXCur;
int cbRval, cbData, cbNeed, cbOffs;
const char *pCur, *pEnd;
char *pData, *pCurData; //, *p1, *p2, *p3;


  if(!pXMLData || !cbLength || !*pXMLData)
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
    return NULL;
  }

  cbRval = 0x1000 * sizeof(CHXMLEntry); // 64k entries
  cbData = 0x100000; // 256k, to start with

  pRval = (CHXMLEntry *)malloc(cbRval);
  pData = malloc(cbData);
  if(!pRval || !pData)
  {
    if(pRval)
    {
      free(pRval);
    }

    if(pData)
    {
      free(pData);
    }

    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
    return NULL; // not enough memory (oops)
  }



  pCur = pXMLData;
  pEnd = pXMLData + cbLength;
  pXCur = pRval;
  pCurData = pData;

  *pData = 0; // ending zero byte - must be present at pData[length]

  pXCur->iNextIndex = 0;       // marks "end of list"
  pXCur->iContainer = -1;      // marks it as "top level"
  pXCur->iContentsIndex = 0;   // no hierarchical contents
  pXCur->nLabelOffset = -1;
  pXCur->nDataOffset = -1;


  while(pCur < pEnd)
  {
    const char *pTemp = pCur;

    // call recursive function that does "one level" of XML and all of its contents

    pCur = InternalParseXML(&pRval, &cbRval, &pXCur, &pData, &cbData, &pCurData, NULL, pCur, pEnd);

    if(!pCur) // error
    {
      WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - Error from InternalParseXML %.*s\n",
                     __FUNCTION__, __LINE__, (int)(pEnd - pTemp), pTemp);
      goto error_exit;
    }

    // TODO:  look at the beginning for things like <?xml version="xx"?> and <!DOCTYPE xxx>

  }

  WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - %d entries\n%s\n",
                 __FUNCTION__, __LINE__, (int)(pXCur - pRval), pCur);


  // at this point 'pXCur' is the pointer to the final entry, and there's room in the array for it.

  pXCur->iNextIndex = 0;    // marks "end of list"
  pXCur->iContainer = -1;   // assign the other default values by convention
  pXCur->iContentsIndex = 0;
  pXCur->nLabelOffset = -1;
  pXCur->nDataOffset = -1;

  cbOffs = ((char *)(pXCur + 1) - (char *)pRval); // offset to where the data is, for fixups
           // I use 'pXCur + 1' here because I'll increment it later.  but I also need it
           // for a limit pointer in the fixup loop, so I don't increment it YET...
  cbNeed = cbOffs + 2 * sizeof(*pXCur)
         + (pCurData - pData); // the actual size of the data

  // CONSOLIDATE DATA INTO 'pRval'
  if(cbNeed > cbRval) // need to re-allocate 'pRval' block
  {
    void *pTemp = realloc(pRval, cbNeed);

    if(!pTemp)
    {
      WB_ERROR_PRINT("%s.%d - Error alloc mem for return struct\n", __FUNCTION__, __LINE__);
      goto error_exit;
    }

    pXCur = (pXCur - pRval) + ((CHXMLEntry *)pTemp); // new 'pXCur'
    pRval = (CHXMLEntry *)pTemp;                     // new 'pRval'
  }

  memcpy((char *)(pXCur + 1), pData, (pCurData - pData) + 1); // copy data portion

  // fix up all of the data indices
  for(pXE=pRval; pXE < pXCur; pXE++)
  {
    if(pXE->nLabelOffset >= 0)
    {
      pXE->nLabelOffset += cbOffs; // fix up the data offset
    }
    else
    {
      pXE->nLabelOffset = -1; // make it -1 to mark it 'unused'
    }

    if(pXE->nDataOffset >= 0)
    {
      pXE->nDataOffset += cbOffs; // fix up the data offset
    }
    else
    {
      pXE->nDataOffset = -1; // make it -1 to mark it 'unused'
    }

    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - entry %d\n"
                   "  iNextIndex     %d\n"
                   "  iContainer     %d\n"
                   "  iContentsIndex %d\n"
                   "  nLabelOffset   %d %s\n"
                   "  nDataOffset    %d %s\n",
                   __FUNCTION__, __LINE__, (int)(pXE - pRval),
                   pXE->iNextIndex,
                   pXE->iContainer,
                   pXE->iContentsIndex,
                   pXE->nLabelOffset,
                   (const char *)(pXE->nLabelOffset < 0 ? "" : (char *)pRval + pXE->nLabelOffset),
                   pXE->nDataOffset,
                   (const char *)(pXE->nDataOffset < 0 ? "" : (char *)pRval + pXE->nDataOffset));
  }

  // and now I'm done!

  goto the_end;

error_exit:

  free(pRval);
  pRval = NULL;
  WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);

the_end:
  free(pData); // not needed any more

  return pRval; // the self-contained structure, or NULL on error
}


// pTag is tag start, i,e '<' pointer, length points just past the '>'
// return value is string pairs as name\tvalue\0.  either 'name' or 'value' can be an empty string
// end of data in returned value is '\0\0'
// tags like CDATA are returned as-is except '<![CDATA[' and ']]>' are stripped away
// the tag name, if any, is always the first entry in the returned string
// the return value is WBAlloc'd and needs to be WBFree'd
char *CHParseXMLTagContents(const char *pTag, int cbLength)
{
const char *pCur, *pEnd;
const char *p1, *p2, *p3;
char *pRval, *pC, *pE, *p4, *p5;
int i1, cTag, nSpecial, cbRval = 4096;


  pCur = pTag;

  if(!pCur)
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
    return NULL;
  }

  if(cbLength < 0)
    cbLength = strlen(pTag);

  pEnd = pTag + cbLength;

  nSpecial = CHGetSpecialXMLTagType(pCur, pEnd);

  if(nSpecial == Special_Comment)
  {
    pRval = malloc(4);
    if(pRval)
      memset(pRval, 0, 4);

    return pRval;
  }
  else if(nSpecial == Special_CDATA)
  {
    i1 = (pEnd - pCur - 12);

    pRval = (char *)malloc(4 + i1);
    if(pRval)
    {
      pRval[0] = '\t';
      memcpy(pRval + 1, pCur + 9, i1);
      pRval[i1 + 1] = 0;
      pRval[i1 + 2] = 0;
      pRval[i1 + 3] = 0;
    }

    return pRval;
  }

  if(nSpecial == Special_Question)
  {
    if((int)(pEnd - pCur) > 2 && *(pEnd - 1) == '>' && *(pEnd - 2) == '?')
    {
      pEnd -= 2;
    }
  }
  else
  {
    pEnd--;
    if(pEnd > pCur && *(pEnd - 1) == '/') // self-closing tag
      pEnd--;
  }

//   pEnd is now just before the tag end
  pCur++; // point to 1st char beyond the '<'

  pC = pRval = WBAlloc(cbRval);
  if(!pRval)
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
    return NULL;
  }

  pE = pRval + cbRval;
  pC[0] = pC[1] = 0;


  // NOTE:  XML spec requires that '>' and '&' be treated special, and quotes ignored for these.
  //        The strings "&amp;" "&lt;" "&gt;" must also be honored inside or outside of quotes.
  //        when I find '>' inside of quotes, I could optionally ignore it, but the spec says "NO"
  //        so the result should be some kind of XML syntax error...

  while(pCur < pEnd && *pCur)
  {
    // find value name (or embedded no-name value)
    while(pCur < pEnd && *pCur && *pCur <= ' ')
    {
      pCur++; // skip white space
    }

    if(pCur >= pEnd || !*pCur)
    {
      break; // went past the end of the buffer with just white space
    }

    p1 = pCur; // this part works for '?' and '!' if there is no white space separating them from the name
    while(pCur < pEnd && *pCur > ' ' && *pCur != '=')
    {
      pCur++; // find end of string
    }

    p2 = pCur; // p2 is end of name

    while(pCur < pEnd && *pCur && *pCur <= ' ')
    {
      pCur++; // skip white space
    }

    // NOTE:  this function does not handle '&amp;' or '&gt;' etc. outside of quotes

    if(pCur < pEnd && *pCur == '=') // value follows
    {
      pCur++;

      while(pCur < pEnd && *pCur && *pCur <= ' ')
      {
        pCur++; // skip white space
      }

      p3 = pCur;

      if(*pCur == '"' || *pCur == '\'') // quoted string
      {
        char c1 = *pCur;
        pCur++;
        while(pCur < pEnd && *pCur &&
              (*pCur != c1 || (pCur < pEnd - 1 && pCur[1] == c1)))
        {
          if(*pCur == c1) // will be < pEnd - 1
          {
            pCur += 2;
          }
          else
          {
            pCur++;
          }
        }

        if(*pCur == c1) // quote char
        {
          pCur++; // now past the quote
        }

        p5 = WBCopyStringN(p3, pCur - p3); // copy all including start/end quotes

        // make de-quoted normalized version
        if(p5)
        {
          WBNormalizeXMLString(p5); // remove quotes and sub '&gt;' '&amp;' etc.
        }
      }
      else
      {
        while(pCur < pEnd && *pCur > ' ')
        {
          pCur++; // find end of string
        }

        if(pCur == p3) // empty value
        {
          goto no_value;
        }

        p5 = WBCopyStringN(p3, pCur - p3);

        // make normalized version
        if(p5)
        {
          WBNormalizeXMLString(p5); // remove quotes and sub '&gt;' '&amp;' etc.
        }
      }

//value_is_now_p5:

      if(!p5)
      {
        WBFree(pRval);
        WB_ERROR_PRINT("%s - not enough memory\n", __FUNCTION__);
        return NULL;
      }

      // value is now p5

      if(pC + (p2 - p1) + strlen(p5) + 4 >= pE)
      {
        i1 = 4096;
        while((int)((p2 - p1) + strlen(p5) + 4) >= i1)
        {
          i1 += 4096; // to make sure it's big enough in 4k increments
        }

        cbRval += i1;
        p4 = WBReAlloc(pRval, cbRval);

        if(!p4)
        {
          WBFree(p5);
          WBFree(pRval);
          WB_ERROR_PRINT("%s - not enough memory\n", __FUNCTION__);
          return NULL;
        }

        if(p4 != pRval)
        {
          pC = p4 + (pC - pRval);
          pRval = p4;
        }

        pE = pRval + cbRval;
      }

      // now do value\tthe value\0\0 and point 'pC' to the 2nd '\0'
      memcpy(pC, p1, p2 - p1);
      pC += p2 - p1;
      *(pC++) = '\t';
      strcpy(pC, p5); // safe to use this, I checked length already
      pC += strlen(pC) + 1;
      *pC = 0;

      WBFree(p5); // done with it
    }
    else // no value
    {
no_value:
      if(pC + (p2 - p1) + 3 >= pE)
      {
        i1 = 4096;
        while((p2 - p1) + 3 >= i1)
        {
          i1 += 4096; // to make sure it's big enough in 4k increments
        }

        cbRval += i1;
        p4 = WBReAlloc(pRval, cbRval);

        if(!p4)
        {
          WBFree(pRval);
          WB_ERROR_PRINT("%s - not enough memory\n", __FUNCTION__);
          return NULL;
        }

        if(p4 != pRval)
        {
          pC = p4 + (pC - pRval);
          pRval = p4;
        }

        pE = pRval + cbRval;
      }

      // now do value\t\0\0 and point 'pC' to the 2nd '\0'
      memcpy(pC, p1, p2 - p1);
      pC += p2 - p1;
      *(pC++) = '\t';
      *(pC++) = 0;
      *pC = 0; // by convention
    }
  }


  return pRval;
}

const char *CHFindNextXMLTag(const char *pTagContents, int cbLength, int nNestingFlags)
{
const char *p1, *pEnd = pTagContents + cbLength;


  if(!pTagContents || cbLength == 0 || !*pTagContents)
  {
    return pTagContents;
  }

  if(cbLength < 0)
  {
    cbLength = strlen(pTagContents);
  }

  p1 = pTagContents;

  // outside of a tag, we don't check quote marks.  however, I do check for parens
  // when the bit flags in 'nNestingFlags' tell me to.
  // TODO:  add a 'quote mark' check to 'nNestingFlags' ?

  // NOTE:  XML spec requires that '>' and '&' be treated special, and quotes ignored for these.
  //        The strings "&amp;" "&lt;" "&gt;" must also be honored inside or outside of quotes.
  //        when I find '>' inside of quotes, I could optionally ignore it, but the spec says "NO"
  //        so the result should be some kind of XML syntax error...

  while(p1 < pEnd && *p1)
  {
    if(*p1 == '<') // next tag (includes comment tags, etc.)
    {
      break;
    }

    // TODO:  exit if I find ending tag?

    if((nNestingFlags & CHPARSEXML_PAREN) && (*p1 == '(' || *p1 == ')'))
    {
      break;
    }

    if((nNestingFlags & CHPARSEXML_BRACKET) && (*p1 == '[' || *p1 == ']'))
    {
      break;
    }

    p1++;
  }

  return p1;
}


const char *CHFindEndOfXMLSection(const char *pTagContents, int cbLength, char cEndChar, int bUseQuotes)
{
register const char *p1 = pTagContents;
const char *pE;


  if(!p1 || !cbLength)
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
    return NULL;
  }

  if(cbLength < 0)
  {
    cbLength = strlen(p1);
  }

  pE = p1 + cbLength;

  // in cases of <!CDATA ... >  and other tags that might have nesting within them, this
  // function needs to pay attention to '[' ']' '(' and ')' that are OUTSIDE of quoted strings

  // NOTE:  XML spec requires that '>' and '&' be treated special, and quotes ignored for these.
  //        The strings "&amp;" "&lt;" "&gt;" must also be honored inside or outside of quotes.
  //        when I find '>' inside of quotes, I could optionally ignore it, but the spec says "NO"
  //        so the result should be some kind of XML syntax error...

  while(p1 < pE && *p1)
  {
    // need to parse out this tag.
    if(bUseQuotes && (*p1 == '"' || *p1 == '\'')) //handle quotes
    {
      char c1 = *p1;
      p1++;
      if(p1 >= pE)
      {
        break;
      }

      while(p1 < pE && *p1 &&
            (*p1 != c1 || ((p1 + 1) < pE && p1[1] == c1)))
      {
        if(*p1 == c1) // doubled quote?
        {
          p1 += 2;
        }
        else
        {
          p1++;
        }
      }

      if(p1 >= pE)
      {
        break;
      }

      if(*p1 == c1)
      {
        p1++; // now past the quote
      }
    }
    else if(*p1 == cEndChar) // end of the tag/section
    {
      break;
    }

    // Assuming we are already "within a tag" it's possible, for some tags, to have
    // a bunch of stuff embedded within them using '( )' '[ ]' etc..  This function
    // will recurse and allow for nested things like that.

    else if(*p1 == '(') // now we look for embedded things
    {
      p1++;
      if(p1 >= pE)
      {
        break;
      }

      p1 = CHFindEndOfXMLSection(p1, pE - p1, ')', 0);

      if(!p1 || p1 >= pE || !*p1 )
      {
        break;
      }

      p1++; // point it past the ')' I just found
    }
    else if(*p1 == '[')
    {
      p1++;
      if(p1 >= pE)
      {
        break;
      }

      p1 = CHFindEndOfXMLSection(p1, pE - p1, ']', 0);

      if(!p1 || p1 >= pE || !*p1 )
      {
        break;
      }

      p1++; // point it past the ']' I just found
    }
    else if(!bUseQuotes && *p1 == '<' && (cEndChar == ']' || cEndChar == ')'))
    {
      // special case within an embedded section surrounded by '[]' or '()'
      //   IF I'm searching for an end bracket/paren, and
      //   IF I've just run across the beginning of a tag, and
      //   IF I'm currently ignoring quote marks
      //   THEN, I want to parse the XML tag with respect to quote marks until the end of the tag

      p1++;
      if(p1 >= pE)
      {
        break;
      }

      p1 = CHFindEndOfXMLSection(p1, pE - p1, '>', 1);

      if(!p1 || p1 >= pE || !*p1 )
      {
        break;
      }

      p1++; // point it past the '>' I just found (it's embedded)
    }
    else
    {
      p1++;
    }
  }

  return p1;
}

const char *CHFindEndOfXMLTag(const char *pTagContents, int cbLength)
{
  return CHFindEndOfXMLSection(pTagContents, cbLength, '>', 1);
}

// *ppTag gets updated with closing tag position; returns next position after closing tag
// if NOT NULL then *ppOpenTagEnd gets next char after opening tag
// tags starting with '<?' or '<!' have special context and are standalone
// self-closing tags have equal *ppOpenTagEnd, *ppTag, and return value
// This includes '<![CDATA[' which ends with ']]>' and '<!--' which ends with '-->'
// see https://en.wikipedia.org/wiki/CDATA on CDATA
const char *CHFindEndingXMLTag(const char **ppTag, int cbLength, const char **ppOpenTagEnd)
{
const char *p1, *p2, *p3, *p4, *pE;
char *pTagName;
int nSpecial = 0, iNest, cbTagName;
enum
{
  Special_NONE = 0,
  Special_Question,
  Special_Comment,
  Special_CDATA,
  Special_BANG
};

  if(ppOpenTagEnd)
    *ppOpenTagEnd = NULL; // pre-assign to NULL

  if(!ppTag || !*ppTag || **ppTag != '<')  // must be a valid tag
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
    return NULL;
  }

  if(cbLength < 0)
    cbLength = strlen(*ppTag);

  p1 = *ppTag;
  pE = p1 + cbLength;


  // Looking for special self-closing tags
  nSpecial = CHGetSpecialXMLTagType(p1, pE);


  if(nSpecial == Special_Question || nSpecial == Special_BANG || nSpecial == Special_CDATA)
  {
    p1 += 2;
  }
  else if(nSpecial == Special_Comment)
  {
    p1 += 3;
  }
  else // if(nSpecial == Special_NONE)
  {
    p1++;
  }

  while(p1 < pE && *p1 && *p1 <= ' ')
    p1++; // skip white space

  if(p1 >= pE)
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
    return NULL;
  }

  p2 = CHFindEndOfXMLSection(p1, pE - p1, '>', 1);

  if(!p2 || p2 <= p1 || *p2 != '>')
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL, p2=%s\n", __FUNCTION__, __LINE__, p2);
    return NULL;
  }

  if(nSpecial == Special_Question || nSpecial == Special_Comment)
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - nSpecial=%d\n", __FUNCTION__, __LINE__, nSpecial);
    while(1)
    {
      const char *p2a;

      if(!p2 || *p2 != '>')
      {
        WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
        return NULL;
      }

      p2++;  // point past the '>'

      if((nSpecial == Special_Comment && (p2 - p1) > 3 && *(p2 - 2) == '-' && *(p2 - 3) == '-')
         || (nSpecial == Special_Question && (p2 - p1) > 2 && *(p2 - 2) == '?'))
      {
        break;
      }

      p2a = p2;
      p2 = CHFindEndOfXMLSection(p2a, pE - p2a, '>', 1);
    }
  }
  else
  {
    p2++;  // point past the '>'
  }

  if(ppOpenTagEnd)
    *ppOpenTagEnd = p2;

  if(nSpecial == Special_Question || nSpecial == Special_BANG ||
     nSpecial == Special_CDATA || nSpecial == Special_Comment)
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - special tag %d\n", __FUNCTION__, __LINE__, nSpecial);
    return p2;  // these are all self-closing
  }
  else if(p2 > p1 + 2 && *(p2 - 2) == '/')   // is this a self-closing tag also, ending with '/>'?
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - self-close tag\n", __FUNCTION__, __LINE__);
    return p2;  // this tag closes itself
  }

  // grab the tag name, which ends in a non-alpha character incl. white space
  p3 = p1;
  while(p3 < pE &&
        ((*p3 >= 'a' && *p3 <= 'z') || (*p3 >= 'A' && *p3 <= 'Z') || (*p3 >= '0' && *p3 <= '9')))
  {
    p3++;
  }

  // save as tag name
  cbTagName = p3 - p1;
  pTagName = WBCopyStringN(p1, cbTagName);

  if(!pTagName)
  {
    WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
    return NULL;
  }

  WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - tag %s found\n", __FUNCTION__, __LINE__, pTagName);

  // search for a matching closing tag

  p1 = p2;
  iNest = 1;
  while(p1 < pE)
  {
    p1 = CHFindNextXMLTag(p1, pE - p1, CHPARSEXML_DEFAULT);

    if(p1 && p1 < pE)
    {
      *ppTag = p1;  // store as a return val in case this is a close tag
      p1++;  // point 1 past the '<'

      while(p1 < pE && *p1 && *p1 <= ' ')
        p1++;

      WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - looking for end tag, %s found\n", __FUNCTION__, __LINE__, p1);
    }
    else // if(!p1 || p1 >= pE)
    {
      break;
    }

    p2 = CHFindEndOfXMLSection(p1, pE - p1, '>', 1);
    if(!p2 || p2 <= p1 || *p2 != '>')
    {
      WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - returning NULL\n", __FUNCTION__, __LINE__);
      return NULL;
    }

    p2++;  // point past the '>'

    p3 = p1;
    if(*p3 == '/') // an ending tag
      p3++;

    p4 = p3; // start of tag name

    while(p3 < pE &&
          ((*p3 >= 'a' && *p3 <= 'z') || (*p3 >= 'A' && *p3 <= 'Z') || (*p3 >= '0' && *p3 <= '9')))
    {
      p3++;
    }

    if((p3 - p4) == cbTagName && !strncasecmp(p4, pTagName, cbTagName)) // tag matches
    {
      WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - matching tag %s found\n", __FUNCTION__, __LINE__, p4);

      if(*p1 == '/')   // it's an ending tag
      {
        iNest--;

        if(iNest <= 0) // end of tag block
        {
          WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - end of tag %s found\n", __FUNCTION__, __LINE__, pTagName);
          free(pTagName);
          return p2;  // 1 past the end of the tag
        }
      }
      else
      {
        iNest++; // nested item with same name
      }
    }

    p1 = p2;
  }

  // assume the ending tag is after the end of text and return as-is
  free(pTagName);
  *ppTag = pE;
  WB_DEBUG_PRINT(DebugLevel_Medium, "%s.%d - no tag end found, returning %s\n", __FUNCTION__, __LINE__, pE);
  return pE;
}


static char __amp_char(char **ppSpot)
{
char *pS = *ppSpot;

  if(!memcmp(pS, "&amp;", 5))
  {
    *ppSpot += 5;
    return '&';
  }
  else if(!memcmp(pS, "&lt;", 4))
  {
    *ppSpot += 4;
    return '<';
  }
  else if(!memcmp(pS, "&gt;", 4))
  {
    *ppSpot += 4;
    return '>';
  }

  return 0;
}

void WBNormalizeXMLString(char *pString)
{
char *p1, *pDest;
char c1, c2;


  // not only de-quoting, but converting '&amp;' '&lt;' '&gt;' to '&' '<' and '>'

  p1 = pDest = pString;

  while(*p1)
  {
    if(*p1 == '"' || *p1 == '\'')
    {
      c1 = *(p1++);

      while(*p1 &&
            (*p1 != c1 || p1[1] == c1))
      {
        if(*p1 == c1)
        {
          p1++;
        }
        else if(*p1 == '&') // substitute
        {
          c2 = __amp_char(&p1);

          if(c2)
          {
            *(pDest++) = c2;
            continue;
          }

          // if it's not recognized, just process it as normal chars
        }

        *(pDest++) = *(p1++);
      }

      if(*p1 == c1)
      {
        p1++;
      }
    }
    else if(*p1 == '&') // substitute
    {
      c2 = __amp_char(&p1);

      if(!c2)
      {
        goto normal_char; // just process it as normal chars
      }

      *(pDest++) = c2;
    }
    else
    {
normal_char:

      if(pDest != p1)
      {
        *pDest = *p1;
      }

      pDest++;
      p1++;
    }
  }

  *pDest = 0; // make sure
}


int WBCheckDebugLevel(WB_UINT64 dwLevel)
{
extern WB_UINT64 iWBDebugLevel;

  if(WB_LIKELY((iWBDebugLevel & DebugLevel_MASK) < (dwLevel & DebugLevel_MASK)))
  {
    return 0;
  }

  if(!WB_UNLIKELY( WBGetDebugLevel() & DebugSubSystem_RESTRICT )) // RESTRICT not specified
  {
    if(!(dwLevel & DebugSubSystem_MASK) ) // no subsystem specified in debug output
    {
      return 1; // this is acceptable - since no subsystem specified, allow debug output if not 'RESTRICT'
    }
  }

  // at this point I have a debug subsystem 'RESTRICT' specified

  if(((dwLevel & DebugSubSystem_MASK) & (iWBDebugLevel & DebugSubSystem_MASK))
     != 0) // check to see that subsystem bits in 'dwLevel' match bits in 'iWBDebugLevel'
  {
    // at least one subsystem bit matches from 'dwLevel' and iWBDebugLevel
    return 1;
  }

  return 0;
}

void WBDebugPrint(const char *fmt, ...)
{
va_list va;

  va_start(va, fmt);

  vfprintf(stderr, fmt, va);
  fflush(stderr); // dump NOW before (possibly) crashing

  // TODO:  log file?

  va_end(va);
}

void WBDebugDump(const char *szTitle, void *pData, int cbData)
{
int i1, i2;
unsigned char *pX = (unsigned char *)pData;
static const int nCols = 16;

  WBDebugPrint("==========================================================================================\n"
               "%s\n", szTitle);

  for(i1=0; i1 < cbData; i1 += nCols, pX += nCols)
  {
    WBDebugPrint("%06x: ", i1); // assume less than 1Mb for now

    for(i2=0; i2 < nCols && (i1 + i2) < cbData; i2++)
    {
      WBDebugPrint("%02x ", pX[i2]);
    }

    for(; i2 < nCols; i2++)
    {
      WBDebugPrint("   ");
    }

    WBDebugPrint("|");

    for(i2=0; i2 < nCols && (i1 + i2) < cbData; i2++)
    {
      if(pX[i2] < ' ' || pX[i2] >= 0x80)
      {
        WBDebugPrint(" .");
      }
      else
      {
        WBDebugPrint(" %c", pX[i2]);
      }
    }

    WBDebugPrint("\n");
  }

  WBDebugPrint("==========================================================================================\n");
}

static int XMLFilterMatch(const char *szLabel, int argc, const char *argv[])
{
int i1, cb;

  if(argc < 1)
    return 1; // all

  for(i1=0; i1 < argc; i1++)
  {
    cb = strlen(argv[i1]);

    if(!cb)
      continue; // unlikely

//    fprintf(stderr, "Test %s against %s\n", szLabel, argv[i1]);

    if(!strncasecmp(szLabel, argv[i1], cb) &&
       (!szLabel[cb] || szLabel[cb] == '.'))
    {
      return 1;
    }
  }

  return 0;
}

void DoPrintXMLLevel(const char *szPrefix, int iIndex, int argc, const char *argv[], CHXMLEntry *pXML)
{
int i1, iCont;
char *p1;


  i1 = iIndex;
  iCont = pXML[i1].iContainer;

  do
  {
    if(szPrefix)
    {
      p1 = WBCopyString(szPrefix);
      if(!p1)
        return;

      if(pXML[i1].nLabelOffset >= 0) // if I have a label, add a dot
        WBCatString(&p1, ".");
    }
    else
    {
      p1 = WBCopyString(""); // no preceding label
    }

    if(!p1)
      return;

    if(pXML[i1].nLabelOffset >= 0)
      WBCatString(&p1, (const char *)pXML + pXML[i1].nLabelOffset);

    if(XMLFilterMatch(p1, argc, argv))
    {
      if(pXML[i1].nDataOffset >= 0)  // only display this if there is a value.
      {
        // TODO filter check
        printf("%s\t%s\n", p1, (const char *)pXML + pXML[i1].nDataOffset);
      }
      else if(pXML[i1].nLabelOffset >= 0 && pXML[i1].iContentsIndex <= 0) // tag with no values
      {
        printf("%s\t\n", p1); // the tab identifies it as a tag with no value
      }
    }

    if(pXML[i1].iContentsIndex > 0)
    {
      DoPrintXMLLevel(p1, pXML[i1].iContentsIndex, argc, argv, pXML);
    }

    if(p1)
      free(p1);

    if((!szPrefix || pXML[i1].iContainer < 0) /* top level */
       && !pXML[i1].iNextIndex /* end of 'chain' */)
    {
      break;  // normal exit point
    }

    i1 = pXML[i1].iNextIndex;

  } while(pXML[i1].iContainer == iCont);

}

void  DoPrintXMLContents(int argc, const char *argv[], CHXMLEntry *pXML)
{
  DoPrintXMLLevel(NULL, 0, argc, argv, pXML);
}



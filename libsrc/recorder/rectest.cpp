#include <comtools.h>
#include <lg.h>
#include <appagg.h>
#include <recapi.h>
#include <mprintf.h>
#include <initguid.h>
#include <ctype.h>
#include <stdlib.h> // getenv

#pragma off (unreference)

DEFINE_LG_GUID(ID_RecGetChar, 0x1);

HRESULT LGAPI RecGetChar(const sRecClientDesc *, sRecCallInfo * pInfo);
int peekChar();
int gTestChar;

eRecMode gMode;


HRESULT LGAPI RecGetChar(const sRecClientDesc *, sRecCallInfo * pInfo)
{
    gTestChar = getchar();
    RecSetData(pInfo->hDataOut, &gTestChar, int);
    return S_OK;
}

tResult LGAPI AppCreateObjects(int, const char **)
{
    char *pszRecVar = (getenv("rec")) ? getenv("rec") : "r";
    switch (tolower(*pszRecVar))
    {
        case 'p':
            gMode = kRecPlayback;
            break;

        case 'r':
            gMode = kRecRecord;
            break;

        case 'i':
        default:
            gMode = kRecInactive;
    }

    RecorderCreate(gMode, "");

    return NOERROR;
}

int peekChar()
{
    int c = getchar();
    ungetc(c, stdin);
    return c;
}

int LGAPI AppMain(int, const char **)
{
    sRecClientDesc myClientInfo;
    sRecCallInfo testCallInfo;
    int result;
    int *pResult = &result;
    int bRecord = TRUE;
    AutoAppIPtr(Recorder);

    // Point the client's function to RecGetChar
    myClientInfo.pClientID = &ID_RecGetChar;
    myClientInfo.pszName = "myClient";
    myClientInfo.pContext = NULL;
    myClientInfo.pfnHandler = RecGetChar;

    // Add the test client
    pRecorder->AddClient(&myClientInfo);

    if (gMode == kRecPlayback)
    {
        printf("Played back:\n");
        while (peekChar() != EOF)
        {
            int humpty,
             dumpty;
            RecSetData(testCallInfo.hDataIn, &humpty, int);
            RecSetData(testCallInfo.hDataOut, &dumpty, int);

            pRecorder->Call(myClientInfo.pClientID, &testCallInfo);

            RecGetData(testCallInfo.hDataOut, pResult, int);
            printf("%c", *pResult);
        }
    }
    else if (gMode == kRecInactive)
    {
        printf("Recorder is disabled:\n");
        while (peekChar() != EOF)
        {
            int humpty,
             dumpty;
            RecSetData(testCallInfo.hDataIn, &humpty, int);
            RecSetData(testCallInfo.hDataOut, &dumpty, int);

            pRecorder->Call(myClientInfo.pClientID, &testCallInfo);

            RecGetData(testCallInfo.hDataOut, pResult, int);
            printf("%c", *pResult);
        }
    }
    else
    {
        printf("Recorded:\n");
        while (peekChar() != EOF)
        {
            pRecorder->Call(myClientInfo.pClientID, &testCallInfo);

            RecGetData(testCallInfo.hDataOut, pResult, int);
            printf("%c", *pResult);
        }
    }

    return S_OK;
}

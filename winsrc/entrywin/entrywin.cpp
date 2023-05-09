///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/entrywin/RCS/entrywin.cpp $
// $Author: TOML $
// $Date: 1996/12/12 15:44:01 $
// $Revision: 1.4 $
//
//
// Main entry point for windowed Windows applications
//

#ifdef _WIN32

#include <windows.h>
#include <lg.h>

EXTERN int LGAPI _AppMain(int argc, const char* argv[]);

extern "C"
{
	int _g_referenceEntryPoint = 0;
}

int main(int argc, const char* argv[])
{

	return _AppMain(argc, argv);
}

/*
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR pszCmdLine, int fShow)
{
	// Turn Windows command line into classic arguments    @TBD (toml 04-24-96):  won't do quoted arguments correctly
	int argc = 1;
	const char* argv[32];

	argv[0] = "";
	argv[1] = strtok(pszCmdLine, " ");
	while (argv[argc] != NULL && argc < 31)
		argv[++argc] = strtok(NULL, " ");

	return _AppMain(argc, argv);
}
*/

#endif
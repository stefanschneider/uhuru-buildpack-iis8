#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <hwebcore.h>

// NOTE: Hosted Web Core (hwebcore) functions require the __stdcall calling convention. 
// Set the project's calling convention to "__stdcall (/Gz)".
// Use Windows Events or IPC communication with the DEA http://msdn.microsoft.com/en-us/library/System.Threading.EventWaitHandle.aspx


HANDLE hStopEvent;

BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal.
	case CTRL_C_EVENT:
		SetEvent(hStopEvent);
		return(TRUE);

	default:
		return FALSE;
	}
}



// First argument is the path to the application host config.
HRESULT _cdecl wmain(int argc, wchar_t * argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	// Create a handle for the Web core DLL.
	HINSTANCE hDLL;

	// Specify the HRESULT for returning errors.
	HRESULT hr = S_OK;

	// Create arrays to hold paths.
	WCHAR wszInetPath[MAX_PATH];
	WCHAR wszDllPath[MAX_PATH];
	WCHAR wszCfgPath[MAX_PATH];
	WCHAR wszRootWebCfgPath[MAX_PATH];
	WCHAR wszInstanceName[30];

	// Retrieve the path of the Inetsrv folder.
	DWORD nSize = ::ExpandEnvironmentStringsW(L"%windir%\\system32\\inetsrv", wszInetPath, MAX_PATH);

	// Exit if the path of the Inetsrv folder cannot be determined.
	if (nSize == 0)
	{
		// Retrieve the last error.
		hr = HRESULT_FROM_WIN32(GetLastError());
		// Return an error status to the console.
		printf("Could not determine the path to the Inetsrv folder.\n");
		printf("Error: 0x%x\n", hr);
		// Return an error from the application and exit.
		return hr;
	}

	// Append the Web core DLL name to the Inetsrv path.
	wcscpy_s(wszDllPath, MAX_PATH - 1, wszInetPath);
	wcscat_s(wszDllPath, MAX_PATH - 1, L"\\");
	wcscat_s(wszDllPath, MAX_PATH - 1, WEB_CORE_DLL_NAME);

	if (argc == 4)
	{
		// Use the first argument from the run command as the config file path
		wcscpy_s(wszCfgPath, MAX_PATH - 1, argv[1]);

		// Use the second argument from the run command as the root web config file path
		wcscpy_s(wszRootWebCfgPath, MAX_PATH - 1, argv[2]);

		// Use the third argument as the name of the web core instance
		wcscpy_s(wszInstanceName, 29, argv[3]);
	}
	else
	{
		printf("Invalid parameters, use the following: iishwc [applicationHost.config] [rootweb.config] [instance-id]");
		return 1;
	}

	hStopEvent = CreateEvent(NULL, true, false, NULL);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

	// Create a pointer to WebCoreActivate.
	PFN_WEB_CORE_ACTIVATE pfnWebCoreActivate = NULL;

	// Create a pointer to WebCoreShutdown.
	PFN_WEB_CORE_SHUTDOWN pfnWebCoreShutdown = NULL;

	// Load the Web core DLL.
	hDLL = ::LoadLibraryW(wszDllPath);

	// Test whether the Web core DLL was loaded successfully.
	if (hDLL == NULL)
	{
		// Retrieve the last error.
		hr = HRESULT_FROM_WIN32(GetLastError());
		// Return an error status to the console.
		printf("Could not load DLL.\n");
		printf("Error: 0x%x\n", hr);
	}
	else
	{
		// Return a success status to the console.
		printf("DLL loaded successfully.\n");
		// Retrieve the address for "WebCoreActivate".
		pfnWebCoreActivate = (PFN_WEB_CORE_ACTIVATE)GetProcAddress(
			hDLL, "WebCoreActivate");
		// Test for an error.
		if (pfnWebCoreActivate == NULL)
		{
			// Retrieve the last error.
			hr = HRESULT_FROM_WIN32(GetLastError());
			// Return an error status to the console.
			printf("Could not resolve WebCoreActivate.\n");
			printf("Error: 0x%x\n", hr);
		}
		else
		{
			// Return a success status to the console.
			printf("WebCoreActivate successfully resolved.\n");
			// Retrieve the address for "WebCoreShutdown".
			pfnWebCoreShutdown = (PFN_WEB_CORE_SHUTDOWN)GetProcAddress(
				hDLL, "WebCoreShutdown");
			// Test for an error.
			if (pfnWebCoreShutdown == NULL)
			{
				// Retrieve the last error.
				hr = HRESULT_FROM_WIN32(GetLastError());
				// Return an error status to the console.
				printf("Could not resolve WebCoreShutdown.\n");
				printf("Error: 0x%x\n", hr);
			}
			else
			{
				// Return a success status to the console.
				printf("WebCoreShutdown successfully resolved.\n");
				// Return an activation status to the console.
				printf("Activating the Web core...\n");
				// Activate the Web core.
				hr = pfnWebCoreActivate(wszCfgPath, wszRootWebCfgPath, wszInstanceName);
				// Test for an error.
				if (FAILED(hr))
				{
					// Return an error status to the console.
					printf("WebCoreActivate failed.\n");
					printf("Error: 0x%x\n", hr);
				}
				else
				{
					// Return a success status to the console.
					printf("WebCoreActivate was successful.\n");

					// Prompt the user to continue.
					printf("Press Ctrl-C to stop.\n");

					// Wait for stop event.
					WaitForSingleObject(hStopEvent, INFINITE);

					// Return a shutdown status to the console.
					printf("Shutting down the Web core...\n");
					// Shut down the Web core.
					hr = pfnWebCoreShutdown(0L);
					// Test for an error.
					if (FAILED(hr))
					{
						// Return an error status to the console.
						printf("WebCoreShutdown failed.\n");
						printf("Error: 0x%x\n", hr);
					}
					else
					{
						// Return a success status to the console.
						printf("WebCoreShutdown was successful.\n");
					}
				}
			}
		}
		// Release the DLL.
		FreeLibrary(hDLL);
	}
	// Return the application status.
	return hr;
}

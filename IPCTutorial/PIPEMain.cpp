#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include<thread>
#include <string>
#include <iostream>

#define BUFSIZE 4096 
//#define TEXT(quote) quote   // r_winnt
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;

void CreateChildProcess(void);
void WriteToPipe(void);
void ReadFromPipe(void);
void ErrorExit(PTSTR);
//std::string getCurrentDirectory()
//{
//	char* buffer;
//	if ((buffer = _getcwd(NULL, 0)) == NULL)
//	{
//		std::cout << "getcwd error" << std::endl;
//		return std::string();
//	}
//	else
//	{
//		std::cout << "getcwd():" << buffer << std::endl;
//		std::string::size_type pos = 0;
//		std::string pwd = std::string(buffer);
//
//		while ((pos = pwd.find_first_of('\\', pos)) != std::string::npos)
//		{
//			pwd.insert(pos, "\\");//
//			pos = pos + 2;
//		}
//		std::cout << "current directory:" << pwd << std::endl;
//		free(buffer);
//		return pwd;
//	}
//}

int _tmain(int argc, TCHAR* argv[])
{
	SECURITY_ATTRIBUTES saAttr;

	printf("\n->Start of parent execution.\n");

	// Set the bInheritHandle flag so pipe handles are inherited. 

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT.  
	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
	{
		TCHAR errorMessage[] = TEXT("StdoutRd CreatePipe");
		ErrorExit(errorMessage);
	}
		

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
	{
		TCHAR errorMessage[] = TEXT("Stdout SetHandleInformation");
		ErrorExit(errorMessage);
	}


	// Create a pipe for the child process's STDIN.  
	if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
	{
		TCHAR errorMessage[] = TEXT("Stdin CreatePipe");
		ErrorExit(errorMessage);
	}


	// Ensure the write handle to the pipe for STDIN is not inherited.  
	if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
	{
		TCHAR errorMessage[] = TEXT("Stdin SetHandleInformation");
		ErrorExit(errorMessage);
	}


	// Create the child process. 
	// g_hChildStd_OUT_Wr and g_hChildStd_IN_Rd will be inherited by child

	CreateChildProcess();

	// Get a handle to an input file for the parent. 
	// This example assumes a plain text file and uses string output to verify data flow. 

	if (argc == 1)
	{
		TCHAR errorMessage[] = TEXT("Please specify an input file.\n");
		ErrorExit(errorMessage);
	}

	g_hInputFile = CreateFile(
		argv[1],
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_READONLY,
		NULL);

	if (g_hInputFile == INVALID_HANDLE_VALUE)
	{
		TCHAR errorMessage[] = TEXT("CreateFile");
		ErrorExit(errorMessage);
	}
		
	// Write to the pipe that is the standard input for a child process. 
	// Data is written to the pipe's buffers, so it is not necessary to wait
	// until the child process is running before writing data.

	WriteToPipe();
	printf("\n->Contents of %s written to child STDIN pipe.\n", argv[1]);

	// Read from pipe that is the standard output for child process. 

	printf("\n->Contents of child process STDOUT:\n\n");
	ReadFromPipe();

	printf("\n->End of parent execution.\n");

	// The remaining open handles are cleaned up when this process terminates. 
	// To avoid resource leaks in a larger application, close handles explicitly. 
	//::system("pause");
	return 0;
}

void CreateChildProcess()
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{
	/*TCHAR szCmdline[] = TEXT("D:\\vs2019program\\IPCTutorial\\x64\\Release\\child.exe");*/
	TCHAR szCmdline[] = TEXT("child.exe");
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	// Set up members of the PROCESS_INFORMATION structure. 

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 

	bSuccess = CreateProcess(NULL,
		szCmdline,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

	 // If an error occurs, exit the application. 
	if (!bSuccess)
	{
		TCHAR errorMessage[] = TEXT("CreateProcess");
		ErrorExit(errorMessage);
	}
	else
	{
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 
		/*g_hChildStd_OUT_Wr the number of it is 2, 
		 *one for child process, one for parent process(current process), 
		 *if does not close it here, ReadFile(g_hChildStd_OUT_Rd) will block
		*/
		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}
}

void WriteToPipe(void)

// Read from a file and write its contents to the pipe for the child's STDIN.
// Stop when there is no more data. 
{
	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE];
	BOOL bSuccess = FALSE;

	for (;;)
	{
		bSuccess = ReadFile(g_hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
		if (!bSuccess || dwRead == 0) break;

		bSuccess = WriteFile(g_hChildStd_IN_Wr, chBuf, dwRead, &dwWritten, NULL);
		if (!bSuccess) break;
	}

	// Close the pipe handle so the child process stops reading. 

	if (!CloseHandle(g_hChildStd_IN_Wr))
	{
		TCHAR errorMessage[] = TEXT("StdInWr CloseHandle");
		ErrorExit(errorMessage);
	}
}

void ReadFromPipe(void)

// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
{
	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE];
	BOOL bSuccess = FALSE;
	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for (;;)
	{
		bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
		if (!bSuccess || dwRead == 0 || (strcmp(chBuf, "over") == 0)) break;

		bSuccess = WriteFile(hParentStdOut, chBuf,
			dwRead, &dwWritten, NULL);
		if (!bSuccess) break;
	}
}

void ErrorExit(PTSTR lpszFunction)

// Format a readable error message, display a message box, 
// and exit from the application.
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(1);
}
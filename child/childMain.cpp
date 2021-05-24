#include <windows.h>
#include <stdio.h>

#define BUFSIZE 4096 

int main(void)
{
	CHAR chBuf[BUFSIZE];
	DWORD dwRead, dwWritten;
	HANDLE hStdin, hStdout;
	BOOL bSuccess;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (
		(hStdout == INVALID_HANDLE_VALUE) ||
		(hStdin == INVALID_HANDLE_VALUE)
		)
		ExitProcess(1);

	// Send something to this process's stdout using printf.
	printf("\n ** This is a message from the child process. ** \n");

	// This simple algorithm uses the existence of the pipes to control execution.
	// It relies on the pipe buffers to ensure that no data is lost.
	// Larger applications would use more advanced process control.

	for (;;)
	{
		// Read from standard input and stop on error or no data.
		bSuccess = ReadFile(hStdin, chBuf, BUFSIZE, &dwRead, NULL);

		if (!bSuccess || dwRead == 0)
		{
			printf("\n->break when ReadFile.\n");
			WriteFile(hStdout, "over", dwRead, &dwWritten, NULL);
			break;
		}
			

		// Write to standard output and stop on error.
		bSuccess = WriteFile(hStdout, chBuf, dwRead, &dwWritten, NULL);

		if (!bSuccess)
		{
			printf("\n->break when WriteFile.\n");
			break;
		}
			
	}
	/*if (!CloseHandle(hStdout))
	{
		printf("\n->fail to close write handle on child process.\n");
	}*/
	printf("\n->End of child process.\n");
	return 0;
}
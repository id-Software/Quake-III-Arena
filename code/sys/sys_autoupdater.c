/*
The code in this file is in the public domain. The rest of ioquake3
is licensed until the GPLv2. Do not mingle code, please!
*/

#ifdef USE_AUTOUPDATER
#  ifndef AUTOUPDATER_BIN
#    error The build system should have defined AUTOUPDATER_BIN
#  endif

#  ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN 1
#    include <windows.h>
#  else
#    include <unistd.h>
#  endif

#  include <stdio.h>
#  include <string.h>
#endif

void Sys_LaunchAutoupdater(int argc, char **argv)
{
#ifdef USE_AUTOUPDATER
	#ifdef _WIN32
	{
		/* We don't need the Unix pipe() tapdance here because Windows lets children wait on parent processes. */
		PROCESS_INFORMATION procinfo;
		STARTUPINFO startinfo;
		char cmdline[128];
		memset(&procinfo, '\0', sizeof (procinfo));
		memset(&startinfo, '\0', sizeof (startinfo));
		startinfo.cb = sizeof (startinfo);
		sprintf(cmdline, "" AUTOUPDATER_BIN " --waitpid %u", (unsigned int) GetCurrentProcessId());

		if (CreateProcessA(AUTOUPDATER_BIN, cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startinfo, &procinfo))
		{
			/* close handles now so child cleans up immediately if nothing to do */
			CloseHandle(procinfo.hProcess);
			CloseHandle(procinfo.hThread);
		}
	}
	#else
	int updater_pipes[2];
	if (pipe(updater_pipes) == 0)
	{
		pid_t pid = fork();
		if (pid == -1)  /* failure, oh well. */
		{
			close(updater_pipes[0]);
			close(updater_pipes[1]);
		}
		else if (pid == 0)  /* child process */
		{
			close(updater_pipes[1]);  /* don't need write end. */
			if (dup2(updater_pipes[0], 3) != -1)
			{
				char pidstr[64];
				char *ptr = strrchr(argv[0], '/');
				if (ptr)
					*ptr = '\0';
				chdir(argv[0]);
				#ifdef __APPLE__
				chdir("../..");  /* put this at base of app bundle so paths make sense later. */
				#endif
				snprintf(pidstr, sizeof (pidstr), "%lld", (long long) getppid());
				execl(AUTOUPDATER_BIN, AUTOUPDATER_BIN, "--waitpid", pidstr, NULL);
			}
			_exit(0);  /* oh well. */
		}
		else   /* parent process */
		{
			/* leave the write end open until we terminate so updater can block on it. */
			close(updater_pipes[0]);
		}
	}
	#endif
#endif

	(void) argc; (void) argv;  /* possibly unused. Pacify compilers. */
}


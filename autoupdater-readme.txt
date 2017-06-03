The updater program's code is public domain. The rest of ioquake3 is not.

The source code to the autoupdater is in the code/autoupdater directory.
There is a small piece of code in ioquake3 itself at startup, too.

(This is all Unix terminology, but similar approaches on Windows apply.)

The updater is a separate program, written in C, with no dependencies on
the game. It (statically) links to libcurl and uses the C runtime, but
otherwise has no external dependencies. It has to be a single binary file
with no shared libraries.

The basic flow looks like this:

- The game launches as usual.
- Right after main() starts, the game creates a pipe, forks off a new process,
  and execs the updater in that process. The game won't ever touch the pipe
  again. It's just there to block the child app until the game terminates.
- The updater has no UI. It writes a log file.
- The updater downloads a manifest from a known URL over https://, using
  libCurl. The base URL is platform-specific (it might be
  https://example.com/mac/, or https://example.com/linux-x86/, whatever).
  The manifest is at $BASEURL/manifest.txt
- The manifest looks like this: three lines per file...

Contents/MacOS/baseq3/uix86_64.dylib
332428
a49bbe77f8eb6c195265ea136f881f7830db58e4d8a883b27f59e1e23e396a20

- That's the file's path, its size in bytes, and an sha256 hash of the data.
- The file will be at this path under the base url on the webserver.
- The manifest only lists files that ever needed updating; it's not necessary
  to list every file in the game's installation (unless you want to allow the
  entire game to download).
- The updater will check each item in the manifest:
    - Does the file not exist in the install? Needs downloading.
    - Does the file have a different size? Needs downloading.
    - Does the file have a different sha256sum? Needs downloading.
    - Otherwise, file is up to date, leave it alone.
- If an item needs downloading, do these same checks against the file in the
  download directory (if it's already there and matches, don't download again.)
- Download necessary files with libcurl, put it in a download directory.
- The downloaded file is also checked for size and sha256 vs the manifest, to
  make sure there was no corruption or confusion. If a downloaded file doesn't
  match what was expected, the updater aborts and will try again next time.
  This could fail checksum due to i/o errors and compromised security, but
  it might just be that a new version was being published and bad luck
  happened, and a retry later could correct everything.
- If the updater itself needs upgrading, we deal with that first. It's
  downloaded, then the updater relaunches from the downloaded binary with
  a special command line. That relaunched process copies itself to the proper
  location, and then relaunches _again_ to restart the normal updating
  process with the new updater in its correct position.
- Once the downloads are complete and the updater itself doesn't need
  upgrading, we are ready to start the normal upgrade. Since we can't replace
  executables on some platforms while they are running, and swapping out a
  game's data files at runtime isn't wise in general, the updater will now
  block until the game terminates. It does this by reading on the pipe that
  the game created when forking the updater; since the game never writes
  anything to this pipe, it causes the updater to block until the pipe closes.
  Since the game never deliberately closes the pipe either, it remains open
  until the OS forcibly closes it as the game process terminates. Being an
  unnamed pipe, it just vaporizes at this point, leaving no state that might
  accidentally hang us up later, like a global semaphore or whatnot. This
  technique also lets us localize the game's code changes to one small block
  of C code, with no need to manage these resources elsewhere.
- As a sanity check, the updater will also kill(game_process_id, 0) until it
  fails, sleeping for 100 milliseconds between each attempt, in case the
  process is still being cleaned up by the OS after closing the pipe.
- Once the updater is confident the game process is gone, it will start
  upgrading the appropriate files. It does this in two steps: it moves
  the old file to a "rollback" directory so it's out of the way but still
  available, then it moves the newly-downloaded file into place. Since these
  are all simple renames and not copies, this can move fast. Any missing
  parent directories are created, in case the update is adding a new file
  in a directory that didn't previously exist.
- If something goes wrong at this point (file i/o error, etc), the updater
  will roll back the changes by deleting the updated files, and moving the
  files in the "rollback" directory back to their original locations. Then
  the updater aborts.
- If nothing went wrong, the rollback files are deleted. And we are officially
  up to date! The updater terminates.


The updater is designed to fail at any point. If a download fails, it'll
pick up and try again next time, etc. Completed downloads will remain, so it
will just need to download any missing/incomplete files.

The server side just needs to be able to serve static files over HTTPS from
any standard Apache/nginx/whatever process.

Failure points:
- If the updater fails when still downloading data, it just picks up on next
  restart.
- If the updater fails when replacing files, it rolls back any changes it has
  made.
- If the updater fails when rolling back, then running the updater again after
  fixing the specific problem (disk error, etc?) will redownload and replace
  any files that were left in an uncertain state. The only true point of
  risk is crashing during a rollback and then having the updater bricked for
  some reason, but that's an extremely small surface area, knock on wood.
- If the updater crashes or totally bricks, ioquake3 should just keep being
  ioquake3. It will still launch and play, even if the updater is quietly
  segfaulting in the background on startup.
- If an update bricks ioquake3 to the point where it can't run the updater,
  running the updater directly should let it recover (assuming a future update
  fixes the problem).


Items to consider for future revisions:
- GPG sign the manifest; if we can be confident that the manifest isn't
  compromised, then the sha256 hashes of each file it contains should protect
  the rest of the process. As it currently stands, we trust the download
  server isn't compromised.
- Maybe put a limit on the number manifest downloads, so we only check once
  every hour? Every day?
- Channels? Stable (what everyone gets by default), Nightly (once a day),
  Experimental (some other work-in-progress branch), Bloody (literally the
  latest commit).
- Let mods update, separate from the main game?

Questions? Ask Ryan: icculus@icculus.org

--ryan.


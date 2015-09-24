#!/bin/sh
echo "Edit this script to change the path to ioquake3's dedicated server executable and which binary if you aren't on x86_64.\n Set the sv_dlURL setting to a url like http://yoursite.com/ioquake3_path for ioquake3 clients to download extra data"
~/ioquake3/ioq3ded.x86_64 +set dedicated 2 +set sv_allowDownload 1 +set sv_dlURL "" +set com_hunkmegs 64 "$@"

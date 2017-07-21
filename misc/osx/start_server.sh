#!/bin/sh
echo "Edit this script to change the path to ioquake3's dedicated server executable."
echo "Set the sv_dlURL setting to a url like http://yoursite.com/ioquake3_path for ioquake3 clients to download extra data."

# sv_dlURL needs to have quotes escaped like \"http://yoursite.com/ioquake3_path\" or it will be set to "http:" in-game.

/Applications/ioquake3/ioquake3.app/Contents/MacOS/ioq3ded +set dedicated 2 +set sv_allowDownload 1 +set sv_dlURL \"\" +set com_hunkmegs 64 "$@"

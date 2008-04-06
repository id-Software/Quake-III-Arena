<?xml version="1.0" standalone="yes"?>
<!-- ioquake3 is the name of the base product -->
<install product="ioquake3"
    desc="ioquake3"
    component="Foo Mod"
    version="1.1"
    >

    <option install="true">
	Foo
	<!--
	install symlink 'foo' into $PATH, pointing to a script
	called startfoo that gets installed into ioquake3's
	directory
	The script could look like this:
	#!/bin/sh
	exec ioquake3 +set fs_game foo "$@"
    	exit 1
	-->
	<binary arch="any" libc="any"
	    symlink="foo"
	    binpath="startfoo">
	    startfoo
	</binary>
	<!--
	extract archive in ioquake3's directory.
	the archive must contain a subdirectory of course
	-->
	<files>
	    foo-1.1.zip
	</files>
    </option>
</install>

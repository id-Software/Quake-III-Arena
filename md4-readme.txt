##########################################################
# Info about the MD4 format supported by the ioQ3 engine #
##########################################################

All models included with the original version of Quake3 from id soft are in
the MD3 format. Animations in this format are realized by saving the position
of every vertex in each frame which can make these files pretty large.

ID started work on a newer format, the MD4 format which they never finished.
This format uses a skeleton with all vertices "attached" to their bones.
Because only the position of the bones must be stored for each frame and the
number of bones is not very high this format is more efficient when
doing animations.

Raven software "finished" this format originally started by ID and included
it in their game EliteForce. They called their model format "MDR" which is
the name I have used throughout the sourcecode and I will continue using in
this readme. Since the code on how to handle those MDR files was released
under a GPL licence a long time ago, I was able to implement this format for
Quake3 and do some efficiency improvements.
To enable the support for this model format, go to qcommon/qfiles.h,
remove the comment slashes for #define RAVENMD4 and then compile the engine.

Including finished MDR models in your projects is easy: just load the model
files in your cgame code as you would normally load an MD3 model. The engine
will expect the models to have a ".mdr" suffix.
The rest is pretty much the same: Selecting the current animation frame,
adding a skin to the model, etc..
You can check out the original eliteforce game sourcecode if you want to
have examples on using the md4s. The source can be got at:
http://eliteforce2.filefront.com/
You can also get reference MDR files there, just go to the model/skin
section there and pick something to download.

Now here comes the tricky part:
Creating files with this format. There are tools to create these kinds of
MDR files, like a plugin for Milkshape.

A pretty good overview about MDR file creation is available at
http://synapse.vgfort.com/
You can find some tools for creating MDR files there.

On a sidenote:
There is an independent implementation of the MD4 file format available
here:
http://gongo.quakedev.com/
At this time, ioquake3 has no support for these models though that may
change in the future. Nevertheless, he has got a tool for skeletal
animations that can possibly be hooked into the MDR format with some
modifications.


Good luck!
 - Thilo Schulz

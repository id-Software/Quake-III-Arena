ioquake3 VoIP support documentation.
Last updated 6/25/2008 by Ryan C. Gordon.

There are two ways to use VoIP in ioquake3. You can either use Mumble as an
 external program, for which ioq3 now supplies some basic hooks, or you can
 use the new built-in VoIP support.

Mumble is here: http://mumble.sourceforge.net/  ... ioquake3 can supply it
 with your in-game position, but everything else is whatever features Mumble
 offers outside of the game. To use it, start Mumble before you start ioq3,
 and run the game with +set cl_useMumble 1. This should work on at least
 Linux, Mac OS X, and Windows, and probably other platforms Mumble supports
 in the future.

The built-in stuff offers tighter in-game integration, works on any platform
 that ioquake3 supports, and doesn't require anything more than a recent build
 of the game. The rest of this document is concerned with the built-in VoIP
 support.


Quick start for servers:
    - run a recent build of ioquake3.
    - Make sure your network settings are set to broadband.

Quick start for clients:
    - run a recent build of ioquake3.
    - Make sure your network settings are set to broadband.
    - +set s_useOpenAL 1
    - \bind q "+voiprecord"
    - Hook up a microphone, connect to a VoIP-supporting server.
    - hold down 'q' key and talk.


Cvars you can set:

sv_voip: set to "1" (the default) to enable server-side VoIP support. Set to
         "0" to disable. Without this, all VoIP packets are refused by the
         server, which means no one gets to use in-game VoIP.

cl_voip: set to "1" (the default) to enable client-side VoIP support. Set to "0"
      to disable. Without this, you will neither be able to transmit voice nor
      hear other people.

s_alCapture: set to "1" (the default) to have the audio layer open an OpenAL
             capture device. Without this set on sound startup, you'll never
             get bits from the microphone. This means you won't transmit, but
             you can still hear other people.

cl_voipSendTarget: a string: "all" to broadcast to everyone, "none" to send
                   to no one, "attacker" to send to the last person that hit
                   you, "crosshair" to send to the people currently in your
                   crosshair, "spatial" to talk to all people in hearing
                   range or a comma-separated list of client numbers, like
                   "0,7,2,23" ... an empty string is treated like "spatial".
                   You can also use a mixed string like
                   "0, spatial, 2, crosshair".
                   This is reset to "spatial" when connecting to a new server.
                   Presumably mods will manage this cvar, not people, but
                   keybind could be useful for the general cases. To send to
                   just your team, or the opposing team, or a buddy list, you
                   have to set a list of numbers.

cl_voipUseVAD: set to "1" to automatically send audio when the game thinks you
               are talking, "0" (the default) to require the user to manually
               start transmitting, presumably with a keybind.

cl_voipVADThreshold: only used if cl_voipUseVAD is "1" ... a value between
                     0.0 and 1.0 that signifies the volume of recorded audio
                     that the game considers to be speech. You can use this
                     to trim out breathing or perhaps the sound of your
                     fingers tapping the keyboard and only transmit audio
                     louder than that. You will have to experiment to find the
                     value that works best for your hardware and play style.
                     The default is "0.25", with "0.0" being silence and "1.0"
                     being pretty-darn-loud.

cl_voipSend: when set to "1", the game will capture audio from the microphone
             and transmit it, when "0", the game will not. The game can
             optimize for the "0" case (perhaps turning off audio recording).
             Lots of things set this on and off, including cl_voipUseVAD, so
             you probably should not touch this directly without knowing what
             you're doing, but perhaps mods can make use of it.

cl_voipGainDuringCapture: This is the volume ("gain") of audio coming out of
                          your speakers while you are recording sound for
                          transmission. This is a value between 0.0 and 1.0,
                          zero being silence and one being no reduction in
                          volume. This prevents audio feedback and echo and
                          such, but if you're listening in headphones that
                          your mic won't pick up, you don't need to turn down
                          the gain. Default is 0.2 (20% of normal volume). You
                          ABSOLUTELY want to make your speakers quiet when you
                          record, if the microphone might pick it up!

cl_voipShowMeter: Set to "1" (the default) to show a volume meter as you are
                  recording from the microphone, so you can see how well the
                  game can "hear" you. Set to "0" to disable the display of
                  the meter.

cl_voipCaptureMult: Multiply recorded audio by this value after denoising.
                    Defaults to 2.0 to _double_ the volume of your voice.
                    This is to make you more audible if denoising eats away
                    too much data. Set this to 1.0 to get no change, less to
                    be quieter.



Console commands:

voip ignore <clientnum>
    Turn off incoming voice from player number <clientnum>. This will refuse to
     play any incoming audio from that player, and instruct the server to stop
     sending it, to save bandwidth. Use unignore to reenable. This is reset to
     unignored when (re)connecting to a server.

voip unignore <clientnum>
    Turn on incoming voice from player number <clientnum>. This will start
     playing audio from this player again if you've previously done a "voip
     ignore", and instruct the server to start sending her voice packets to
     you again.

voip muteall
    Turn off all incoming voice. This will refuse to play any incoming audio,
     and instruct the server to stop sending it, to save bandwidth. Use
     unmuteall to reenable. This is reset to unmuted when (re)connecting to
     a server.

voip unmuteall
    Turn on incoming voice. This will start playing audio again if you've
     previously done a "voip muteall", and instruct the server to start
     sending voice packets to you again.

voip gain <clientnum> <gain>
    Sets the volume ("gain") for player number <clientnum> to <gain> ...
     A gain of 0.0 is silence, and 2.0 doubles the volume. Use this if someone
     is too quiet or too loud.




Actions:

+voiprecord: The action you should bind to a key to record. This basically
             toggles cl_voipSend on and off. You don't need this if you're
             using cl_voipUseVAD, since that'll just record all the time and
             decide what parts of the recording are worth sending.



More detailed/technical info:

By default, all of this is enabled. You can build with or without VoIP
 support explicitly with USE_VOIP=[1|0] on the make command line.

You currently must use OpenAL to speak, as we have ALC_EXT_capture support
 in place to pull data from the microphone. If you are using the SDL backend,
 you can still hear people, but not speak.

There is no in-game UI to speak of: we encourage mods to add some. Largely
 they will just need to set cvars and run console commands for choosing
 voice targets and ignoring people, etc.

This requires patched builds to be useful, but remains network compatible with
 legacy quake3 clients and servers. Clients and servers both report in their
 info strings whether they support VoIP, and won't send VoIP data to those not
 reporting support. If a stray VoIP packet makes it to a legacy build, it will
 be ignored without incident.

VoIP packets are saved in demo files! You will be able to playback what you
 heard and what you said on VoIP-compatible clients. Legacy clients can also
 play demo files with VoIP packets in them, but just won't play the voice
 track. For VoIP-supported builds, it's nice to have a record of the
 trash-talk.

Data is processed using the Speex narrowband codec, and is cross-platform.
 Bigendian and littleendian systems can speak to each other, as can 32 and
 64-bit platforms.

Bandwidth: VoIP data is broken up into 20 millisecond frames (this is a Speex
 requirement), and we try to push up to 12 Speex frames in one UDP packet
 (about a quarter of a second of audio)...we're using the narrowband codec:
 8000Hz sample rate. In practice, a client should send about 2 kilobytes per
 second more when speaking, spread over about four bursts per second, plus a
 few bytes of state information. For comparison, this is less than the server
 sends when downloading files to the client without an http redirect. The
 server needs to rebroadcast the packet to all clients that should receive it
 (which may be less than the total connected players), so servers should
 assume they'll need to push (number of players speaking at once times number
 of people that should hear it) * 2 kilobytes per second. It shouldn't be a
 problem for any client or server on a broadband connection, although it may
 be painful for dialup users (but then again, everything is. They can just
 disable the cvar). The game will refuse to enable VoIP support if your have
 your network settings lower than "Cable/xDSL/LAN", just in case.

The initial VoIP work was done by Ryan C. Gordon <icculus@icculus.org>, and
 he can be contacted with technical questions, if the ioq3 mailing list or
 forums aren't helpful.

// end of voip-README.txt ...




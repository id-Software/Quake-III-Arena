#ifndef _ALCTYPES_H_
#define _ALCTYPES_H_

#if !defined(_WIN32)
struct _AL_device;
typedef struct _AL_device ALCdevice;

typedef void ALCcontext;
#endif /* _WIN32 */

typedef int ALCenum;

/** ALC boolean type. */
typedef char ALCboolean;

/** ALC 8bit signed byte. */
typedef char ALCbyte;

/** ALC 8bit unsigned byte. */
typedef unsigned char ALCubyte;

/** OpenAL 8bit char */
typedef char ALCchar;

/** ALC 16bit signed short integer type. */
typedef short ALCshort;

/** ALC 16bit unsigned short integer type. */
typedef unsigned short ALCushort;

/** ALC 32bit unsigned integer type. */
typedef unsigned ALCuint;

/** ALC 32bit signed integer type. */
typedef int ALCint;

/** ALC 32bit floating point type. */
typedef float ALCfloat;

/** ALC 64bit double point type. */
typedef double ALCdouble;

/** ALC 32bit type. */
typedef int ALCsizei;

/** ALC void type */
typedef void ALCvoid;

/* Enumerant values begin at column 50. No tabs. */

/* bad value */
#define ALC_INVALID                              0

/* Boolean False. */
#define ALC_FALSE                                0

/* Boolean True. */
#define ALC_TRUE                                 1

/**
 * followed by <int> Hz
 */
#define ALC_FREQUENCY                            0x1007

/**
 * followed by <int> Hz
 */
#define ALC_REFRESH                              0x1008

/**
 * followed by AL_TRUE, AL_FALSE
 */
#define ALC_SYNC                                 0x1009

/**
 * followed by <int> Num of requested Mono (3D) Sources
 */
#define ALC_MONO_SOURCES                         0x1010

/**
 * followed by <int> Num of requested Stereo Sources
 */
#define ALC_STEREO_SOURCES                       0x1011

/**
 * errors
 */

/**
 * No error
 */
#define ALC_NO_ERROR                             ALC_FALSE

/**
 * No device
 */
#define ALC_INVALID_DEVICE                       0xA001

/**
 * invalid context ID
 */
#define ALC_INVALID_CONTEXT                      0xA002

/**
 * bad enum
 */
#define ALC_INVALID_ENUM                         0xA003

/**
 * bad value
 */
#define ALC_INVALID_VALUE                        0xA004

/**
 * Out of memory.
 */
#define ALC_OUT_OF_MEMORY                        0xA005



/**
 * The Specifier string for default device
 */
#define ALC_DEFAULT_DEVICE_SPECIFIER             0x1004
#define ALC_DEVICE_SPECIFIER                     0x1005
#define ALC_EXTENSIONS                           0x1006

#define ALC_MAJOR_VERSION                        0x1000
#define ALC_MINOR_VERSION                        0x1001

#define ALC_ATTRIBUTES_SIZE                      0x1002
#define ALC_ALL_ATTRIBUTES                       0x1003

/**
 * Capture extension
 */
#define ALC_CAPTURE_DEVICE_SPECIFIER             0x310
#define ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER     0x311
#define ALC_CAPTURE_SAMPLES                      0x312


#endif /* _ALCTYPES_H */

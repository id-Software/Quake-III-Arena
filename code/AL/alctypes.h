#ifndef _ALCTYPES_H_
#define _ALCTYPES_H_

#if !defined(_WIN32)
struct _AL_device;
typedef struct _AL_device ALCdevice;

typedef void ALCcontext;
#endif /* _WIN32 */

typedef int ALCenum;

/* Enumerant values begin at column 50. No tabs. */

/* bad value */
#define ALC_INVALID                              0

/**
 * followed by <int> Hz
 */
#define ALC_FREQUENCY                            0x100

/**
 * followed by <int> Hz
 */
#define ALC_REFRESH                              0x101

/**
 * followed by AL_TRUE, AL_FALSE
 */
#define ALC_SYNC                                 0x102

/**
 * errors
 */

/**
 * No error
 */
#define ALC_NO_ERROR                             0

/**
 * No device
 */
#define ALC_INVALID_DEVICE                       0x200

/**
 * invalid context ID
 */
#define ALC_INVALID_CONTEXT                      0x201

/**
 * bad enum
 */
#define ALC_INVALID_ENUM                         0x202

/**
 * bad value
 */
#define ALC_INVALID_VALUE                        0x203

/**
 * Out of memory.
 */
#define ALC_OUT_OF_MEMORY                        0x204



/**
 * The Specifier string for default device
 */
#define ALC_DEFAULT_DEVICE_SPECIFIER             0x300
#define ALC_DEVICE_SPECIFIER                     0x301
#define ALC_EXTENSIONS                           0x302

#define ALC_MAJOR_VERSION                        0x303
#define ALC_MINOR_VERSION                        0x304

#define ALC_ATTRIBUTES_SIZE                      0x305
#define ALC_ALL_ATTRIBUTES                       0x306

/**
 * Not sure if the following are conformant
 */
#define ALC_FALSE                                0
#define ALC_TRUE                                 (!(ALC_FALSE))

#endif /* _ALCTYPES_H */

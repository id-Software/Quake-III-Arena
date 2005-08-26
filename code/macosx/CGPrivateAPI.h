/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


typedef unsigned long CGSNotificationType;
typedef void * CGSNotificationData;
typedef void * CGSNotificationArg;
typedef void * CGSWindowID;
typedef void * CGSConnectionID;

typedef unsigned long long      CGSUInt64;
typedef long long               CGSInt64;
typedef unsigned long           CGSUInt32;
typedef long                    CGSInt32;
typedef unsigned short          CGSUInt16;
typedef short                   CGSInt16;
typedef unsigned char           CGSUInt8;
typedef char                    CGSInt8;
typedef float                   CGSFloat32;

typedef CGSUInt32 CGSByteCount;
typedef CGSUInt16 CGSEventRecordVersion;
typedef unsigned long CGSEventType;
typedef CGSUInt64 CGSEventRecordTime;  /* nanosecond timer */
typedef unsigned long CGSEventFlag;
typedef CGSUInt32  CGSError;


typedef	union {
    struct {	/* For mouse events */
        CGSUInt8	subx;			/* sub-pixel position for x */
        CGSUInt8	suby;			/* sub-pixel position for y */
        CGSInt16	eventNum;		/* unique identifier for this button */
        CGSInt32	click;			/* click state of this event */
        CGSUInt8	pressure;	/* pressure value: 0=none, 255=full */
        CGSInt8		_reserved1;
        CGSInt16	_reserved2;
        CGSInt16	deltaX;
        CGSInt16	deltaY;
        CGSInt32	_padding[8];
    } mouse;
    struct {	/* For pointer movement events */
        CGSInt16	_obsolete_deltaX;	/* Revert to subX, subY, eventNum */
        CGSInt16	_obsolete_deltaY;	/* for Gonzo 1H */
        CGSInt32	click;			/* click state of this event */
        CGSUInt8	pressure;	/* pressure value: 0=none, 255=full */
        CGSInt8		_reserved1;
        CGSInt16	_reserved2;
        CGSInt16	deltaX;
        CGSInt16	deltaY;
        CGSInt32	_padding[8];
    } move;
    struct {	/* For key-down and key-up events */
        CGSInt16	reserved;
        CGSInt16	repeat;		/* for key-down: nonzero if really a repeat */
        CGSUInt16	charSet;	/* character set code */
        CGSUInt16	charCode;	/* character code in that set */
        CGSUInt16	keyCode;	/* device-dependent virtual key code */
        CGSInt16	keyData;	/* device-dependent info */
        CGSInt16	specialKey;	/* CPSSpecialKeyID if kCGSFlagsMaskSpecialKey is set */
        CGSInt16	_pad;
        CGSInt32	_padding[8];
    } key;
    struct {	/* For mouse-entered and mouse-exited events */
        CGSInt16	reserved;
        CGSInt16	eventNum;	/* unique identifier from mouse down event */
        CGSInt32	trackingNum;	/* unique identifier from settrackingrect */
        CGSInt32	userData;	/* unCGSInt32erpreted CGSInt32eger from settrackingrect */
        CGSInt32	_padding[9];
    } tracking;
    struct {	/* For process-related events */
        CGSUInt16	notifyCode;	/* CPSNotificationCodes in CPSProcesses.h */
        CGSUInt16	flags;		/* CPSEventFlags in CPSProcesses.h */
        CGSUInt32	targetHiPSN;	/* hiword of PSN */
        CGSUInt32	targetLoPSN;	/* loword of PSN */
        CGSInt32	status;		/* operation result */
        CGSInt32	_padding[8];
    } process;
    struct {		/* For scroll wheel events */
        CGSInt16	deltaAxis1;
        CGSInt16	deltaAxis2;
        CGSInt16	deltaAxis3;
        CGSInt16	reserved1;
        CGSInt32	reserved2;
        CGSInt32	_padding[9];
    } scrollWheel;
    struct {
        CGSInt32	x;	/* absolute x coordinate in tablet space at full tablet resolution */
        CGSInt32	y;	/* absolute y coordinate in tablet space at full tablet resolution */
        CGSInt32	z;	/* absolute z coordinate in tablet space at full tablet resolution */
        CGSUInt16	buttons;	/* one bit per button - bit 0 is first button - 1 = closed */
        CGSUInt16	pressure;	/* scaled pressure value; MAXPRESSURE=(2^16)-1, MINPRESSURE=0 */
        struct {
            CGSInt16 x;	/* scaled tilt x value; range is -((2^15)-1) to (2^15)-1 (-32767 to 32767) */
            CGSInt16 y;	/* scaled tilt y value; range is -((2^15)-1) to (2^15)-1 (-32767 to 32767) */
        } tilt;
        CGSUInt16	rotation;	/* Fixed-point representation of device rotation in a 10.6 format */
        CGSInt16	tangentialPressure;	/* tangential pressure on the device; range same as tilt */
        CGSUInt16	deviceID;	/* system-assigned unique device ID - matches to deviceID field in proximity event */
        CGSInt16	vendor1;	/* vendor-defined signed 16-bit integer */
        CGSInt16	vendor2;	/* vendor-defined signed 16-bit integer */
        CGSInt16	vendor3;	/* vendor-defined signed 16-bit integer */
        CGSInt32	_padding[4];
    } tablet;
    struct {
        CGSUInt16	vendorID;	/* vendor-defined ID - typically will be USB vendor ID */
        CGSUInt16	tabletID;	/* vendor-defined tablet ID - typically will be USB product ID for the tablet */
        CGSUInt16	pointerID;	/* vendor-defined ID of the specific pointing device */
        CGSUInt16	deviceID;	/* system-assigned unique device ID - matches to deviceID field in tablet event */
        CGSUInt16	systemTabletID;	/* system-assigned unique tablet ID */
        CGSUInt16	vendorPointerType;	/* vendor-defined pointer type */
        CGSUInt32  pointerSerialNumber;	/* vendor-defined serial number of the specific pointing device */
        CGSUInt64	uniqueID;	/* vendor-defined unique ID for this pointer */
        CGSUInt32	capabilityMask;	/* mask representing the capabilities of the device */
        CGSUInt8	pointerType;	/* type of pointing device - enum to be defined */
        CGSUInt8	enterProximity;	/* non-zero = entering; zero = leaving */
        CGSInt16	reserved1;
        CGSInt32	_padding[4];
    } proximity;
    struct {	/* For AppKit-defined, sys-defined, and app-defined events */
        CGSInt16	reserved;
        CGSInt16	subtype;	/* event subtype for compound events */
        union {
            CGSFloat32	F[11];	/* for use in compound events */
            CGSInt32	L[11];	/* for use in compound events */
            CGSInt16	S[22];	/* for use in compound events */
            CGSInt8	C[44];	/* for use in compound events */
        } misc;
    } compound;
} CGSEventRecordData;


struct _CGSEventRecord {
    CGSEventRecordVersion major;
    CGSEventRecordVersion minor;
    CGSByteCount length;	/* Length of complete event record */
    CGSEventType type;		/* An event type from above */
    CGPoint location;		/* Base coordinates (global), from upper-left */
    CGPoint windowLocation;	/* Coordinates relative to window */
    CGSEventRecordTime time;	/* nanoseconds since startup */
    CGSEventFlag flags;		/* key state flags */
    CGSWindowID	window;		/* window number of assigned window */
    CGSConnectionID connection;	/* connection the event came from */
    CGSEventRecordData data;	/* type-dependent data: 40 bytes */
};
typedef struct _CGSEventRecord CGSEventRecord;
typedef CGSEventRecord *CGSEventRecordPtr;


typedef void (*CGSNotifyProcPtr)(CGSNotificationType type,
                                CGSNotificationData data,
                                CGSByteCount dataLength,
                                CGSNotificationArg arg);

// Define a type for the 'CGSRegisterNotifyProc' call.  Don't reference it explicitly since we don't want link errors if Apple removes this private function.
typedef CGSError (*CGSRegisterNotifyProcType)(CGSNotifyProcPtr proc,
                                              CGSNotificationType type,
                                              CGSNotificationArg arg);


#define kCGSEventNotificationMouseMoved                    (710 + 5)
#define kCGSEventNotificationLeftMouseDragged              (710 + 6)
#define kCGSEventNotificationRightMouseDragged             (710 + 7)
#define kCGSEventNotificationNotificationOtherMouseDragged (710 + 27)



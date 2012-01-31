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
#import "macosx_glimp.h"

#include "tr_local.h"
#import "macosx_local.h"
#import "macosx_display.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <mach/mach.h>
#import <mach/mach_error.h>


#warning Using Mach Ports SMP acceleration implementation

/*
===========================================================

SMP acceleration

===========================================================
*/

#import <pthread.h>

#define USE_MACH_PORTS 1

// This is a small cover layer that makes for easier calling

typedef struct _MsgPort {
#if USE_MACH_PORTS    
    mach_port_t port; 
    id nsPort;
#else    
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    volatile unsigned int   status;
    unsigned int   msgCode;
    void          *msgData;
#endif    
} MsgPort;
 
static BOOL portsInited = NO;
static pthread_mutex_t logMutex;

static unsigned int renderMsgOutstanding;
static unsigned int rendererProcessingCommand;

static MsgPort rendererMsgPort;
static MsgPort frontEndMsgPort;

enum {
    MsgNone,
    MsgPending,
};

enum {
    MsgCodeInvalid = 0,
    RenderCommandMsg = 1,
    RenderCompletedMsg = 2,
};

static /*inline*/ void MsgPortInit(MsgPort *port)
{
#if USE_MACH_PORTS  
    port->nsPort = [[NSMachPort alloc] init];
    port->port = [port->nsPort machPort];
      
    //rc = mach_port_allocate(mach_task_self(), MACH_PORT_TYPE_SEND_RECEIVE, &port->port);
    //if (rc) {
    //  fprintf(stderr, "MsgPortInit: mach_port_allocate returned: %d: %s \n",rc, mach_error_string(rc));
   // }
#else
    int rc;
    rc = pthread_mutex_init(&port->mutex, NULL);
    if (rc) {
        ri.Printf(PRINT_ALL, "MsgPortInit: pthread_mutex_init returned: %d: %s\n", rc, strerror(rc));
    }
    rc = pthread_cond_init(&port->condition, NULL);
    if (rc) {
        ri.Printf(PRINT_ALL, "EventInit: pthread_cond_init returned %d: %s\n", rc, strerror(rc));
    }
    port->status = MsgNone;
    port->msgCode = MsgCodeInvalid;
    port->msgData = NULL;
#endif    
}

static /*inline*/ void _SendMsg(MsgPort *port, unsigned int msgCode, void *msgData, 
                         const char *functionName, const char *portName, const char *msgName)
{
    int rc;
    
#if USE_MACH_PORTS
    mach_msg_header_t msg;

    //printf("SendMsg: %s %s %s (%d %08lx)\n",functionName, portName, msgName, msgCode, msgData);
/*
 typedef	struct
 {
   mach_msg_bits_t	msgh_bits;
   mach_msg_size_t	msgh_size;
   mach_port_t		msgh_remote_port;
   mach_port_t		msgh_local_port;
   mach_msg_size_t 	msgh_reserved;
   mach_msg_id_t		msgh_id;
 } mach_msg_header_t;
*/
    msg.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,MACH_MSG_TYPE_MAKE_SEND_ONCE);
    msg.msgh_size=sizeof(msg);
    //msg.msg_type=MSG_TYPE_NORMAL;
    msg.msgh_local_port=MACH_PORT_NULL;
    msg.msgh_remote_port=port->port;
    msg.msgh_reserved = 0;
    msg.msgh_id=(mach_msg_id_t)msgData; // HACK

    rc = mach_msg_send(&msg);
    if(rc) {
        fprintf(stderr,"SendMsg: mach_msg_send returned %d: %s\n", rc, mach_error_string(rc));
    }
#else    
    //printf("SendMsg: %s %s %s (%d %08lx)\n",functionName, portName, msgName, msgCode, msgData);
    rc = pthread_mutex_lock(&port->mutex);
    if(rc) {
        fprintf(stderr,"SendMsg: pthread_mutex_lock returned %d: %s\n", rc, strerror(rc));
    }
    
    /* Block until port is empty */
    while(port->status != MsgNone) {
      //fprintf(stderr, "SendMsg: %s blocking until port %s is empty\n", functionName, portName);      
      rc = pthread_cond_wait(&port->condition, &port->mutex);
      if(rc) {
        fprintf(stderr, "SendMsg: pthread_cond_wait returned %d: %s\n", rc, strerror(rc));
      }
    }
    
    /* Queue msg */
    port->msgCode = msgCode;
    port->msgData = msgData;
    port->status = MsgPending;
    
    /* Unlock port */
    rc = pthread_mutex_unlock(&port->mutex);
    if(rc) {
        fprintf(stderr, "SendMsg: pthread_mutex_unlock returned %d: %s\n", rc, strerror(rc));
    }    

    /* Wake up any threads blocked waiting for a message */
    rc = pthread_cond_broadcast(&port->condition);
    if(rc) {
       fprintf(stderr, "SendMsg: pthread_cond_broadcast returned %d: %s\n", rc, strerror(rc));
    }
#endif            
}

static /*inline*/ void _WaitMsg(MsgPort *port, unsigned int *msgCode, void **msgData, 
                                const char *functionName, const char *portName)
{
    int rc;
#if USE_MACH_PORTS
    mach_msg_empty_rcv_t msg;

    //printf("WaitMsg: %s %s\n",functionName, portName);
    
    msg.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,MACH_MSG_TYPE_MAKE_SEND_ONCE);
    msg.header.msgh_size= sizeof(msg);
    //msg.msg_type=MSG_TYPE_NORMAL;
    msg.header.msgh_local_port=port->port;
    msg.header.msgh_remote_port=MACH_PORT_NULL;
    msg.header.msgh_reserved = 0;
    msg.header.msgh_id=(mach_msg_id_t)msgData; // HACK

    rc = mach_msg_receive(&msg.header);
    if(rc) {
        fprintf(stderr,"SendMsg: mach_msg_receive returned %d: %s\n", rc, mach_error_string(rc));
    }

    *msgData = (void *)msg.header.msgh_id;
    //printf("WaitMsg: %s %s got %08lx\n",functionName, portName, *msgData);
#else   
    //printf("WaitMsg: %s %s\n",functionName, portName);
    
    rc = pthread_mutex_lock(&port->mutex);
    if(rc) {
        fprintf(stderr, "WaitMsg: pthread_mutex_lock returned %d: %s\n", rc, strerror(rc));
    }
    
    /* Block until port is empty */
    while(port->status != MsgPending) {
      rc = pthread_cond_wait(&port->condition, &port->mutex);
      if(rc) {
        fprintf(stderr, "WaitMsg: pthread_cond_wait returned %d: %s\n", rc, strerror(rc));
      }
    }
    
    /* Remove msg */
    *msgCode = port->msgCode;
    *msgData = port->msgData;

    //printf("WaitMsg: %s %s got %d %08lx\n",functionName, portName, *msgCode, *msgData);

    port->status = MsgNone;
    port->msgCode = 0;
    port->msgData = NULL;
    
    rc = pthread_mutex_unlock(&port->mutex);
    if(rc) {
        fprintf(stderr, "WaitMsg: pthread_mutex_unlock returned %d: %s\n", rc, strerror(rc));
    }    

    /* Wake up any threads blocked waiting for port to be empty. */
    rc = pthread_cond_broadcast(&port->condition);
    if(rc) {
       fprintf(stderr, "SendMsg: pthread_cond_broadcast returned %d: %s\n", rc, strerror(rc));
    }
#endif            
}


#define SendMsg(p, c, d) _SendMsg(p, c, d, __PRETTY_FUNCTION__, #p, #c)
#define WaitMsg(p, c, d) _WaitMsg(p, c, d, __PRETTY_FUNCTION__, #p)

#if 0
static void _Log(const char *msg)
{
    int rc;
    
    rc = pthread_mutex_lock(&logMutex);
    if (rc)
        ri.Printf(PRINT_ALL, "_Log: pthread_mutex_lock returned %d: %s\n", rc, strerror(rc));

    fputs(msg,stderr);
    fflush(stderr);
    
    rc = pthread_mutex_unlock(&logMutex);
    if (rc)
        ri.Printf(PRINT_ALL, "_Log: pthread_mutex_unlock returned %d: %s\n", rc, strerror(rc));
}
#endif


//
// The main Q3 SMP API
//

static void (*glimpRenderThread)( void ) = NULL;

static void *GLimp_RenderThreadWrapper(void *arg)
{
    Com_Printf("Render thread starting\n");

    glimpRenderThread();

#ifndef USE_CGLMACROS
    // Unbind the context before we die
    OSX_GLContextClearCurrent();
#endif

    // Send one last message back to front end before we die...
    // This is somewhat of a hack.. fixme.
    if (rendererProcessingCommand) {
        SendMsg(&frontEndMsgPort, RenderCompletedMsg, NULL);
        rendererProcessingCommand = NO;
    }

    Com_Printf("Render thread terminating\n");
	
    return arg;
}

qboolean GLimp_SpawnRenderThread( void (*function)( void ) )
{
    pthread_t renderThread;
    int       rc;

    if (!portsInited) {
        portsInited = YES;
        MsgPortInit(&rendererMsgPort);
        MsgPortInit(&frontEndMsgPort);
        renderMsgOutstanding = NO;
        rendererProcessingCommand = NO;
        pthread_mutex_init(&logMutex, NULL);
    }
    
    glimpRenderThread = function;

    rc = pthread_create(&renderThread,
                        NULL, // attributes
                        GLimp_RenderThreadWrapper,
                        NULL); // argument
    if (rc) {
        ri.Printf(PRINT_ALL, "pthread_create returned %d: %s", rc, strerror(rc));
        return qfalse;
    } else {
        rc = pthread_detach(renderThread);
        if (rc) {
            ri.Printf(PRINT_ALL, "pthread_detach returned %d: %s", rc, strerror(rc));
        }
    }

    return qtrue;
}

static	volatile void	*smpData;

// TJW - This is calling in the rendering thread to wait until another
// command buffer is ready.  The command buffer returned might be NULL,
// indicating that the rendering thread should exit.
void *GLimp_RendererSleep(void)
{
    //_Log(__PRETTY_FUNCTION__ " entered");
    unsigned int msgCode;
    void *msgData;
    
    GLSTAMP("GLimp_RendererSleep start", 0);

#ifndef USE_CGLMACROS
    // Clear the current context while we sleep so the main thread can access it
    OSX_GLContextClearCurrent();
#endif

    // Let the main thread we are idle and that no work is queued
    //_Log("rs0\n");
    /* If we actually had some work to do, then tell the front end we completed it. */
    if (rendererProcessingCommand) {
        SendMsg(&frontEndMsgPort, RenderCompletedMsg, NULL);
        rendererProcessingCommand = NO;
    }
    
    // Wait for new msg
    for (;;) {
        WaitMsg(&rendererMsgPort, &msgCode, &msgData);
        if (1 || msgCode == RenderCommandMsg) {
            smpData = msgData;
            break;
        } else {
            printf("renderer received unknown message: %d\n",msgCode);
        }
    }
    
#ifndef USE_CGLMACROS
    // We are going to render a frame... retake the context
    OSX_GLContextSetCurrent();
#endif

    rendererProcessingCommand = YES;
    
    GLSTAMP("GLimp_RendererSleep end", 0);

    return (void *)smpData;
}


// TJW - This is from the main thread to wait until the rendering thread
// has completed the command buffer that it has
void GLimp_FrontEndSleep(void)
{
    unsigned int msgCode;
    void *msgData;
    
    GLSTAMP("GLimp_FrontEndSleep start", 1);

    if (renderMsgOutstanding) {
        for (;;) {
            WaitMsg(&frontEndMsgPort, &msgCode, &msgData);
            if(1 || msgCode == RenderCompletedMsg) {
                break;
            } else {
                printf("front end received unknown message: %d\n",msgCode);
            }
        }
        renderMsgOutstanding = NO;
    }

#ifndef USE_CGLMACROS
    // We are done waiting for the background thread, take the current context back.
    OSX_GLContextSetCurrent();
#endif

    GLSTAMP("GLimp_FrontEndSleep end", 1);
}


// TJW - This is called in the main thread to issue another command
// buffer to the rendering thread.  This is always called AFTER
// GLimp_FrontEndSleep, so we know that there is no command
// pending in 'smpData'.
void GLimp_WakeRenderer( void *data )
{
    GLSTAMP("GLimp_WakeRenderer start", 1);

#ifndef USE_CGLMACROS
    // We want the background thread to draw stuff.  Give up the current context
    OSX_GLContextClearCurrent();
#endif

    SendMsg(&rendererMsgPort, RenderCommandMsg, data);   
	
    // Don't set flag saying that the renderer is processing something if it's just
    // being told to exit.
    //if(data != NULL)     
    renderMsgOutstanding = YES;

    GLSTAMP("GLimp_WakeRenderer end", 1);
}

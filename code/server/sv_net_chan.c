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

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "server.h"

/*
==============
SV_Netchan_Encode

	// first four bytes of the data are always:
	long reliableAcknowledge;

==============
*/
static void SV_Netchan_Encode( client_t *client, msg_t *msg ) {
	long reliableAcknowledge, i, index;
	byte key, *string;
        int	srdc, sbit, soob;
        
	if ( msg->cursize < SV_ENCODE_START ) {
		return;
	}

        srdc = msg->readcount;
        sbit = msg->bit;
        soob = msg->oob;
        
        msg->bit = 0;
        msg->readcount = 0;
        msg->oob = 0;
        
	reliableAcknowledge = MSG_ReadLong(msg);

        msg->oob = soob;
        msg->bit = sbit;
        msg->readcount = srdc;
        
	string = (byte *)client->lastClientCommandString;
	index = 0;
	// xor the client challenge with the netchan sequence number
	key = client->challenge ^ client->netchan.outgoingSequence;
	for (i = SV_ENCODE_START; i < msg->cursize; i++) {
		// modify the key with the last received and with this message acknowledged client command
		if (!string[index])
			index = 0;
		if (string[index] > 127 || string[index] == '%') {
			key ^= '.' << (i & 1);
		}
		else {
			key ^= string[index] << (i & 1);
		}
		index++;
		// encode the data with this key
		*(msg->data + i) = *(msg->data + i) ^ key;
	}
}

/*
==============
SV_Netchan_Decode

	// first 12 bytes of the data are always:
	long serverId;
	long messageAcknowledge;
	long reliableAcknowledge;

==============
*/
static void SV_Netchan_Decode( client_t *client, msg_t *msg ) {
	int serverId, messageAcknowledge, reliableAcknowledge;
	int i, index, srdc, sbit, soob;
	byte key, *string;

        srdc = msg->readcount;
        sbit = msg->bit;
        soob = msg->oob;
        
        msg->oob = 0;
        
        serverId = MSG_ReadLong(msg);
	messageAcknowledge = MSG_ReadLong(msg);
	reliableAcknowledge = MSG_ReadLong(msg);

        msg->oob = soob;
        msg->bit = sbit;
        msg->readcount = srdc;
        
	string = (byte *)client->reliableCommands[ reliableAcknowledge & (MAX_RELIABLE_COMMANDS-1) ];
	index = 0;
	//
	key = client->challenge ^ serverId ^ messageAcknowledge;
	for (i = msg->readcount + SV_DECODE_START; i < msg->cursize; i++) {
		// modify the key with the last sent and acknowledged server command
		if (!string[index])
			index = 0;
		if (string[index] > 127 || string[index] == '%') {
			key ^= '.' << (i & 1);
		}
		else {
			key ^= string[index] << (i & 1);
		}
		index++;
		// decode the data with this key
		*(msg->data + i) = *(msg->data + i) ^ key;
	}
}

/*
=================
SV_Netchan_TransmitNextFragment
=================
*/
void SV_Netchan_TransmitNextFragment( client_t *client ) {
	Netchan_TransmitNextFragment( &client->netchan );
	if (!client->netchan.unsentFragments)
	{
		// make sure the netchan queue has been properly initialized (you never know)
		if (!client->netchan_end_queue) {
			Com_Error(ERR_DROP, "netchan queue is not properly initialized in SV_Netchan_TransmitNextFragment\n");
		}
		// the last fragment was transmitted, check wether we have queued messages
		if (client->netchan_start_queue) {
			netchan_buffer_t *netbuf;
			Com_DPrintf("#462 Netchan_TransmitNextFragment: popping a queued message for transmit\n");
			netbuf = client->netchan_start_queue;
			SV_Netchan_Encode( client, &netbuf->msg );
			Netchan_Transmit( &client->netchan, netbuf->msg.cursize, netbuf->msg.data );
			// pop from queue
			client->netchan_start_queue = netbuf->next;
			if (!client->netchan_start_queue) {
				Com_DPrintf("#462 Netchan_TransmitNextFragment: emptied queue\n");
				client->netchan_end_queue = &client->netchan_start_queue;
			}
			else
				Com_DPrintf("#462 Netchan_TransmitNextFragment: remaining queued message\n");
			Z_Free(netbuf);
		} 
	}	
}


/*
===============
SV_Netchan_Transmit
TTimo
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=462
if there are some unsent fragments (which may happen if the snapshots
and the gamestate are fragmenting, and collide on send for instance)
then buffer them and make sure they get sent in correct order
================
*/

void SV_Netchan_Transmit( client_t *client, msg_t *msg) {	//int length, const byte *data ) {
	MSG_WriteByte( msg, svc_EOF );
	if (client->netchan.unsentFragments) {
		netchan_buffer_t *netbuf;
		Com_DPrintf("#462 SV_Netchan_Transmit: unsent fragments, stacked\n");
		netbuf = (netchan_buffer_t *)Z_Malloc(sizeof(netchan_buffer_t));
		// store the msg, we can't store it encoded, as the encoding depends on stuff we still have to finish sending
		MSG_Copy(&netbuf->msg, netbuf->msgBuffer, sizeof( netbuf->msgBuffer ), msg);
		netbuf->next = NULL;
		// insert it in the queue, the message will be encoded and sent later
		*client->netchan_end_queue = netbuf;
		client->netchan_end_queue = &(*client->netchan_end_queue)->next;
		// emit the next fragment of the current message for now
		Netchan_TransmitNextFragment(&client->netchan);
	} else {
		SV_Netchan_Encode( client, msg );
		Netchan_Transmit( &client->netchan, msg->cursize, msg->data );
	}
}

/*
=================
Netchan_SV_Process
=================
*/
qboolean SV_Netchan_Process( client_t *client, msg_t *msg ) {
	int ret;
	ret = Netchan_Process( &client->netchan, msg );
	if (!ret)
		return qfalse;
	SV_Netchan_Decode( client, msg );
	return qtrue;
}


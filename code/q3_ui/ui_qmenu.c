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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
/**********************************************************************
	UI_QMENU.C

	Quake's menu framework system.
**********************************************************************/
#include "ui_local.h"

sfxHandle_t menu_in_sound;
sfxHandle_t menu_move_sound;
sfxHandle_t menu_out_sound;
sfxHandle_t menu_buzz_sound;
sfxHandle_t menu_null_sound;
sfxHandle_t weaponChangeSound;

static qhandle_t	sliderBar;
static qhandle_t	sliderButton_0;
static qhandle_t	sliderButton_1;

vec4_t menu_text_color	    = {1.0f, 1.0f, 1.0f, 1.0f};
vec4_t menu_dim_color       = {0.0f, 0.0f, 0.0f, 0.75f};
vec4_t color_black	    = {0.00f, 0.00f, 0.00f, 1.00f};
vec4_t color_white	    = {1.00f, 1.00f, 1.00f, 1.00f};
vec4_t color_yellow	    = {1.00f, 1.00f, 0.00f, 1.00f};
vec4_t color_blue	    = {0.00f, 0.00f, 1.00f, 1.00f};
vec4_t color_lightOrange    = {1.00f, 0.68f, 0.00f, 1.00f };
vec4_t color_orange	    = {1.00f, 0.43f, 0.00f, 1.00f};
vec4_t color_red	    = {1.00f, 0.00f, 0.00f, 1.00f};
vec4_t color_dim	    = {0.00f, 0.00f, 0.00f, 0.25f};

// current color scheme
vec4_t pulse_color          = {1.00f, 1.00f, 1.00f, 1.00f};
vec4_t text_color_disabled  = {0.50f, 0.50f, 0.50f, 1.00f};	// light gray
vec4_t text_color_normal    = {1.00f, 0.43f, 0.00f, 1.00f};	// light orange
vec4_t text_color_highlight = {1.00f, 1.00f, 0.00f, 1.00f};	// bright yellow
vec4_t listbar_color        = {1.00f, 0.43f, 0.00f, 0.30f};	// transluscent orange
vec4_t text_color_status    = {1.00f, 1.00f, 1.00f, 1.00f};	// bright white	

// action widget
static void	Action_Init( menuaction_s *a );
static void	Action_Draw( menuaction_s *a );

// radio button widget
static void	RadioButton_Init( menuradiobutton_s *rb );
static void	RadioButton_Draw( menuradiobutton_s *rb );
static sfxHandle_t RadioButton_Key( menuradiobutton_s *rb, int key );

// slider widget
static void Slider_Init( menuslider_s *s );
static sfxHandle_t Slider_Key( menuslider_s *s, int key );
static void	Slider_Draw( menuslider_s *s );

// spin control widget
static void	SpinControl_Init( menulist_s *s );
static void	SpinControl_Draw( menulist_s *s );
static sfxHandle_t SpinControl_Key( menulist_s *l, int key );

// text widget
static void Text_Init( menutext_s *b );
static void Text_Draw( menutext_s *b );

// scrolllist widget
static void	ScrollList_Init( menulist_s *l );
sfxHandle_t ScrollList_Key( menulist_s *l, int key );

// proportional text widget
static void PText_Init( menutext_s *b );
static void PText_Draw( menutext_s *b );

// proportional banner text widget
static void BText_Init( menutext_s *b );
static void BText_Draw( menutext_s *b );

/*
=================
Text_Init
=================
*/
static void Text_Init( menutext_s *t )
{
	t->generic.flags |= QMF_INACTIVE;
}

/*
=================
Text_Draw
=================
*/
static void Text_Draw( menutext_s *t )
{
	int		x;
	int		y;
	char	buff[512];	
	float*	color;

	x = t->generic.x;
	y = t->generic.y;

	buff[0] = '\0';

	// possible label
	if (t->generic.name)
		strcpy(buff,t->generic.name);

	// possible value
	if (t->string)
		strcat(buff,t->string);
		
	if (t->generic.flags & QMF_GRAYED)
		color = text_color_disabled;
	else
		color = t->color;

	UI_DrawString( x, y, buff, t->style, color );
}

/*
=================
BText_Init
=================
*/
static void BText_Init( menutext_s *t )
{
	t->generic.flags |= QMF_INACTIVE;
}

/*
=================
BText_Draw
=================
*/
static void BText_Draw( menutext_s *t )
{
	int		x;
	int		y;
	float*	color;

	x = t->generic.x;
	y = t->generic.y;

	if (t->generic.flags & QMF_GRAYED)
		color = text_color_disabled;
	else
		color = t->color;

	UI_DrawBannerString( x, y, t->string, t->style, color );
}

/*
=================
PText_Init
=================
*/
static void PText_Init( menutext_s *t )
{
	int	x;
	int	y;
	int	w;
	int	h;
	float	sizeScale;

	sizeScale = UI_ProportionalSizeScale( t->style );

	x = t->generic.x;
	y = t->generic.y;
	w = UI_ProportionalStringWidth( t->string ) * sizeScale;
	h =	PROP_HEIGHT * sizeScale;

	if( t->generic.flags & QMF_RIGHT_JUSTIFY ) {
		x -= w;
	}
	else if( t->generic.flags & QMF_CENTER_JUSTIFY ) {
		x -= w / 2;
	}

	t->generic.left   = x - PROP_GAP_WIDTH * sizeScale;
	t->generic.right  = x + w + PROP_GAP_WIDTH * sizeScale;
	t->generic.top    = y;
	t->generic.bottom = y + h;
}

/*
=================
PText_Draw
=================
*/
static void PText_Draw( menutext_s *t )
{
	int		x;
	int		y;
	float *	color;
	int		style;

	x = t->generic.x;
	y = t->generic.y;

	if (t->generic.flags & QMF_GRAYED)
		color = text_color_disabled;
	else
		color = t->color;

	style = t->style;
	if( t->generic.flags & QMF_PULSEIFFOCUS ) {
		if( Menu_ItemAtCursor( t->generic.parent ) == t ) {
			style |= UI_PULSE;
		}
		else {
			style |= UI_INVERSE;
		}
	}

	UI_DrawProportionalString( x, y, t->string, style, color );
}

/*
=================
Bitmap_Init
=================
*/
void Bitmap_Init( menubitmap_s *b )
{
	int	x;
	int	y;
	int	w;
	int	h;

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height;
	if( w < 0 ) {
		w = -w;
	}
	if( h < 0 ) {
		h = -h;
	}

	if (b->generic.flags & QMF_RIGHT_JUSTIFY)
	{
		x = x - w;
	}
	else if (b->generic.flags & QMF_CENTER_JUSTIFY)
	{
		x = x - w/2;
	}

	b->generic.left   = x;
	b->generic.right  = x + w;
	b->generic.top    = y;
	b->generic.bottom = y + h;

	b->shader      = 0;
	b->focusshader = 0;
}

/*
=================
Bitmap_Draw
=================
*/
void Bitmap_Draw( menubitmap_s *b )
{
	float	x;
	float	y;
	float	w;
	float	h;
	vec4_t	tempcolor;
	float*	color;

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height;

	if (b->generic.flags & QMF_RIGHT_JUSTIFY)
	{
		x = x - w;
	}
	else if (b->generic.flags & QMF_CENTER_JUSTIFY)
	{
		x = x - w/2;
	}

	// used to refresh shader
	if (b->generic.name && !b->shader)
	{
		b->shader = trap_R_RegisterShaderNoMip( b->generic.name );
		if (!b->shader && b->errorpic)
			b->shader = trap_R_RegisterShaderNoMip( b->errorpic );
	}

	if (b->focuspic && !b->focusshader)
		b->focusshader = trap_R_RegisterShaderNoMip( b->focuspic );

	if (b->generic.flags & QMF_GRAYED)
	{
		if (b->shader)
		{
			trap_R_SetColor( colorMdGrey );
			UI_DrawHandlePic( x, y, w, h, b->shader );
			trap_R_SetColor( NULL );
		}
	}
	else
	{
		if (b->shader)
			UI_DrawHandlePic( x, y, w, h, b->shader );

		if (  ( (b->generic.flags & QMF_PULSE) 
			|| (b->generic.flags & QMF_PULSEIFFOCUS) )
		      && (Menu_ItemAtCursor( b->generic.parent ) == b))
		{	
			if (b->focuscolor)			
			{
				tempcolor[0] = b->focuscolor[0];
				tempcolor[1] = b->focuscolor[1];
				tempcolor[2] = b->focuscolor[2];
				color        = tempcolor;	
			}
			else
				color = pulse_color;
			color[3] = 0.5+0.5*sin(uis.realtime/PULSE_DIVISOR);

			trap_R_SetColor( color );
			UI_DrawHandlePic( x, y, w, h, b->focusshader );
			trap_R_SetColor( NULL );
		}
		else if ((b->generic.flags & QMF_HIGHLIGHT) || ((b->generic.flags & QMF_HIGHLIGHT_IF_FOCUS) && (Menu_ItemAtCursor( b->generic.parent ) == b)))
		{	
			if (b->focuscolor)
			{
				trap_R_SetColor( b->focuscolor );
				UI_DrawHandlePic( x, y, w, h, b->focusshader );
				trap_R_SetColor( NULL );
			}
			else
				UI_DrawHandlePic( x, y, w, h, b->focusshader );
		}
	}
}

/*
=================
Action_Init
=================
*/
static void Action_Init( menuaction_s *a )
{
	int	len;

	// calculate bounds
	if (a->generic.name)
		len = strlen(a->generic.name);
	else
		len = 0;

	// left justify text
	a->generic.left   = a->generic.x; 
	a->generic.right  = a->generic.x + len*BIGCHAR_WIDTH;
	a->generic.top    = a->generic.y;
	a->generic.bottom = a->generic.y + BIGCHAR_HEIGHT;
}

/*
=================
Action_Draw
=================
*/
static void Action_Draw( menuaction_s *a )
{
	int		x, y;
	int		style;
	float*	color;

	style = 0;
	color = menu_text_color;
	if ( a->generic.flags & QMF_GRAYED )
	{
		color = text_color_disabled;
	}
	else if (( a->generic.flags & QMF_PULSEIFFOCUS ) && ( a->generic.parent->cursor == a->generic.menuPosition ))
	{
		color = text_color_highlight;
		style = UI_PULSE;
	}
	else if (( a->generic.flags & QMF_HIGHLIGHT_IF_FOCUS ) && ( a->generic.parent->cursor == a->generic.menuPosition ))
	{
		color = text_color_highlight;
	}
	else if ( a->generic.flags & QMF_BLINK )
	{
		style = UI_BLINK;
		color = text_color_highlight;
	}

	x = a->generic.x;
	y = a->generic.y;

	UI_DrawString( x, y, a->generic.name, UI_LEFT|style, color );

	if ( a->generic.parent->cursor == a->generic.menuPosition )
	{
		// draw cursor
		UI_DrawChar( x - BIGCHAR_WIDTH, y, 13, UI_LEFT|UI_BLINK, color);
	}
}

/*
=================
RadioButton_Init
=================
*/
static void RadioButton_Init( menuradiobutton_s *rb )
{
	int	len;

	// calculate bounds
	if (rb->generic.name)
		len = strlen(rb->generic.name);
	else
		len = 0;

	rb->generic.left   = rb->generic.x - (len+1)*SMALLCHAR_WIDTH;
	rb->generic.right  = rb->generic.x + 6*SMALLCHAR_WIDTH;
	rb->generic.top    = rb->generic.y;
	rb->generic.bottom = rb->generic.y + SMALLCHAR_HEIGHT;
}

/*
=================
RadioButton_Key
=================
*/
static sfxHandle_t RadioButton_Key( menuradiobutton_s *rb, int key )
{
	switch (key)
	{
		case K_MOUSE1:
			if (!(rb->generic.flags & QMF_HASMOUSEFOCUS))
				break;

		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
		case K_ENTER:
		case K_KP_ENTER:
		case K_KP_LEFTARROW:
		case K_LEFTARROW:
		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			rb->curvalue = !rb->curvalue;
			if ( rb->generic.callback )
				rb->generic.callback( rb, QM_ACTIVATED );

			return (menu_move_sound);
	}

	// key not handled
	return 0;
}

/*
=================
RadioButton_Draw
=================
*/
static void RadioButton_Draw( menuradiobutton_s *rb )
{
	int	x;
	int y;
	float *color;
	int	style;
	qboolean focus;

	x = rb->generic.x;
	y = rb->generic.y;

	focus = (rb->generic.parent->cursor == rb->generic.menuPosition);

	if ( rb->generic.flags & QMF_GRAYED )
	{
		color = text_color_disabled;
		style = UI_LEFT|UI_SMALLFONT;
	}
	else if ( focus )
	{
		color = text_color_highlight;
		style = UI_LEFT|UI_PULSE|UI_SMALLFONT;
	}
	else
	{
		color = text_color_normal;
		style = UI_LEFT|UI_SMALLFONT;
	}

	if ( focus )
	{
		// draw cursor
		UI_FillRect( rb->generic.left, rb->generic.top, rb->generic.right-rb->generic.left+1, rb->generic.bottom-rb->generic.top+1, listbar_color ); 
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color);
	}

	if ( rb->generic.name )
		UI_DrawString( x - SMALLCHAR_WIDTH, y, rb->generic.name, UI_RIGHT|UI_SMALLFONT, color );

	if ( !rb->curvalue )
	{
		UI_DrawHandlePic( x + SMALLCHAR_WIDTH, y + 2, 16, 16, uis.rb_off);
		UI_DrawString( x + SMALLCHAR_WIDTH + 16, y, "off", style, color );
	}
	else
	{
		UI_DrawHandlePic( x + SMALLCHAR_WIDTH, y + 2, 16, 16, uis.rb_on );
		UI_DrawString( x + SMALLCHAR_WIDTH + 16, y, "on", style, color );
	}
}

/*
=================
Slider_Init
=================
*/
static void Slider_Init( menuslider_s *s )
{
	int len;

	// calculate bounds
	if (s->generic.name)
		len = strlen(s->generic.name);
	else
		len = 0;

	s->generic.left   = s->generic.x - (len+1)*SMALLCHAR_WIDTH; 
	s->generic.right  = s->generic.x + (SLIDER_RANGE+2+1)*SMALLCHAR_WIDTH;
	s->generic.top    = s->generic.y;
	s->generic.bottom = s->generic.y + SMALLCHAR_HEIGHT;
}

/*
=================
Slider_Key
=================
*/
static sfxHandle_t Slider_Key( menuslider_s *s, int key )
{
	sfxHandle_t	sound;
	int			x;
	int			oldvalue;

	switch (key)
	{
		case K_MOUSE1:
			x           = uis.cursorx - s->generic.x - 2*SMALLCHAR_WIDTH;
			oldvalue    = s->curvalue;
			s->curvalue = (x/(float)(SLIDER_RANGE*SMALLCHAR_WIDTH)) * (s->maxvalue-s->minvalue) + s->minvalue;

			if (s->curvalue < s->minvalue)
				s->curvalue = s->minvalue;
			else if (s->curvalue > s->maxvalue)
				s->curvalue = s->maxvalue;
			if (s->curvalue != oldvalue)
				sound = menu_move_sound;
			else
				sound = 0;
			break;

		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			if (s->curvalue > s->minvalue)
			{
				s->curvalue--;
				sound = menu_move_sound;
			}
			else
				sound = menu_buzz_sound;
			break;			

		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			if (s->curvalue < s->maxvalue)
			{
				s->curvalue++;
				sound = menu_move_sound;
			}
			else
				sound = menu_buzz_sound;
			break;			

		default:
			// key not handled
			sound = 0;
			break;
	}

	if ( sound && s->generic.callback )
		s->generic.callback( s, QM_ACTIVATED );

	return (sound);
}

#if 1
/*
=================
Slider_Draw
=================
*/
static void Slider_Draw( menuslider_s *s ) {
	int			x;
	int			y;
	int			style;
	float		*color;
	int			button;
	qboolean	focus;
	
	x =	s->generic.x;
	y = s->generic.y;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if( s->generic.flags & QMF_GRAYED ) {
		color = text_color_disabled;
		style = UI_SMALLFONT;
	}
	else if( focus ) {
		color  = text_color_highlight;
		style = UI_SMALLFONT | UI_PULSE;
	}
	else {
		color = text_color_normal;
		style = UI_SMALLFONT;
	}

	// draw label
	UI_DrawString( x - SMALLCHAR_WIDTH, y, s->generic.name, UI_RIGHT|style, color );

	// draw slider
	UI_SetColor( color );
	UI_DrawHandlePic( x + SMALLCHAR_WIDTH, y, 96, 16, sliderBar );
	UI_SetColor( NULL );

	// clamp thumb
	if( s->maxvalue > s->minvalue )	{
		s->range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );
		if( s->range < 0 ) {
			s->range = 0;
		}
		else if( s->range > 1) {
			s->range = 1;
		}
	}
	else {
		s->range = 0;
	}

	// draw thumb
	if( style & UI_PULSE) {
		button = sliderButton_1;
	}
	else {
		button = sliderButton_0;
	}

	UI_DrawHandlePic( (int)( x + 2*SMALLCHAR_WIDTH + (SLIDER_RANGE-1)*SMALLCHAR_WIDTH* s->range ) - 2, y - 2, 12, 20, button );
}
#else
/*
=================
Slider_Draw
=================
*/
static void Slider_Draw( menuslider_s *s )
{
	float *color;
	int	style;
	int	i;
	int x;
	int y;
	qboolean focus;
	
	x =	s->generic.x;
	y = s->generic.y;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	style = UI_SMALLFONT;
	if ( s->generic.flags & QMF_GRAYED )
	{
		color = text_color_disabled;
	}
	else if (focus)
	{
		color  = text_color_highlight;
		style |= UI_PULSE;
	}
	else
	{
		color = text_color_normal;
	}

	if ( focus )
	{
		// draw cursor
		UI_FillRect( s->generic.left, s->generic.top, s->generic.right-s->generic.left+1, s->generic.bottom-s->generic.top+1, listbar_color ); 
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color);
	}

	// draw label
	UI_DrawString( x - SMALLCHAR_WIDTH, y, s->generic.name, UI_RIGHT|style, color );

	// draw slider
	UI_DrawChar( x + SMALLCHAR_WIDTH, y, 128, UI_LEFT|style, color);
	for ( i = 0; i < SLIDER_RANGE; i++ )
		UI_DrawChar( x + (i+2)*SMALLCHAR_WIDTH, y, 129, UI_LEFT|style, color);
	UI_DrawChar( x + (i+2)*SMALLCHAR_WIDTH, y, 130, UI_LEFT|style, color);

	// clamp thumb
	if (s->maxvalue > s->minvalue)
	{
		s->range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );
		if ( s->range < 0)
			s->range = 0;
		else if ( s->range > 1)
			s->range = 1;
	}
	else
		s->range = 0;

	// draw thumb
	if (style & UI_PULSE) {
		style &= ~UI_PULSE;
		style |= UI_BLINK;
	}
	UI_DrawChar( (int)( x + 2*SMALLCHAR_WIDTH + (SLIDER_RANGE-1)*SMALLCHAR_WIDTH* s->range ), y, 131, UI_LEFT|style, color);
}
#endif

/*
=================
SpinControl_Init
=================
*/
static void SpinControl_Init( menulist_s *s ) {
	int	len;
	int	l;
	const char* str;

	if (s->generic.name)
		len = strlen(s->generic.name) * SMALLCHAR_WIDTH;
	else
		len = 0;

	s->generic.left	= s->generic.x - SMALLCHAR_WIDTH - len;

	len = s->numitems = 0;
	while ( (str = s->itemnames[s->numitems]) != 0 )
	{
		l = strlen(str);
		if (l > len)
			len = l;

		s->numitems++;
	}		

	s->generic.top	  =	s->generic.y;
	s->generic.right  =	s->generic.x + (len+1)*SMALLCHAR_WIDTH;
	s->generic.bottom =	s->generic.y + SMALLCHAR_HEIGHT;
}

/*
=================
SpinControl_Key
=================
*/
static sfxHandle_t SpinControl_Key( menulist_s *s, int key )
{
	sfxHandle_t	sound;

	sound = 0;
	switch (key)
	{
		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
		case K_MOUSE1:
			s->curvalue++;
			if (s->curvalue >= s->numitems)
				s->curvalue = 0;
			sound = menu_move_sound;
			break;
		
		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			s->curvalue--;
			if (s->curvalue < 0)
				s->curvalue = s->numitems-1;
			sound = menu_move_sound;
			break;
	}

	if ( sound && s->generic.callback )
		s->generic.callback( s, QM_ACTIVATED );

	return (sound);
}

/*
=================
SpinControl_Draw
=================
*/
static void SpinControl_Draw( menulist_s *s )
{
	float *color;
	int	x,y;
	int	style;
	qboolean focus;

	x = s->generic.x;
	y =	s->generic.y;

	style = UI_SMALLFONT;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if ( s->generic.flags & QMF_GRAYED )
		color = text_color_disabled;
	else if ( focus )
	{
		color = text_color_highlight;
		style |= UI_PULSE;
	}
	else if ( s->generic.flags & QMF_BLINK )
	{
		color = text_color_highlight;
		style |= UI_BLINK;
	}
	else
		color = text_color_normal;

	if ( focus )
	{
		// draw cursor
		UI_FillRect( s->generic.left, s->generic.top, s->generic.right-s->generic.left+1, s->generic.bottom-s->generic.top+1, listbar_color ); 
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color);
	}

	UI_DrawString( x - SMALLCHAR_WIDTH, y, s->generic.name, style|UI_RIGHT, color );
	UI_DrawString( x + SMALLCHAR_WIDTH, y, s->itemnames[s->curvalue], style|UI_LEFT, color );
}

/*
=================
ScrollList_Init
=================
*/
static void ScrollList_Init( menulist_s *l )
{
	int		w;

	l->oldvalue = 0;
	l->curvalue = 0;
	l->top      = 0;

	if( !l->columns ) {
		l->columns = 1;
		l->seperation = 0;
	}
	else if( !l->seperation ) {
		l->seperation = 3;
	}

	w = ( (l->width + l->seperation) * l->columns - l->seperation) * SMALLCHAR_WIDTH;

	l->generic.left   =	l->generic.x;
	l->generic.top    = l->generic.y;	
	l->generic.right  =	l->generic.x + w;
	l->generic.bottom =	l->generic.y + l->height * SMALLCHAR_HEIGHT;

	if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
		l->generic.left -= w / 2;
		l->generic.right -= w / 2;
	}
}

/*
=================
ScrollList_Key
=================
*/
sfxHandle_t ScrollList_Key( menulist_s *l, int key )
{
	int	x;
	int	y;
	int	w;
	int	i;
	int	j;	
	int	c;
	int	cursorx;
	int	cursory;
	int	column;
	int	index;

	switch (key)
	{
		case K_MOUSE1:
			if (l->generic.flags & QMF_HASMOUSEFOCUS)
			{
				// check scroll region
				x = l->generic.x;
				y = l->generic.y;
				w = ( (l->width + l->seperation) * l->columns - l->seperation) * SMALLCHAR_WIDTH;
				if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
					x -= w / 2;
				}
				if (UI_CursorInRect( x, y, w, l->height*SMALLCHAR_HEIGHT ))
				{
					cursorx = (uis.cursorx - x)/SMALLCHAR_WIDTH;
					column = cursorx / (l->width + l->seperation);
					cursory = (uis.cursory - y)/SMALLCHAR_HEIGHT;
					index = column * l->height + cursory;
					if (l->top + index < l->numitems)
					{
						l->oldvalue = l->curvalue;
						l->curvalue = l->top + index;

						if (l->oldvalue != l->curvalue && l->generic.callback)
						{
							l->generic.callback( l, QM_GOTFOCUS );
							return (menu_move_sound);
						}
					}
				}
			
				// absorbed, silent sound effect
				return (menu_null_sound);
			}
			break;

		case K_KP_HOME:
		case K_HOME:
			l->oldvalue = l->curvalue;
			l->curvalue = 0;
			l->top      = 0;

			if (l->oldvalue != l->curvalue && l->generic.callback)
			{
				l->generic.callback( l, QM_GOTFOCUS );
				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_KP_END:
		case K_END:
			l->oldvalue = l->curvalue;
			l->curvalue = l->numitems-1;
			if( l->columns > 1 ) {
				c = (l->curvalue / l->height + 1) * l->height;
				l->top = c - (l->columns * l->height);
			}
			else {
				l->top = l->curvalue - (l->height - 1);
			}
			if (l->top < 0)
				l->top = 0;			

			if (l->oldvalue != l->curvalue && l->generic.callback)
			{
				l->generic.callback( l, QM_GOTFOCUS );
				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_PGUP:
		case K_KP_PGUP:
			if( l->columns > 1 ) {
				return menu_null_sound;
			}

			if (l->curvalue > 0)
			{
				l->oldvalue = l->curvalue;
				l->curvalue -= l->height-1;
				if (l->curvalue < 0)
					l->curvalue = 0;
				l->top = l->curvalue;
				if (l->top < 0)
					l->top = 0;

				if (l->generic.callback)
					l->generic.callback( l, QM_GOTFOCUS );

				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_PGDN:
		case K_KP_PGDN:
			if( l->columns > 1 ) {
				return menu_null_sound;
			}

			if (l->curvalue < l->numitems-1)
			{
				l->oldvalue = l->curvalue;
				l->curvalue += l->height-1;
				if (l->curvalue > l->numitems-1)
					l->curvalue = l->numitems-1;
				l->top = l->curvalue - (l->height-1);
				if (l->top < 0)
					l->top = 0;

				if (l->generic.callback)
					l->generic.callback( l, QM_GOTFOCUS );

				return (menu_move_sound);
			}
			return (menu_buzz_sound);

		case K_KP_UPARROW:
		case K_UPARROW:
			if( l->curvalue == 0 ) {
				return menu_buzz_sound;
			}

			l->oldvalue = l->curvalue;
			l->curvalue--;

			if( l->curvalue < l->top ) {
				if( l->columns == 1 ) {
					l->top--;
				}
				else {
					l->top -= l->height;
				}
			}

			if( l->generic.callback ) {
				l->generic.callback( l, QM_GOTFOCUS );
			}

			return (menu_move_sound);

		case K_KP_DOWNARROW:
		case K_DOWNARROW:
			if( l->curvalue == l->numitems - 1 ) {
				return menu_buzz_sound;
			}

			l->oldvalue = l->curvalue;
			l->curvalue++;

			if( l->curvalue >= l->top + l->columns * l->height ) {
				if( l->columns == 1 ) {
					l->top++;
				}
				else {
					l->top += l->height;
				}
			}

			if( l->generic.callback ) {
				l->generic.callback( l, QM_GOTFOCUS );
			}

			return menu_move_sound;

		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			if( l->columns == 1 ) {
				return menu_null_sound;
			}

			if( l->curvalue < l->height ) {
				return menu_buzz_sound;
			}

			l->oldvalue = l->curvalue;
			l->curvalue -= l->height;

			if( l->curvalue < l->top ) {
				l->top -= l->height;
			}

			if( l->generic.callback ) {
				l->generic.callback( l, QM_GOTFOCUS );
			}

			return menu_move_sound;

		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			if( l->columns == 1 ) {
				return menu_null_sound;
			}

			c = l->curvalue + l->height;

			if( c >= l->numitems ) {
				return menu_buzz_sound;
			}

			l->oldvalue = l->curvalue;
			l->curvalue = c;

			if( l->curvalue > l->top + l->columns * l->height - 1 ) {
				l->top += l->height;
			}

			if( l->generic.callback ) {
				l->generic.callback( l, QM_GOTFOCUS );
			}

			return menu_move_sound;
	}

	// cycle look for ascii key inside list items
	if ( !Q_isprint( key ) )
		return (0);

	// force to lower for case insensitive compare
	if ( Q_isupper( key ) )
	{
		key -= 'A' - 'a';
	}

	// iterate list items
	for (i=1; i<=l->numitems; i++)
	{
		j = (l->curvalue + i) % l->numitems;
		c = l->itemnames[j][0];
		if ( Q_isupper( c ) )
		{
			c -= 'A' - 'a';
		}

		if (c == key)
		{
			// set current item, mimic windows listbox scroll behavior
			if (j < l->top)
			{
				// behind top most item, set this as new top
				l->top = j;
			}
			else if (j > l->top+l->height-1)
			{
				// past end of list box, do page down
				l->top = (j+1) - l->height;
			}
			
			if (l->curvalue != j)
			{
				l->oldvalue = l->curvalue;
				l->curvalue = j;
				if (l->generic.callback)
					l->generic.callback( l, QM_GOTFOCUS );
				return ( menu_move_sound );			
			}

			return (menu_buzz_sound);
		}
	}

	return (menu_buzz_sound);
}

/*
=================
ScrollList_Draw
=================
*/
void ScrollList_Draw( menulist_s *l )
{
	int			x;
	int			u;
	int			y;
	int			i;
	int			base;
	int			column;
	float*		color;
	qboolean	hasfocus;
	int			style;

	hasfocus = (l->generic.parent->cursor == l->generic.menuPosition);

	x =	l->generic.x;
	for( column = 0; column < l->columns; column++ ) {
		y =	l->generic.y;
		base = l->top + column * l->height;
		for( i = base; i < base + l->height; i++) {
			if (i >= l->numitems)
				break;

			if (i == l->curvalue)
			{
				u = x - 2;
				if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
					u -= (l->width * SMALLCHAR_WIDTH) / 2 + 1;
				}

				UI_FillRect(u,y,l->width*SMALLCHAR_WIDTH,SMALLCHAR_HEIGHT+2,listbar_color);
				color = text_color_highlight;

				if (hasfocus)
					style = UI_PULSE|UI_LEFT|UI_SMALLFONT;
				else
					style = UI_LEFT|UI_SMALLFONT;
			}
			else
			{
				color = text_color_normal;
				style = UI_LEFT|UI_SMALLFONT;
			}
			if( l->generic.flags & QMF_CENTER_JUSTIFY ) {
				style |= UI_CENTER;
			}

			UI_DrawString(
				x,
				y,
				l->itemnames[i],
				style,
				color);

			y += SMALLCHAR_HEIGHT;
		}
		x += (l->width + l->seperation) * SMALLCHAR_WIDTH;
	}
}

/*
=================
Menu_AddItem
=================
*/
void Menu_AddItem( menuframework_s *menu, void *item )
{
	menucommon_s	*itemptr;

	if (menu->nitems >= MAX_MENUITEMS)
		trap_Error ("Menu_AddItem: excessive items");

	menu->items[menu->nitems] = item;
	((menucommon_s*)menu->items[menu->nitems])->parent        = menu;
	((menucommon_s*)menu->items[menu->nitems])->menuPosition  = menu->nitems;
	((menucommon_s*)menu->items[menu->nitems])->flags        &= ~QMF_HASMOUSEFOCUS;

	// perform any item specific initializations
	itemptr = (menucommon_s*)item;
	if (!(itemptr->flags & QMF_NODEFAULTINIT))
	{
		switch (itemptr->type)
		{
			case MTYPE_ACTION:
				Action_Init((menuaction_s*)item);
				break;

			case MTYPE_FIELD:
				MenuField_Init((menufield_s*)item);
				break;

			case MTYPE_SPINCONTROL:
				SpinControl_Init((menulist_s*)item);
				break;

			case MTYPE_RADIOBUTTON:
				RadioButton_Init((menuradiobutton_s*)item);
				break;

			case MTYPE_SLIDER:
				Slider_Init((menuslider_s*)item);
				break;

			case MTYPE_BITMAP:
				Bitmap_Init((menubitmap_s*)item);
				break;

			case MTYPE_TEXT:
				Text_Init((menutext_s*)item);
				break;

			case MTYPE_SCROLLLIST:
				ScrollList_Init((menulist_s*)item);
				break;

			case MTYPE_PTEXT:
				PText_Init((menutext_s*)item);
				break;

			case MTYPE_BTEXT:
				BText_Init((menutext_s*)item);
				break;

			default:
				trap_Error( va("Menu_Init: unknown type %d", itemptr->type) );
		}
	}

	menu->nitems++;
}

/*
=================
Menu_CursorMoved
=================
*/
void Menu_CursorMoved( menuframework_s *m )
{
	void (*callback)( void *self, int notification );
	
	if (m->cursor_prev == m->cursor)
		return;

	if (m->cursor_prev >= 0 && m->cursor_prev < m->nitems)
	{
		callback = ((menucommon_s*)(m->items[m->cursor_prev]))->callback;
		if (callback)
			callback(m->items[m->cursor_prev],QM_LOSTFOCUS);
	}
	
	if (m->cursor >= 0 && m->cursor < m->nitems)
	{
		callback = ((menucommon_s*)(m->items[m->cursor]))->callback;
		if (callback)
			callback(m->items[m->cursor],QM_GOTFOCUS);
	}
}

/*
=================
Menu_SetCursor
=================
*/
void Menu_SetCursor( menuframework_s *m, int cursor )
{
	if (((menucommon_s*)(m->items[cursor]))->flags & (QMF_GRAYED|QMF_INACTIVE))
	{
		// cursor can't go there
		return;
	}

	m->cursor_prev = m->cursor;
	m->cursor      = cursor;

	Menu_CursorMoved( m );
}

/*
=================
Menu_SetCursorToItem
=================
*/
void Menu_SetCursorToItem( menuframework_s *m, void* ptr )
{
	int	i;

	for (i=0; i<m->nitems; i++)
	{
		if (m->items[i] == ptr)
		{
			Menu_SetCursor( m, i );
			return;
		}
	}
}

/*
** Menu_AdjustCursor
**
** This function takes the given menu, the direction, and attempts
** to adjust the menu's cursor so that it's at the next available
** slot.
*/
void Menu_AdjustCursor( menuframework_s *m, int dir ) {
	menucommon_s	*item = NULL;
	qboolean		wrapped = qfalse;

wrap:
	while ( m->cursor >= 0 && m->cursor < m->nitems ) {
		item = ( menucommon_s * ) m->items[m->cursor];
		if (( item->flags & (QMF_GRAYED|QMF_MOUSEONLY|QMF_INACTIVE) ) ) {
			m->cursor += dir;
		}
		else {
			break;
		}
	}

	if ( dir == 1 ) {
		if ( m->cursor >= m->nitems ) {
			if ( m->wrapAround ) {
				if ( wrapped ) {
					m->cursor = m->cursor_prev;
					return;
				}
				m->cursor = 0;
				wrapped = qtrue;
				goto wrap;
			}
			m->cursor = m->cursor_prev;
		}
	}
	else {
		if ( m->cursor < 0 ) {
			if ( m->wrapAround ) {
				if ( wrapped ) {
					m->cursor = m->cursor_prev;
					return;
				}
				m->cursor = m->nitems - 1;
				wrapped = qtrue;
				goto wrap;
			}
			m->cursor = m->cursor_prev;
		}
	}
}

/*
=================
Menu_Draw
=================
*/
void Menu_Draw( menuframework_s *menu )
{
	int				i;
	menucommon_s	*itemptr;

	// draw menu
	for (i=0; i<menu->nitems; i++)
	{
		itemptr = (menucommon_s*)menu->items[i];

		if (itemptr->flags & QMF_HIDDEN)
			continue;

		if (itemptr->ownerdraw)
		{
			// total subclassing, owner draws everything
			itemptr->ownerdraw( itemptr );
		}	
		else 
		{
			switch (itemptr->type)
			{	
				case MTYPE_RADIOBUTTON:
					RadioButton_Draw( (menuradiobutton_s*)itemptr );
					break;

				case MTYPE_FIELD:
					MenuField_Draw( (menufield_s*)itemptr );
					break;
		
				case MTYPE_SLIDER:
					Slider_Draw( (menuslider_s*)itemptr );
					break;
 
				case MTYPE_SPINCONTROL:
					SpinControl_Draw( (menulist_s*)itemptr );
					break;
		
				case MTYPE_ACTION:
					Action_Draw( (menuaction_s*)itemptr );
					break;
		
				case MTYPE_BITMAP:
					Bitmap_Draw( (menubitmap_s*)itemptr );
					break;

				case MTYPE_TEXT:
					Text_Draw( (menutext_s*)itemptr );
					break;

				case MTYPE_SCROLLLIST:
					ScrollList_Draw( (menulist_s*)itemptr );
					break;
				
				case MTYPE_PTEXT:
					PText_Draw( (menutext_s*)itemptr );
					break;

				case MTYPE_BTEXT:
					BText_Draw( (menutext_s*)itemptr );
					break;

				default:
					trap_Error( va("Menu_Draw: unknown type %d", itemptr->type) );
			}
		}
#ifndef NDEBUG
		if( uis.debug ) {
			int	x;
			int	y;
			int	w;
			int	h;

			if( !( itemptr->flags & QMF_INACTIVE ) ) {
				x = itemptr->left;
				y = itemptr->top;
				w = itemptr->right - itemptr->left + 1;
				h =	itemptr->bottom - itemptr->top + 1;

				if (itemptr->flags & QMF_HASMOUSEFOCUS) {
					UI_DrawRect(x, y, w, h, colorYellow );
				}
				else {
					UI_DrawRect(x, y, w, h, colorWhite );
				}
			}
		}
#endif
	}

	itemptr = Menu_ItemAtCursor( menu );
	if ( itemptr && itemptr->statusbar)
		itemptr->statusbar( ( void * ) itemptr );
}

/*
=================
Menu_ItemAtCursor
=================
*/
void *Menu_ItemAtCursor( menuframework_s *m )
{
	if ( m->cursor < 0 || m->cursor >= m->nitems )
		return NULL;

	return m->items[m->cursor];
}

/*
=================
Menu_ActivateItem
=================
*/
sfxHandle_t Menu_ActivateItem( menuframework_s *s, menucommon_s* item ) {
	if ( item->callback ) {
		item->callback( item, QM_ACTIVATED );
		if( !( item->flags & QMF_SILENT ) ) {
			return menu_move_sound;
		}
	}

	return 0;
}

/*
=================
Menu_DefaultKey
=================
*/
sfxHandle_t Menu_DefaultKey( menuframework_s *m, int key )
{
	sfxHandle_t		sound = 0;
	menucommon_s	*item;
	int				cursor_prev;

	// menu system keys
	switch ( key )
	{
		case K_MOUSE2:
		case K_ESCAPE:
			UI_PopMenu();
			return menu_out_sound;
	}

	if (!m || !m->nitems)
		return 0;

	// route key stimulus to widget
	item = Menu_ItemAtCursor( m );
	if (item && !(item->flags & (QMF_GRAYED|QMF_INACTIVE)))
	{
		switch (item->type)
		{
			case MTYPE_SPINCONTROL:
				sound = SpinControl_Key( (menulist_s*)item, key );
				break;

			case MTYPE_RADIOBUTTON:
				sound = RadioButton_Key( (menuradiobutton_s*)item, key );
				break;

			case MTYPE_SLIDER:
				sound = Slider_Key( (menuslider_s*)item, key );
				break;

			case MTYPE_SCROLLLIST:
				sound = ScrollList_Key( (menulist_s*)item, key );
				break;

			case MTYPE_FIELD:
				sound = MenuField_Key( (menufield_s*)item, &key );
				break;
		}

		if (sound) {
			// key was handled
			return sound;		
		}
	}

	// default handling
	switch ( key )
	{
#ifndef NDEBUG
		case K_F11:
			uis.debug ^= 1;
			break;

		case K_F12:
			trap_Cmd_ExecuteText(EXEC_APPEND, "screenshot\n");
			break;
#endif
		case K_KP_UPARROW:
		case K_UPARROW:
			cursor_prev    = m->cursor;
			m->cursor_prev = m->cursor;
			m->cursor--;
			Menu_AdjustCursor( m, -1 );
			if ( cursor_prev != m->cursor ) {
				Menu_CursorMoved( m );
				sound = menu_move_sound;
			}
			break;

		case K_TAB:
		case K_KP_DOWNARROW:
		case K_DOWNARROW:
			cursor_prev    = m->cursor;
			m->cursor_prev = m->cursor;
			m->cursor++;
			Menu_AdjustCursor( m, 1 );
			if ( cursor_prev != m->cursor ) {
				Menu_CursorMoved( m );
				sound = menu_move_sound;
			}
			break;

		case K_MOUSE1:
		case K_MOUSE3:
			if (item)
				if ((item->flags & QMF_HASMOUSEFOCUS) && !(item->flags & (QMF_GRAYED|QMF_INACTIVE)))
					return (Menu_ActivateItem( m, item ));
			break;

		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
		case K_AUX1:
		case K_AUX2:
		case K_AUX3:
		case K_AUX4:
		case K_AUX5:
		case K_AUX6:
		case K_AUX7:
		case K_AUX8:
		case K_AUX9:
		case K_AUX10:
		case K_AUX11:
		case K_AUX12:
		case K_AUX13:
		case K_AUX14:
		case K_AUX15:
		case K_AUX16:
		case K_KP_ENTER:
		case K_ENTER:
			if (item)
				if (!(item->flags & (QMF_MOUSEONLY|QMF_GRAYED|QMF_INACTIVE)))
					return (Menu_ActivateItem( m, item ));
			break;
	}

	return sound;
}

/*
=================
Menu_Cache
=================
*/
void Menu_Cache( void )
{
	uis.charset			= trap_R_RegisterShaderNoMip( "gfx/2d/bigchars" );
	uis.charsetProp		= trap_R_RegisterShaderNoMip( "menu/art/font1_prop.tga" );
	uis.charsetPropGlow	= trap_R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	uis.charsetPropB	= trap_R_RegisterShaderNoMip( "menu/art/font2_prop.tga" );
	uis.cursor          = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	uis.rb_on           = trap_R_RegisterShaderNoMip( "menu/art/switch_on" );
	uis.rb_off          = trap_R_RegisterShaderNoMip( "menu/art/switch_off" );

	uis.whiteShader = trap_R_RegisterShaderNoMip( "white" );
	if ( uis.glconfig.hardwareType == GLHW_RAGEPRO ) {
		// the blend effect turns to shit with the normal 
		uis.menuBackShader	= trap_R_RegisterShaderNoMip( "menubackRagePro" );
	} else {
		uis.menuBackShader	= trap_R_RegisterShaderNoMip( "menuback" );
	}
	uis.menuBackNoLogoShader = trap_R_RegisterShaderNoMip( "menubacknologo" );

	menu_in_sound	= trap_S_RegisterSound( "sound/misc/menu1.wav", qfalse );
	menu_move_sound	= trap_S_RegisterSound( "sound/misc/menu2.wav", qfalse );
	menu_out_sound	= trap_S_RegisterSound( "sound/misc/menu3.wav", qfalse );
	menu_buzz_sound	= trap_S_RegisterSound( "sound/misc/menu4.wav", qfalse );
	weaponChangeSound	= trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );

	// need a nonzero sound, make an empty sound for this
	menu_null_sound = -1;

	sliderBar = trap_R_RegisterShaderNoMip( "menu/art/slider2" );
	sliderButton_0 = trap_R_RegisterShaderNoMip( "menu/art/sliderbutt_0" );
	sliderButton_1 = trap_R_RegisterShaderNoMip( "menu/art/sliderbutt_1" );
}
	

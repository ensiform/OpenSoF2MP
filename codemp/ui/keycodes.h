/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#pragma once

// these are the key numbers that should be passed to KeyEvent

typedef enum
{
	K_TAB = 9,
	K_ENTER = 13,
	K_ESCAPE = 27,
	K_SPACE = 32,
	
	K_PLING,
	K_DOUBLE_QUOTE,
	K_HASH,
	K_STRING,
	K_PERCENT,
	K_AND,
	K_SINGLE_QUOTE,
	K_OPEN_BRACKET,
	K_CLOSE_BRACKET,
	K_STAR,
	K_PLUS,
	K_COMMA,
	K_MINUS,
	K_PERIOD,
	K_FORWARD_SLASH,

	K_0,
	K_1,
	K_2,
	K_3,
	K_4,
	K_5,
	K_6,
	K_7,
	K_8,
	K_9,

	K_COLON,
	K_SEMICOLON,
	K_LESSTHAN,
	K_EQUALS,
	K_GREATERTHAN,
	K_QUESTION,
	K_AT,

	K_CAP_A,
	K_CAP_B,
	K_CAP_C,
	K_CAP_D,
	K_CAP_E,
	K_CAP_F,
	K_CAP_G,
	K_CAP_H,
	K_CAP_I,
	K_CAP_J,
	K_CAP_K,
	K_CAP_L,
	K_CAP_M,
	K_CAP_N,
	K_CAP_O,
	K_CAP_P,
	K_CAP_Q,
	K_CAP_R,
	K_CAP_S,
	K_CAP_T,
	K_CAP_U,
	K_CAP_V,
	K_CAP_W,
	K_CAP_X,
	K_CAP_Y,
	K_CAP_Z,

	K_OPEN_SQUARE,
	K_BACKSLASH,
	K_CLOSE_SQUARE,
	K_CARET,
	K_UNDERSCORE,
	K_LEFT_SINGLE_QUOTE,

	K_LOW_A,
	K_LOW_B,
	K_LOW_C,
	K_LOW_D,
	K_LOW_E,
	K_LOW_F,
	K_LOW_G,
	K_LOW_H,
	K_LOW_I,
	K_LOW_J,
	K_LOW_K,
	K_LOW_L,
	K_LOW_M,
	K_LOW_N,
	K_LOW_O,
	K_LOW_P,
	K_LOW_Q,
	K_LOW_R,
	K_LOW_S,
	K_LOW_T,
	K_LOW_U,
	K_LOW_V,
	K_LOW_W,
	K_LOW_X,
	K_LOW_Y,
	K_LOW_Z,

	K_OPEN_BRACE,
	K_BAR,
	K_CLOSE_BRACE,
	K_TILDE,

	K_BACKSPACE = 127,

	K_COMMAND = 128,
	K_CAPSLOCK,
	K_POWER,
	K_PAUSE,

	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,
	K_SHIFT,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_F13,
	K_F14,
	K_F15,

	K_SCROLL,

	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_MINUS,
	K_KP_PLUS,
	K_KP_NUMLOCK,
	K_KP_STAR,
	K_KP_EQUALS,

	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,

	K_MWHEELDOWN,
	K_MWHEELUP,

	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4,
	K_JOY5,
	K_JOY6,
	K_JOY7,
	K_JOY8,
	K_JOY9,
	K_JOY10,
	K_JOY11,
	K_JOY12,
	K_JOY13,
	K_JOY14,
	K_JOY15,
	K_JOY16,
	K_JOY17,
	K_JOY18,
	K_JOY19,
	K_JOY20,
	K_JOY21,
	K_JOY22,
	K_JOY23,
	K_JOY24,
	K_JOY25,
	K_JOY26,
	K_JOY27,
	K_JOY28,
	K_JOY29,
	K_JOY30,
	K_JOY31,
	K_JOY32,

	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,

	K_CONSOLE,

	K_LAST_KEY		// this had better be <256!
} keyNum_t;


// The menu code needs to get both key and char events, but
// to avoid duplicating the paths, the char events are just
// distinguished by or'ing in K_CHAR_FLAG (ugly)
#define	K_CHAR_FLAG		1024

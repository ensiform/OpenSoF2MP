/*
===========================================================================
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

#include "qcommon/sstring.h"	// stl string class won't compile in here (MS shite), so use Gil's.
#include "tr_local.h"
#include "tr_font.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This file is shared in the single and multiplayer codebases, so be CAREFUL WHAT YOU ADD/CHANGE!!!!!
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////



//================================================
//

class CFontInfo
{
private:
	// From the fontdat file
	glyphInfo_t		mGlyphs[GLYPH_COUNT];

	// end of fontdat data

	int				mShader;   				// handle to the shader with the glyph

public:
	char			m_sFontName[MAX_QPATH];	// eg "fonts/lcd"	// needed for korean font-hint if we need >1 hangul set
	int				mPointSize;
	int				mHeight;
	int				mAscender;
	int				mDescender;

	int				m_iThisFont;	// handle to itself

	CFontInfo(const char *fontName);
//	CFontInfo(int fill) { memset(this, fill, sizeof(*this)); }	// wtf?
	~CFontInfo(void) {}

	const int GetPointSize(void) const { return(mPointSize); }
	const int GetHeight(void) const { return(mHeight); }
	const int GetAscender(void) const { return(mAscender); }
	const int GetDescender(void) const { return(mDescender); }

	const glyphInfo_t *GetLetter(const unsigned int uiLetter, int *piShader = NULL);

	const int GetLetterWidth(const unsigned int uiLetter);
	const int GetLetterHorizAdvance(const unsigned int uiLetter);
	const int GetShader(void) const { return(mShader); }
};

//================================================

int							g_iCurrentFontIndex;	// entry 0 is reserved index for missing/invalid, else ++ with each new font registered
std::vector<CFontInfo *>			g_vFontArray;
typedef std::map<sstring_t, int>	FontIndexMap_t;
							FontIndexMap_t g_mapFontIndexes;
//paletteRGBA_c				lastcolour;

// ============================================================================

// takes char *, returns integer char at that point, and advances char * on by enough bytes to move
//	past the letter (either western 1 byte or Asian multi-byte)...
//
// looks messy, but the actual execution route is quite short, so it's fast...
//
// Note that I have to have this 3-param form instead of advancing a passed-in "const char **psText" because of VM-crap where you can only change ptr-contents, not ptrs themselves. Bleurgh. Ditto the qtrue:qfalse crap instead of just returning stuff straight through.
//
unsigned int AnyLanguage_ReadCharFromString( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation /* = NULL */)
{
	const byte *psString = (const byte *) psText;	// avoid sign-promote bug
	unsigned int uiLetter;

	// ... must not have been an MBCS code...
	//
	uiLetter = psString[0];
	*piAdvanceCount = 1;

	if (pbIsTrailingPunctuation)
	{
		*pbIsTrailingPunctuation = (uiLetter == '!' ||
									uiLetter == '?' ||
									uiLetter == ',' ||
									uiLetter == '.' ||
									uiLetter == ';' ||
									uiLetter == ':'
									) ? qtrue : qfalse;
	}

	return uiLetter;
}


// ======================================================================
// name is (eg) "ergo" or "lcd", no extension.
//
//  If path present, it's a special language hack for SBCS override languages, eg: "lcd/russian", which means
//	  just treat the file as "russian", but with the "lcd" part ensuring we don't find a different registered russian font
//
CFontInfo::CFontInfo(const char *_fontName)
{
	int			len, i;
	void		*buff;
	dfontdat_t	*fontdat;

	// remove any special hack name insertions...
	//
	char fontName[MAX_QPATH];
	sprintf(fontName,"fonts/%s.fontdat",COM_SkipPath(const_cast<char*>(_fontName)));	// COM_SkipPath should take a const char *, but it's just possible people use it as a char * I guess, so I have to hack around like this <groan>

	// clear some general things...
	//
	m_iThisFont = -1;

	len = ri->FS_ReadFile(fontName, NULL);
	if (len == sizeof(dfontdat_t))
	{
		ri->FS_ReadFile(fontName, &buff);
		fontdat = (dfontdat_t *)buff;

		for(i = 0; i < GLYPH_COUNT; i++)
		{
#ifdef Q3_BIG_ENDIAN
			mGlyphs[i].width = LittleShort(fontdat->mGlyphs[i].width);
			mGlyphs[i].height = LittleShort(fontdat->mGlyphs[i].height);
			mGlyphs[i].horizAdvance = LittleShort(fontdat->mGlyphs[i].horizAdvance);
			mGlyphs[i].horizOffset = LittleShort(fontdat->mGlyphs[i].horizOffset);
			mGlyphs[i].baseline = LittleLong(fontdat->mGlyphs[i].baseline);
			mGlyphs[i].s = LittleFloat(fontdat->mGlyphs[i].s);
			mGlyphs[i].t = LittleFloat(fontdat->mGlyphs[i].t);
			mGlyphs[i].s2 = LittleFloat(fontdat->mGlyphs[i].s2);
			mGlyphs[i].t2 = LittleFloat(fontdat->mGlyphs[i].t2);
#else
			mGlyphs[i] = fontdat->mGlyphs[i];
#endif
		}
		mPointSize = LittleShort(fontdat->mPointSize);
		mHeight = LittleShort(fontdat->mHeight);
		mAscender = LittleShort(fontdat->mAscender);
		mDescender = LittleShort(fontdat->mDescender);

		// cope with bad fontdat headers...
		//
		if (mHeight == 0)
		{
			mHeight = mPointSize;
            mAscender = mPointSize - Round( ((float)mPointSize/10.0f)+2 );	// have to completely guess at the baseline... sigh.
            mDescender = mHeight - mAscender;
		}

		ri->FS_FreeFile(buff);
	}
	else
	{
		mHeight = 0;
		mShader = 0;
	}

	Q_strncpyz(m_sFontName, fontName, sizeof(m_sFontName));
	COM_StripExtension( m_sFontName, m_sFontName, sizeof( m_sFontName ) );	// so we get better error printing if failed to load shader (ie lose ".fontdat")
	mShader = RE_RegisterShaderNoMip(m_sFontName);

	// finished...
	g_vFontArray.resize(g_iCurrentFontIndex + 1);
	g_vFontArray[g_iCurrentFontIndex++] = this;
}

static CFontInfo *GetFont_Actual(int index)
{
	index &= SET_MASK;
	if((index >= 0) && (index < g_iCurrentFontIndex))
	{
		CFontInfo *pFont = g_vFontArray[index];
		return pFont;
	}
	return(NULL);
}


// needed to add *piShader param because of multiple TPs,
//	if not passed in, then I also skip S,T calculations for re-usable static asian glyphinfo struct...
//
const glyphInfo_t *CFontInfo::GetLetter(const unsigned int uiLetter, int *piShader /* = NULL */)
{ 	
	if (piShader)
	{
		*piShader = GetShader();
	}

	const glyphInfo_t *pGlyph = &mGlyphs[ uiLetter & 0xff ];
	return pGlyph;
}

const int CFontInfo::GetLetterWidth(unsigned int uiLetter)
{
	const glyphInfo_t *pGlyph = GetLetter( uiLetter );
	return pGlyph->width ? pGlyph->width : mGlyphs[(int)'.'].width;
}

const int CFontInfo::GetLetterHorizAdvance(unsigned int uiLetter)
{
	const glyphInfo_t *pGlyph = GetLetter( uiLetter );
	return pGlyph->horizAdvance ? pGlyph->horizAdvance : mGlyphs[(int)'.'].horizAdvance;
}

CFontInfo *GetFont(int index)
{
	CFontInfo *pFont = GetFont_Actual( index );
	return pFont;
}

int RE_Font_StrLenPixels(const char *psText, const int iFontHandle, const float fScale)
{
	float		maxWidth = 0.f;
	float		thisWidth= 0.f;
	CFontInfo	*curfont;

	curfont = GetFont(iFontHandle);
	if(!curfont)
	{
		return(0);
	}

	while(*psText)
	{
		int iAdvanceCount;
		unsigned int uiLetter = AnyLanguage_ReadCharFromString( psText, &iAdvanceCount, NULL );
		psText += iAdvanceCount;

		if (uiLetter == '^' )
		{
			if (*psText >= '0' &&
				*psText <= '9')
			{
				uiLetter = AnyLanguage_ReadCharFromString( psText, &iAdvanceCount, NULL );
				psText += iAdvanceCount;
				continue;
			}
		}

		if (uiLetter == 0x0A)
		{
			thisWidth = 0.f;
		}
		else
		{
			int iPixelAdvance = curfont->GetLetterHorizAdvance( uiLetter );
	
			float fValue = iPixelAdvance * fScale;
			thisWidth += fValue;
			if (thisWidth > maxWidth)
			{
				maxWidth = thisWidth;
			}
		}
	}

	return Round(maxWidth);
}

int RE_Font_HeightPixels(const int iFontHandle, const float fScale)
{
	CFontInfo	*curfont;

	curfont = GetFont(iFontHandle);
	if(curfont)
	{
		float fValue = curfont->GetPointSize() * fScale;
		return Round(fValue);
	}
	return(0);
}

// limit is 0 for "all of string", else display count...
//
void RE_Font_DrawString(int x, int y, qhandle_t font, float scale, vec4_t color, const char* text, int limit, int flags, int cursorPos, char cursor)
{
	float				curX, curY;
	int					colour;
	const glyphInfo_t	*letter;
	qhandle_t			shader;

	assert (text);

	if(!cursor && (font & STYLE_BLINK))
	{
		if((ri->Milliseconds() >> 7) & 1)
		{
			return;
		}
	}

	CFontInfo *curfont = GetFont(font);
	if(!curfont || !text)
	{
		return;
	}

	RE_SetColor( color );

	curX = x;
	y += (curfont->GetHeight() - curfont->GetDescender()) * scale;

	qboolean nextTextWouldOverflow = qfalse;
	const char *textStart = text;
	while (*text && !nextTextWouldOverflow)
	{
		int advanceCount;
		unsigned int uiLetter = AnyLanguage_ReadCharFromString( text, &advanceCount, NULL );
		text += advanceCount;

		switch( uiLetter )
		{
		case 10:						//linefeed
			curX = x;
			y += curfont->GetPointSize() * scale;
			break;
		case 13:						// Return
			break;
		case 32:						// Space
			letter = curfont->GetLetter(' ');			
			curX += letter->horizAdvance * scale;
			nextTextWouldOverflow = (limit && (text - textStart) > limit) ? qtrue : qfalse;	// yeuch
			break;
		case '^':
			if (*text >= '0' &&
				*text <= '9')
			{
				colour = ColorIndex(*text++);
				RE_SetColor( g_color_table[colour] );
				break;
			}
			//purposely falls thrugh
		default:
			letter = curfont->GetLetter( uiLetter, &shader );			// Description of letter
			if(!letter->width)
			{
				letter = curfont->GetLetter('.');
			}

			float advancePixels = letter->horizAdvance * scale;
			nextTextWouldOverflow = (limit && (text - textStart) > limit) ? qtrue : qfalse;	// yeuch
			if (!nextTextWouldOverflow)
			{
				curY = y - letter->baseline * scale;

				RE_StretchPic ( curX + letter->horizOffset * scale, // float x
								curY,	// float y
								letter->width * scale,	// float w
								letter->height * scale, // float h
								letter->s,						// float s1
								letter->t,						// float t1
								letter->s2,					// float s2
								letter->t2,					// float t2
								//lastcolour.c, 
								shader							// qhandle_t hShader
								);

				curX += advancePixels;
			}

			break;
		}
	}

	if (cursor && (ri->Milliseconds() >> 7) & 1) {
		// draw blinking cursor
		curX = x;
		for ( int i = 0 ; i < cursorPos ; ++i ) {
			letter = curfont->GetLetter( textStart[i] );			// Description of letter
			curX += letter->horizAdvance * scale;
		}
		letter = curfont->GetLetter( cursor, &shader );			// Description of letter
		if(!letter->width)
		{
			letter = curfont->GetLetter('.');
		}
		curY = y - letter->baseline * scale;
		RE_StretchPic ( curX + letter->horizOffset * scale, // float x
						curY,	// float y
						letter->width * scale,	// float w
						letter->height * scale, // float h
						letter->s,						// float s1
						letter->t,						// float t1
						letter->s2,					// float s2
						letter->t2,					// float t2
						//lastcolour.c, 
						shader							// qhandle_t hShader
						);
	}
	RE_SetColor(NULL);
}

int RE_RegisterFont(const char *psName)
{
	FontIndexMap_t::iterator it = g_mapFontIndexes.find(psName);
	if (it != g_mapFontIndexes.end() )
	{
		int iFontIndex = (*it).second;
		return iFontIndex;
	}

	// not registered, so...
	//
	{
		CFontInfo *pFont = new CFontInfo(psName);
		if (pFont->GetPointSize() > 0)
		{
			int iFontIndex = g_iCurrentFontIndex - 1;
			g_mapFontIndexes[psName] = iFontIndex;
			pFont->m_iThisFont = iFontIndex;
			return iFontIndex;
		}
		else
		{
			g_mapFontIndexes[psName] = 0;	// missing/invalid
		}
	}

	return 0;
}

void R_InitFonts(void)
{
	g_iCurrentFontIndex = 0;
}

/*
===============
R_FontList_f
===============
*/
void R_FontList_f( void ) {
	Com_Printf ("------------------------------------\n");

	FontIndexMap_t::iterator it;
	for (it = g_mapFontIndexes.begin(); it != g_mapFontIndexes.end(); ++it)
	{
		CFontInfo *font = GetFont((*it).second);
		if( font )
		{
			Com_Printf("%3i:%s  ps:%hi h:%hi a:%hi d:%hi\n", (*it).second, font->m_sFontName,
				font->mPointSize, font->mHeight, font->mAscender, font->mDescender);
		}
	}
	Com_Printf ("------------------------------------\n");
}

void R_ShutdownFonts(void)
{
	for(int i = 0; i < g_iCurrentFontIndex; i++)
	{
		delete g_vFontArray[i];
	}
	g_mapFontIndexes.clear();
	g_vFontArray.clear();
	g_iCurrentFontIndex = 0;
}

// this is only really for debugging while tinkering with fonts, but harmless to leave in...
//
void R_ReloadFonts_f(void)
{
	// first, grab all the currently-registered fonts IN THE ORDER THEY WERE REGISTERED...
	//
	std::vector <sstring_t> vstrFonts;

	int iFontToFind;
	for (iFontToFind = 0; iFontToFind < g_iCurrentFontIndex; iFontToFind++)
	{
		FontIndexMap_t::iterator it;
		for (it = g_mapFontIndexes.begin(); it != g_mapFontIndexes.end(); ++it)
		{
			if (iFontToFind == (*it).second)
			{
				vstrFonts.push_back( (*it).first );
				break;
			}
		}
		if (it == g_mapFontIndexes.end() )
		{
			break;	// couldn't find this font
		}
	}
	if ( iFontToFind == g_iCurrentFontIndex ) // found all of them?
	{
		// now restart the font system...
		//
		R_ShutdownFonts();
		R_InitFonts();
		//
		// and re-register our fonts in the same order as before (note that some menu items etc cache the string lengths so really a vid_restart is better, but this is just for my testing)
		//
		for (size_t iFont = 0; iFont < vstrFonts.size(); iFont++)
		{
#ifdef _DEBUG
			int iNewFontHandle = RE_RegisterFont( vstrFonts[iFont].c_str() );
			assert( iNewFontHandle == (int)(iFont+1) );
#else
			RE_RegisterFont( vstrFonts[iFont].c_str() );
#endif
		}
		Com_Printf( "Done.\n" );
	}
	else
	{
		Com_Printf( "Problem encountered finding current fonts, ignoring.\n" );	// poo. Oh well, forget it.
	}
}


// end

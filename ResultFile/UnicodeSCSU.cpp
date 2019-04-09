#include "stdafx.h"
#include "UnicodeSCSU.h"

using namespace DFW2;

void CUnicodeSCSU::WriteSCSUSymbol(int Symbol)
{
	char window = windowState;
	int w;
	if (Symbol >= 0 && Symbol <= 0x10ffff)
	{
		if (Symbol > 0xffff)
		{
			/* encode a supplementary code point as a surrogate pair */
			WriteSCSUSymbol(0xd7c0 + (Symbol >> 10));
			WriteSCSUSymbol(0xdc00 + (Symbol & 0x3ff));
			return;
		}
		if (!IsUnicodeMode)
		{
			/* single-byte mode */
			if (Symbol < 0x20)
			{
				/*
				* Encode C0 control code:
				* Check the code point against the bit mask 0010 0110 0000 0001
				* which contains 1-bits at the bit positions corresponding to
				* code points 0D 0A 09 00 (CR LF TAB NUL)
				* which are encoded directly.
				* All other C0 control codes are quoted with SQ0.
				*/
				if (Symbol <= 0xf && ((1 << Symbol) & 0x2601) == 0)
					Out(SQ0);
				Out(Symbol);
			}
			else
				if (Symbol <= 0x7f)
				{
					/* encode US-ASCII directly */
					Out(Symbol);
				}
				else
					if (isInWindow(offsets[window], Symbol))
					{
						/* use the current dynamic window */
						Out(0x80 + (Symbol - offsets[window]));
					}
					else
						if ((w = getWindow(Symbol)) >= 0)
						{
							if (w <= 7)
							{
								/* switch to a dynamic window */
								Out(SC0 + w);
								Out(0x80 + (Symbol - offsets[w]));
								windowState = window = (char)w;
							}
							else
							{
								/* quote from a static window */
								Out(SQ0 + (w - 8));
								Out(Symbol - offsets[w]);
							}
						}
						else
							if (Symbol == 0xfeff)
							{
								/* encode the signature character U+FEFF with SQU */
								Out(SQU);
								Out(0xfe);
								Out(0xff);
							}
							else
							{
								/* switch to Unicode mode */
								Out(SCU);
								IsUnicodeMode = true;
								WriteSCSUSymbol(Symbol);
							}
		}
		else
		{
			/* Unicode mode */
			if (Symbol <= 0x7f)
			{
				/* US-ASCII: switch to single-byte mode with the previous dynamic window */
				IsUnicodeMode = false;
				Out(UC0 + window);
				WriteSCSUSymbol(Symbol);
			}
			else
				if ((w = getWindow(Symbol)) >= 0 && w <= 7)
				{
					/* switch to single-byte mode with a matching dynamic window */
					Out(UC0 + w);
					windowState = window = (char)w;
					IsUnicodeMode = false;
					WriteSCSUSymbol(Symbol);
				}
				else
				{
					if (0xe000 <= Symbol && Symbol <= 0xf2ff)
						Out(UQU);
					Out(Symbol >> 8);
					Out(Symbol);
				}
		}
	}
}

void CUnicodeSCSU::WriteSCSU(const _TCHAR* pString)
{
	if (m_pFile)
	{
		size_t nLen = _tcslen(pString);
		for (const _TCHAR *pChar = pString; pChar < pString + nLen; pChar++)
			WriteSCSUSymbol(static_cast<int>(*pChar));
	}
	else
		throw CFileWriteException(NULL);
}

int CUnicodeSCSU::ReadSCSUSymbol()
{
	unsigned int Symbol = In();
	if (IsUnicodeMode)
	{
		if (Symbol == UQU)
		{
			Symbol = In() << 8;
			Symbol |= In();
		}
		else
			if (Symbol >= UC0 && Symbol <= UC0 + 7)
			{
				int window = Symbol - UC0;
				windowState = window;
				IsUnicodeMode = false;
				return ReadSCSUSymbol();
			}
			else
				if (Symbol == UQU)
				{
					Symbol = In();
				}

		Symbol <<= 8;
		Symbol |= In();
	}
	else
	{
		if (Symbol == SQU)		// quote unicode
		{
			Symbol = In() << 8;
			Symbol |= In();
		}
		else
			if (Symbol == SCU)	// switch to unicode
			{
				IsUnicodeMode = true;
				return ReadSCSUSymbol();
			}
			else
				if (Symbol == SQ0)		// Skip control code C0
				{
					Symbol = In();
				}
				else
					if (Symbol >= SC0 && Symbol <= SC0 + 7)		// dynamic window with lock
					{
						windowState = Symbol - SC0;
						Symbol = In() + offsets[windowState] - 0x80;
					}
					else
						if (Symbol > SQ0 && Symbol <= SQ0 + 7)		// static window for single character
						{
							Symbol = In() + offsets[Symbol - SQ0 + 8];
						}
						else  // transform symbol using selected window
						{
							if ((Symbol < 0x20 || Symbol > 0x7f) && Symbol != 0xd && Symbol != 0xa && Symbol != 0x9)
								Symbol += offsets[windowState] - 0x80;
						}
	}

	if (Symbol > 0x10ffff)
		throw CFileReadException(m_pFile);

	return Symbol;
}

void CUnicodeSCSU::ReadSCSU(wstring& String, size_t nLen)
{
	if (m_pFile)
	{
		String.resize(nLen, _T('\x0'));
		for (unsigned int nSymbol = 0; nSymbol < nLen; nSymbol++)
			String[nSymbol] = ReadSCSUSymbol();
	}
	else
		throw CFileWriteException(NULL);
}


char CUnicodeSCSU::isInWindow(int offset, int Symbol)
{
	return (char)(offset <= Symbol && Symbol <= (offset + 0x7f));
}

int CUnicodeSCSU::getWindow(int Symbol)
{
	int i;
	for (i = 0; i<16; ++i) {
		if (isInWindow(offsets[i], Symbol))
			return i;
	}
	return -1;
}

CUnicodeSCSU::CUnicodeSCSU(FILE *pFile) :	m_pFile(pFile),
											windowState(0),
											IsUnicodeMode(false)
{

}

void CUnicodeSCSU::Out(unsigned char Symbol)
{
	if (!m_pFile) throw CFileWriteException(NULL);
	if (fwrite(&Symbol, sizeof(unsigned char), 1, m_pFile) != 1) throw CFileWriteException(m_pFile);
}

unsigned char CUnicodeSCSU::In()
{
	if (!m_pFile) throw CFileWriteException(NULL);
	unsigned char Symbol = '\x0';
	if (fread(&Symbol, sizeof(unsigned char), 1, m_pFile) != 1) throw CFileWriteException(m_pFile);
	return Symbol;
}

/* constant offsets for the 8 static and 8 dynamic windows */
const int CUnicodeSCSU::offsets[16] =
{
	/* initial offsets for the 8 dynamic (sliding) windows */
	0x0080, /* Latin-1 */
	0x00C0, /* Latin Extended A */
	0x0400, /* Cyrillic */
	0x0600, /* Arabic */
	0x0900, /* Devanagari */
	0x3040, /* Hiragana */
	0x30A0, /* Katakana */
	0xFF00, /* Fullwidth ASCII */

	/* offsets for the 8 static windows */
	0x0000, /* ASCII for quoted tags */
	0x0080, /* Latin - 1 Supplement (for access to punctuation) */
	0x0100, /* Latin Extended-A */
	0x0300, /* Combining Diacritical Marks */
	0x2000, /* General Punctuation */
	0x2080, /* Currency Symbols */
	0x2100, /* Letterlike Symbols and Number Forms */
	0x3000  /* CJK Symbols and punctuation */
};


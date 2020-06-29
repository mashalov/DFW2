#pragma once
#include "..\dfw2\FileExceptions.h"

namespace DFW2
{
	class CUnicodeSCSU
	{
	protected:
		FILE *m_pFile;
		enum
		{
			SQ0 = 0x01, /* Quote from window pair 0 */
			SQU = 0x0E, /* Quote a single Unicode character */
			SCU = 0x0F, /* Change to Unicode mode */
			SC0 = 0x10, /* Select window 0 */

			UC0 = 0xE0, /* Select window 0 */
			UQU = 0xF0  /* Quote a single Unicode character */
		};

		static const int offsets[16];
		char windowState;
		static char isInWindow(int offset, int Symbol);
		static int getWindow(int Symbol);
		bool IsUnicodeMode;
		void Out(unsigned char Symbol);
		unsigned char In();
		void WriteSCSUSymbol(int Symbol);
		int ReadSCSUSymbol();
	public:
		CUnicodeSCSU(FILE *pFile);
		void WriteSCSU(std::wstring_view String);
		void ReadSCSU(std::wstring& String, size_t nLen);
	};
}

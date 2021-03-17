#pragma once
#include "fstream"
#include "filesystem"
#include "FileExceptions.h"

namespace DFW2
{
	class CStreamedFile : protected std::fstream
	{
		std::string filename;
	public:

		bool is_open() const { return std::fstream::is_open(); }

		std::fstream& seekg(std::streamoff off, ios_base::seekdir way)
		{
			std::fstream::seekg(off, way);
			if (std::fstream::fail())
				throw CFileExceptionGLE<CFileReadException>(*this);
			return *this;
		}

		template<typename T>
		std::fstream& write(T* s, std::streamsize n)
		{
			std::fstream::write(static_cast<const char*>(static_cast<const void*>(s)), n);
			if (std::fstream::fail())
				throw CFileExceptionGLE<CFileWriteException>(*this);
			return *this;
		}


		template<typename T>
		std::fstream& read(T* s, std::streamsize n)
		{
			std::fstream::read(static_cast<char*>(static_cast<void*>(s)), n);
			if (std::fstream::fail())
				throw CFileExceptionGLE<CFileReadException>(*this);
			return *this;
		}

		std::streampos tellg()
		{
			return std::fstream::tellg();
		}

		template <typename T>
		CStreamedFile& operator << (const T& v)
		{
			std::fstream::operator << (v);
			return *this;
		}

		std::filebuf* rdbuf()
		{
			return std::fstream::rdbuf();
		}





		void open(std::string_view FileName, ios_base::openmode mode = ios_base::in | ios_base::out)
		{
			filename = FileName;
			std::fstream::open(stringutils::utf8_decode(filename), mode);
			if (std::fstream::fail())
				if(mode & std::ios_base::in)
					throw CFileExceptionGLE<CFileReadException>(*this);
				else
					throw CFileExceptionGLE<CFileWriteException>(*this);
		}

		void close() 
		{ 
			if(std::fstream::is_open())
				std::fstream::close(); 
		}

		std::string_view path()
		{
			return filename;
		}

		void truncate()
		{
			try
			{
				std::filesystem::resize_file(filename, tellg());
			}
			catch (std::filesystem::filesystem_error&)
			{
				throw CFileExceptionGLE<CFileWriteException>(*this);
			}
		}
	};
}

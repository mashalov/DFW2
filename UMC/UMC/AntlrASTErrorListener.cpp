#include "AntlrASTErrorListener.h"


std::string AntlrASTErrorListener::GetSourceLineFromSource(std::string_view source, size_t line)
{
    size_t offset = 0, newoffset;
    while ((newoffset = source.find("\n", offset)) != std::string::npos)
    {
        if (--line == 0)
            break;
        offset = newoffset + 1;
    }
    std::string ret(source.substr(offset, newoffset - offset));
    trim(ret);
    return ret;
}

std::string AntlrASTErrorListener::ParserError(std::string_view What, std::string_view FullSourceText, std::string_view Offending, size_t line, size_t pos)
{
    return fmt::format(u8"(����������) {} \"{}\" ������ {} ������� {} � \"{}\"", What, Offending, line, pos, GetSourceLineFromSource(FullSourceText, line));
}

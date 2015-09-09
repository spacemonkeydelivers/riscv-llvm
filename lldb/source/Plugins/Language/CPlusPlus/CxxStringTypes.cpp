//===-- CXXStringTypes.cpp --------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CxxStringTypes.h"

#include "llvm/Support/ConvertUTF.h"

#include "lldb/Core/DataBufferHeap.h"
#include "lldb/Core/Error.h"
#include "lldb/Core/Stream.h"
#include "lldb/Core/ValueObject.h"
#include "lldb/Core/ValueObjectConstResult.h"
#include "lldb/DataFormatters/StringPrinter.h"
#include "lldb/DataFormatters/TypeSummary.h"
#include "lldb/Host/Endian.h"
#include "lldb/Symbol/ClangASTContext.h"
#include "lldb/Target/SectionLoadList.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Utility/ProcessStructReader.h"

#include <algorithm>

#if __ANDROID_NDK__
#include <sys/types.h>
#endif

#include "lldb/Host/Time.h"

using namespace lldb;
using namespace lldb_private;
using namespace lldb_private::formatters;

bool
lldb_private::formatters::Char16StringSummaryProvider (ValueObject& valobj, Stream& stream, const TypeSummaryOptions&)
{
    ProcessSP process_sp = valobj.GetProcessSP();
    if (!process_sp)
        return false;
    
    lldb::addr_t valobj_addr = valobj.GetValueAsUnsigned(0);
    
    if (!valobj_addr)
        return false;
    
    ReadStringAndDumpToStreamOptions options(valobj);
    options.SetLocation(valobj_addr);
    options.SetProcessSP(process_sp);
    options.SetStream(&stream);
    options.SetPrefixToken('u');
    
    if (!StringPrinter::ReadStringAndDumpToStream<StringElementType::UTF16>(options))
    {
        stream.Printf("Summary Unavailable");
        return true;
    }
    
    return true;
}

bool
lldb_private::formatters::Char32StringSummaryProvider (ValueObject& valobj, Stream& stream, const TypeSummaryOptions&)
{
    ProcessSP process_sp = valobj.GetProcessSP();
    if (!process_sp)
        return false;
    
    lldb::addr_t valobj_addr = valobj.GetValueAsUnsigned(0);
    
    if (!valobj_addr)
        return false;
    
    ReadStringAndDumpToStreamOptions options(valobj);
    options.SetLocation(valobj_addr);
    options.SetProcessSP(process_sp);
    options.SetStream(&stream);
    options.SetPrefixToken('U');
    
    if (!StringPrinter::ReadStringAndDumpToStream<StringElementType::UTF32>(options))
    {
        stream.Printf("Summary Unavailable");
        return true;
    }
    
    return true;
}

bool
lldb_private::formatters::WCharStringSummaryProvider (ValueObject& valobj, Stream& stream, const TypeSummaryOptions&)
{
    ProcessSP process_sp = valobj.GetProcessSP();
    if (!process_sp)
        return false;
    
    lldb::addr_t data_addr = 0;
    
    if (valobj.IsPointerType())
        data_addr = valobj.GetValueAsUnsigned(0);
    else if (valobj.IsArrayType())
        data_addr = valobj.GetAddressOf();
    
    if (data_addr == 0 || data_addr == LLDB_INVALID_ADDRESS)
        return false;
    
    // Get a wchar_t basic type from the current type system
    CompilerType wchar_clang_type = valobj.GetCompilerType().GetBasicTypeFromAST(lldb::eBasicTypeWChar);
    
    if (!wchar_clang_type)
        return false;
    
    const uint32_t wchar_size = wchar_clang_type.GetBitSize(nullptr); // Safe to pass NULL for exe_scope here
    
    ReadStringAndDumpToStreamOptions options(valobj);
    options.SetLocation(data_addr);
    options.SetProcessSP(process_sp);
    options.SetStream(&stream);
    options.SetPrefixToken('L');
    
    switch (wchar_size)
    {
        case 8:
            return StringPrinter::ReadStringAndDumpToStream<StringElementType::UTF8>(options);
        case 16:
            return StringPrinter::ReadStringAndDumpToStream<StringElementType::UTF16>(options);
        case 32:
            return StringPrinter::ReadStringAndDumpToStream<StringElementType::UTF32>(options);
        default:
            stream.Printf("size for wchar_t is not valid");
            return true;
    }
    return true;
}

bool
lldb_private::formatters::Char16SummaryProvider (ValueObject& valobj, Stream& stream, const TypeSummaryOptions&)
{
    DataExtractor data;
    Error error;
    valobj.GetData(data, error);
    
    if (error.Fail())
        return false;
    
    std::string value;
    valobj.GetValueAsCString(lldb::eFormatUnicode16, value);
    if (!value.empty())
        stream.Printf("%s ", value.c_str());
    
    ReadBufferAndDumpToStreamOptions options(valobj);
    options.SetData(data);
    options.SetStream(&stream);
    options.SetPrefixToken('u');
    options.SetQuote('\'');
    options.SetSourceSize(1);
    options.SetBinaryZeroIsTerminator(false);
    
    return StringPrinter::ReadBufferAndDumpToStream<StringElementType::UTF16>(options);
}

bool
lldb_private::formatters::Char32SummaryProvider (ValueObject& valobj, Stream& stream, const TypeSummaryOptions&)
{
    DataExtractor data;
    Error error;
    valobj.GetData(data, error);
    
    if (error.Fail())
        return false;
    
    std::string value;
    valobj.GetValueAsCString(lldb::eFormatUnicode32, value);
    if (!value.empty())
        stream.Printf("%s ", value.c_str());
    
    ReadBufferAndDumpToStreamOptions options(valobj);
    options.SetData(data);
    options.SetStream(&stream);
    options.SetPrefixToken('U');
    options.SetQuote('\'');
    options.SetSourceSize(1);
    options.SetBinaryZeroIsTerminator(false);
    
    return StringPrinter::ReadBufferAndDumpToStream<StringElementType::UTF32>(options);
}

bool
lldb_private::formatters::WCharSummaryProvider (ValueObject& valobj, Stream& stream, const TypeSummaryOptions&)
{
    DataExtractor data;
    Error error;
    valobj.GetData(data, error);
    
    if (error.Fail())
        return false;
    
    ReadBufferAndDumpToStreamOptions options(valobj);
    options.SetData(data);
    options.SetStream(&stream);
    options.SetPrefixToken('L');
    options.SetQuote('\'');
    options.SetSourceSize(1);
    options.SetBinaryZeroIsTerminator(false);
    
    return StringPrinter::ReadBufferAndDumpToStream<StringElementType::UTF16>(options);
}

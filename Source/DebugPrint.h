// Copyright (c) 2022 Sultim Tsyrendashiev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "RTGL1/RTGL1.h"

#include <cassert>
#include <format>
#include <functional>
#include <string_view>

// TODO: fmt instead of std::format? for compile-time checks

namespace RTGL1
{
namespace debug
{
    namespace detail
    {
        using DebugPrintFn = std::function< void( std::string_view, RgMessageSeverityFlags ) >;
        extern DebugPrintFn g_print;

        extern RgMessageSeverityFlags g_printSeverity;
        extern bool                   g_breakOnError;

        inline void Print( RgMessageSeverityFlags severity, std::string_view msg )
        {
            if( ( g_printSeverity & severity ) == 0 )
            {
                return;
            }

            if( g_print )
            {
                g_print( msg, severity );
            }

#ifdef WIN32
#ifndef NDEBUG
            if( g_breakOnError && ( severity & RG_MESSAGE_SEVERITY_ERROR ) )
            {
                auto str = std::format( "{}{}\n"
                                        "\n\'Cancel\' to suppress all dialog boxes."
                                        "\n\'Try Again\' to cause a breakpoint."
                                        "\n\'Continue\' to skip this one message.",
                                        msg,
                                        msg.ends_with( '.' ) ? "" : "." );

                int r = MessageBoxA( nullptr,
                                     str.c_str(), // null-terminated
                                     "RTGL1 Error Message",
                                     MB_CANCELTRYCONTINUE | MB_DEFBUTTON3 | MB_ICONERROR );
                if( r == IDTRYAGAIN )
                {
                    if( IsDebuggerPresent() )
                    {
                        DebugBreak();
                    }
                }
                else if( r == IDCANCEL )
                {
                    g_breakOnError = false;
                }
            }
#endif
#endif
        }

        template< typename... Args >
        void Print( RgMessageSeverityFlags severity, std::string_view msg, Args&&... args )
        {
            if( ( g_printSeverity & severity ) == 0 )
            {
                return;
            }

            auto str = std::vformat( msg, std::make_format_args( args... ) );

            Print( severity, std::string_view( str ) );
        }
    }

    inline void Verbose( std::string_view msg )
    {
        detail::Print( RG_MESSAGE_SEVERITY_VERBOSE, msg );
    }

    inline void Info( std::string_view msg )
    {
        detail::Print( RG_MESSAGE_SEVERITY_INFO, msg );
    }

    inline void Warning( std::string_view msg )
    {
        detail::Print( RG_MESSAGE_SEVERITY_WARNING, msg );
    }

    inline void Error( std::string_view msg )
    {
        detail::Print( RG_MESSAGE_SEVERITY_ERROR, msg );
    }

    template< typename... Args >
    void Verbose( std::string_view fmt, Args&&... args )
    {
        detail::Print( RG_MESSAGE_SEVERITY_VERBOSE, fmt, std::forward< Args >( args )... );
    }

    template< typename... Args >
    void Info( std::string_view fmt, Args&&... args )
    {
        detail::Print( RG_MESSAGE_SEVERITY_INFO, fmt, std::forward< Args >( args )... );
    }

    template< typename... Args >
    void Warning( std::string_view fmt, Args&&... args )
    {
        detail::Print( RG_MESSAGE_SEVERITY_WARNING, fmt, std::forward< Args >( args )... );
    }

    template< typename... Args >
    void Error( std::string_view fmt, Args&&... args )
    {
        detail::Print( RG_MESSAGE_SEVERITY_ERROR, fmt, std::forward< Args >( args )... );
    }

}
}

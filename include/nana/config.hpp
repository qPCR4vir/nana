/**
 *	Nana Configuration
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/config.hpp
 */

#ifndef NANA_CONFIG_HPP
#define NANA_CONFIG_HPP

// Select platform  ......

	 // Windows:
	#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

		#define NANA_WINDOWS 1

		// MINGW ...
		#if defined(__MINGW32__) || defined(__MINGW64__) || defined(MINGW)
			#define NANA_MINGW 1
		#endif // MINGW

	// end Windows


	 // MacOS:     who define APPLE ??
      //#define APPLE
    #elif defined(APPLE)
		#define NANA_MACOS 1
		#define NANA_X11 1
		// how to add this:  include_directories(/opt/X11/include/)
	// end MacOS

	 // Linux:    (not sure about __GNU__ ??)
	#elif (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)) && !defined(_CRAYC)
		#define NANA_LINUX 1
		#define NANA_X11 1
	// end Linux
	#else
	#	static_assert(false, "Only Windows and Unix are supported now (Mac OS is experimental)");
	#endif // Select platform

	#if defined(NANA_LINUX) || defined(NANA_MACOS)
		#define NANA_POSIX 1
		#undef NANA_WINDOWS
	#endif
// End Select platform  ......

// compilers ...

// MSVC++ versions
#if defined(_MSC_VER)
	#define _SCL_SECURE_NO_WARNNGS
	#define _CRT_SECURE_NO_DEPRECATE
	#pragma warning(disable : 4996)

	#if (_MSC_VER == 1900)
		// google: break any code that tries to use codecvt<char16_t> or codecvt<char32_t>.
		// google: It appears the C++ libs haven't been compiled with native char16_t/char32_t support.
		// google: Those definitions are for codecvt<wchar_t>::id, codecvt<unsigned short>::id and codecvt<char>::id respectively.
		// However, the codecvt<char16_t>::id and codecvt<char32_t>::id definitions aren't there, and indeed, if you look at locale0.cpp in the CRT source code you'll see they're not defined at all.
		// google: That's a known issue, tracked by an active bug (DevDiv#1060849). We were able to update the STL's headers in response to char16_t/char32_t, but we still need to update the separately compiled sources.
		#define STD_CODECVT_NOT_SUPPORTED 1
	#endif // _MSC_VER == 1900
#endif // _MSVC

#if defined(__clang__)
	#if defined(__GLIBCPP__) || defined(__GLIBCXX__)
		//<codecvt> is a known issue on libstdc++, it works on libc++
		#define STD_CODECVT_NOT_SUPPORTED 1
	#endif

#elif defined(__GNUC__) //GCC
	#if defined(__GLIBCPP__) || defined( __GLIBCXX__ )
		//<codecvt> is a known issue on libstdc++, it works on libc++
		#define STD_CODECVT_NOT_SUPPORTED 1
	#endif

	#if (__GNUC__ == 4)
		#if ((__GNUC_MINOR__ < 8) || (__GNUC_MINOR__ == 8 && __GNUC_PATCHLEVEL__ <= 1))
			#define STD_THREAD_NOT_SUPPORTED 1

			//boost.thread is preferred
			//but if USE_github_com_meganz_mingw_std_threads is enabled,
			//boost.thread will be replaced with meganz's mingw-std-threads.
			// https://github.com/meganz/mingw-std-threads
        	#if !defined( USE_github_com_meganz_mingw_std_threads )
			    //#define USE_github_com_meganz_mingw_std_threads 1
            #endif
		#endif

		#if defined(NANA_MINGW)
			//It's a known issue under MinGW
			#define STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED 1
		#endif

		#if ((__GNUC_MINOR__ < 8) || defined(NANA_MINGW))
			#define STD_TO_STRING_NOT_SUPPORTED 1
		#endif
	#endif
#endif



// End compilers ...


// Here defines some flags that tell Nana what features will be supported.

///////////////////
//Support for PNG
//	Define the NANA_ENABLE_PNG to enable the support of PNG.
//
//#define NANA_ENABLE_PNG	//!
//#define USE_LIBPNG_FROM_OS // Un-Comment it to use libpng from operating system.
#if defined(NANA_ENABLE_PNG)
	#if !defined(USE_LIBPNG_FROM_OS)
		#define NANA_LIBPNG
	#endif
#endif

///////////////////
//Support for JPEG
//	Define the NANA_ENABLE_JPEG to enable the support of JPEG.
//
//#define NANA_ENABLE_JPEG	//!
//#define USE_LIBJPEG_FROM_OS // Un-Comment it to use libjpeg from operating system.
#if defined(NANA_ENABLE_JPEG)
	#if !defined(USE_LIBJPEG_FROM_OS)
		#define NANA_LIBJPEG
	#endif
#endif



// always define NANA_UNICODE ?? it will be deprecated ?.
#ifndef NANA_UNICODE
	#define NANA_UNICODE
#endif

#if defined(NANA_UNICODE) && defined(NANA_WINDOWS)
	#ifndef _UNICODE
		#define _UNICODE
	#endif

	#ifndef UNICODE
		#define UNICODE
	#endif
#endif


#define VERBOSE_PREPROCESSOR 0

#endif	//NANA_CONFIG_HPP

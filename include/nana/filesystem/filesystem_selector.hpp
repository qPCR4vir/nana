/**
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file nana\filesystem\filesystem_selector.hpp
*   @autor by Ariel Vina-Rodriguez:
*	@brief A "ISO C++" filesystem Implementation selector
*    The ISO C++ File System Technical Specification is optional.
*               http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4100.pdf
*    This is not a workaround, but an user option.
*    The library maybe available in the std library in use or from Boost (almost compatible)
*               http://www.boost.org/doc/libs/1_60_0/libs/filesystem/doc/index.htm
*    or you can choose to use the (partial, but functional) implementation provided by nana.
*    If you include the file <nana/filesystem/filesystem_selector.hpp>
*    The selected option will be set by nana into std::experimental::filesystem
*    By default Nana will use the ISO TS if available, or nana if not.
*    Boost will be use only if explicitily changed 
*    nana Now mimic std::experimental::filesystem::v1   (boost v3)
*    
*/

#ifndef NANA_FILESYSTEM_SELECTOR
#define NANA_FILESYSTEM_SELECTOR

#include <nana/config.hpp>

#if defined(NANA_BOOST_FILESYSTEM_AVAILABLE) && ( defined(NANA_BOOST_FILESYSTEM_FORCE) || (defined(STD_FILESYSTEM_NOT_SUPPORTED) && defined(NANA_BOOST_FILESYSTEM_PREFERRED) ) )

#   include <boost/filesystem.hpp>

	// add boost::filesystem into std::experimental::filesystem
namespace std {
	namespace experimental {
		namespace filesystem {

#       ifdef CXX_NO_INLINE_NAMESPACE
				using namespace boost::experimental::filesystem;
#       else
				using namespace boost::experimental::filesystem::v3;
#       endif

		} // filesystem
	} // experimental
} // std


#elif defined(STD_FILESYSTEM_NOT_SUPPORTED)

#   include <nana/filesystem/filesystem.hpp>

namespace std {
	namespace experimental {
		namespace filesystem {

#       ifdef CXX_NO_INLINE_NAMESPACE
				using namespace nana::experimental::filesystem;
#       else
				using namespace nana::experimental::filesystem::v1;
#       endif

		} // filesystem
	} // experimental
} // std

#else
#    include <filesystem>
#endif

#ifndef __cpp_lib_experimental_filesystem
#   define __cpp_lib_experimental_filesystem 1
#endif


#endif // NANA_FILESYSTEM_SELECTOR             
 





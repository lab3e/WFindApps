/* miniz_export.h — stub for static compilation (no CMake needed) */
#pragma once

#ifndef MINIZ_EXPORT
#  define MINIZ_EXPORT
#endif

#ifndef MINIZ_NO_EXPORT
#  define MINIZ_NO_EXPORT
#endif

#ifndef MINIZ_DEPRECATED
#  define MINIZ_DEPRECATED
#endif

#ifndef MINIZ_DEPRECATED_EXPORT
#  define MINIZ_DEPRECATED_EXPORT MINIZ_EXPORT MINIZ_DEPRECATED
#endif

#ifndef MINIZ_DEPRECATED_NO_EXPORT
#  define MINIZ_DEPRECATED_NO_EXPORT MINIZ_NO_EXPORT MINIZ_DEPRECATED
#endif

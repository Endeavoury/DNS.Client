#ifndef RATATOSKR_EXPORT_H
#define RATATOSKR_EXPORT_H

#if defined(RATOS_STATIC)
#  define RATOS_API
#elif defined(_WIN32)
#  if defined(RATOS_BUILDING_LIBRARY)
#    define RATOS_API __declspec(dllexport)
#  else
#    define RATOS_API __declspec(dllimport)
#  endif
#else
#  define RATOS_API __attribute__((visibility("default")))
#endif

#endif

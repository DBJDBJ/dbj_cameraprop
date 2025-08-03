#pragma once
#ifndef HRLOG_H
#define HRLOG_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>



// only while debugging
// log the com error message made from HRESULT hr
// if  SUCCEEDED(hr) return true
// otherwise return false

bool hrlog(HRESULT hr, int line_number, const char * prompt_, ...);

#ifdef _DEBUG
#define HRLOG(hr, ...) hrlog(hr, __LINE__, __VA_ARGS__)
#define HRLOG_EXEC(expr) \
    (hrlog((expr), __LINE__, #expr))
#else
#define HRLOG(hr, ...)
#define HRLOG_EXEC(expr) ( SUCCEEDED(expr) ? true : false )
#endif


#endif // HRLOG_H

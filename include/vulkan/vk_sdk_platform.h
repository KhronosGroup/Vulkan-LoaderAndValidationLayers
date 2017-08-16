//
// File: vk_sdk_platform.h
//
/*
 * Copyright (c) 2015-2017 The Khronos Group Inc.
 * Copyright (c) 2015-2017 Valve Corporation
 * Copyright (c) 2015-2017 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VK_SDK_PLATFORM_H
#define VK_SDK_PLATFORM_H

#if defined(_WIN32)
// Silence redefinition warnings on MinGW-w64
#undef NOMINMAX
#define NOMINMAX
#ifndef __cplusplus
#undef inline
#define inline __inline
#endif // __cplusplus

#if (defined(_MSC_VER) && _MSC_VER < 1900 /*vs2015*/)
// C99:
// Microsoft didn't implement C99 in Visual Studio; but started adding it with
// VS2013.  However, VS2013 still didn't have snprintf().  The following is a
// work-around (Note: The _CRT_SECURE_NO_WARNINGS macro must be set in the
// "CMakeLists.txt" file).
// NOTE: This is fixed in Visual Studio 2015.
#define snprintf _snprintf
#endif

#define strdup _strdup

//This is needed to make InitOnceExecuteOnce and friends
// available on MinGW-w64.
#ifdef __MINGW64__
    #include <sdkddkver.h>
    #ifdef WINVER
        #undef WINVER
    #endif
    #ifdef _WIN32_WINNT
        #undef _WIN32_WINNT
    #endif
    #define WINVER _WIN32_WINNT_WIN7 // Windows 7 minimum
    #define _WIN32_WINNT _WIN32_WINNT_WIN7
#endif

#endif // _WIN32

#endif // VK_SDK_PLATFORM_H

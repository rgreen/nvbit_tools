/* Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Author: Oreste Villa, ovilla@nvidia.com - 2018 */

#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <iomanip>  // std::setw

#define _assert(condition)                                                     \
    {                                                                          \
        if (!(condition)) {                                                    \
            std::cout << "ASSERT FAIL: " << __FILE__ << ":" << __LINE__ << ":" \
                      << __PRETTY_FUNCTION__ << ": FAIL !(" #condition << ")"  \
                      << std::endl;                                            \
            fflush(stdout);                                                    \
            _exit(1);                                                          \
        }                                                                      \
    }

#define _assert_msg(condition, msg)                                            \
    {                                                                          \
        if (!(condition)) {                                                    \
            std::cout << "ASSERT FAIL: " << __FILE__ << ":" << __LINE__ << ":" \
                      << __PRETTY_FUNCTION__ << ": FAIL !(" #condition         \
                      << ") MSG: " << msg << std::endl;                        \
            fflush(stdout);                                                    \
            _exit(1);                                                          \
        }                                                                      \
    }

#define _cuda_safe(x)                                                          \
    {                                                                          \
        CUresult status = (x);                                                 \
        if (status != CUDA_SUCCESS) {                                          \
            const char *str1, *str2;                                           \
            cuGetErrorName(status, &str1);                                     \
            cuGetErrorString(status, &str2);                                   \
            printf("ERROR file %s line %d: %s %s\n", __FILE__, __LINE__, str1, \
                   str2);                                                      \
            fflush(stdout);                                                    \
            _exit(1);                                                          \
        }                                                                      \
    }

#define SYSTEM(...)                                                            \
    {                                                                          \
        char CMD[2048];                                                        \
        snprintf(CMD, 2048, __VA_ARGS__);                                      \
        int res = system(CMD);                                                 \
        if (WIFSIGNALED(res) || res != 0) {                                    \
            printf(                                                            \
                "Command \"%s\" exited with signal %d(%d). Is the command in " \
                "your PATH?\n",                                                \
                CMD, WTERMSIG(res), res);                                      \
            fflush(stdout);                                                    \
            _exit(1);                                                          \
        }                                                                      \
    }

/***************Environment variables capturing macros ***********************/

#define PRINT_VAR(env_var, help, var)                                          \
    if (strcmp(help, "__internal__"))                                          \
        std::cout << std::setw(20) << env_var << " = " << var << " - " << help \
                  << std::endl;

#define GET_VAR_INT(var, env_var, def, help) \
    if (getenv(env_var)) {                   \
        var = atoi(getenv(env_var));         \
    } else {                                 \
        var = def;                           \
    }                                        \
    PRINT_VAR(env_var, help, var)

#define GET_VAR_LONG(var, env_var, def, help) \
    if (getenv(env_var)) {                    \
        var = atol(getenv(env_var));          \
    } else {                                  \
        var = def;                            \
    }                                         \
    PRINT_VAR(env_var, help, var)

#define GET_VAR_STR(var, env_var, help) \
    if (getenv(env_var)) {              \
        std::string s(getenv(env_var)); \
        var = s;                        \
    }                                   \
    PRINT_VAR(env_var, help, var)


//---------------------------------------------------------------------------------------
// charencoding.h
//---------------------------------------------------------------------------------------
//
// Copyright (c) 2004, Steffen Schümann <s.schuemann@pobox.com>
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
//
//---------------------------------------------------------------------------------------
#pragma once

#include <string>

typedef unsigned short UCS2;

class CharacterMapper
{
public:
    // CharacterMapper(int enFrom,int enTo);
    CharacterMapper(const char* pszFrom, const char* pszTo);
    ~CharacterMapper();

    bool IsOK() const { return _bInitialised; }

    bool IsToUtf8() const { return _toUtf8; }

    int Map(int c) const;
    std::string Map(const std::string& sText) const;

    static bool IsSupported(const std::string& sEnc);

private:
    void Init(const char* pszFrom, const char* pszTo);

    bool _bInitialised = false;
    bool _fromUtf8 = false;
    bool _toUtf8 = false;
    char* _pcCharMap = nullptr;
    UCS2* _pwWordMap = nullptr;
};

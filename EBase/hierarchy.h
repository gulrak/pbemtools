//---------------------------------------------------------------------------------------
// hierarchy.h
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

#include <map>
#include <set>
#include <string>
#include <vector>

#include "Utility.h"

class CHierarchyInfo
{
public:
    typedef std::shared_ptr<CHierarchyInfo> ptr;
    typedef std::vector<std::string> KeyNames;
    typedef std::map<std::string, size_t> KeyMap;
    typedef std::set<std::string> SubBlocks;
    typedef std::set<std::string> Attributes;

    CHierarchyInfo(CConfigFile& oCF, const std::string& sName);
    std::string m_sName;
    bool m_bHasUniqueID;
    KeyNames m_csKeys;
    KeyMap m_ciKeyMap;
    SubBlocks m_csSubBlocks;
    Attributes m_csAttributes;
};

class CHierarchy
{
public:
    typedef std::map<std::string, CHierarchyInfo::ptr> InfoMap;
    typedef std::set<std::string> BlockNames;

    static bool IsInitialized();
    static void Init(const std::string& sFName);
    static void Free();
    static bool IsChild(const std::string& sBlockName, const std::string& sChildName);
    static CHierarchyInfo::ptr Lookup(const std::string& sBlockName);

private:
    static bool m_bInitialized;
    static InfoMap m_coHInfos;
    static BlockNames m_csBlockNames;
};

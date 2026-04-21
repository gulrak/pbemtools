//---------------------------------------------------------------------------------------
// hierarchy.cpp
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
// #include "StdAfx.h"
#include "hierarchy.h"

extern std::string GetConfigFileName();

bool CHierarchy::m_bInitialized = false;
CHierarchy::InfoMap CHierarchy::m_coHInfos;
CHierarchy::BlockNames CHierarchy::m_csBlockNames;

CHierarchyInfo::CHierarchyInfo(CConfigFile& oCF, const std::string& sName)
    : m_sName(sName)
{
    size_t i = 1;
    size_t j;
    while (true) {
        if (!oCF.FetchLine("CRTags", i++, false))
            break;
        if (IsEqual(oCF.GetString(0), sName.c_str())) {
            j = 1;
            while (oCF.IsString(j)) {
                m_csAttributes.insert(oCF.GetString(j++));
            }
        }
    }
}

bool CHierarchy::IsInitialized()
{
    return m_bInitialized;
}

void CHierarchy::Init(const std::string& sFName)
{
    CConfigFile oCF(sFName);
    CHierarchyInfo::ptr pHI;
    std::string sBlk;
    size_t i = 1;
    size_t j;
    size_t k;
    while (true) {
        if (!oCF.FetchLine("CRHierarchy", i, false))
            break;
        pHI = CHierarchyInfo::ptr(new CHierarchyInfo(oCF, oCF.GetString(0)));
        if (!oCF.FetchLine("CRHierarchy", i++, false))
            break;
        m_csBlockNames.insert(Flatten(pHI->m_sName));
        pHI->m_bHasUniqueID = oCF.GetLong(1) != 0;
        j = size_t(oCF.GetLong(4));
        for (k = 0; k < j; k++) {
            pHI->m_csKeys.push_back(Flatten(oCF.GetString(k + 5)));
            pHI->m_ciKeyMap[Flatten(oCF.GetString(k + 5))] = k;
        }
        while (true) {
            sBlk = Flatten(oCF.GetString(k + 5));
            if (sBlk.empty())
                break;
            pHI->m_csSubBlocks.insert(sBlk);
            m_csBlockNames.insert(sBlk);
            k++;
        }
        m_coHInfos[Flatten(pHI->m_sName)] = pHI;
    }
    m_bInitialized = true;
}

void CHierarchy::Free()
{
    m_coHInfos.clear();
}

bool CHierarchy::IsChild(const std::string& sBlockName, const std::string& sChildName)
{
    if (!IsInitialized()) {
        Init(GetConfigFileName());
    }

    InfoMap::iterator hi = m_coHInfos.find(Flatten(sBlockName));
    if (hi != m_coHInfos.end() && (*hi).second) {
        CHierarchyInfo::SubBlocks::iterator hii = (*hi).second->m_csSubBlocks.find(Flatten(sChildName));
        return hii != (*hi).second->m_csSubBlocks.end();
    }

    return false;
}

CHierarchyInfo::ptr CHierarchy::Lookup(const std::string& sBlockName)
{
    CHierarchyInfo::ptr pHI;

    if (!IsInitialized()) {
        Init(GetConfigFileName());
    }

    InfoMap::iterator hi = m_coHInfos.find(Flatten(sBlockName));
    if (hi != m_coHInfos.end()) {
        pHI = (*hi).second;
    }
    else {
        CConfigFile oCF(GetConfigFileName());

        pHI = CHierarchyInfo::ptr(new CHierarchyInfo(oCF, sBlockName));
        if (!pHI->m_csAttributes.empty()) {
            m_csBlockNames.insert(Flatten(sBlockName));
            m_coHInfos[Flatten(sBlockName)] = pHI;
        }
        else {
            m_coHInfos[Flatten(sBlockName)] = CHierarchyInfo::ptr();
        }
    }

    return pHI;
}

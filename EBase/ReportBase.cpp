
#include "ReportBase.h"
#include <algorithm>
#include "Utility.h"

CReportObjID::CReportObjID(CReportStream& oRS)
{
    int num = oRS.GetNumDat();
    for (int i = 0; i < num; i++) {
        m_coIDVals.push_back(Value((int32_t)oRS.GetDat(i)));
    }
}

CReportObjID::~CReportObjID() {}

const CReportObjID& CReportObjID::operator=(const CReportObjID& oROID)
{
    m_coIDVals.assign(oROID.m_coIDVals.begin(), oROID.m_coIDVals.end());
    return *this;
}

bool CReportObjID::operator==(const CReportObjID& oROID)
{
    int num = int(m_coIDVals.size());
    if (num != int(oROID.m_coIDVals.size()))
        return false;
    for (int i = 0; i < num; i++) {
        if (m_coIDVals[size_t(i)] != oROID.m_coIDVals[size_t(i)])
            return false;
    }
    return true;
}

bool CReportObjID::operator<(const CReportObjID& oROID)
{
    int num = int((std::min)(m_coIDVals.size(), oROID.m_coIDVals.size()));
    for (int i = 0; i < num; i++) {
        if (m_coIDVals[size_t(i)] >= oROID.m_coIDVals[size_t(i)])
            return false;
    }
    if (m_coIDVals.size() > oROID.m_coIDVals.size())
        return false;
    return true;
}

CReportObj::CReportObj(CReportStream& oRS)
{
    m_sName = oRS.GetValue();
    oRS.Next();
    while (!oRS.EOS() && oRS.GetType() != CReportStream::enBLOCK) {
        //		switch
        oRS.Next();
    }
}

CReportObj::~CReportObj() {}

void CReportObj::Write(CReportStream& oRS) {}

Value CReportObj::Get(const std::string& sName) const
{
    return Value();
}

Value CReportObj::Get(int32_t nSID) const
{
    return Value();
}

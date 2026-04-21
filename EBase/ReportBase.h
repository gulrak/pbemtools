/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/EBase/Report.h,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:55:53 $
 * $Revision: 1.10 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: ERESSEA-Datenklassen inclusive CR-Parser
 *****************************************************************************
 *
 * $Log: Report.h,v $
 *
 *****************************************************************************/
#pragma once

#include <map>
#include <string>
#include <vector>
#include "ReportStream.h"

class CReportObjID
{
public:
    CReportObjID(CReportStream& oRS);
    ~CReportObjID();
    const CReportObjID& operator=(const CReportObjID& oROID);
    bool operator==(const CReportObjID& oROID);
    bool operator<(const CReportObjID& oROID);

private:
    std::vector<Value> m_coIDVals;
};

typedef std::vector<Value> CReportAttrib;

class CReportObj
{
public:
    CReportObj(CReportStream& oRS);
    virtual ~CReportObj();
    virtual void Write(CReportStream& oRS);

    Value Get(const std::string& sName) const;
    Value Get(int32_t nSID) const;

    int32_t NumNN() const { return int(m_coNNAttribs.size()); }

    Value GetNN(int32_t nIdx) const { return m_coNNAttribs[size_t(nIdx)]; }

private:
    std::string m_sName;
    std::map<int32_t, Value> m_coAttribs;
    std::map<CReportObjID, CReportObj*> m_coSubObj;
    std::vector<Value> m_coNNAttribs;
};

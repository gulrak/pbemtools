//---------------------------------------------------------------------------------------
// CRNE.h
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

#include <EBase/Utility.h>
#include <cctype>
#include <map>
#include <string>
#include <vector>

class CRNEException
{
public:
    CRNEException(const char* txt)
        : _text(txt)
    {
    }

    CRNEException(const std::string& txt)
        : _text(txt)
    {
    }

    std::string why() const { return _text; }

private:
    std::string _text;
};

class CRNENode
{
public:
    CRNENode() { m_nRefCnt++; }

    virtual ~CRNENode();
    virtual std::string GenAVal() = 0;
    virtual std::string String() = 0;
    static CRNENode* CreateFromString(const std::string& sExp);
    static void AddToPool(const std::string& sName, const std::string& sRNE);
    static void DumpRNEPool();
    static void ReadRules(const std::string& sFileName);
    static void ClearRules();

protected:
    typedef std::map<std::string, CRNENode*> RNEPOOL;
    static int32_t m_nRefCnt;
    static RNEPOOL m_cpoRNEPool;
};

CRNENode* CreateNewRNENode(std::string::const_iterator& it, const std::string::const_iterator& iend);

class CRNENode_Literal : public CRNENode
{
public:
    CRNENode_Literal(std::string::const_iterator& it, const std::string::const_iterator& iend)
    {
        while (it != iend && (IsNameChar(*it) || *it == '#')) {
            if (*it == '#') {
                m_sLiteral += ' ';
                it++;
            }
            else
                m_sLiteral += *it++;
        }
    }

    virtual std::string GenAVal() { return m_sLiteral; }

    virtual std::string String()
    {
        std::string sVal;
        for (size_t i = 0; i < m_sLiteral.size(); i++) {
            if (m_sLiteral[i] == ' ')
                sVal += '#';
            else
                sVal += m_sLiteral[i];
        }
        return sVal;
    }

private:
    std::string m_sLiteral;
};

class CRNENode_Reference : public CRNENode
{
public:
    CRNENode_Reference(std::string::const_iterator& it, const std::string::const_iterator& iend)
    {
        it++;
        while (it != iend && IsAlNum(*it))
            m_sName += *it++;
    }

    virtual std::string GenAVal()
    {
        RNEPOOL::iterator i = m_cpoRNEPool.find(m_sName);
        if (i != m_cpoRNEPool.end()) {
            return (*i).second->GenAVal();
        }
        else {
            return std::string("-");
        }
    }

    virtual std::string String() { return std::string("$") + m_sName; }

private:
    std::string m_sName;
};

class CRNENode_Selection : public CRNENode
{
    typedef std::vector<CRNENode*> ALTERNATIVES;

public:
    CRNENode_Selection(std::string::const_iterator& it, const std::string::const_iterator& iend)
        : m_nMin(1)
        , m_nMax(1)
    {
        CRNENode* poNode;
        int v1, v2;
        if (it == iend || *it++ != '[')
            throw CRNEException("Erwartetes '[' nicht gefunden!");
        while (it != iend && *it != ']') {
            poNode = CreateNewRNENode(it, iend);
            m_cpoAlternatives.push_back(poNode);
            if (*it != ']' && *it != '|')
                throw CRNEException("Erwartete '|' oder ']' aber fand beides nicht!");
            if (*it == '|')
                it++;
        }
        it++;
        if (it != iend && IsDigit(*it)) {
            v1 = 0;
            v2 = 0;
            while (it != iend && IsDigit(*it))
                v1 = v1 * 10 + (*it++) - '0';
            if (*it == ':') {
                it++;
                while (it != iend && IsDigit(*it))
                    v2 = v2 * 10 + (*it++) - '0';
                m_nMin = v1;
                m_nMax = v2;
            }
            else {
                m_nMin = 0;
                m_nMax = v1;
            }
        }
    }

    virtual ~CRNENode_Selection()
    {
        for (size_t i = 0; i < m_cpoAlternatives.size(); i++) {
            delete m_cpoAlternatives[i];
        }
    }

    virtual std::string GenAVal()
    {
        std::string sVal;
        int n = (rand() % (m_nMax - m_nMin + 1)) + m_nMin;
        for (int i = 0; i < n; i++)
            sVal += m_cpoAlternatives[size_t(rand() % (int)m_cpoAlternatives.size())]->GenAVal();
        return sVal;
    }

    virtual std::string String()
    {
        std::string sVal("[");
        for (ALTERNATIVES::iterator i = m_cpoAlternatives.begin(); i < m_cpoAlternatives.end(); i++) {
            sVal += (*i)->String() + "|";
        }
        sVal[sVal.size() - 1] = ']';
        return sVal;
    }

private:
    ALTERNATIVES m_cpoAlternatives;
    int m_nMin, m_nMax;
};

class CRNENode_Combination : public CRNENode
{
    typedef std::vector<CRNENode*> PARTS;

public:
    CRNENode_Combination(std::string::const_iterator& it, const std::string::const_iterator& iend)
    {
        CRNENode* poNode;
        if (it == iend || *it++ != '(')
            throw CRNEException("Erwartetes '(' nicht gefunden!");
        while (it != iend && *it != ')') {
            poNode = CreateNewRNENode(it, iend);
            m_cpoParts.push_back(poNode);
        }
        it++;
    }

    virtual ~CRNENode_Combination()
    {
        for (size_t i = 0; i < m_cpoParts.size(); i++) {
            delete m_cpoParts[i];
        }
    }

    virtual std::string GenAVal()
    {
        std::string sVal;
        for (size_t i = 0; i < m_cpoParts.size(); i++)
            sVal += m_cpoParts[i]->GenAVal();
        return sVal;
    }

    virtual std::string String()
    {
        std::string sVal("(");
        for (PARTS::iterator i = m_cpoParts.begin(); i < m_cpoParts.end(); i++) {
            sVal += (*i)->String();
        }
        sVal += ')';
        return sVal;
    }

private:
    PARTS m_cpoParts;
};

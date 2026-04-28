//---------------------------------------------------------------------------------------
// CRNE.cpp
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
#include <cstdio>
#include <cstdlib>
#include <ctime>

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif
#include <fstream>
#include <iostream>
#include <string>

#include <EBase/Utility.h>
#include "CRNE.h"

int32_t CRNENode::m_nRefCnt = 0;
CRNENode::RNEPOOL CRNENode::m_cpoRNEPool;

CRNENode::~CRNENode()
{
    m_nRefCnt--;
    if (!m_nRefCnt) {
        for (RNEPOOL::iterator i = m_cpoRNEPool.begin(); i != m_cpoRNEPool.end(); i++) {
            delete (*i).second;
        }
    }
}

CRNENode* CRNENode::CreateFromString(const std::string& sExp)
{
    std::string::const_iterator it = sExp.begin();
    return CreateNewRNENode(it, sExp.end());
}

void CRNENode::AddToPool(const std::string& sName, const std::string& sRNE)
{
    RNEPOOL::iterator i = m_cpoRNEPool.find(sName);
    CRNENode* poNode;
    std::string::const_iterator it = sRNE.begin();
    poNode = CreateNewRNENode(it, sRNE.end());
    if (i != m_cpoRNEPool.end()) {
        delete (*i).second;
        m_cpoRNEPool.erase(i);
    }
    m_cpoRNEPool.insert(RNEPOOL::value_type(sName, poNode));
}

void CRNENode::DumpRNEPool()
{
    for (RNEPOOL::iterator i = m_cpoRNEPool.begin(); i != m_cpoRNEPool.end(); i++) {
        std::cout << "$" << (*i).first << " = " << (*i).second->String() << "\n";
    }
}

CRNENode* CreateNewRNENode(std::string::const_iterator& it, const std::string::const_iterator& iend)
{
    CRNENode* poNode = NULL;
    while (it != iend && IsSpace(*it))
        it++;
    if (it != iend) {
        switch (*it) {
            case '(':
                poNode = new CRNENode_Combination(it, iend);
                break;
            case '[':
                poNode = new CRNENode_Selection(it, iend);
                break;
            case '$':
                poNode = new CRNENode_Reference(it, iend);
                break;
            default:
                if (IsAlpha(*it) || *it == '#') {
                    poNode = new CRNENode_Literal(it, iend);
                }
                else
                    throw CRNEException("Weder Namenszeichen, noch '#' gefunden!");
        }
    }
    return poNode;
}

void CRNENode::ReadRules(const std::string& sFileName)
{
    std::fstream oIS;
    std::string sLine;
    size_t p;
    int l = 0;

    srand((uint32_t)time(0));

    oIS.open(sFileName.c_str(), std::ios::in);

    if (oIS.fail()) {
        ERRMSG(0, ("FEHLER: Konnte Regeldatei '%s' fuer Namensgenerator nicht oeffnen!\n", sFileName.c_str()));
        return;
    }

    while (true) {
        std::getline(oIS, sLine);
        if (oIS.fail())
            break;
        l++;
        while (!sLine.empty() && sLine[sLine.size() - 1] < 32)
            sLine.erase(sLine.size() - 1, 1);
        while (!sLine.empty() && sLine[0] <= 32)
            sLine.erase(0, 1);
        if (!sLine.empty() && sLine[0] != ';') {
            std::string sName;
            if (sLine[0] != '$') {
                ERRMSG(0, ("%s(%d) : FEHLER: Erwarte '$' zu Beginn eines Regelnamens!\n", sFileName.c_str(), l));
                return;
            }
            p = sLine.find('=');
            if (p == std::string::npos) {
                ERRMSG(0, ("%s(%d) : FEHLER: Erwarte '=' in Namensregel!\n", sFileName.c_str(), l));
                return;
            }
            sName = sLine.substr(1, p - 1);
            while (!sName.empty() && sName[sName.size() - 1] <= 32)
                sName.erase(sName.size() - 1, 1);
            try {
                CRNENode::AddToPool(sName, sLine.substr(p + 1));
            }
            catch (CRNEException e) {
                ERRMSG(0, ("%s(%d) : FEHLER: Fehlerhafter Namensausdruck (%s)!\n", sFileName.c_str(), l, e.why().c_str()));
            }
        }
    }
}

void CRNENode::ClearRules()
{
    for (RNEPOOL::iterator i = m_cpoRNEPool.begin(); i != m_cpoRNEPool.end(); i++) {
        delete (*i).second;
    }
}

#if 0

int main( int argc, char* argv[] )
{
	std::fstream oIS;
	std::string sLine;
	CRNENode* poRNE;
	size_t p;
	int i, l = 0;

//	CRNENode::AddToPool( std::string("MDVorname"), std::string("[Anton|Bert|Casper|Dieter|Emil|Friedrich|Gerd|Heiko|Ingo|Jochen|Klaus|Lars|Martin|Norbert|Oskar|Paul|Rolf|Stephan|Torsten|Uwe]") );
//	CRNENode::AddToPool( std::string("MDwarfName"), std::string("([B|G|K|R|T|S][a|e|i|o|u][b|l|m|n|v][grat|galk|gerk|pak|perk|polk|rak|rek|ralk|tark|tolk|terk])") );
//	CRNENode::AddToPool( std::string("MDwarfClan"), std::string("[Bargh|Groth|Kreth|Tarsh|Lorkh|Prath|Ralsh]") );

	if( argc<3 || argc>4 )
	{
		FILE* io;

		if( argc <= 1 )
			io = stdout;
		else
			io = stderr;

		fprintf( io, "\nxnamer V1.0 a rule based name generator\n" );
		fprintf( io, "(c) Copyright 1999 by Steffen Schuemann, Hamburg, Germany\n" );
		fprintf( io, "\nUSAGE: xNamer <rulefile> <rule> [<count>]\n" );
		fprintf( io, "\n<rulefile>    A textfile containing emty lines, comment\n" );
		fprintf( io, "              lines (starting with ';') or rule defines:\n" );
		fprintf( io, "<rule>        A rule, describing what to produce, the\n" );
		fprintf( io, "              simplest rule ist $name, using rule 'name'\n" );
		fprintf( io, "              from the used rule fine.\n" );
		fprintf( io, "<count>       How much names schould be createt (one is\n" );
		fprintf( io, "              the default)\n" );
		fprintf( io, "\nThe rules:\n" );
		fprintf( io, "(<rule> <rule> ...)    comines the output of the rules\n" );
		fprintf( io, "[<rule>|<rule>|...]    selects randomly one of the rules\n" );
		fprintf( io, "[<rule>|<rule>|...]n   selects n times\n" );
		fprintf( io, "[<rule>|<rule>|...]n:m selects n to m times randomly\n" );
		fprintf( io, "$name                  use output of rule named 'name'\n" );
		fprintf( io, "any letters            insert ths letters (#=space)\n" );
		fprintf( io, "\nExample rule file example.txt:\n" );
		fprintf( io, "\n; This are some forenames\n" );
		fprintf( io, "$fore = [Carl|Pete|Hank]\n" );
		fprintf( io, "\n; This are some surnames\n" );
		fprintf( io, "$sur  = [Higgins|Johnson|Smith]\n" );
		fprintf( io, "\n; This Rule defines a name\n" );
		fprintf( io, "$name = ($fore # $sur)\n" );
		fprintf( io, "\nExample usage:\n" );
		fprintf( io, "  xnamer example.txt \x22$name\x22 5\n" );
		fprintf( io, "\nResults in something like:\n" );
		fprintf( io, "  Carl Johnson\n" );
		fprintf( io, "  Hank Higgins\n" );
		fprintf( io, "  Hank Johnson\n" );
		fprintf( io, "  Pete Smith\n" );
		fprintf( io, "  Carl Higgins\n" );
		exit( 0 );
	}

	srand( time(0) );

	std::string sFileName = argv[1];
	std::string sExpression = argv[2];
	int32_t nCount = (argc==4?atoi( argv[3] ):1);

	oIS.open( sFileName.c_str(), std::ios::in );

	if( oIS.fail() )
	{
		fprintf( stderr, "Error: Could not open rules file!" );
		exit( 1 );
	}

	while( true )
	{
		std::getline( oIS, sLine );
		if( oIS.fail() )
			break;
		l++;
		while( !sLine.empty() && sLine[sLine.size()-1]<32 )
			sLine.erase( sLine.size()-1, 1 );
		while( !sLine.empty() && sLine[0]<=32 )
			sLine.erase( 0, 1 );
		if( !sLine.empty() && sLine[0]!=';' )
		{
			std::string sName;
			if( sLine[0]!='$' )
			{
				fprintf( stderr, "Error line %d: Expected '$' at begin of rulename!\n", l );
				exit( 1 );
			}
			p = sLine.find( '=' );
			if( p==std::string::npos )
			{
				fprintf( stderr, "Error line %d: Epected '=' in rule!\n", l );
				exit( 1 );
			}
			sName = sLine.substr( 1, p-1 );
			while( !sName.empty() && sName[sName.size()-1]<=32 )
				sName.erase( sName.size()-1, 1 );
			try
			{
				CRNENode::AddToPool( sName, sLine.substr( p+1 ) );
			}
			catch(CRNEException e)
			{
				fprintf( stderr, "Line %d: Error in expression!\n", l );
				poRNE = 0;
			}
		}
	}

	if( !strcmp( argv[2], "?" ) )
	{
		CRNENode::DumpRNEPool();
	}
	else
	{
		try
		{
			poRNE = CRNENode::CreateFromString( sExpression );
		}
		catch(CRNEException e)
		{
			std::cerr << "Error in expression!\n";
			poRNE = 0;
		}
		if( poRNE )
			for( i=0 ; i<nCount;  i++ ) std::cout << poRNE->GenAVal() << "\n";
	}

	return 0;
}

#endif

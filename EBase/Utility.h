/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/EBase/Utility.h,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:55:53 $
 * $Revision: 1.8 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Algemeine Utility-Funktionen
 *****************************************************************************
 *
 * $Log: Utility.h,v $
 * Revision 1.8  2000/02/24 09:55:53  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.7  1999/11/28 17:38:11  S.Schuemann
 * - Mannigfaltige �nderungen f�r Vorlage V1.4 beta 9
 *
 * Revision 1.6  1999/11/17 08:58:15  S.Schuemann
 * - support f�r multiple CRs
 *
 * - vielfache �nderungen f�r Vorlage 1.4 beta 8
 *
 * Revision 1.5  1999/11/08 11:10:45  S.Schuemann
 * - verbessertes Luxusgut-Handling
 * - Korrektur der Kapazitaetsberechnung
 * - Neue Flags
 *
 * Revision 1.4  1999/11/03 10:21:38  S.Schuemann
 * - Anpassungen an Vorlage 1.4 beta 7
 *
 * Revision 1.3  1999/10/20 02:22:33  S.Schuemann
 * - Anpassungen der Reportklassen fuer die Features von Vorlage 1.4 b 3
 *
 * Revision 1.2  1999/10/18 21:31:46  S.Schuemann
 * - Diverse Aenderungen, fuer die Versionen 1.3.1, 1.3.2, 1.3.3 sowie 1.4 b 1 und 1.4 b 2
 *
 * Revision 1.1.1.1  1999/09/20 14:55:45  Steffen
 * - Initial CVS-checkin;
 * - Basierend auf dem Stand von Vorlage V1.3b6 gesaeubert und aufgeteilt;
 * - Fehler in Kapazitaetsberechnung behoben;
 *
 *****************************************************************************/

#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <iomanip>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>

#define IsAlpha(c) isalpha((unsigned char)(c))
#define IsAlNum(c) isalnum((unsigned char)(c))
#define IsDigit(c) isdigit((unsigned char)(c))
#define IsSpace(c) isspace((unsigned char)(c))

inline bool IsUmlaut(char cc)
{
    unsigned char c = static_cast<unsigned char>(cc);
    return (c == 0xC4 || c == 0xD6 || c == 0xDC || c == 0xE4 || c == 0xF6 || c == 0xFC || c == 0xDF || c == '~' || c == '_' || c == '@');
}

#define IsDelim(c) (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '&' || c == '|' || c == '<' || c == '>' || c == '=' || c == '^' || c == '(' || c == ')' || c == ',' || c == '=' || c == '[' || c == ']' || c == '.' || c == '!')
#define IsNameChar(c) (isalpha((unsigned char)(c)) || (unsigned char)(c) >= 0xc0 || (c) == '\'')

#define ToLower(c) ((char)(g_toLowerMapping[(unsigned char)(c)]))

#define INFOMSG(msg) InfoMessage(FormatMsg msg)
#define ERRMSG(pobj, msg) ErrorMessage((void*)pobj, FormatMsg msg)
// #define VERRMSG( msg ) VErrorMessage( FormatMsg msg )
#define CONMSG(msg) ConsoleMessage(FormatMsg msg)
#define TRACEMSG(msg) TraceMessage(FormatMsg msg)

extern int g_toLowerMapping[256];
extern std::string g_sSrcFile;
extern int32_t g_nSrcLine;
extern bool g_bForceEOL;
extern uint32_t g_nStepCount;
extern uint32_t g_nLastErrorStep;
extern int g_returnCode;
char* FormatMsg(const char* msg, ...);
void InfoMessage(const char* pszStr);
void ErrorMessage(void* pDat, const char* pszStr);
void ConsoleMessage(const char* pszStr);
void TraceMessage(const char* pszStr);
extern void (*g_pfErrorFunc)(void*, const char*);

std::string iso2utf8(const std::string& txt, bool doit);
std::string& Utf8toIso885915(std::string& text);
std::string& iso885915ToUtf8(std::string& text);
void SetBreakHandler(void (*pfHandler)(void));
void Message(const char* pszStr);
const char* itoa36(int i);
const char* itoan(int32_t n, int base);
int32_t EinheitenNummer(const std::string& sENStr);
int32_t FindNextENum(const std::string& sTxt, std::string::size_type& p);
bool FindNextRegion(const std::string& sTxt, std::string::size_type& p, int& x, int& y, int& z);
bool IsMetaCommand(const std::string& sLine);
std::string DeUmlaut(const std::string& sText);
std::string Flatten(const std::string& sText);
std::string Escape(const std::string& sText, bool bTildenize = false);
std::string DeEscape(const std::string& sText);
bool IsIdentifier(const std::string& sText, bool bVar = true);
bool IsEqual(const char* pcS1, const char* pcS2);
bool IsEqual(const std::string& sS1, const char* pcS2);
bool FileExists(const std::string& sFName);
bool IsConsole(FILE* hFile);
bool GetConsoleSize(int& width, int& height);
void WriteToConsole(FILE* hFile, const char* pcText);
std::string InputFromConsole();
std::string GetExecutablePathname();
std::string PathedFileName(const std::string& sFName);
std::string Wrap(std::string& sTxt, size_t len);
std::string ToString(int32_t n);
std::string ToString(double f, unsigned n = 2);
std::string ToStringS(int32_t n);
std::string ToStringS(double f, unsigned n = 2);

int32_t Random(int32_t seed = 0);

typedef std::vector<std::string> Args;
void split(Args& dest, const std::string& s, const std::string& sep, const std::string& caps, char esc);

enum VORLAGENFLAGS {
    VF_SORTBURGEN,
    VF_SORTTALENTE,
    VF_SHOWTALENTE,
    VF_SHOWGEGENSTAENDE,
    VF_SHOWMINIKARTE,
    VF_SHOWMESSAGES,
    VF_PRIVATMETA,
    VF_SHOWLASTEN,
    VF_SHOWKOMPKARTE,
    VF_SORTISLANDS,
    VF_SHOWHANDEL,
    VF_SHOWPRIVAT,
    VF_HEXMAP,
    VF_BASE36,
    VF_CROUTPUT,
    VF_SORTPRIVAT,
    VF_SHOWUNITS,
    VF_SHOWUNITSVERBOSE,
    VF_SHOWMATPOOL,
    VF_SHOWLUXUS,
    VF_SHOWUNITSNEW,
    VF_SUPPRESSUNITS,
    VF_SHOWLPROD,
    VF_SHOWTDIFF,
    VF_SHOWTRIBEOVERVIEW,
    VF_SORTFOREIGN,
    VF_SHOWBESCHREIBUNG,
    VF_FULLBASE36,
    VF_SUPPRESSKEYWARN,
    VF_DEBUGMODE,
    VF_PROGRESSINFO,
    VF_DONTKILLCOMMANDS,
    VF_TRACEONERROR,
    VF_NEWERESSEASTATI,
    VF_SORTKOMMANDO,
    VF_NOCONSOLE,
    VF_PAGER,
    VF_RESOURCEBLOCKS,
    VF_SHOWVERBOSEINFO,
    VF_NOWARNINGS,
    VF_NOSKILLPOINTS,
    VF_SUPPRESSTURNOUTPUT,
    VF_DIAGPEDANTIC,
    VF_SHOWEMULATEDDAYS,
    VF_VERSION2WARNING,
    VF_RUNALLVISIBLEREGIONS,
    VF_SUPPRESSMULTIERRORS,
    VF_FORCEDECLARES,
    VF_EXPORTWITHROUND,
    VF_FULLCOMMANDOUTPUT,
    VF_SHOWWORLDKARTE,
    VF_STRIPDUPLICATEDESCR,
    VF_RESTRICTED,
    VF_NOBATTLEMESSAGES,
    VF_FIXENCODINGS
};

// extern int32_t g_nFlags;
extern std::set<int> g_coFlags;
#define IsFlag(n) (g_coFlags.find(n) != g_coFlags.end())

typedef std::set<std::string> Keywords;
extern Keywords g_coKeywords;

inline void AddKeyword(const std::string& sText)
{
    g_coKeywords.insert(DeUmlaut(sText));
}

inline bool IsKeyword(const std::string& sText)
{
    Keywords::iterator ki = g_coKeywords.find(DeUmlaut(sText));
    return ki != g_coKeywords.end();
}

inline std::string getAndReset(std::ostringstream& os)
{
    auto result = os.str();
    os.str("");
    os.clear();
    return result;
}

/*
inline int IsSpace( char x )
{
    return isspace(x); //x>0 && x<33;
}
*/

class CConfigFile
{
public:
    CConfigFile(const std::string& sFName);
    ~CConfigFile();
    bool FetchLine(const std::string& sChapter, size_t nIdx, bool bForce = true);
    bool IsString(size_t nIdx);
    std::string GetString(size_t nIdx);
    int32_t GetLong(size_t nIdx);
    double GetReal(size_t nIdx);

protected:
    std::string m_sFName;
    std::string m_sFName2;
    std::string m_sChapter;
    std::string m_sLine;
    size_t m_nLine;
    size_t m_nChapterStart;
    std::vector<std::string> m_csStrings;
    std::vector<std::string> m_csLines;
    std::vector<std::string> m_csLines2;
    std::vector<size_t> m_cnPos;
    std::vector<size_t> m_cnPos2;
};

class CValArray;

class COutputTable
{
public:
    enum FORMAT { enLEFT, enRIGHT, enCENTER };

protected:
    typedef std::pair<FORMAT, std::string> TABENTRY;
    typedef std::vector<TABENTRY> TABLEROW;
    typedef std::list<TABLEROW> TABLE;

public:
    COutputTable() { Clear(); }

    void Clear()
    {
        m_coTable.clear();
        m_nMaxCols = 0;
        Next();
    }

    COutputTable& Col(const std::string& sText, FORMAT enFormat = enLEFT);
    COutputTable& Col(const char* pcText, FORMAT enFormat = enLEFT);
    COutputTable& Col(int32_t nNum, FORMAT enFormat = enRIGHT);
    COutputTable& Col(double fNum, int nScale = 2, FORMAT enFormat = enRIGHT);

    void Next()
    {
        m_nCol = 0;
        m_coTable.push_back(TABLEROW());
        m_pRow = &m_coTable.back();
    }

    void Output(const std::string& sTarget, const std::string& sPfx);
    void Output(CValArray& oDump, const std::string& sPfx);
    void Output(FILE* pStream);

protected:
    void Format();

    size_t m_nCol, m_nMaxCols;
    TABLEROW* m_pRow;
    TABLE m_coTable;
};

/*
class CStringDB
{
public:
        static int32_t Insert( const std::string& sText );
        static const std::string& Get( int32_t nSID );

private:
        static int32_t m_nStrCnt;
        static std::map<std::string,int32_t> m_coStrToSID;
        static std::map<int32_t,std::string> m_coSIDToStr;
};
*/
class CStringDB
{
public:
    static int32_t Str2SID(const std::string& sStr);
    static const std::string& SID2Str(int32_t nSID);
    static std::map<std::string, size_t> m_coS2I;
    static std::map<size_t, std::string> m_coI2S;
};

class CTranslationDB
{
public:
    static const std::string& ToLocale(const std::string& sStr);
    static const std::string& ToInternal(const std::string& sStr);
    static std::map<std::string, std::string> m_coI2L;
    static std::map<std::string, std::string> m_coL2I;
};

class CharacterMapper;

class COutput
{
public:
    COutput(const std::string& sFileName, bool bFlushed = false);
    COutput(FILE* hFile);
    ~COutput();

    bool IsOkay() const { return m_bIsOkay; }

    std::string FileName() const { return m_sFileName; }

    void Write(const char* pcTxt);
    void Write(const std::string& sTxt);
    void Printf(const char* msg, ...);

    bool Disconnect(bool bSuicide = true);

    static void TWrite(const std::string& sID, const std::string& sTxt);
    static void TPrintf(const std::string& sID, const char* msg, ...);

    // Compile-time-safe formatted output via fmtlib.
    // fmt::format_string<Args...> validates the format string and argument types
    // at compile time — wrong types or argument count are caught as errors, not
    // as runtime crashes or silent truncation like TPrintf.
    template <typename... Args>
    static void TPrint(const std::string& sID, fmt::format_string<Args...> fmt, Args&&... args)
    {
        Target(sID)->Write(fmt::format(fmt, std::forward<Args>(args)...));
    }

    static void CloseTargets();
    static void SetTarget(const std::string& sID, COutput* poTrace);
    static void SetFilter(const std::string& sID, const std::string& sFilter);
    static void RenameTarget(const std::string& sIDOld, const std::string& sIDNew);
    static COutput* Target(const std::string& sID);
    static void SetEncoding(const std::string& sEnc);

private:
    void DoWrite(const char* pcTxt);
    void FilterWrite(const char* pcTxt);

    bool m_bIsOkay;
    bool m_bStdStream;
    bool m_bFlushed;
    bool m_bWritePrefix = true;
    int m_nRefCount;
    std::string m_sOutBuffer;
    std::string m_sFileName;
    std::string m_sTargetName;
    FILE* m_hFile;
    COutput* m_poRoute;

    static std::map<std::string, std::string> g_csFilter;
    static std::map<std::string, COutput*> g_cpoTargets;
    static std::map<std::string, COutput*> g_cpoDestinations;
};

template <typename T>
inline std::string to_hex(T i)
{
    std::stringstream stream;
    stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
    return stream.str();
}

template <>
inline std::string to_hex<char>(char i)
{
    std::stringstream stream;
    stream << "0x" << std::setfill('0') << std::setw(2) << std::hex << (unsigned)((unsigned char)i);
    return stream.str();
}

std::string mixed_utf8_latin1_to_latin1(std::string_view input, char replacement = '?');

#endif  // __UTILITY_H__

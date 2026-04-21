/****************************************************************************
 *   $Source: D:\\Development\\Repository/ETools/Vorlage/VPP.cpp,v $
 *   $Author: ssh $
 *     $Date: 2003/07/01 09:39:30 $
 * $Revision: 1.1 $
 *    $State: Exp $
 * Copyright: (c) Copyright 2000 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Algemeine Utility-Funktionen
 *****************************************************************************
 *
 * $Log: VPP.cpp,v $
 * Revision 1.1  2003/07/01 09:39:30  ssh
 * *** empty log message ***
 *
 * Revision 1.1  2003/07/01 09:19:28  ssh
 * Initial recvsing of Source...
 *
 *
 *****************************************************************************/

#include <EBase/Expression.h>
#include <EBase/Utility.h>
#include <EBase/regexp.h>
#include <pbemtools/version.h>

#include <cstring>
#include <ctime>
#include <fstream>

size_t g_nLineSize = 100;

extern std::string GetConfigFileName();
extern bool DoUserFunction(const std::string& sName, ArgumentList& coArgs, Value* poVal);

#define VERSIONINFO "VPP " PBEMTOOLS_VERSION_STRING_LONG

std::string GetConfigFileName()
{
    return "";
}

bool DoUserFunction(const std::string& sName, ArgumentList& coArgs, Value* poVal)
{
    return false;
}

int main(int argc, char* argv[])
{
    std::fstream oIS;
    std::string sLine;
    std::string sFileName;
    int32_t nLineNumber = 0;
    int32_t nInSize = 0;
    int32_t nOutSize = 0;
    FILE* hOut = stdout;
    bool bFile = false;
    bool bSpaceBreak = false;
    size_t p;
    int i = 1;

#ifdef _WIN32
    int NLSIZE = 2;
#else
    int NLSIZE = 1;
#endif

    setlocale(LC_CTYPE, "German");

    if (i >= argc) {
        fprintf(stderr, "\nVorlage-Post-Prozessor\n%s (Build %d) [%s]\n(C) Copyright 2000-2026 by Steffen Schuemann\n", VERSIONINFO, PBEMTOOLS_BUILD_NUMBER_EMU, __DATE__);
        fprintf(stderr, "\nAufruf:\n   VPP [Optionen] <zugvorlage> [> <ausgabedatei>]\n\n");
        fprintf(stderr, "    -o f   Die Ausgabe der Zugdatei erfolgt nicht auf stdout, sondern\n");
        fprintf(stderr, "           in die Datei mit dem Namen f\n");
        fprintf(stderr, "    -w l   Nachrichten und Info-Zeilen auf l Zeichen umbrechen\n");
        fprintf(stderr, "    -s     Nur an Spaces umbrechen und diese loeschen (z.B. fuer Sitanleta)\n");
        exit(10);
    }

    while (i < argc) {
        if (i < argc && !std::strcmp(argv[i], "-w")) {
            if (++i < argc)
                g_nLineSize = (size_t)atoi(argv[i]);
            else {
                fprintf(stderr, "FEHLER: Keine Zeilenlaenge fuer die Option '-w'!\n");
                exit(10);
            }
        }
        else if (i < argc && !strcmp(argv[i], "-o")) {
            if (++i < argc) {
                hOut = fopen(argv[i], "w+");
                if (!hOut) {
                    fprintf(stderr, "FEHLER: Konnte Datei '%s' nicht zur Ausgabe oeffnen!\n", argv[i]);
                }
                else
                    bFile = true;
            }
            else {
                fprintf(stderr, "FEHLER: Kein Dateiname fuer die Ausgabe mit Option '-o'!\n");
                exit(10);
            }
        }
        else if (!strcmp(argv[i], "-s")) {
            bSpaceBreak = true;
        }
        else if (argv[i][0] != '-') {
            if (!sFileName.empty()) {
                fprintf(stderr, "FEHLER: Mehr als eine Zugdatei angegeben!");
                exit(10);
            }
            sFileName = argv[i];
        }
        i++;
    }

    oIS.open(sFileName.c_str(), std::ios::in);

    if (oIS.fail()) {
        fprintf(stderr, "Auf die Vorlage-Datei '%s' kann nicht zugegriffen werden!", sFileName.c_str());
        exit(10);
    }

    while (1) {
        sLine.clear();

        do {
            if (!sLine.empty() && sLine[sLine.size() - 1] == '\\') {
                sLine.erase(sLine.size() - 1, 1);
            }
            std::string sLinePart;
            std::getline(oIS, sLinePart);
            if (oIS.fail())
                break;
            p = sLinePart.find_first_not_of(" \t");
            if (p == std::string::npos) {
                p = sLinePart.size();
            }
            sLinePart.erase(0, p);
            sLine += sLinePart;
            nInSize += (int)sLine.length() + NLSIZE;
            while (!sLine.empty() && sLine[sLine.size() - 1] < 32)
                sLine.erase(sLine.size() - 1, 1);
            nLineNumber++;
        } while (!sLine.empty() && sLine[sLine.size() - 1] == '\\');

        if (oIS.fail())
            break;

        if (!sLine.empty()) {
            if (sLine[0] == ';') {
                p = sLine.find_first_not_of(" \t", 1);
                if (p != std::string::npos && IsEqual(sLine.substr(p, 6), "ECHECK")) {
                    if (sLine.length() > 78)
                        fprintf(stderr, "Zeile %ld moeglicherweise zu lang:\n%s\n", nLineNumber, sLine.c_str());
                    fprintf(hOut, "%s\n", sLine.c_str());
                    nOutSize += (int)sLine.length() + NLSIZE;
                }
                else if (p != std::string::npos && IsEqual(sLine.substr(p, 7), "VERSION")) {
                    if (sLine.length() > 78)
                        fprintf(stderr, "Zeile %ld moeglicherweise zu lang:\n%s\n", nLineNumber, sLine.c_str());
                    fprintf(hOut, "%s\n", sLine.c_str());
                    nOutSize += (int)sLine.length() + NLSIZE;
                }
            }
            else {
                if (IsEqual(sLine.substr(0, 7), "EINHEIT")) {
                    std::string::size_type q, r;
                    p = sLine.find_last_of(']');
                    if (p != std::string::npos) {
                        sLine.erase(p + 1);
                        q = sLine.rfind(std::string(",b"));
                        if (q == std::string::npos)
                            q = sLine.rfind(std::string(",B"));
                        r = sLine.find_last_of('[');
                        if (r != std::string::npos && q != std::string::npos && q > r)
                            sLine.erase(q, p - q);
                    }
                }
                else if (IsEqual(sLine.substr(0, 6), "REGION")) {
                    p = sLine.find_last_of('(');
                    if (p != std::string::npos)
                        sLine.erase(p);
                }

                {
                    std::string sTemp;
                    CRegExp oRE;
                    p = 0;
                    oRE.Prepare("([^ \\t\\n\\r\\f'\"]|'([^']|\\\\')+'|\"([^\"]|\\\\\")+\")+");
                    while (oRE.Find(sLine, (int)p)) {
                        if (p) {
                            //                            putchar( ' ' );
                            sTemp += ' ';
                        }
                        //                        printf( "%s", sLine.substr( oRE.Begin(), oRE.Size() ).c_str() );
                        sTemp += sLine.substr((size_t)oRE.Begin(), (size_t)oRE.Size());
                        p = (size_t)oRE.End() + 1;
                    }
                    //                    puts("");
                    sLine = sTemp;
                }
                //                int p = sLine.find_last_of( "~ " );
                //                if(  sLine[0]!='/' && ( sLine[sLine.size()-1]=='"' ||
                //                    ( p != std::string::npos && sLine[p]=='~' ) ) )
                {
                    while (sLine.length() > g_nLineSize) {
                        if (bSpaceBreak) {
                            size_t j;
                            for (j = g_nLineSize - 1; j > 0 && sLine[j] != ' '; j--)
                                ;
                            if (j > 0) {
                                fprintf(hOut, "%s\\\n", sLine.substr(0, j).c_str());
                                sLine.erase(0, j);
                                nOutSize += i + 1 + NLSIZE;
                            }
                            else {
                                break;
                            }
                        }
                        else {
                            if (sLine[g_nLineSize - 2] == ' ') {
                                fprintf(hOut, "%s\\\n", sLine.substr(0, g_nLineSize - 1).c_str());
                                sLine.erase(0, g_nLineSize - 1);
                                nOutSize += (int)g_nLineSize + NLSIZE;
                            }
                            else {
                                fprintf(hOut, "%s\\\n", sLine.substr(0, g_nLineSize - 2).c_str());
                                sLine.erase(0, g_nLineSize - 2);
                                nOutSize += (int)g_nLineSize - 1 + NLSIZE;
                            }
                        }
                    }
                }
                if (sLine.length() > g_nLineSize) {
                    fprintf(stderr, "Zeile %ld moeglicherweise zu lang:\n%s\n", nLineNumber, sLine.c_str());
                }
                fprintf(hOut, "%s\n", sLine.c_str());
                nOutSize += (int)sLine.length() + NLSIZE;
            }
        }
    }

    if (bFile)
        fclose(hOut);

    fprintf(stderr, "\nEs konnten %.1f%% der Zeichen entfernt werden.\n", 100.0 - ((float)nOutSize * 100) / nInSize);
    //    char Buff[128];
    //    gets( Buff );

    return 0;
}

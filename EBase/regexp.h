//---------------------------------------------------------------------------------------
// regexp.h
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

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <list>
#include <string>
#include <vector>

/**
 * C++-Kapselung der Regular-Expression-Library libpcre.
 *
 * Diese Klasse kapselt die Benutzung regulärer Ausdrücke auf eine für
 * C++-Anwendung angenehmere Weise.
 * Libpcre ist von Philip Hazel, nähere Infos unter http://www.pcre.org
 *
 * @attention Die Klasse unterstützt zur Zeit nicht mehr als 96 Subexpressions!
 * @author Steffen Schümann, Hamburg
 */
class CRegExp
{
public:
    /** Veränderungsfunktion für ReplaceCall.
     * @param string Hier wird der Treffer übergeben, kann verändert werden
     *               und wird hinterher wieder eingefügt.
     */
    typedef void (*ModifyFunction)(std::string&);
    /** Veränderungsfunktion für ReplaceCall.
     * @param string1 Hier wird der Replace-Text übergeben
     * @param string2 Hier wird der Treffer übergeben, kann verändert werden
     *                und wird hinterher wieder eingefügt.
     */
    typedef void (*ModifyFunction2)(const std::string&, std::string&);

    /** Konstruktor der Klasse.
     * Der Konstruktor initialisiert, bei übergebenem Pattern, die Auswertung.
     * @param sRE Ausdruck der für das Matching gelten soll.
     * @see   Prepare()
     */
    CRegExp(const std::string& sRE = std::string(), int nMode = 0);

    /** Destruktor. */
    ~CRegExp();

    /** Neues Muster übergeben und initialisieren
     * @param  sRE Ausdruck der für das Matching gelten soll.
     * @param  RE-Optionen aus libpcre
     * @return Trat ein Fehler auf, wird <tt>false</tt>, andernfalls
     *         <tt>true</tt> zurückgegeben.
     */
    bool Prepare(const std::string& sRE, int nMode = 0);

    /** Durchsuchen eines Textes.
     * Die Methode dient dem Durchführen einer Suche auf einem übergebenen Text,
     * ab einer optional angegebenen Position.
     * @param  sText Text der duchsucht werden soll.
     * @param  nPos  Position ab der gesucht werden soll.
     * @return Wurde ein Treffer gefunden wird <tt>true</tt>, andernfalls
     *         <tt>false</tt> zurückgegeben.
     */
    bool Find(const std::string& sText, int nPos = 0);

    /** Anzahl der Teilausdrücke inclusive Gesamttreffer.
     * Mit dieser Methode wird die Anzahl der (Teil-)Ausdrücke zurückgegeben
     * die gepasst haben. Dabei ist der Treffer insgesammt enthalten.
     */
    int Count() const { return m_nCount; }

    /** Startoffset des nIdx-ten (Teil-)Ausdrucks ermitteln (0=Gesammtausdruck)*/
    int Begin(int nIdx = 0) const { return (m_nCount > 0 && nIdx >= 0 && nIdx < m_nCount) ? (int)m_pOffsets[nIdx * 2] : 0; }

    /** Endoffset des nIdx-ten (Teil-)Ausdrucks ermitteln (0=Gesammtausdruck)*/
    int End(int nIdx = 0) const { return (m_nCount > 0 && nIdx >= 0 && nIdx < m_nCount) ? (int)m_pOffsets[nIdx * 2 + 1] : 0; }

    /** Größe des nIdx-ten (Teil-)Ausdrucks ermitteln (0=Gesammtausdruck)*/
    int Size(int nIdx = 0) const { return (m_nCount > 0 && nIdx >= 0 && nIdx < m_nCount) ? (int)(m_pOffsets[nIdx * 2 + 1] - m_pOffsets[nIdx * 2]) : 0; }

    /** Teilstring aus Text gemäß eines Subpatterns extrahieren (0=Gesammtausdruck)*/
    std::string SubStr(const std::string& sText, int nIdx = 0) const { return Begin(nIdx) < 0 ? "" : sText.substr(size_t(Begin(nIdx)), size_t(Size(nIdx))); }

    /** Fehlermeldung eines aufgetretenen Fehlers in dem Ausdruck ermitteln */
    std::string Error() const { return m_sLastError; }

    /** Verändern der verwendeten Locale-Einstellungen des Pattern-Matchers.
     */
    static void UseLocale();

    /** Vereinfachte Abfrage, ob der Ausdruck auf den Text passt.
     * Um Festzustellen ob der Text den Ausdruck kommplett erfüllt,
     * muß der Ausdruck mit Ankern versehen sein, also '^' bzw. '$'.
     * @param  sText Text der duchsucht werden soll.
     * @param  sRE   Regulärer Ausdruck der auf den Text passen soll.
     * @return Wurde ein Treffer gefunden wird <tt>true</tt>, andernfalls
     *         <tt>false</tt> zurückgegeben.
     * @attention Um auf viele Texte nach einem Treffer zu suchen sollte
     *            besser eine CRegExp-Instanz erzeugt werden und mit
     *            Find() gesucht werden, um kein wiederholtes übersetzen
     *            des regulären Ausdrucks zu erzwingen.
     */
    static bool Match(const std::string& sText, const std::string& sRE);

    /** Vereinfachter Aufruf um alle Vorkommen eines Ausdrucks in einem Text zu ersetzen.
     * @param  sText Text der duchsucht werden soll.
     * @param  sRE   Regulärer Ausdruck der auf den Text passen soll.
     * @param  sRep  Text durch den die Treffer ersetzt werden sollen.
     * @return Ging alles gut wird <tt>true</tt>, bei einem Fehler im Ausdruck
     *         <tt>false</tt> zurückgegeben.
     */
    static bool Replace(std::string& sText, const std::string& sRE, const std::string& sRep);

    /** Vereinfachter Aufruf um alle Vorkommen diverse Ausdrücke in einem Text zu ersetzen.
     * @param  sText Text der duchsucht werden soll.
     * @param  coRE  Vector regulärer Ausdrücke die auf den Text passen sollen.
     * @param  coRep Vector von Ersetzungstexten mit gleicher Anzahl wie coRE.
     * @return Ging alles gut wird <tt>true</tt>, bei einem Fehler im Ausdruck
     *         <tt>false</tt> zurückgegeben.
     */
    static bool Replace(std::string& sText, const std::vector<std::string>& coRE, const std::vector<std::string>& coRep);

    /** Vereinfachter Aufruf um alle Vorkommen diverse Ausdrücke in einem Text zu ersetzen.
     * @param  sText Text der duchsucht werden soll.
     * @param  ppcRE  Array regulärer Ausdrücke die auf den Text passen sollen.
     * @param  ppcRep Array von Ersetzungstexten mit gleicher Anzahl wie ppcRE.
     * @return Ging alles gut wird <tt>true</tt>, bei einem Fehler im Ausdruck
     *         <tt>false</tt> zurückgegeben.
     */
    static bool Replace(std::string& sText, const char** ppcRE, const char** ppcRep);

    /** Vereinfachter Aufruf um Vorkommen eines Ausdrucks CallBack-Modifiziert zu ersetzen/verändern.
     * @param  sText Text der duchsucht werden soll.
     * @param  sRE   Regulärer Ausdruck der auf den Text passen soll.
     * @param  pfMod Funktionszeiger auf eine Funktion vom Typ ModifyFunction, die
     *               die Treffer per Referenz bekommt und verändern kann.
     * @return Ging alles gut wird <tt>true</tt>, bei einem Fehler im Ausdruck
     *         <tt>false</tt> zurückgegeben.
     */
    static bool ReplaceCall(std::string& sText, const std::string& sRE, ModifyFunction pfMod);

    /** Vereinfachter Aufruf um Vorkommen eines Ausdrucks CallBack-Modifiziert zu ersetzen/verändern.
     * @param  sText Text der duchsucht werden soll.
     * @param  sRE   Regulärer Ausdruck der auf den Text passen soll.
     * @param  sRep  Text-(Vorschlag) durch den die Treffer ersetzt werden sollen.
     * @param  pfMod Funktionszeiger auf eine Funktion vom Typ ModifyFunction, die
     *               den Textvorschlag und die Treffer per Referenz bekommt und
     *               verändern kann.
     * @return Ging alles gut wird <tt>true</tt>, bei einem Fehler im Ausdruck
     *         <tt>false</tt> zurückgegeben.
     */
    static bool ReplaceCall(std::string& sText, const std::string& sRE, const std::string& sRep, ModifyFunction2 pfMod);

    /** Vereinfachter Aufruf um alle Vorkommen eines Ausdrucks in einem Text regelbasiert zu ersetzen.
     * Es finden die Ersetzungsregeln von Perl Verwendung, d.h. im Ersetzungstext können Regeln
     * angegeben werden:
     * $0 o. $& Fügt den gesammten Treffertext ein.
     * $i       Fügt den Treffertext des i-ten Subpatterns ein.
     * $+       Fügt den Treffertext des letzten Subpatterns ein.
     * $`       Fügt den Text vor dem Treffer ein.
     * $´       Fügt den Text nach dem Treffer ein.
     * @param  sText Text der duchsucht werden soll.
     * @param  sRE   Regulärer Ausdruck der auf den Text passen soll.
     * @param  sRep  Text durch den die Treffer ersetzt werden sollen.
     * @return Ging alles gut wird <tt>true</tt>, bei einem Fehler im Ausdruck
     *         <tt>false</tt> zurückgegeben.
     */
    static bool RuledReplace(std::string& sText, const std::string& sRE, const std::string& sRep);

    /** Vereinfachter Aufruf um alle Vorkommen diverser Ausdrücke in einem Text regelbasiert zu ersetzen.
     * Es finden die Ersetzungsregeln von Perl Verwendung, d.h. im Ersetzungstext können Regeln
     * angegeben werden:
     * $0 o. $& Fügt den gesammten Treffertext ein.
     * $i       Fügt den Treffertext des i-ten Subpatterns ein.
     * $+       Fügt den Treffertext des letzten Subpatterns ein.
     * $`       Fügt den Text vor dem Treffer ein.
     * $´       Fügt den Text nach dem Treffer ein.
     * @param  sText  Text der duchsucht werden soll.
     * @param  coRE   Vector mit regulären Ausdrücken die auf den Text passen sollen.
     * @param  coRep  Vector mit Texten durch den die Treffer ersetzt werden sollen.
     * @return Ging alles gut wird <tt>true</tt>, bei einem Fehler im Ausdruck
     *         <tt>false</tt> zurückgegeben.
     */
    static bool RuledReplace(std::string& sText, const std::vector<std::string>& coRE, const std::vector<std::string>& coRep);

    /** Vereinfachter Aufruf um alle Vorkommen diverser Ausdrücke in einem Text regelbasiert zu ersetzen.
     * Es finden die Ersetzungsregeln von Perl Verwendung, d.h. im Ersetzungstext können Regeln
     * angegeben werden:
     * $0 o. $& Fügt den gesammten Treffertext ein.
     * $i       Fügt den Treffertext des i-ten Subpatterns ein.
     * $+       Fügt den Treffertext des letzten Subpatterns ein.
     * $`       Fügt den Text vor dem Treffer ein.
     * $´       Fügt den Text nach dem Treffer ein.
     * @param  sText  Text der duchsucht werden soll.
     * @param  ppcRE  Array mit regulären Ausdrücken die auf den Text passen sollen.
     * @param  ppcRep Array mit Texten durch den die Treffer ersetzt werden sollen.
     * @return Ging alles gut wird <tt>true</tt>, bei einem Fehler im Ausdruck
     *         <tt>false</tt> zurückgegeben.
     */
    static bool RuledReplace(std::string& sText, const char** ppcRE, const char** ppcRep);

private:
    std::string m_sRE;
    std::string m_sLastError;
    pcre2_code* m_pRE;
    pcre2_match_data* m_pMatchData;
    int m_nCount;
    size_t* m_pOffsets;

    static const unsigned char* m_pCharTables;
};

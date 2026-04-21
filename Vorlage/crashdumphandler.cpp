//---------------------------------------------------------------------------------------
// crashdumphandler.cpp
//---------------------------------------------------------------------------------------
//
// Copyright (c) 2005, Steffen Schümann <s.schuemann@pobox.com>
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
#define NOMINMAX

#include "crashdumphandler.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <vector>

extern std::vector<std::string> g_coCallStack;

#ifdef _WIN32
#include <direct.h>
#include <imagehlp.h>
#include <thirdparty/tinyformat/tinyformat.h>
#include <windows.h>

class CrashDumpHandler::Private
{
private:
    LPTOP_LEVEL_EXCEPTION_FILTER _pfnOldHandler;
    static Private* g_pCDH;

    typedef BOOL(__stdcall* SYMINITIALIZEPROC)(HANDLE, LPSTR, BOOL);
    typedef BOOL(__stdcall* SYMCLEANUPPROC)(HANDLE);
    typedef BOOL(__stdcall* STACKWALKPROC)(DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID, PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE, PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);
    typedef LPVOID(__stdcall* SYMFUNCTIONTABLEACCESSPROC)(HANDLE, DWORD);
    typedef DWORD(__stdcall* SYMGETMODULEBASEPROC)(HANDLE, DWORD);
    typedef BOOL(__stdcall* SYMGETSYMFROMADDRPROC)(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL);
    typedef DWORD(__stdcall* SYMGETOPTIONSPROC)();
    typedef DWORD(__stdcall* SYMSETOPTIONSPROC)(DWORD);
    SYMINITIALIZEPROC _SymInitialize;
    SYMCLEANUPPROC _SymCleanup;
    STACKWALKPROC _StackWalk;
    SYMFUNCTIONTABLEACCESSPROC _SymFunctionTableAccess;
    SYMGETMODULEBASEPROC _SymGetModuleBase;
    SYMGETSYMFROMADDRPROC _SymGetSymFromAddr;
    SYMGETOPTIONSPROC _SymGetOptions;
    SYMSETOPTIONSPROC _SymSetOptions;
    std::string _sAppInfo;

public:
    Private(const std::string& sAppInfo)
        : _sAppInfo(sAppInfo)
    {
        g_pCDH = this;
        _pfnOldHandler = SetUnhandledExceptionFilter(UnhandledExceptionFilter);
    }

    ~Private()
    {
        SetUnhandledExceptionFilter(_pfnOldHandler);
        _pfnOldHandler = 0;
        g_pCDH = 0;
    }

    static LONG WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
    {
        PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;
        if (pExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT) {
            std::ofstream dumpFile("crashinfo.txt");
            time_t ti;
            time(&ti);
            dumpFile << "Application: " << g_pCDH->_sAppInfo << " (Win32)\nCrash-Time: " << ctime(&ti) << std::endl;
            dumpFile << "Skript stack:\n";
            for (size_t i = 0; i < g_coCallStack.size(); i++) {
                dumpFile << i << "  " << g_coCallStack[i] << std::endl;
            }
            dumpFile << std::endl;

            g_pCDH->HandleCrash(dumpFile, pExceptionInfo);

            dumpFile.close();
            fflush(stdout);
            fprintf(stderr, "\n\nVorlage wurde wegen eines schwerwiegenden Fehlers beendet! Infos bitte der angelegten Datei 'crashinfo.txt' entnehmen und bei Bedarf an s.schuemann@pobox.com senden!\n\n");
            fflush(stderr);
            abort();
            return EXCEPTION_CONTINUE_SEARCH;
        }
        if (g_pCDH->_pfnOldHandler) {
            return g_pCDH->_pfnOldHandler(pExceptionInfo);
        }
        else {
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }

    void HandleCrash(std::ostream& os, PEXCEPTION_POINTERS pExceptionInfo)
    {
        PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;
        os << tfm::format("Exception code: %08X\n", pExceptionRecord->ExceptionCode);
        TCHAR szFaultingModule[MAX_PATH];
        DWORD section, offset;
        GetLogicalAddress(pExceptionRecord->ExceptionAddress, szFaultingModule, sizeof(szFaultingModule), section, offset);
        os << tfm::format("Fault address:  %08X %02X:%08X %s\n", pExceptionRecord->ExceptionAddress, section, offset, szFaultingModule);
        PCONTEXT pCtx = pExceptionInfo->ContextRecord;  // Show the registers
#ifdef _M_IX86                                          // Intel Only!
        os << "\nRegisters:\n";
        os << tfm::format("EAX:%08X  EBX:%08X  ECX:%08X  EDX:%08X\nESI:%08X  EDI:%08X                EBP:%08X\n", pCtx->Eax, pCtx->Ebx, pCtx->Ecx, pCtx->Edx, pCtx->Esi, pCtx->Edi, pCtx->Ebp);
        os << tfm::format("CS:EIP:      %04X:%08X  SS:ESP:      %04X:%08X\n", pCtx->SegCs, pCtx->Eip, pCtx->SegSs, pCtx->Esp);
        os << tfm::format("DS:%04X  ES:%04X  FS:%04X  GS:%04X      Flags:%08X\n", pCtx->SegDs, pCtx->SegEs, pCtx->SegFs, pCtx->SegGs, pCtx->EFlags);
#endif
        os.flush();
        if (!InitImagehlpFunctions()) {
            os << "(IMAGEHLP.DLL or its exported procs not found, using fallback.)\n";
#ifdef _M_IX86  // Intel Only!
            // Walk the stack using x86 specific code
            WintelStackWalk(os, pCtx);
#else
            os << "Unsopported plattform, couldn't walk stack.\n";
#endif
            return;
        }
        ImagehlpStackWalk(os, pCtx);
        _SymCleanup(GetCurrentProcess());
        os << std::endl;
    }

    void WintelStackWalk(std::ostream& os, PCONTEXT pContext)
    {
        os << "\nCall stack:\n";
        os << "L#  Address   Frame     Logical addr  Module\n";
        DWORD pc = pContext->Eip;
        PDWORD pFrame, pPrevFrame;
        pFrame = (PDWORD)pContext->Ebp;
        int lvl = 0;
        do {
            TCHAR szModule[MAX_PATH] = "";
            DWORD section = 0, offset = 0;
            GetLogicalAddress((PVOID)pc, szModule, sizeof(szModule), section, offset);
            os << tfm::format("%02d  %08X  %08X  %04X:%08X %s\n", lvl++, pc, pFrame, section, offset, szModule);
            pc = pFrame[1];
            pPrevFrame = pFrame;
            pFrame = (PDWORD)pFrame[0];  // proceed to next higher frame on stack
            if ((DWORD)pFrame & 3)       // Frame pointer must be aligned on a
                break;                   // DWORD boundary.  Bail if not so.
            if (pFrame <= pPrevFrame)
                break;
            // Can two DWORDs be read from the supposed frame address?
            if (IsBadWritePtr(pFrame, sizeof(PVOID) * 2))
                break;
        } while (1);
    }

    // Walks the stack, and writes the results to the report file
    void ImagehlpStackWalk(std::ostream& os, PCONTEXT pContext)
    {
        os << "\nCall stack:\n";
        os << "L#  Address   Frame     Logical addr  Symbol/Module\n";

        // Could use SymSetOptions here to add the SYMOPT_DEFERRED_LOADS flag
        STACKFRAME sf;
        memset(&sf, 0, sizeof(sf));
        // Initialize the STACKFRAME structure for the first call.  This is only
        // necessary for Intel CPUs, and isn't mentioned in the documentation.
        sf.AddrPC.Offset = pContext->Eip;
        sf.AddrPC.Mode = AddrModeFlat;
        sf.AddrStack.Offset = pContext->Esp;
        sf.AddrStack.Mode = AddrModeFlat;
        sf.AddrFrame.Offset = pContext->Ebp;
        sf.AddrFrame.Mode = AddrModeFlat;
        int lvl = 0;
        while (1) {
            if (!_StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &sf, pContext, 0, _SymFunctionTableAccess, _SymGetModuleBase, 0))
                break;
            if (0 == sf.AddrFrame.Offset)  // Basic sanity check to make sure
                break;                     // the frame is OK.  Bail if not.
            os << tfm::format("%02d  %08X  %08X  ", lvl++, sf.AddrPC.Offset, sf.AddrFrame.Offset);
            // IMAGEHLP is wacky, and requires you to pass in a pointer to an
            // IMAGEHLP_SYMBOL structure.  The problem is that this structure is
            // variable length.  That is, you determine how big the structure is
            // at runtime.  This means that you can't use sizeof(struct).
            // So...make a buffer that's big enough, and make a pointer
            // to the buffer.  We also need to initialize not one, but TWO
            // members of the structure before it can be used.
            BYTE symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 512];
            PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
            pSymbol->SizeOfStruct = sizeof(symbolBuffer);
            pSymbol->MaxNameLength = 512;
            DWORD symDisplacement = 0;  // Displacement of the input address,
                                        // relative to the start of the symbol
            TCHAR szModule[MAX_PATH] = "";
            DWORD section = 0, offset = 0;
            GetLogicalAddress((PVOID)sf.AddrPC.Offset, szModule, sizeof(szModule), section, offset);

            if (_SymGetSymFromAddr(GetCurrentProcess(), sf.AddrPC.Offset, &symDisplacement, pSymbol)) {
                os << tfm::format("%04X:%08X %hs+%X %s\n", section, offset, pSymbol->Name, symDisplacement, szModule);
            }
            else {
                os << tfm::format("%04X:%08X %s\n", section, offset, szModule);
            }
        }
    }

    // by the len parameter (in characters!)
    bool GetLogicalAddress(PVOID addr, PTSTR szModule, DWORD len, DWORD& section, DWORD& offset)
    {
        if (addr == NULL)
            return false;

        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(addr, &mbi, sizeof(mbi)))
            return false;
        DWORD hMod = (DWORD)mbi.AllocationBase;
        if (!GetModuleFileName((HMODULE)hMod, szModule, len))
            return false;  // Point to the DOS header in memory
        PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;
        // From the DOS header, find the NT (PE) header
        PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)(hMod + pDosHdr->e_lfanew);
        PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNtHdr);
        DWORD rva = (DWORD)addr - hMod;  // RVA is offset from module load address
        // Iterate through the section table, looking for the one that encompasses
        // the linear address.
        for (unsigned i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++, pSection++) {
            DWORD sectionStart = pSection->VirtualAddress;
            DWORD sectionEnd = sectionStart + std::max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);
            // Is the address in this section???
            if ((rva >= sectionStart) && (rva <= sectionEnd)) {
                // Yes, address is in the section.  Calculate section and offset,
                // and store in the "section" & "offset" params, which were
                // passed by reference.
                section = i + 1;
                offset = rva - sectionStart;
                return true;
            }
        }
        return false;  // Should never get here!
    }

    bool InitImagehlpFunctions()
    {
        HMODULE hModImagehlp = LoadLibrary("IMAGEHLP.DLL");
        if (!hModImagehlp)
            return FALSE;
        _SymInitialize = (SYMINITIALIZEPROC)GetProcAddress(hModImagehlp, "SymInitialize");
        if (!_SymInitialize)
            return FALSE;
        _SymCleanup = (SYMCLEANUPPROC)GetProcAddress(hModImagehlp, "SymCleanup");
        if (!_SymCleanup)
            return FALSE;
        _StackWalk = (STACKWALKPROC)GetProcAddress(hModImagehlp, "StackWalk");
        if (!_StackWalk)
            return FALSE;
        _SymFunctionTableAccess = (SYMFUNCTIONTABLEACCESSPROC)GetProcAddress(hModImagehlp, "SymFunctionTableAccess");
        if (!_SymFunctionTableAccess)
            return FALSE;
        _SymGetModuleBase = (SYMGETMODULEBASEPROC)GetProcAddress(hModImagehlp, "SymGetModuleBase");
        if (!_SymGetModuleBase)
            return FALSE;
        _SymGetSymFromAddr = (SYMGETSYMFROMADDRPROC)GetProcAddress(hModImagehlp, "SymGetSymFromAddr");
        if (!_SymGetSymFromAddr)
            return FALSE;
        _SymSetOptions = (SYMSETOPTIONSPROC)GetProcAddress(hModImagehlp, "SymSetOptions");
        if (!_SymSetOptions)
            return FALSE;
        _SymGetOptions = (SYMGETOPTIONSPROC)GetProcAddress(hModImagehlp, "SymGetOptions");
        if (!_SymGetOptions)
            return FALSE;

        DWORD dwOpts = _SymGetOptions();

        // Always defer loading to make life faster.
        _SymSetOptions(dwOpts | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

        char pcPath[_MAX_PATH];
        std::string sPath;
        if (_getcwd(pcPath, _MAX_PATH) != NULL) {
            sPath = pcPath;
            sPath += ";";
        }
        else {
            sPath = ".;";
        }
        sPath += getenv("PATH");
        if (!_SymInitialize(GetCurrentProcess(), (LPSTR)sPath.c_str(), TRUE))
            return FALSE;
        return TRUE;
    }
};

CrashDumpHandler::Private* CrashDumpHandler::Private::g_pCDH = 0;

#else

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>

class CrashDumpHandler::Private
{
private:
    sig_t _pfnOldSEGVHandler;
    sig_t _pfnOldILLHandler;
    sig_t _pfnOldFPEHandler;
    sig_t _pfnOldPIPEHandler;
    // sig_t _pfnOldBUSHandler;
    // sig_t _pfnOldABRTHandler;
    std::string _sAppInfo;

public:
    Private(const std::string& sAppInfo);
    ~Private();
    void HandleCrash(const char* reason);
    void DumpTrace(std::ofstream& os);
};

CrashDumpHandler::Private* g_pCDH = 0;
bool doneDump = false;

static void SigSEGVHandler(int rc)
{
    g_pCDH->HandleCrash("segment violation");
}

static void SigILLHandler(int rc)
{
    g_pCDH->HandleCrash("illegal instruction");
}

static void SigFPEHandler(int rc)
{
    g_pCDH->HandleCrash("floating point exception");
}

static void SigPIPEHandler(int rc)
{
    g_pCDH->HandleCrash("write on single ended pipe");
}

/*
static void SigBUSHandler(int rc)
{
    g_pCDH->HandleCrash( "bus error" );
}

static void SigABRTHandler(int rc)
{
    g_pCDH->HandleCrash( "abort program" );
}
 */

CrashDumpHandler::Private::Private(const std::string& sAppInfo)
    : _sAppInfo(sAppInfo)
{
    g_pCDH = this;
    _pfnOldSEGVHandler = signal(SIGSEGV, &SigSEGVHandler);
    _pfnOldILLHandler = signal(SIGILL, &SigILLHandler);
    _pfnOldFPEHandler = signal(SIGFPE, &SigFPEHandler);
    _pfnOldPIPEHandler = signal(SIGPIPE, &SigPIPEHandler);
    //    _pfnOldBUSHandler = signal(SIGBUS, &SigBUSHandler);
    //    _pfnOldABRTHandler = signal(SIGABRT, &SigABRTHandler);
}

CrashDumpHandler::Private::~Private()
{
    signal(SIGSEGV, _pfnOldSEGVHandler);
    signal(SIGILL, _pfnOldILLHandler);
    signal(SIGFPE, _pfnOldFPEHandler);
    signal(SIGPIPE, _pfnOldPIPEHandler);
    //    signal(SIGBUS, _pfnOldBUSHandler);
    //    signal(SIGABRT, _pfnOldABRTHandler);
}

void CrashDumpHandler::Private::HandleCrash(const char* reason)
{
    std::ofstream dumpFile("crashinfo.txt");
    time_t ti;
    static void* array[10];
    size_t size;
    char** strings;
    size_t i;

    time(&ti);
    dumpFile << "Application: " << _sAppInfo << " (Linux)\nCrash-Time: " << ctime(&ti) << "\nReason: " << reason << std::endl;

    dumpFile << "Skript stack:\n";
    for (i = 0; i < g_coCallStack.size(); i++) {
        dumpFile << i << "  " << g_coCallStack[i] << std::endl;
    }
    size = static_cast<size_t>(backtrace(array, 10));
    strings = backtrace_symbols(array, static_cast<int>(size));
    dumpFile << "\nProcess trace (" << size << " frames):" << std::endl;

    for (i = 0; i < size; i++)
        dumpFile << strings[i] << std::endl;

    free(strings);
    dumpFile.close();

    fflush(stdout);
    fprintf(stderr, "\n\nVorlage wurde wegen eines schwerwiegenden Fehlers beendet! Infos bitte der angelegten Datei 'crashinfo.txt' entnehmen und bei Bedarf an s.schuemann@pobox.com senden!\n\n");
    fflush(stderr);
    abort();
}

void CrashDumpHandler::Private::DumpTrace(std::ofstream& os) {}

#endif  // _WIN32

CrashDumpHandler::CrashDumpHandler(const std::string& sAppInfo)
    : _pimpl(new Private(sAppInfo))
{
}

CrashDumpHandler::~CrashDumpHandler()
{
    delete _pimpl;
}

#include "Assert.hpp"
#include "IO/Log.hpp"
#include "Memory/String.hpp"

void AzCore::_AssertFailure(const char *file, const char *line, const char *message) {
	AzCore::io::cerr.PrintLn("\033[96m", file, "\033[0m:\033[96m", line, "\033[0m Assert failed: \033[91m", message, "\033[0m");
	AzCore::PrintBacktrace(io::cerr);
	exit(1);
}

void AzCore::_AssertFailure(const char *file, const char *line, const AzCore::String &message) {
	AzCore::io::cerr.PrintLn("\033[96m", file, "\033[0m:\033[96m", line, "\033[0m Assert failed: \033[91m", message, "\033[0m");
	AzCore::PrintBacktrace(io::cerr);
	exit(1);
}

#ifdef __unix
#include <execinfo.h>
void AzCore::PrintBacktrace(io::Log &log) {
	constexpr size_t stackSizeMax = 256;
	void *array[stackSizeMax];
	int size;

	size = backtrace(array, stackSizeMax);
	log.PrintLn("Backtrace:");
	char **memory = backtrace_symbols(array, size);
	log.IndentMore();
	for (i32 i = 0; i < size; i++) {
		log.PrintLn(memory[i]);
	}
	log.IndentLess();
	free(memory);
}

#elif defined(_MSC_VER)

#include <windows.h>
#include <winnt.h>
#include <imagehlp.h>
#include <psapi.h>

namespace AzCore {

void* _GetImageBasePointer(HANDLE process, DWORD pid) {
	HMODULE module;
	DWORD dummy;
	if (!EnumProcessModules(process, &module, sizeof(HMODULE), &dummy)) {
		fprintf(stderr, "Failed to EnumProcessModules: %i\n", GetLastError());
		return nullptr;
	}
	MODULEINFO moduleInfo;
	if (!GetModuleInformation(process, module, &moduleInfo, sizeof(MODULEINFO))) {
		fprintf(stderr, "Failed to GetModuleInformation: %i\n", GetLastError());
		return nullptr;
	}
	return moduleInfo.lpBaseOfDll;
}

STACKFRAME64 _GetStackFrame(const CONTEXT &context) {
	STACKFRAME64 stackFrame;
	stackFrame.AddrPC.Offset = context.Rip;
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context.Rsp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.Rbp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	return stackFrame;
}

void _PrintSymbolName(io::Log &log, HANDLE hProcess, DWORD64 offset) {
	IMAGEHLP_SYMBOL64 *symbol;
	size_t sizeofSymbol = sizeof(IMAGEHLP_SYMBOL64) + 1024;
	symbol = (IMAGEHLP_SYMBOL64*)malloc(sizeofSymbol);
	memset(symbol, 0, sizeofSymbol);
	symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	symbol->MaxNameLength = 1024;
	DWORD64 displacement;
	if (!SymGetSymFromAddr64(hProcess, offset, &displacement, symbol)) {
		log.Print("<symbol name error ", (i32)GetLastError(), ">");
	} else {
		char undecoratedName[1024];
		UnDecorateSymbolName(symbol->Name, undecoratedName, 1024, UNDNAME_COMPLETE);
		log.Print(undecoratedName);
	}
	free(symbol);
}

void PrintBacktrace(io::Log &log) {
	HANDLE hProcess = GetCurrentProcess();
	DWORD pid = GetCurrentProcessId();
	HANDLE hThread = GetCurrentThread();
	void *imageBasePointer = _GetImageBasePointer(hProcess, pid);
	if (imageBasePointer == nullptr) return;
	IMAGE_NT_HEADERS *ntHeaders = ImageNtHeader(imageBasePointer);
	
	if (!SymInitialize(hProcess, NULL, true)) {
		log.PrintLn("SymInitialize failed :(");
		return;
	}
	
	SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
	
	CONTEXT context;
	RtlCaptureContext(&context);
	
	STACKFRAME64 stackFrame = _GetStackFrame(context);
	IMAGEHLP_LINE64 line = {0};
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	
	log.PrintLn("Backtrace:");
	i32 frame = 0;
	log.IndentMore();
	while (StackWalk64(ntHeaders->FileHeader.Machine, hProcess, hThread, &stackFrame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
		log.Print(frame, "\t");
		if (stackFrame.AddrPC.Offset != 0) {
			_PrintSymbolName(log, hProcess, stackFrame.AddrPC.Offset);
			DWORD displacement;
			if (SymGetLineFromAddr64(hProcess, stackFrame.AddrPC.Offset, &displacement, &line)) {
				log.Print("\t", (const char *)line.FileName, ":", (i32)line.LineNumber);
			}
		} else {
			break;
		}
		frame++;
		log.Newline();
	}
	log.IndentLess();
	SymCleanup(hProcess);
}

} // namespace AzCore

#endif // __unix, _MSC_VER
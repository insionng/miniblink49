﻿
#include "electron.h"

#include "common/ThreadCall.h"
#include "common/NodeRegisterHelp.h"
#include "common/NodeThread.h"
#include "common/AtomCommandLine.h"
#include "gin/unzip.h"
#include "NodeBlink.h"
#include <windows.h>
#include <ShellAPI.h>

using namespace v8;
using namespace node;

#if USING_VC6RT == 1
void __cdecl operator delete(void * p, unsigned int)
{
    ::free(p);
}

extern "C" int __security_cookie = 0;
#endif

namespace atom {

unzFile gPeResZip = nullptr;

NODE_MODULE_CONTEXT_AWARE_BUILTIN_SCRIPT_DECLARE_IN_MAIN(atom_browser_web_contents)
NODE_MODULE_CONTEXT_AWARE_BUILTIN_SCRIPT_DECLARE_IN_MAIN(atom_browser_app)
NODE_MODULE_CONTEXT_AWARE_BUILTIN_SCRIPT_DECLARE_IN_MAIN(atom_browser_electron)
NODE_MODULE_CONTEXT_AWARE_BUILTIN_SCRIPT_DECLARE_IN_MAIN(atom_browser_window)
NODE_MODULE_CONTEXT_AWARE_BUILTIN_SCRIPT_DECLARE_IN_MAIN(atom_browser_menu);

static void registerNodeMod() {
    NODE_MODULE_CONTEXT_AWARE_BUILTIN_SCRIPT_DEFINDE_IN_MAIN(atom_browser_web_contents);
    NODE_MODULE_CONTEXT_AWARE_BUILTIN_SCRIPT_DEFINDE_IN_MAIN(atom_browser_app);
    NODE_MODULE_CONTEXT_AWARE_BUILTIN_SCRIPT_DEFINDE_IN_MAIN(atom_browser_electron);
    NODE_MODULE_CONTEXT_AWARE_BUILTIN_SCRIPT_DEFINDE_IN_MAIN(atom_browser_window);
    NODE_MODULE_CONTEXT_AWARE_BUILTIN_SCRIPT_DEFINDE_IN_MAIN(atom_browser_menu);
}

static void initPeRes(HINSTANCE hInstance) {
    PIMAGE_DOS_HEADER pDosH = (PIMAGE_DOS_HEADER)hInstance;
    if (pDosH->e_magic != IMAGE_DOS_SIGNATURE)
        return; // DOS头不正确
    PIMAGE_NT_HEADERS32 pNtH = (PIMAGE_NT_HEADERS32)((DWORD)hInstance + (DWORD)pDosH->e_lfanew);
    if (pNtH->Signature != IMAGE_NT_SIGNATURE)
        return; // NT头不正确
    PIMAGE_SECTION_HEADER pSecHTemp = IMAGE_FIRST_SECTION(pNtH); // 区段头

    for (size_t index = 0; index < pNtH->FileHeader.NumberOfSections; index++) {
        if (memcmp(pSecHTemp->Name, ".pack\0\0\0", 8) == 0) { // 比较区段名
            // 找到资源包区段
            gPeResZip = unzOpen(NULL, hInstance + pSecHTemp->VirtualAddress + pSecHTemp->PointerToRawData, pSecHTemp->Misc.VirtualSize);
        }
        ++pSecHTemp;
    }
}

void nodeInitCallBack(node::NodeArgc* n) {
//     gcTimer.data = n->childEnv->isolate();
//     uv_timer_init(n->childLoop, &gcTimer);
//     uv_timer_start(&gcTimer, gcTimerCallBack, 1000 * 10, 1);

    uv_loop_t* loop = n->childLoop;
    atom::ThreadCall::init(loop);
    atom::ThreadCall::messageLoop(loop, n->v8platform, v8::Isolate::GetCurrent());
}

void nodePreInitCallBack(node::NodeArgc* n) {
    //base::SetThreadName("NodeCore");
    ThreadCall::createBlinkThread(n->v8platform);
}

} // atom

int APIENTRY wWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int argc = 0;
    wchar_t** wargv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    // Convert argv to to UTF8
    char** argv = new char*[argc];
    for (int i = 0; i < argc; i++) {
        // Compute the size of the required buffer
        DWORD size = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL);
        if (size == 0) {
            // This should never happen.
            fprintf(stderr, "Could not convert arguments to utf8.");
            return 0;
        }
        // Do the actual conversion
        argv[i] = new char[size];
        DWORD result = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], size, NULL, NULL);
        if (result == 0) {
            // This should never happen.
            fprintf(stderr, "Could not convert arguments to utf8.");
            return 0;
        }
    }
    atom::AtomCommandLine::Init(argc, argv);
    atom::AtomCommandLine::InitW(argc, wargv);
    atom::initPeRes(hInstance); // 初始化PE打包的资源
    atom::registerNodeMod();
       
    //std::wstring initScript = atom::getResourcesPath(L"init.js");
    //const wchar_t* argv1[] = { L"electron.exe", initScript.c_str() };
//     const wchar_t* argv1[] = { L"electron.exe", L"E:\\mycode\\miniblink49\\trunk\\electron\\lib\\init.js" };
//     node::NodeArgc* node = node::nodeRunThread(2, argv1, atom::nodeInitCallBack, atom::nodePreInitCallBack, nullptr);

    atom::NodeArgc* node = atom::runNodeThread();
    
    uv_loop_t* loop = uv_default_loop();
    atom::ThreadCall::messageLoop(loop, nullptr, nullptr);
    atom::ThreadCall::shutdown();

    //delete atom::kResPath;

	return 0;
}

int main() {
    return wWinMain(::GetModuleHandle(NULL), 0, 0, 0);
}
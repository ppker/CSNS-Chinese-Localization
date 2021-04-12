﻿#include "Main.h"

void main(int argc, const char* argv[])
{
    SetConsoleTitle("CSN:S汉化程序");
    Misc->DisableConsoleQuickEdit();
    Misc->SetConsoleSize(370, 380);
    Misc->SetConsoleCursor(false);
    //Misc->SetConsoleEncoding(CP_UTF8);
    Arg->Open(argc, argv);

    Misc->ConsoleClear();

    if (!mem->Attach("cstrike-online.exe"))
    {
        cout << "cstrike-online.exe 未启动...\n"
             << "请在右下角加载N社反作弊时开启此程序!\n\n"
             << "请注意以下事项：\n"
             << "- 如字体变宋体或异常请安装华康圆体W9\n"
             << "- 不保证绝对不会出现程序BUG\n"
             << "- 不保证游戏数次更新后仍可用\n\n"
             << "Made by dounai2333 (QQ1328600509)\n"
             << "二次开发: github.com/dounai2333\n\n";
        Exit(1);
    }

    cout <<"cstrike-online.exe (PID: "<< mem->m_dwProcessId << ") attached.\n"
         <<"Attach handle: "<< (int)mem->m_hProcess <<"("<< Misc->DecimalToHex((int)mem->m_hProcess, true) <<")\n\n";

    Module* hw = mem->GetModule("hw.dll");
    Module* filesystem = mem->GetModule("filesystem_nar.dll");
    cout << "hw\t\t- 0x" << (int*)hw->GetImage() << "\n";
    cout << "filesystem_nar\t- 0x" << (int*)filesystem->GetImage() << "\n\n";

    if (mem->GetModule("mp.dll"))
    {
        Misc->ConsoleClear();
        cout << "游戏已完成加载!开启此程序不会生效!\n请确保在反作弊加载时再运行!\n\n";
        Exit(2);
    }

    if (Arg->Exist("-killac"))
        system("taskkill /f /im BlackXchg.aes >nul");

    // string, original content: /cstrike_na_en/ (if user's game language isn't English, na_en will be others like na_ru etc...)
    // if filesysyem_nar.dll get any update then we need to check this
    // don't remove + 0x30 or any file in nar will override same file come from pak
    // address - 0x30 = /fixtrike/
    DWORD cstrike_na_en_addr = mem->Read<DWORD>(filesystem->GetImage() + 0xCC6E4) + 0x30;
    // string, original content: /lstrike/locale_na_en/ (if user's game language isn't English, na_en will be others like na_ru etc...)
    // if filesystem_nar.dll get any update then we need to check this
    // address + 0x30(= +0x48) = /lstrike/locale_na/ , + 0x60(= +0x78) = /lstrike/common/
    // note: the Packer (.pak) files will load from "lstrike"
    DWORD lstrike_na_en_addr = mem->Read<DWORD>(mem->Read<DWORD>(filesystem->GetImage() + 0xCC6E4) + 0x18);
    // string, original content: na_en (it's protected so we can not scan it with "writable")
    // this signature is little dangers, may it will not working anymore if has any update
    DWORD lang_addr = hw->GetImage() + find_ptr(hw, "C7 00 ? ? ? ? C7 40 04 ? ? ? ? E8 ? ? ? ? 83 C4 10 6A 00 56 57 83 EC 08 8B C4", 0x2, 0x3C);

    string cstrike_na_en_addr_text = mem->Read<str>(cstrike_na_en_addr).text;
    string lstrike_na_en_addr_text = mem->Read<str>(lstrike_na_en_addr).text;
    string lang_addr_text = mem->Read<str>(lang_addr).text;

    if (cstrike_na_en_addr_text == "/cstrike_chn/" && lstrike_na_en_addr_text == "/lstrike/locale_chn/" && lang_addr_text == "chn")
    {
        Misc->ConsoleClear();
        cout << "请勿重复操作!待游戏加载完毕后即可加载汉化\n";
        Exit(0);
    }
    else if (cstrike_na_en_addr_text != "/cstrike_na_en/" || lstrike_na_en_addr_text != "/lstrike/locale_na_en/" || lang_addr_text != "na_en")
    {
        Misc->ConsoleClear();
        cout << "检测到非法内容,可能由于游戏更新基址已过期\n\n如果您的游戏语言不是英语,\n请修改为英语后重试!\n二次开发: github.com/dounai2333\n";
        Exit(1);
    }

    mem->Write(cstrike_na_en_addr, "/cstrike_chn/");
    cout << "国服nar重定向已完成\n";

    mem->Write(lstrike_na_en_addr, "/lstrike/locale_chn/");
    cout << "国服pak重定向已完成\n";

    mem->WriteProtected(lang_addr, "chn");
    cout << "简体中文重定向已完成\n\n";

    if (!Arg->Exist("-dontblock"))
    {
        // pause the main thread so we can do the memory scan without trouble
        HANDLE handle_t = OpenThread(THREAD_ALL_ACCESS, FALSE, mem->GetThreadById(mem->m_dwProcessId));
        SuspendThread(handle_t);

        DWORD resource_addr = NULL;
        string memfile_missing_list = CheckMemFile();
        if (memfile_missing_list == "")
        {
            DWORD exitcode = RunMemScanAndGetExitCode(mem->m_dwProcessId, "h", "2F 63 73 74 72 69 6B 65 5F 63 68 6E 2F 72 65 73 6F 75 72 63 65 2F 62 61 63 6B 67 72 6F 75 6E 64 2F 38 30 30 5F 32 5F 64 5F 61 64 76 69 63 65 2E 74 67 61 00 33 00 00 00 33");
            if ((string)mem->Read<str>(exitcode).text == "/cstrike_chn/resource/background/800_2_d_advice.tga")
                resource_addr = exitcode;
            else if (exitcode == 2)
                cout << "\nnar文件缺失!\n其应被放置在Data内且不得改名!\n\n如果您正在尝试读取非国服的文件,\n那么您应该对此程序进行二次开发:\ngithub.com/dounai2333\n";
            else
                cout << "\n内存扫描出现意外错误!已跳过扫描!\n返回数值: " << Misc->DecimalToHex(exitcode, true) << "\n";
        }
        else
        {
            cout << "\n因缺少内存扫描必要文件,已跳过扫描!\n不进行内存扫描可能会导致游戏出现问题!\n缺少文件: " << memfile_missing_list << "\n";
        }

        int muted = 0;
        if (resource_addr != NULL)
        {
            for (int i = 0; i < 2048; i++)
            {
                string filename = mem->Read<str>(resource_addr + i * 0x40).text;
                if (filename.find("/cstrike_chn") == -1)
                    break;
                filename.replace(filename.begin(), filename.begin() + filename.find("/", 2) + 1, "");

                if (filename == "resource/bad_words.csv"                /*脏话屏蔽列表*/ ||
                    filename == "resource/item.csv"                     /*游戏里各种道具的定义*/ ||
                    filename == "resource/relation_product_ver2.csv"    /*商城道具的定义*/ ||
                    filename == "resource/res/popup_login.res"          /*登录UI*/ ||
                    filename == "sound/training/cstrain11.wav"          /*教程关的语音提示*/ ||
                    filename == "sound/training/cstrain1.wav"           /*(我要听小姐姐!!国服两声提示音爬爬爬)*/ ||
                    filename == "sound/training/cstrain10.wav"          ||
                    filename == "sound/training/cstrain12.wav"          ||
                    filename == "sound/training/cstrain12a.wav"         ||
                    filename == "sound/training/cstrain13.wav"          ||
                    filename == "sound/training/cstrain15.wav"          ||
                    filename == "sound/training/cstrain16.wav"          ||
                    filename == "sound/training/cstrain17.wav"          ||
                    filename == "sound/training/cstrain18.wav"          ||
                    filename == "sound/training/cstrain19.wav"          ||
                    filename == "sound/training/cstrain2.wav"           ||
                    filename == "sound/training/cstrain2a.wav"          ||
                    filename == "sound/training/cstrain3.wav"           ||
                    filename == "sound/training/cstrain3a.wav"          ||
                    filename == "sound/training/cstrain4.wav"           ||
                    filename == "sound/training/cstrain5.wav"           ||
                    filename == "sound/training/cstrain6.wav"           ||
                    filename == "sound/training/cstrain7.wav"           ||
                    filename == "sound/training/cstrain7a.wav"          ||
                    filename == "sound/training/cstrain8.wav"           ||
                    filename == "sound/training/cstrain9.wav"           ||
                    filename == "sound/training/cstrain9a.wav"          ||
                    false /* change the list here! */)
                {
                    muted++;
                    mem->Write(resource_addr + i * 0x40, "null");
                }
            }
        }

        if (Arg->Exist("-newblockmethod"))
        {
            // resume the main thread because we need to debug now
            ResumeThread(handle_t);
            CloseHandle(handle_t);

            cout << "开始监控游戏文件读取状态...\n请勿关闭汉化程序! 否则游戏将崩溃!!!\n按住Del键可在游戏加载文件时终止监控\n\n";

            // set priority of both to improve performance
            DWORD gamepri = GetPriorityClass(mem->m_hProcess);
            SetPriorityClass(GetCurrentProcess(), ((thread::hardware_concurrency() >= 4) ? REALTIME_PRIORITY_CLASS : HIGH_PRIORITY_CLASS));
            SetPriorityClass(mem->m_hProcess, HIGH_PRIORITY_CLASS);

            debug: DebugActiveProcess(mem->m_dwProcessId);
            DebugSetProcessKillOnExit(false);

            HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, mem->GetThreadById(mem->m_dwProcessId));

            // todo: add utf8 asm to the new block method and disable memory scan of resource if user are using new one

            // compilation, asm context:
            // UTF-8: mov dl,[esi+eax]
            // UTF-16: movzx esi,word ptr [eax]
            //DWORD filesystem_asm_utf8_addr = filesystem->GetImage() + find_ptr(filesystem, "8A 14 06 8A 18 3A D3 75 08 41 40 3B CF 72 F1", 0, 0, true, false);
            DWORD filesystem_asm_utf16_addr = filesystem->GetImage() + find_ptr(filesystem, "? ? 14 07 ? ? 30 66 3B D6 75 0E 83 C0 02 83 E9 01 75 EC", 0, 0, true, false);
            /*
            CONTEXT ctx_utf8;
            ctx_utf8.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            GetThreadContext(hThread, &ctx_utf8);
            ctx_utf8.Dr0 = filesystem_asm_utf8_addr;
            ctx_utf8.Dr7 = 1;
            SetThreadContext(hThread, &ctx_utf8);
            */
            CONTEXT ctx_utf16;
            ctx_utf16.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            GetThreadContext(hThread, &ctx_utf16);
            ctx_utf16.Dr0 = filesystem_asm_utf16_addr;
            ctx_utf16.Dr7 = 1;
            SetThreadContext(hThread, &ctx_utf16);

            DEBUG_EVENT dbgEvent;
            DWORD dbgFlag = DBG_CONTINUE;
            bool loop = true;
            while (loop)
            {
                if (!WaitForDebugEvent(&dbgEvent, INFINITE))
                {
                    cout << "调试器出现意外错误!已跳过监控!\n返回数值: " << Misc->DecimalToHex(GetLastError(), true) << "\n";
                    break;
                }

                dbgFlag = DBG_CONTINUE;
                if (dbgEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT && dbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
                {
                    // this error occur after new dll module has been loaded and game trying to read it but our debugger didn't know about new module
                    // a refresh is needed or process will hanging forever
                    CloseHandle(hThread);
                    DebugActiveProcessStop(mem->m_dwProcessId);
                    goto debug;
                }

                if (dbgEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT && dbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP)
                {
                    if (dbgEvent.u.Exception.ExceptionRecord.ExceptionAddress == (LPVOID)filesystem_asm_utf16_addr)
                    {
                        CONTEXT ctx1;
                        ctx1.ContextFlags = CONTEXT_FULL;
                        GetThreadContext(hThread, &ctx1);
                        ctx1.EFlags |= 0x10000;
                        SetThreadContext(hThread, &ctx1);

                        if (ctx1.Eax < 0x100)
                        {
                            ContinueDebugEvent(dbgEvent.dwProcessId, dbgEvent.dwThreadId, dbgFlag);
                            continue;
                        }

                        wstring tempws(mem->Read<bigstr>(ctx1.Eax).text);
                        // not safe, will lost data if any word is a non-ascii character
                        // find a better way to do this!
                        string filename(tempws.begin(), tempws.end());
                        if (filename.find("trike/") == -1)
                            continue;

                        if (filename == "lstrike/locale_chn/resource/item.csv" ||
                            filename == "lstrike/locale_chn/resource/bad_words.csv" ||
                            filename == "lstrike/locale_chn/resource/relation_product_ver2.csv" ||
                            filename == "lstrike/locale_chn/resource/res/popup_login.res" ||
                            false /* change the list here! */)
                        {
                            muted++;
                            mem->Write(ctx1.Eax, L"null");
                        }

                        if (filename == "lstrike/common/resource/quest/medalclosed_l.tga")
                        {
                            // restore the original function
                            GetThreadContext(hThread, &ctx_utf16);
                            ctx_utf16.Dr0 = 0;
                            ctx_utf16.Dr7 = 0x400;
                            SetThreadContext(hThread, &ctx_utf16);
                            loop = false;
                            cout << "游戏已加载完毕, 终止监控...\n";
                        }
                        
                        //Misc->SetConsoleSize(900, 380);
                        //cout << filename << "   \t\r" << flush;
                    }
                    else
                    {
                        dbgFlag = DBG_EXCEPTION_NOT_HANDLED;
                    }
                }
                else
                {
                    dbgFlag = DBG_EXCEPTION_NOT_HANDLED;
                }

                if (GetAsyncKeyState(VK_DELETE) & 0x8000)
                {
                    // restore the original function
                    GetThreadContext(hThread, &ctx_utf16);
                    ctx_utf16.Dr0 = 0;
                    ctx_utf16.Dr7 = 0x400;
                    SetThreadContext(hThread, &ctx_utf16);
                    loop = false;
                    cout << "已手动按住Del终止监控...\n";
                }

                ContinueDebugEvent(dbgEvent.dwProcessId, dbgEvent.dwThreadId, dbgFlag);
            }
            SetPriorityClass(mem->m_hProcess, gamepri);
            DebugActiveProcessStop(mem->m_dwProcessId);
            CloseHandle(hThread);
        }
        else
        {
            DWORD addresses[CHAR_MAX];
            for (int i = 0; i < CHAR_MAX; i++)
                addresses[i] = NULL;
            RunMemScanAndGetAllAddress(mem->m_dwProcessId, "s", "lstrike/locale_chn/resource/item.csv", addresses, "utf-16");
            for (int i = 0; i < CHAR_MAX; i++)
            {
                if (addresses[i] == NULL)
                    break;
                PackerMuteMultiFile(addresses[i], "lstrike/locale_chn/resource/item.csv", 0x58, true);
                if (i == 0)
                    muted++;
            }
            for (int i = 0; i < CHAR_MAX; i++)
                addresses[i] = NULL;
            RunMemScanAndGetAllAddress(mem->m_dwProcessId, "s", "lstrike/locale_chn/resource/bad_words.csv", addresses, "utf-16");
            for (int i = 0; i < CHAR_MAX; i++)
            {
                if (addresses[i] == NULL)
                    break;
                PackerMuteMultiFile(addresses[i], "lstrike/locale_chn/resource/bad_words.csv", 0x68, true);
                if (i == 0)
                    muted++;
            }
            for (int i = 0; i < CHAR_MAX; i++)
                addresses[i] = NULL;
            RunMemScanAndGetAllAddress(mem->m_dwProcessId, "s", "lstrike/locale_chn/resource/res/popup_login.res", addresses, "utf-16");
            for (int i = 0; i < CHAR_MAX; i++)
            {
                if (addresses[i] == NULL)
                    break;
                PackerMuteMultiFile(addresses[i], "lstrike/locale_chn/resource/res/popup_login.res", 0x68, true);
                if (i == 0)
                    muted++;
            }
            for (int i = 0; i < CHAR_MAX; i++)
                addresses[i] = NULL;
            RunMemScanAndGetAllAddress(mem->m_dwProcessId, "s", "lstrike/locale_chn/resource/relation_product_ver2.csv", addresses, "utf-16");
            for (int i = 0; i < CHAR_MAX; i++)
            {
                if (addresses[i] == NULL)
                    break;
                PackerMuteMultiFile(addresses[i], "lstrike/locale_chn/resource/relation_product_ver2.csv", 0x78, true);
                if (i == 0)
                    muted++;
            }

            // resume the main thread because everything is done
            ResumeThread(handle_t);
            CloseHandle(handle_t);
        }

        cout << "已阻止 " << muted << " 个不应该被加载的文件.\n\n";
    }

    cout << "操作执行完毕,已加载汉化! :)\nMade by dounai2333(QQ1328600509)\n\n";
    Exit(0);
}

void PackerMuteMultiFile(DWORD address, string file, DWORD index, bool safeblock)
{
    bool needbreak = false;
    DWORD startaddr = address - (index * mem->Read<byte>(address - 2) * (0xFF + 1) + index * mem->Read<byte>(address - 3));

    for (int i = 0; i < 0xFF; i++)
    {
        if (needbreak)
            break;
        for (int j = 0; j < 0xFF; j++)
        {
            DWORD addr = startaddr + (index * i * (0xFF + 1) + index * j);
            if (mem->Read<byte>(addr - 2) != i || mem->Read<byte>(addr - 3) != j)
            {
                needbreak = true;
                break;
            }
            else
            {
                wstring tempws(mem->Read<bigstr>(addr).text);
                // not safe, will lost data if any word is a non-ascii character
                // find a better way to do this!
                string filename(tempws.begin(), tempws.end());
                if (filename == file)
                {
                    if (!safeblock)
                    {
                        mem->Write(addr, L"null");
                    }
                    else
                    {
                        // safe block: upper the first character, simple and working
                        string temp;
                        temp += filename[0];
                        mem->Write<byte>(addr, Misc->ToUpper(temp)[0]);
                    }
                }
            }
        }
    }
}

string CheckMemFile()
{
    string filelist[5] = { "clrcompression.dll", "clrjit.dll", "coreclr.dll", "MemScan.exe", "mscordaccore.dll" };
    string mem_file_missing_list = "";
    for (int i = 0; i < 5; i++)
    {
        if (!Misc->FileExist(filelist[i]))
        {
            if (mem_file_missing_list.length() == 0)
                mem_file_missing_list += filelist[i];
            else
                mem_file_missing_list += ", " + filelist[i];
        }
    }
    return mem_file_missing_list;
}

DWORD RunMemScanAndGetExitCode(DWORD ProcessID, string Type, string Value, string Encoding)
{
    string strtemp = "MemScan.exe -pid=" + to_string(ProcessID) + " -t=" + Type + " -value=\"" + Value + "\" " + ((Type == "s") ? ("-encoding=" + Encoding) : "");
    LPSTR start_str = strdup(strtemp.c_str());
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD exit_code;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcess(NULL, start_str, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return exit_code;
    }

    return NULL;
}

void RunMemScanAndGetAllAddress(DWORD ProcessID, string Type, string Value, DWORD(&output)[CHAR_MAX], string Encoding)
{
    string strtemp = "MemScan.exe -pid=" + to_string(ProcessID) + " -t=" + Type + " -value=\"" + Value + "\" " + ((Type == "s") ? ("-encoding=" + Encoding) : "") + " >%temp%\\address.log";
    system(strtemp.c_str());

    ifstream file(string(getenv("TEMP")) + "\\address.log");
    if (file.is_open())
    {
        int index = 0;
        for (std::string line; getline(file, line); )
        {
            output[index] = Misc->HexToDecimal(line);
            index++;
        }
        file.close();
        
        system("del /q %temp%\\address.log");
    }
}

void Exit(int exitcode)
{
    mem->Detach();

    if (Arg->Exist("-autoexit"))
    {
        for (int i = 5; i >= 1; i--)
        {
            cout << "程序将在 " << i << " 秒后退出." << "\t\r" << flush;
            Sleep(1000);
        }
    }
    else
    {
        cout << "按下任意键关闭汉化程序...\n";
        Misc->ReadKey();
    }
    exit(exitcode);
}
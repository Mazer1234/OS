#include <iostream>
#include <Windows.h>
#include <string>
using namespace std;


void get_sys_info(){
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    cout << "Number of processors: " << si.dwNumberOfProcessors << endl;
    cout << "Type of processor: " << si.dwProcessorType << endl;
    cout << "Memory page size: " << si.dwPageSize << endl;
    system("pause");
}

void menu(){
    cout << "---MENU---" << endl;
    cout << "1 - Get information about calculate system" << endl;
    cout << "2 - Get status of virtual memory" << endl;
    cout << "3 - Get status of certain area of memory" << endl;
    cout << "4 - Separate reservations" << endl;
    cout << "5 - Simultaneous reservations" << endl;
    cout << "0 - Exit" << endl;
}

void gl_mem_satus(){
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatusEx(&ms);

    cout << "Procent of using memory: " << ms.dwMemoryLoad << "%" << endl;
    cout << "Total memory size: " << ms.ullTotalPhys / (1024 * 1024) << endl;
    cout << "Can use memory size: " << ms.ullAvailPhys / (1024 * 1024) << endl;
    cout << "Total virtual memory size: " << ms.ullTotalVirtual / (1024 * 1024) << endl;
    cout << "Can use virtual memory size: " << ms.ullAvailVirtual / (1024 * 1024) << endl;
}

void virt_quary(){
    MEMORY_BASIC_INFORMATION mbi;
    size_t size = VirtualQuery((LPVOID)0x10000000, &mbi, sizeof(mbi));

    if (size == 0){
        cerr << "Error: " << GetLastError() << endl;
        exit(EXIT_FAILURE);
    }else{
        cout << "Basic adress: " << mbi.BaseAddress << endl;
        cout << "Basic adress of block of the memory: " << mbi.AllocationBase << endl;
        cout << "Protect of the block of memory: " << mbi.AllocationProtect << endl;
        cout << "Size of virtual memory: " << mbi.RegionSize << endl;
        cout << "Status of the virtual memory: " << mbi.State << endl;
        cout << "Protect of the virtual memory: " << mbi.Protect << endl;
        cout << "Type of the virtual memory: " << mbi.Type << endl;
    }
    system("pause");
}


void record_data(LPVOID addr, size_t region_size){
    int flag;
    cout << "Memory adress is: " << addr << endl;
    cout << "Size of commit region: " << region_size << endl;

    while (true){
        size_t offset;
        int value;

        cout << "\nEnter the offset (0 to " << (region_size - 1) << ") or -1 for exit : ";
        cin >> offset;

        if (offset == (size_t)-1){
            break;
        }

        if (offset >= region_size){
            cerr << "Error: too big offset" << endl;
            continue;
        }

        cout << "Enter the digit (0 to 255): ";
        cin >> value;
        if (value < 0 || value > 255){
            cerr << "Incorrect value, try again" << endl;
            continue;
        }

        BYTE* ptr = reinterpret_cast<BYTE*>(addr) + offset;
        *ptr = static_cast<BYTE>(value);

        cout << "Value " << value << " success record to adress " << addr << endl;

        cout << "Check the value: " << static_cast<int>(*ptr) << endl;
    }

    cout << "Do you want to change protect of the memory to read only?(1/0): ";
    cin >> flag;

    if (flag == 1){
        DWORD oldProtect;
        if(!VirtualProtect(addr, region_size, PAGE_READONLY, &oldProtect)){
            cerr << "Error Virtual Protect: " << GetLastError() << endl;
            return;
        }

        cout << "Memory protecting changed to read only" << endl;
    }

    cout << "Do you want to change protect of the memory to read and write?(1/0): ";
    cin >> flag;

    if (flag == 1){
        DWORD oldProtect;
        if(!VirtualProtect(addr, region_size, PAGE_READWRITE, &oldProtect)){
            cerr << "Error Virtual Protect: " << GetLastError() << endl;
            return;
        }

        cout << "Memory protecting changed to read and write" << endl;
    }
}

void sep_automatic_alloc(){
    size_t size = 4096;
    DWORD flProtect = PAGE_READWRITE;
    DWORD fdwAlloc = MEM_RESERVE;

    LPVOID lpAdress = VirtualAlloc(nullptr, size, fdwAlloc, flProtect);

    cout << "Memory is reserved" << endl;

    lpAdress = VirtualAlloc(lpAdress, size, MEM_COMMIT, flProtect);
    cout << "Memory is commit" << endl;

    if (lpAdress == nullptr){
        cerr << "Virtual alloc failed: " << GetLastError() << endl;
        return;
    }

    cout << "Memory allocated at adress: " << lpAdress << endl;

    int flag;
    cout << "Do you want to record the data?(1/0): ";
    cin >> flag;

    if (flag == 1){
        record_data(lpAdress, size);
    }

    if (!VirtualFree(lpAdress, 0, MEM_RELEASE)){
        cerr << "VirtualFree error: " << GetLastError() << endl;
        return;
    }

    cout << "Memory is free" << endl;
    system("pause");
}

void sep_handle_alloc(){
    LPVOID prefferedAddress;
    cout << "Enter the prefferred starting adress (in hexadecimal 0x10000 for example): ";
    string input;
    cin >> hex >> input;
    prefferedAddress = (LPVOID)stoull(input, nullptr, 16);

    size_t size = 4096;
    DWORD flProtect = PAGE_READWRITE;
    DWORD fdwAlloc = MEM_RESERVE;

    LPVOID lpAdress = VirtualAlloc(prefferedAddress, size, fdwAlloc, flProtect);

    cout << "Memory is reserved" << endl;

    lpAdress = VirtualAlloc(lpAdress, size, MEM_COMMIT, flProtect);
    cout << "Memory is commit" << endl;

    if (lpAdress == nullptr){
        cerr << "Error Virtual alloc: " << GetLastError() << endl;
        return;
    }

    cout << "Memory allocated at adress: " << lpAdress << endl;

    int flag;
    cout << "Do you want to record the data?(1/0): ";
    cin >> flag;

    if (flag == 1){
        record_data(lpAdress, size);
    }

    if (!VirtualFree(lpAdress, 0, MEM_RELEASE)){
        cerr << "Error VirtualFree: " << GetLastError() << endl;
        return;
    }

    cout << "Memory is free" << endl;
    system("pause");
}

void sep_semult_auto(){
    size_t size = 4096;

    LPVOID addr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (addr == nullptr){
        cerr << "Error of VirtualAlloc: " << GetLastError() << endl;
        return;
    }

    cout << "Memory is reserved and commit at adress: " << addr << endl;

    int flag;
    cout << "Do you want to record the data?(1/0): ";
    cin >> flag;

    if (flag == 1){
        record_data(addr, size);
    }

    if (!VirtualFree(addr, 0, MEM_RELEASE)){
        cerr << "Error of VirtualFree: " << GetLastError() << endl;
        return;
    }

    cout << "Memory is free" << endl;
    system("pause");
}

void sep_semult_handle(){
    LPVOID prefferedAdress;
    size_t size = 4096;
    cout << "Enter the prefferred starting adress (in hexadecimal 0x10000 for example): ";
    string input;
    cin >> hex >> input;

    LPVOID addr = VirtualAlloc(prefferedAdress, size, MEM_RELEASE | MEM_COMMIT, PAGE_READWRITE);

    if (addr == nullptr){
        cerr << "Error VirtualAlloc: " << GetLastError() << endl;
        return;
    }

    cout << "Memory release and commit at adress: " << addr << endl;
    
    int flag;
    cout << "Do you want to record the data?(1/0): ";
    cin >> flag;

    if (flag == 1){
        record_data(addr, size);
    }

    if (!VirtualFree(addr, 0, MEM_RELEASE)){
        cerr << "Error VirtualFree : " << GetLastError() << endl;
        return;
    }

    cout << "Memory is free" << endl;
    system("pause");
}

int main(){
    int choice = -1;
    int flag;

    do{
        menu();
        cout << "Enter the option: ";
        cin >> choice;
        switch (choice){
            case 1:
                get_sys_info();
                break;
            case 2:
                gl_mem_satus();
                break;
            case 3:
                virt_quary();
                break;
            case 4:
                cout << "Do you want to automatic adress or handle?(1/0) : ";
                cin >> flag;
                if (flag == 1){
                    sep_automatic_alloc();
                    break;
                }else if (flag == 0){
                    sep_handle_alloc();
                    break;
                }else{
                    cout << "Incorrect answer, try again";
                    break;
                }
            case 5:
                cout << "Do you want to automatic adress or handle?(1/0) : ";
                cin >> flag;
                if (flag == 1){
                    sep_semult_auto();
                    break;
                }else if (flag == 0){
                    sep_semult_handle();
                    break;
                }else{
                    cout << "Incorrect answer, try again" << endl;
                    break;
                }
            case 0:
                cout << "You choose exit" << endl;
                break;
            default:
                cout << "Please try again" << endl;
                break;
        }
        
    }while (choice != 0);
}
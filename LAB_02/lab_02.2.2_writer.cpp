#include<Windows.h>
#include<iostream>
#include<string>

#define FILE_SIZE 1024

using namespace std;

HANDLE hFile = INVALID_HANDLE_VALUE;
HANDLE hMapping = nullptr;
LPVOID pView = nullptr;
void menu(){
    wcout << L"----Menu Writter----" << endl;
    wcout << L"1 - Create file" << endl;
    wcout << L"2 - Create mapping object" << endl;
    wcout << L"3 - Mapping file to memory" << endl;
    wcout << L"4 - Record data to file" << endl;
    wcout << L"5 - Delete map of file" << endl;
    wcout << L"0 - Exit" << endl;
}

void create_file(){
    wstring path;
    wcout << "Enter the path to file(C:/Users/..): ";
    wcin.ignore();
    getline(wcin, path);

    hFile = CreateFileW(path.c_str(), 
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (hFile == INVALID_HANDLE_VALUE){
        wcerr << L"Error openning file: " << GetLastError() << endl;
        CloseHandle(hFile);
        exit(EXIT_FAILURE);
    }

    cout << "SUCCESSFULL create file" << endl;
}

void create_mapping_file(){
    hMapping = CreateFileMappingW(hFile, NULL, PAGE_READWRITE, 0, FILE_SIZE, L"MyMappingObj");

    if (hMapping == NULL){
        cerr << "File mapping error: " << GetLastError() << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Create mapping file successfull" << endl;
}

void MapViewOfFilefunc(){
    if (hMapping == nullptr){
        cerr << "ERROR of open hMapping file" << endl;
        exit(EXIT_FAILURE);
    }

    if (pView != nullptr){
        cerr << "This file already view" << endl;
        exit(EXIT_FAILURE);
    }

    pView = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, FILE_SIZE);

    if (pView == nullptr){
        cerr << "ERROR map view if file" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Successfull mapping file" << endl;
}

void writeData(){
    if (pView == nullptr){
        cerr << "File not view" << endl;
        return;
    }

    cout << "Enter the data for record: ";
    string input;
    cin.ignore();
    getline(cin, input);

    if (input.length() >= FILE_SIZE) {
        cerr << "Too big input data" << endl;
        return;
    }
    
    memcpy(pView, input.c_str(), input.length() + 1);
    cout << "Data record to view file" << endl;
}

void unmapView(){
    if (pView == nullptr){
        cerr << "File not view" << endl;
        return;
    }

    if (UnmapViewOfFile(pView) == 0){
        cerr << "Error of the unmap" << endl;
        return;
    }else{
        cout << "Successfull unmapping file" << endl;
        pView = nullptr;
    }
}

void closeHandle(){
    if (pView != nullptr){
        unmapView();
    }

    if (hMapping != nullptr){
        CloseHandle(hMapping);
        hMapping = nullptr;
    }

    if (hFile != nullptr){
        CloseHandle(hFile);
        hFile = nullptr;
    }
}
int main(){
    int choice;

    do{
        menu();
        cout << "Enter the option: ";
        cin >> choice;
        switch(choice){
            case 1:
                create_file();
                break;
            case 2:
                create_mapping_file();
                break;
            case 3:
                MapViewOfFilefunc();
                break;
            case 4:
                writeData();
                break;
            case 5:
                unmapView();
                break;
            case 0:
                cout << "You choose exit" << endl;
                break;
            default:
                cout << "Try again." << endl;
                break;
        }
    }while(choice != 0);

    closeHandle();
    return 0;
}
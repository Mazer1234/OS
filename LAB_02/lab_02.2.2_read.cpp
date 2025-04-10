#include <windows.h>
#include <iostream>
#include <string>
using namespace std;

#define FILE_SIZE 1024

HANDLE hFileMapping = nullptr;
LPVOID pView = NULL;

void menu(){
    cout << "---MENU reader---" << endl;
    cout << "1 - Open mapping object" << endl;
    cout << "2 - Mapping file to memory" << endl;
    cout << "3 - Read data from mapping file" << endl;
    cout << "4 - Unmapp file" << endl;
    cout << "0 - Exit" << endl;
}

void OpenFileMappingfunc(){
    if (hFileMapping != nullptr){
        cerr << "File already open" << endl;
        exit(EXIT_FAILURE);
    }
    
    hFileMapping = OpenFileMappingW(
        FILE_MAP_READ,
        FALSE,
        L"MyMappingObj"
    );

    if (hFileMapping == nullptr){
        cerr << "Error open file" << endl;
        exit(EXIT_FAILURE);
    }else{
        cout << " Open successfull" << endl;
    }
}

void mapView(){
    if (hFileMapping == nullptr){
        cerr << "File not mapping" << endl;
        exit(EXIT_FAILURE);
    }

    if (pView != nullptr){
        cerr << "File already view" << endl;
        exit(EXIT_FAILURE);
    }

    pView = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, FILE_SIZE);

    if (pView == nullptr){
        cerr << "Error of view map file" << endl;
        exit(EXIT_FAILURE);
    }else{
        cout << "Successfull mapping file" << endl;
    }
}

void readData(){
    if (pView == nullptr){
        cerr << "File not view" << endl;
        exit(EXIT_FAILURE);
    }

    char* data = static_cast<char*>(pView);
    cout << "---Data in the file----" << endl;
    cout << data << endl;
}

void unmapView() {
    if (pView == nullptr){
        cerr << "File not view" << endl;
        exit(EXIT_FAILURE);
    }

    if (UnmapViewOfFile(pView) == 0){
        cerr << "Error of file unmapping" << endl;
        exit(EXIT_FAILURE);
    }else{
        cout << "Successfull unmap file" << endl;
        pView = nullptr;
    }
}

void closeHandle(){
    if (pView != nullptr){
        unmapView();
    }
    if (hFileMapping != nullptr){
        CloseHandle(hFileMapping);
        hFileMapping = nullptr;
    }
}

int main(){
    int option;

    do{
        menu();
        cin >> option;
        switch(option){
            case 1:
                OpenFileMappingfunc();
                break;
            case 2:
                mapView();
                break;
            case 3:
                readData();
                break;
            case 4:
                unmapView();
            case 0:
                cout << "You choose exit" << endl;
                break;
            default:
                cout << "Try again" << endl;
        }
    }while (option != 0);

    closeHandle();
    return 0;
}
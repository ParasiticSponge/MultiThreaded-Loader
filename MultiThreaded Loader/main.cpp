#include <Windows.h>
#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include "resource.h"
#include "ThreadPool.h"
#include <iostream>

using std::cout;
using std::thread;   using std::vector;
using std::wstring;  using std::mutex;

#define WINDOW_CLASS_NAME L"MultiThreaded Loader Tool"
const unsigned int _kuiWINDOWWIDTH = 1200;
const unsigned int _kuiWINDOWHEIGHT = 1200;
#define MAX_FILES_TO_OPEN 50
#define MAX_CHARACTERS_IN_FILENAME 25

//Global Variables
vector<wstring> g_vecImageFileNames;
vector<wstring> g_vecSoundFileNames;
vector<HWND> ImageWindows;

int threads;

HINSTANCE g_hInstance;
bool g_bIsFileLoaded = false;
LPCWSTR ImageLoadTime, SoundLoadTime;
std::wstring ImgCnt, SndCnt;
HBITMAP LoaderFile;
mutex g_Lock;


bool ChooseImageFilesToLoad(HWND _hwnd)
{
	OPENFILENAME ofn;
	SecureZeroMemory(&ofn, sizeof(OPENFILENAME)); // Better to use than ZeroMemory
	wchar_t wsFileNames[MAX_FILES_TO_OPEN * MAX_CHARACTERS_IN_FILENAME + MAX_PATH]; //The string to store all the filenames selected in one buffer togther with the complete path name.
	wchar_t _wsPathName[MAX_PATH + 1];
	wchar_t _wstempFile[MAX_PATH + MAX_CHARACTERS_IN_FILENAME]; //Assuming that the filename is not more than 20 characters
	wchar_t _wsFileToOpen[MAX_PATH + MAX_CHARACTERS_IN_FILENAME];
	ZeroMemory(wsFileNames, sizeof(wsFileNames));
	ZeroMemory(_wsPathName, sizeof(_wsPathName));
	ZeroMemory(_wstempFile, sizeof(_wstempFile));

	//Fill out the fields of the structure
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = _hwnd;
	ofn.lpstrFile = wsFileNames;
	ofn.nMaxFile = MAX_FILES_TO_OPEN * 20 + MAX_PATH;  //The size, in charactesr of the buffer pointed to by lpstrFile. The buffer must be atleast 256(MAX_PATH) characters long; otherwise GetOpenFileName and 
													   //GetSaveFileName functions return False
													   // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
													   // use the contents of wsFileNames to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = L"Bitmap Images(.bmp)\0*.bmp\0"; //Filter for bitmap images
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

	//If the user makes a selection from the  open dialog box, the API call returns a non-zero value
	if (GetOpenFileName(&ofn) != 0) //user made a selection and pressed the OK button
	{
		//Extract the path name from the wide string -  two ways of doing it
		//First way: just work with wide char arrays
		wcsncpy_s(_wsPathName, wsFileNames, ofn.nFileOffset);
		int i = ofn.nFileOffset;
		int j = 0;

		while (true)
		{
			if (*(wsFileNames + i) == '\0')
			{
				_wstempFile[j] = *(wsFileNames + i);
				wcscpy_s(_wsFileToOpen, _wsPathName);
				wcscat_s(_wsFileToOpen, L"\\");
				wcscat_s(_wsFileToOpen, _wstempFile);
				g_vecImageFileNames.push_back(_wsFileToOpen);
				j = 0;
			}
			else
			{
				_wstempFile[j] = *(wsFileNames + i);
				j++;
			}
			if (*(wsFileNames + i) == '\0' && *(wsFileNames + i + 1) == '\0')
			{
				break;
			}
			else
			{
				i++;
			}

		}

		g_bIsFileLoaded = true;
		return true;
	}
	else // user pressed the cancel button or closed the dialog box or an error occured
	{
		return false;
	}

}

bool ChooseSoundFilesToLoad(HWND _hwnd)
{
	OPENFILENAME ofn;
	SecureZeroMemory(&ofn, sizeof(OPENFILENAME)); // Better to use than ZeroMemory
	wchar_t wsFileNames[MAX_FILES_TO_OPEN * MAX_CHARACTERS_IN_FILENAME + MAX_PATH]; //The string to store all the filenames selected in one buffer togther with the complete path name.
	wchar_t _wsPathName[MAX_PATH + 1];
	wchar_t _wstempFile[MAX_PATH + MAX_CHARACTERS_IN_FILENAME]; //Assuming that the filename is not more than 20 characters
	wchar_t _wsFileToOpen[MAX_PATH + MAX_CHARACTERS_IN_FILENAME];
	ZeroMemory(wsFileNames, sizeof(wsFileNames));
	ZeroMemory(_wsPathName, sizeof(_wsPathName));
	ZeroMemory(_wstempFile, sizeof(_wstempFile));

	//Fill out the fields of the structure
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = _hwnd;
	ofn.lpstrFile = wsFileNames;
	ofn.nMaxFile = MAX_FILES_TO_OPEN * 20 + MAX_PATH;  //The size, in charactesr of the buffer pointed to by lpstrFile. The buffer must be atleast 256(MAX_PATH) characters long; otherwise GetOpenFileName and 
													   //GetSaveFileName functions return False
													   // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
													   // use the contents of wsFileNames to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.lpstrFilter = L"Wave Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0"; //Filter for wav files
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

	//If the user makes a selection from the  open dialog box, the API call returns a non-zero value
	if (GetOpenFileName(&ofn) != 0) //user made a selection and pressed the OK button
	{
		//Extract the path name from the wide string -  two ways of doing it
		//Second way: work with wide strings and a char pointer 

		wstring _wstrPathName = ofn.lpstrFile;

		_wstrPathName.resize(ofn.nFileOffset, '\\');

		wchar_t* _pwcharNextFile = &ofn.lpstrFile[ofn.nFileOffset];

		while (*_pwcharNextFile)
		{
			wstring _wstrFileName = _wstrPathName + _pwcharNextFile;

			g_vecSoundFileNames.push_back(_wstrFileName);

			_pwcharNextFile += lstrlenW(_pwcharNextFile) + 1;
		}

		g_bIsFileLoaded = true;
		return true;
	}
	else // user pressed the cancel button or closed the dialog box or an error occured
	{
		return false;
	}

}

std::mutex dataLock;
void count(const int pLowerLimit, const int pUpperLimit, vector<wstring> g_FileNames, HWND _hwnd)
{
	if (g_FileNames == g_vecImageFileNames) //check if images have been loaded
	{
		g_Lock.lock();
		for (int i = pLowerLimit; i < pUpperLimit; i++)
		{
			//output the contents of the vector to the HWND handler
			ImgCnt = g_FileNames[i] + L"\n";
		}
		g_Lock.unlock();
	}
	if (g_FileNames == g_vecSoundFileNames) //check if sounds have been loaded
	{
		g_Lock.lock();
		for (int i = pLowerLimit; i < pUpperLimit; i++)
		{
			//output the contents of the vector to the HWND handler
			SndCnt = g_FileNames[i] + L"\n";
		}
		g_Lock.unlock();
	}
}

void verifyThreads(vector<wstring> g_FileNames)
{
	if (g_FileNames.size() == 1 || g_FileNames.size() == 2) //the number is even
	{
		threads = 1;
	}
	else if (g_FileNames.size() % 2 == 0) //the number is even
		for (int i = 2; i < MAX_FILES_TO_OPEN; i++) //avoid dividing the number by 1, continue up from 2
		{
			//if the remainder of a number divided by the thread size is 0
			//also making sure it doesn't divide by itself, or by 2
			if (g_FileNames.size() % i == 0 && i != g_FileNames.size() && i != g_FileNames.size() / 2)
			{
				threads = i;
			}
		}
	else if (g_FileNames.size() % 2 > 0) //if the number is odd
		for (int i = 1; i < MAX_FILES_TO_OPEN; i++) //start from 1
		{
			//if the remainder of a number divided by the thread size is 0
			//also making sure it doesn't divide by itself, or by 2
			if (g_FileNames.size() % i == 0 && i != g_FileNames.size() && i != g_FileNames.size() / 2)
			{
				threads = i;
			}
		}
}

//takes in imageFileNames or soundFileNames and processes into threads
void countThr(vector<wstring> g_FileNames, HWND _hwnd)
{
	verifyThreads(g_FileNames);
	int THREAD_COUNT = threads; //find the best number divided by the vector size
	//cout << "There are " << THREAD_COUNT << " files per thread count\n";

	//thread myThreads[5]; //make 5 threads
	thread* myThreads = new thread[THREAD_COUNT];

	int lowLimit = 0;

	//upperLimit = 2
	//upper limit is how many values should be stored in each thread, evenly distributed between threads.
	int upperLimit = g_FileNames.size() / threads;

	//keep a variable that stores the count of each thread to increase by after each iteration of the loop
	const int increasingLimit = upperLimit;

	for (int i = 0; i < THREAD_COUNT; i++) //from 0 to 5
	{
		*myThreads = thread(count, lowLimit, upperLimit, g_FileNames, _hwnd); //from [0] to [1]
		myThreads->join();

		//move onto the next thread
		myThreads++;

		lowLimit += increasingLimit; //add to lowerLimit
		upperLimit += increasingLimit; //add to upperLimit
	}
}

LRESULT CALLBACK WindowProc(HWND _hwnd, UINT _uiMsg, WPARAM _wparam, LPARAM _lparam)
{
	PAINTSTRUCT ps;
	HDC _hWindowDC;
	//RECT rect;
	switch (_uiMsg)
	{
	case WM_KEYDOWN:
	{
		switch (_wparam)
		{
		case VK_ESCAPE: //get the esc button to exit the program is pressed
		{
			SendMessage(_hwnd, WM_CLOSE, 0, 0);
			return(0);
		}
		break;
		default:
			break;
		}
	}
	break;
	case WM_PAINT:
	{

		_hWindowDC = BeginPaint(_hwnd, &ps);
		//Do all our painting here

		EndPaint(_hwnd, &ps);
		return (0);
	}
	break;
	case WM_COMMAND: //detect if an action occurs
	{
		switch (LOWORD(_wparam))
		{


		case ID_FILE_LOADIMAGE:
		{
			if (ChooseImageFilesToLoad(_hwnd))
			{
				//if 1 image is selected
				if (g_vecImageFileNames.size() == 1)
				{
					MessageBox(_hwnd, L"Image Selected", L"Message", MB_OK);
				}
				//if multiple are selected
				if (g_vecImageFileNames.size() > 1)
				{
					MessageBox(_hwnd, L" Multiple Images Selected", L"Notification", MB_DEFBUTTON4);
				}
				if (g_vecImageFileNames.size() > MAX_FILES_TO_OPEN)
				{
					g_vecImageFileNames.resize(MAX_FILES_TO_OPEN);
				}

				auto ImageLoadStart = std::chrono::high_resolution_clock::now();
				//start threads based on number of files selected
				countThr(g_vecImageFileNames, _hwnd);
				auto ImageLoadStop = std::chrono::high_resolution_clock::now();
				auto ImageLoadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(ImageLoadStop - ImageLoadStart); //find difference

				//add the output time below the image display
				std::wstring OutTime = L"\n";
				OutTime += std::to_wstring(ImageLoadDuration.count());
				OutTime += L" ms to load images";
				ImgCnt += OutTime;
				ImageLoadTime = ImgCnt.c_str();

				//																			posX, posY, width, height
				_hwnd = CreateWindow(L"STATIC", ImageLoadTime, WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 20, 600, 250, _hwnd, NULL, NULL, NULL);

			}
			else
			{
				MessageBox(_hwnd, L"No Image File selected", L"Error Message", MB_ICONWARNING);
			}

			return (0);
		}
		break;
		case ID_FILE_LOADSOUND:
		{
			if (ChooseSoundFilesToLoad(_hwnd))
			{
				//if 1 image is selected
				if (g_vecSoundFileNames.size() == 1)
				{
					MessageBox(_hwnd, L"Image Selected", L"Message", MB_OK);
				}
				//if multiple are selected
				if (g_vecSoundFileNames.size() > 1)
				{
					MessageBox(_hwnd, L" Multiple Images Selected", L"Notification", MB_DEFBUTTON4);
				}
				if (g_vecSoundFileNames.size() > MAX_FILES_TO_OPEN)
				{
					g_vecSoundFileNames.resize(MAX_FILES_TO_OPEN);
				}

				auto SoundLoadStart = std::chrono::high_resolution_clock::now();
				//start threads based on number of files selected
				countThr(g_vecSoundFileNames, _hwnd);
				auto SoundLoadStop = std::chrono::high_resolution_clock::now();
				auto SoundLoadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(SoundLoadStop - SoundLoadStart); //find difference

				//add the output time below the sound file display
				std::wstring OutTime = L"\n";
				OutTime += std::to_wstring(SoundLoadDuration.count());
				OutTime += L" ms to load sounds";
				SndCnt += OutTime;
				SoundLoadTime = ImgCnt.c_str();

				//																			posX, posY, width, height
				_hwnd = CreateWindow(L"STATIC", SoundLoadTime, WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 20, 600, 250, _hwnd, NULL, NULL, NULL);
			}
			else
			{
				MessageBox(_hwnd, L"No Sound File selected", L"Error Message", MB_ICONWARNING);
			}
			return (0);
		}
		break;
		case ID_EXIT:
		{
			SendMessage(_hwnd, WM_CLOSE, 0, 0);
			return (0);
		}
		break;
		default:
			break;
		}
	}
	break;
	case WM_CLOSE:
	{
		PostQuitMessage(0);
	}
	break;
	default:
		break;
	}
	return (DefWindowProc(_hwnd, _uiMsg, _wparam, _lparam));
}

HWND CreateAndRegisterWindow(HINSTANCE _hInstance)
{
	WNDCLASSEX winclass; // This will hold the class we create.
	HWND hwnd;           // Generic window handle.

						 // First fill in the window class structure.
	winclass.cbSize = sizeof(WNDCLASSEX);
	winclass.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = WindowProc;
	winclass.cbClsExtra = 0;
	winclass.cbWndExtra = 0;
	winclass.hInstance = _hInstance;
	winclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground =
		static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	winclass.lpszMenuName = NULL;
	winclass.lpszClassName = WINDOW_CLASS_NAME;
	winclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// register the window class
	if (!RegisterClassEx(&winclass))
	{
		return (0);
	}

	HMENU _hMenu = LoadMenu(_hInstance, MAKEINTRESOURCE(IDR_MENU1));

	// create the window
	hwnd = CreateWindowEx(NULL, // Extended style.
		WINDOW_CLASS_NAME,      // Class.
		L"MultiThreaded Loader Tool",   // Title.
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		10, 10,                    // Initial x,y.
		_kuiWINDOWWIDTH, _kuiWINDOWHEIGHT,                // Initial width, height.
		NULL,                   // Handle to parent.
		_hMenu,                   // Handle to menu.
		_hInstance,             // Instance of this application.
		NULL);                  // Extra creation parameters.

	return hwnd;
}

int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE _hPrevInstance, LPSTR _lpCmdLine, int _nCmdShow)
{
	MSG msg;  //Generic Message

	HWND _hwnd = CreateAndRegisterWindow(_hInstance);

	if (!(_hwnd))
	{
		return (0);
	}


	// Enter main event loop
	while (true)
	{
		// Test if there is a message in queue, if so get it.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{

			// Test if this is a quit.
			if (msg.message == WM_QUIT)
			{
				break;
			}

			// Translate any accelerator keys.
			TranslateMessage(&msg);
			// Send the message to the window proc.
			DispatchMessage(&msg);
		}

	}

	// Return to Windows like this...
	return (static_cast<int>(msg.wParam));
}
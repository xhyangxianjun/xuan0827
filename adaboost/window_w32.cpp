
#include "stdafx.h"
#include "mmplab_highgui.h"

#if defined WIN32 || defined WIN64

#if _MSC_VER >= 1200
#pragma warning( disable: 4710 )
#endif

#include <commctrl.h>
#include <winuser.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

static const char* trackbar_text =
"                                                                                             ";

#if defined WIN64 || defined EM64T

#define icvGetWindowLongPtr GetWindowLongPtr
#define icvSetWindowLongPtr( hwnd, id, ptr ) SetWindowLongPtr( hwnd, id, (LONG_PTR)(ptr) )
#define icvGetClassLongPtr  GetClassLongPtr

#define CV_USERDATA GWLP_USERDATA
#define CV_WNDPROC GWLP_WNDPROC
#define CV_HCURSOR GCLP_HCURSOR
#define CV_HBRBACKGROUND GCLP_HBRBACKGROUND

#else

#define icvGetWindowLongPtr GetWindowLong
#define icvSetWindowLongPtr( hwnd, id, ptr ) SetWindowLong( hwnd, id, (size_t)ptr )
#define icvGetClassLongPtr GetClassLong

#define CV_USERDATA GWL_USERDATA
#define CV_WNDPROC GWL_WNDPROC
#define CV_HCURSOR GCL_HCURSOR
#define CV_HBRBACKGROUND GCL_HBRBACKGROUND

#endif

void  FillBitmapInfo( BITMAPINFO* bmi, int width, int height, int bpp, int origin )
{
    assert( bmi && width >= 0 && height >= 0 && (bpp == 8 || bpp == 24 || bpp == 32));

    BITMAPINFOHEADER* bmih = &(bmi->bmiHeader);

    memset( bmih, 0, sizeof(*bmih));
    bmih->biSize = sizeof(BITMAPINFOHEADER);
    bmih->biWidth = width;
    bmih->biHeight = origin ? abs(height) : -abs(height);
    bmih->biPlanes = 1;
    bmih->biBitCount = (unsigned short)bpp;
    bmih->biCompression = BI_RGB;

    if( bpp == 8 )
    {
        RGBQUAD* palette = bmi->bmiColors;
        int i;
        for( i = 0; i < 256; i++ )
        {
            palette[i].rgbBlue = palette[i].rgbGreen = palette[i].rgbRed = (BYTE)i;
            palette[i].rgbReserved = 0;
        }
    }
}

struct CvWindow;

typedef struct CvTrackbar
{
    int signature;
    HWND hwnd;
    char* name;
    CvTrackbar* next;
    CvWindow* parent;
    HWND buddy;
    int* data;
    int pos;
    int maxval;
    void (*notify)(int);
    int id;
}
CvTrackbar;


typedef struct CvWindow
{
    int signature;
    HWND hwnd;
    char* name;
    CvWindow* prev;
    CvWindow* next;
    HWND frame;

    HDC dc;
    HGDIOBJ image;
    int last_key;
    int flags;

    CvMouseCallback on_mouse;
    void* on_mouse_param;

    struct
    {
        HWND toolbar;
        int pos;
        int rows;
        WNDPROC toolBarProc;
        CvTrackbar* first;
    }
    toolbar;
}
CvWindow;


#define HG_BUDDY_WIDTH  130

#ifndef TBIF_SIZE
    #define TBIF_SIZE  0x40
#endif

#ifndef TB_SETBUTTONINFO
    #define TB_SETBUTTONINFO (WM_USER + 66)
#endif

#ifndef TBM_GETTOOLTIPS
    #define TBM_GETTOOLTIPS  (WM_USER + 30)
#endif

static LRESULT CALLBACK HighGUIProc(  HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WindowProc(  HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK MainWindowProc(  HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void icvUpdateWindowPos( CvWindow* window );

static CvWindow* hg_windows = 0;
static CvWin32WindowCallback hg_on_preprocess = 0, hg_on_postprocess = 0;
static HINSTANCE hg_hinstance = 0;

static void icvCleanupHighgui()
{
    cvDestroyAllWindows();
}

CV_IMPL int cvInitSystem( int, char** )
{
    static int wasInitialized = 0;

    // check initialization status
    if( !wasInitialized )
    {
        // Initialize the stogare
        hg_windows = 0;

        // Register the class
        WNDCLASS wndc;
        wndc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
        wndc.lpfnWndProc = WindowProc;
        wndc.cbClsExtra = 0;
        wndc.cbWndExtra = 0;
        wndc.hInstance = hg_hinstance;
        wndc.lpszClassName = "HighGUI class";
        wndc.lpszMenuName =  "HighGUI class";
        wndc.hIcon = LoadIcon(0, IDI_APPLICATION);
        wndc.hCursor = (HCURSOR)LoadCursor(0, (LPCSTR)(size_t)IDC_CROSS );
        wndc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);

        RegisterClass(&wndc);

        wndc.lpszClassName = "Main HighGUI class";
        wndc.lpszMenuName = "Main HighGUI class";
        wndc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
        wndc.lpfnWndProc = MainWindowProc;

        RegisterClass(&wndc);
        atexit( icvCleanupHighgui );

        wasInitialized = 1;
    }

    return 0;
}

CV_IMPL int cvStartWindowThread(){
    return 0;
}

static CvWindow* icvFindWindowByName( const char* name )
{
    CvWindow* window = hg_windows;

    for( ; window != 0 && strcmp( name, window->name) != 0; window = window->next )
        ;

    return window;
}


static CvWindow* icvWindowByHWND( HWND hwnd )
{
    CvWindow* window = (CvWindow*)icvGetWindowLongPtr( hwnd, CV_USERDATA );
    return window != 0 && hg_windows != 0 &&
           window->signature == CV_WINDOW_MAGIC_VAL ? window : 0;
}


static CvTrackbar* icvTrackbarByHWND( HWND hwnd )
{
    CvTrackbar* trackbar = (CvTrackbar*)icvGetWindowLongPtr( hwnd, CV_USERDATA );
    return trackbar != 0 && trackbar->signature == CV_TRACKBAR_MAGIC_VAL &&
           trackbar->hwnd == hwnd ? trackbar : 0;
}


static const char* icvWindowPosRootKey = "Software\\OpenCV\\HighGUI\\Windows\\";

// Window positions saving/loading added by Philip Gruebele.
//<a href="mailto:pgruebele@cox.net">pgruebele@cox.net</a>
// Restores the window position from the registry saved position.
static void
icvLoadWindowPos( const char* name, CvRect& rect )
{
    HKEY hkey;
    char szKey[1024];
    strcpy( szKey, icvWindowPosRootKey );
    strcat( szKey, name );

    rect.x = rect.y = CW_USEDEFAULT;
    rect.width = rect.height = 320;

//++++++++++++++  add by jacky ++++++++++++++++++++++++++++++//
	int nSize1 = MultiByteToWideChar(CP_ACP, 0, szKey, -1, NULL, 0);
	LPWSTR wszKey = new WCHAR[nSize1];
	MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszKey, nSize1);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

    if( RegOpenKeyEx(HKEY_CURRENT_USER,(LPSTR)wszKey,0,KEY_QUERY_VALUE,&hkey) == ERROR_SUCCESS )
    {
        // Yes we are installed.
        DWORD dwType = 0;
        DWORD dwSize = sizeof(int);

        RegQueryValueEx(hkey, "Left", NULL, &dwType, (BYTE*)&rect.x, &dwSize);
        RegQueryValueEx(hkey, "Top", NULL, &dwType, (BYTE*)&rect.y, &dwSize);
        RegQueryValueEx(hkey, "Width", NULL, &dwType, (BYTE*)&rect.width, &dwSize);
        RegQueryValueEx(hkey, "Height", NULL, &dwType, (BYTE*)&rect.height, &dwSize);

        if( rect.x != (int)CW_USEDEFAULT && (rect.x < -200 || rect.x > 3000) )
            rect.x = 100;
        if( rect.y != (int)CW_USEDEFAULT && (rect.y < -200 || rect.y > 3000) )
            rect.y = 100;

        if( rect.width != (int)CW_USEDEFAULT && (rect.width < 0 || rect.width > 3000) )
            rect.width = 100;
        if( rect.height != (int)CW_USEDEFAULT && (rect.height < 0 || rect.height > 3000) )
            rect.height = 100;

        RegCloseKey(hkey);
    }

	delete [] wszKey;
}


// Window positions saving/loading added by Philip Gruebele.
//<a href="mailto:pgruebele@cox.net">pgruebele@cox.net</a>
// philipg.  Saves the window position in the registry
static void
icvSaveWindowPos( const char* name, CvRect rect )
{
    static const DWORD MAX_RECORD_COUNT = 100;
    HKEY hkey;
    char szKey[1024];
    char rootKey[1024];
    strcpy( szKey, icvWindowPosRootKey );
    strcat( szKey, name );
   
	//++++++++++++++  add by jacky ++++++++++++++++++++++++++++++//
	int nSize1 = MultiByteToWideChar(CP_ACP, 0, szKey, -1, NULL, 0);
	LPWSTR wszKey = new WCHAR[nSize1];
	MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszKey, nSize1);
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

    if( RegOpenKeyEx( HKEY_CURRENT_USER,(LPCSTR)wszKey,0,KEY_READ,&hkey) != ERROR_SUCCESS )
    {
        HKEY hroot;
        DWORD count = 0;
        FILETIME oldestTime = { UINT_MAX, UINT_MAX };
        char oldestKey[1024];
        char currentKey[1024];

        strcpy( rootKey, icvWindowPosRootKey );
        rootKey[strlen(rootKey)-1] = '\0';

        int nSize1 = MultiByteToWideChar(CP_ACP, 0, rootKey, -1, NULL, 0);
		LPWSTR lprootKey = new WCHAR[nSize1];
        MultiByteToWideChar(CP_ACP, 0, rootKey, -1, lprootKey, nSize1);


        if( RegCreateKeyEx(HKEY_CURRENT_USER, (LPCSTR)lprootKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ+KEY_WRITE, 0, &hroot, NULL) != ERROR_SUCCESS )
		{
            //RegOpenKeyEx( HKEY_CURRENT_USER,rootKey,0,KEY_READ,&hroot) != ERROR_SUCCESS )
			delete [] lprootKey;
			lprootKey = NULL;
            return;
		}

        for(;;)
        {
            DWORD csize = sizeof(currentKey);
            FILETIME accesstime = { 0, 0 };
            
			WCHAR wcurrentKey[1024];                    //add by Jacky
            LPWSTR pcurrentKey = wcurrentKey;           //add by Jacky
			MultiByteToWideChar(CP_ACP, 0, currentKey, -1, pcurrentKey, 1024);

            LONG code = RegEnumKeyEx( hroot, count, (LPSTR)pcurrentKey, &csize, NULL, NULL, NULL, &accesstime );
            if( code != ERROR_SUCCESS && code != ERROR_MORE_DATA )
                break;
            count++;
            if( oldestTime.dwHighDateTime > accesstime.dwHighDateTime ||
                oldestTime.dwHighDateTime == accesstime.dwHighDateTime &&
                oldestTime.dwLowDateTime > accesstime.dwLowDateTime )
            {
                oldestTime = accesstime;
                strcpy( oldestKey, currentKey );
            }
        }

		WCHAR woldestKey[1024];
		LPWSTR poldestKey = woldestKey;
	    MultiByteToWideChar(CP_ACP, 0, oldestKey, -1, poldestKey, 1024);

        if( count >= MAX_RECORD_COUNT )
            RegDeleteKey( hroot, (LPCSTR)poldestKey );
        RegCloseKey( hroot );

        if( RegCreateKeyEx(HKEY_CURRENT_USER,(LPCSTR)wszKey,0,NULL,REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &hkey, NULL) != ERROR_SUCCESS )
		{
			delete [] wszKey;
            wszKey = NULL;
			return;
		}
		delete [] wszKey;
		wszKey = NULL;

		delete [] lprootKey;
		lprootKey = NULL;
    }
    else
    {
        RegCloseKey( hkey );
        if( RegOpenKeyEx( HKEY_CURRENT_USER,(LPCSTR)wszKey,0,KEY_WRITE,&hkey) != ERROR_SUCCESS )
		{
			delete [] wszKey;
			wszKey = NULL;
            return;
		}
    }
    
    RegSetValueEx(hkey,"Left", 0, REG_DWORD, (BYTE*)&rect.x, sizeof(rect.x));
    RegSetValueEx(hkey,"Top", 0, REG_DWORD, (BYTE*)&rect.y, sizeof(rect.y));
    RegSetValueEx(hkey,"Width", 0, REG_DWORD, (BYTE*)&rect.width, sizeof(rect.width));
    RegSetValueEx(hkey,"Height", 0, REG_DWORD, (BYTE*)&rect.height, sizeof(rect.height));
    RegCloseKey(hkey);
}


CV_IMPL int cvNamedWindow( const char* name, int flags )
{
    int result = 0;
    CV_FUNCNAME( "cvNamedWindow" );

    __BEGIN__;

    HWND hWnd, mainhWnd;
    CvWindow* window;
    DWORD defStyle = WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
    int len;
    CvRect rect;

    cvInitSystem(0,0);

    if( !name )
        CV_ERROR( CV_StsNullPtr, "NULL name string" );

    // Check the name in the storage
    if( icvFindWindowByName( name ) != 0 )
    {
        result = 1;
        EXIT;
    }

    if( (flags & CV_WINDOW_AUTOSIZE) == 0 )
        defStyle |= WS_SIZEBOX;

    icvLoadWindowPos( name, rect );

    int nSize1 = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);  
    LPWSTR lpname = new WCHAR[nSize1];
    MultiByteToWideChar(CP_ACP, 0, name, -1, lpname, nSize1);

    mainhWnd = CreateWindow( "Main HighGUI class", (LPCSTR)lpname, defStyle | WS_OVERLAPPED,
                             rect.x, rect.y, rect.width, rect.height, 0, 0, hg_hinstance, 0 );
    if( !mainhWnd )
	{
		delete [] lpname;
		lpname = NULL;
        CV_ERROR( CV_StsError, "Frame window can not be created" );
	}

	if(lpname !=NULL)
		delete [] lpname;
	lpname = NULL;

    ShowWindow(mainhWnd, SW_SHOW);

    hWnd = CreateWindow("HighGUI class", (LPCSTR)"", defStyle | WS_CHILD | WS_SIZEBOX,
                        CW_USEDEFAULT, 0, rect.width, rect.height, mainhWnd, 0, hg_hinstance, 0);
    if( !hWnd )
        CV_ERROR( CV_StsError, "Frame window can not be created" );

    ShowWindow(hWnd, SW_SHOW);

    len = (int)strlen(name);
    CV_CALL( window = (CvWindow*)cvAlloc(sizeof(CvWindow) + len + 1));

    window->signature = CV_WINDOW_MAGIC_VAL;
    window->hwnd = hWnd;
    window->frame = mainhWnd;
    window->name = (char*)(window + 1);
    memcpy( window->name, name, len + 1 );
    window->flags = flags;
    window->image = 0;
    window->dc = CreateCompatibleDC(0);
    window->last_key = 0;

    window->on_mouse = 0;
    window->on_mouse_param = 0;

    memset( &window->toolbar, 0, sizeof(window->toolbar));

    window->next = hg_windows;
    window->prev = 0;
    if( hg_windows )
        hg_windows->prev = window;
    hg_windows = window;
    icvSetWindowLongPtr( hWnd, CV_USERDATA, window );
    icvSetWindowLongPtr( mainhWnd, CV_USERDATA, window );

    // Recalculate window position
    icvUpdateWindowPos( window );

    result = 1;
    __END__;

    return result;
}


static void icvRemoveWindow( CvWindow* window )
{
    CvTrackbar* trackbar;
    RECT wrect;

    GetWindowRect( window->frame, &wrect );
    icvSaveWindowPos( window->name, cvRect(wrect.left, wrect.top,
        wrect.right-wrect.left, wrect.bottom-wrect.top) );

    icvSetWindowLongPtr( window->hwnd, CV_USERDATA, 0 );
    icvSetWindowLongPtr( window->frame, CV_USERDATA, 0 );

    if( window->prev )
        window->prev->next = window->next;
    else
        hg_windows = window->next;

    if( window->next )
        window->next->prev = window->prev;

    window->prev = window->next = 0;

    if( window->image )
        DeleteObject(SelectObject(window->dc,window->image));

    if( window->dc )
        DeleteDC(window->dc);

    for( trackbar = window->toolbar.first; trackbar != 0; )
    {
        CvTrackbar* next = trackbar->next;
        icvSetWindowLongPtr( trackbar->hwnd, CV_USERDATA, 0 );
        cvFree( &trackbar );
        trackbar = next;
    }

    cvFree( &window );
}


CV_IMPL void cvDestroyWindow( const char* name )
{
    CV_FUNCNAME( "cvDestroyWindow" );

    __BEGIN__;

    CvWindow* window;
    HWND mainhWnd;

    if(!name)
        CV_ERROR( CV_StsNullPtr, "NULL name string" );

    window = icvFindWindowByName( name );
    if( !window )
        EXIT;

    mainhWnd = window->frame;

    SendMessage(window->hwnd, WM_CLOSE, 0, 0);
    SendMessage( mainhWnd, WM_CLOSE, 0, 0);
    // Do NOT call _remove_window -- CvWindow list will be updated automatically ...

    __END__;
}


static void icvScreenToClient( HWND hwnd, RECT* rect )
{
    POINT p;
    p.x = rect->left;
    p.y = rect->top;
    ScreenToClient(hwnd, &p);
    OffsetRect( rect, p.x - rect->left, p.y - rect->top );
}


/* Calculatess the window coordinates relative to the upper left corner of the mainhWnd window */
static RECT icvCalcWindowRect( CvWindow* window )
{
    const int gutter = 1;
    RECT crect, trect, rect;

    assert(window);

    GetClientRect(window->frame, &crect);
    if(window->toolbar.toolbar)
    {
        GetWindowRect(window->toolbar.toolbar, &trect);
        icvScreenToClient(window->frame, &trect);
        SubtractRect( &rect, &crect, &trect);
    }
    else
        rect = crect;

    rect.top += gutter;
    rect.left += gutter;
    rect.bottom -= gutter;
    rect.right -= gutter;

    return rect;
}

// returns TRUE if there is a problem such as ERROR_IO_PENDING.
static bool icvGetBitmapData( CvWindow* window, SIZE* size, int* channels, void** data )
{
    BITMAP bmp;
    GdiFlush();
    HGDIOBJ h = GetCurrentObject( window->dc, OBJ_BITMAP );
    if( size )
        size->cx = size->cy = 0;
    if( data )
        *data = 0;

    if (h == NULL)
        return true;
    if (GetObject(h, sizeof(bmp), &bmp) == 0)
        return true;

    if( size )
    {
        size->cx = abs(bmp.bmWidth);
        size->cy = abs(bmp.bmHeight);
    }

    if( channels )
        *channels = bmp.bmBitsPixel/8;

    if( data )
        *data = bmp.bmBits;

    return false;
}


static void icvUpdateWindowPos( CvWindow* window )
{
    RECT rect;
    assert(window);

    if( (window->flags & CV_WINDOW_AUTOSIZE) && window->image )
    {
        int i;
        SIZE size = {0,0};
        icvGetBitmapData( window, &size, 0, 0 );

        // Repeat two times because after the first resizing of the mainhWnd window
        // toolbar may resize too
        for(i = 0; i < (window->toolbar.toolbar ? 2 : 1); i++)
        {
            RECT rmw, rw = icvCalcWindowRect(window );
            MoveWindow(window->hwnd, rw.left, rw.top,
                rw.right - rw.left + 1, rw.bottom - rw.top + 1, FALSE);
            GetClientRect(window->hwnd, &rw);
            GetWindowRect(window->frame, &rmw);
            // Resize the mainhWnd window in order to make the bitmap fit into the child window
            MoveWindow(window->frame, rmw.left, rmw.top,
                rmw.right - rmw.left + size.cx - rw.right + rw.left,
                rmw.bottom  - rmw.top + size.cy - rw.bottom + rw.top, TRUE );
        }
    }

    rect = icvCalcWindowRect(window);
    MoveWindow(window->hwnd, rect.left, rect.top,
               rect.right - rect.left + 1,
               rect.bottom - rect.top + 1, TRUE );
}


CV_IMPL void
cvShowImage( const char* name, const CvArr* arr )
{
    CV_FUNCNAME( "cvShowImage" );

    __BEGIN__;

    CvWindow* window;
    SIZE size = { 0, 0 };
    int channels = 0;
    void* dst_ptr = 0;
    const int channels0 = 3;
    int origin = 0;
    CvMat stub, dst, *image;
    bool changed_size = false; // philipg

    if( !name )
        CV_ERROR( CV_StsNullPtr, "NULL name" );

    window = icvFindWindowByName(name);
    if( !window || !arr )
        EXIT; // keep silence here.

    if( CV_IS_IMAGE_HDR( arr ))
        origin = ((IplImage*)arr)->origin;

    CV_CALL( image = cvGetMat( arr, &stub ));

    if (window->image)
        // if there is something wrong with these system calls, we cannot display image...
        if (icvGetBitmapData( window, &size, &channels, &dst_ptr ))
            return;

    if( size.cx != image->width || size.cy != image->height || channels != channels0 )
    {
        changed_size = true;

        uchar buffer[sizeof(BITMAPINFO) + 255*sizeof(RGBQUAD)];
        BITMAPINFO* binfo = (BITMAPINFO*)buffer;

        DeleteObject( SelectObject( window->dc, window->image ));
        window->image = 0;

        size.cx = image->width;
        size.cy = image->height;
        channels = channels0;

        FillBitmapInfo( binfo, size.cx, size.cy, channels*8, 1 );

        window->image = SelectObject( window->dc, CreateDIBSection(window->dc, binfo,
                                      DIB_RGB_COLORS, &dst_ptr, 0, 0));
    }

    cvInitMatHeader( &dst, size.cy, size.cx, CV_8UC3,
                     dst_ptr, (size.cx * channels + 3) & -4 );
    cvConvertImage( image, &dst, origin == 0 ? CV_CVTIMG_FLIP : 0 );

    // ony resize window if needed
    if (changed_size)
        icvUpdateWindowPos(window);
    InvalidateRect(window->hwnd, 0, 0);
    // philipg: this is not needed and just slows things down
//    UpdateWindow(window->hwnd);

    __END__;
}


CV_IMPL void cvResizeWindow(const char* name, int width, int height )
{
    CV_FUNCNAME( "cvResizeWindow" );

    __BEGIN__;

    int i;
    CvWindow* window;
    RECT rmw, rw, rect;

    if( !name )
        CV_ERROR( CV_StsNullPtr, "NULL name" );

    window = icvFindWindowByName(name);
    if(!window)
        EXIT;

    // Repeat two times because after the first resizing of the mainhWnd window
    // toolbar may resize too
    for(i = 0; i < (window->toolbar.toolbar ? 2 : 1); i++)
    {
        rw = icvCalcWindowRect(window);
        MoveWindow(window->hwnd, rw.left, rw.top,
            rw.right - rw.left + 1, rw.bottom - rw.top + 1, FALSE);
        GetClientRect(window->hwnd, &rw);
        GetWindowRect(window->frame, &rmw);
        // Resize the mainhWnd window in order to make the bitmap fit into the child window
        MoveWindow(window->frame, rmw.left, rmw.top,
            rmw.right - rmw.left + width - rw.right + rw.left,
            rmw.bottom  - rmw.top + height - rw.bottom + rw.top, TRUE);
    }

    rect = icvCalcWindowRect(window);
    MoveWindow(window->hwnd, rect.left, rect.top,
        rect.right - rect.left + 1, rect.bottom - rect.top + 1, TRUE);

    __END__;
}


CV_IMPL void cvMoveWindow( const char* name, int x, int y )
{
    CV_FUNCNAME( "cvMoveWindow" );

    __BEGIN__;

    CvWindow* window;
    RECT rect;

    if( !name )
        CV_ERROR( CV_StsNullPtr, "NULL name" );

    window = icvFindWindowByName(name);
    if(!window)
        EXIT;

    GetWindowRect( window->frame, &rect );
    MoveWindow( window->frame, x, y, rect.right - rect.left, rect.bottom - rect.top, TRUE);

    __END__;
}


static LRESULT CALLBACK
MainWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CvWindow* window = icvWindowByHWND( hwnd );
    if( !window )
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch(uMsg)
    {
    case WM_DESTROY:

        icvRemoveWindow(window);
        // Do nothing!!!
        //PostQuitMessage(0);
        break;

    case WM_GETMINMAXINFO:
        if( !(window->flags & CV_WINDOW_AUTOSIZE) )
        {
            MINMAXINFO* minmax = (MINMAXINFO*)lParam;
            RECT rect;
            LRESULT retval = DefWindowProc(hwnd, uMsg, wParam, lParam);

            minmax->ptMinTrackSize.y = 100;
            minmax->ptMinTrackSize.x = 100;

            if( window->toolbar.first )
            {
                GetWindowRect( window->toolbar.first->hwnd, &rect );
                minmax->ptMinTrackSize.y += window->toolbar.rows*(rect.bottom - rect.top);
                minmax->ptMinTrackSize.x = MAX(rect.right - rect.left + HG_BUDDY_WIDTH, HG_BUDDY_WIDTH*2);
            }
            return retval;
        }
        break;

    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS* pos = (WINDOWPOS*)lParam;

            // Update the toolbar position/size
            if(window->toolbar.toolbar)
            {
                RECT rect;
                GetWindowRect(window->toolbar.toolbar, &rect);
                MoveWindow(window->toolbar.toolbar, 0, 0, pos->cx, rect.bottom - rect.top, TRUE);
            }

            if(!(window->flags & CV_WINDOW_AUTOSIZE))
                icvUpdateWindowPos(window);

            break;
        }

    case WM_ACTIVATE:
        if(LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
            SetFocus(window->hwnd);
        break;

    case WM_ERASEBKGND:
        {
            RECT cr, tr, wrc;
            HRGN rgn, rgn1, rgn2;
            int ret;
            HDC hdc = (HDC)wParam;
            GetWindowRect(window->hwnd, &cr);
            icvScreenToClient(window->frame, &cr);
            if(window->toolbar.toolbar)
            {
                GetWindowRect(window->toolbar.toolbar, &tr);
                icvScreenToClient(window->frame, &tr);
            }
            else
                tr.left = tr.top = tr.right = tr.bottom = 0;

            GetClientRect(window->frame, &wrc);

            rgn = CreateRectRgn(0, 0, wrc.right, wrc.bottom);
            rgn1 = CreateRectRgn(cr.left, cr.top, cr.right, cr.bottom);
            rgn2 = CreateRectRgn(tr.left, tr.top, tr.right, tr.bottom);
            ret = CombineRgn(rgn, rgn, rgn1, RGN_DIFF);
            ret = CombineRgn(rgn, rgn, rgn2, RGN_DIFF);

            if(ret != NULLREGION && ret != ERROR)
                FillRgn(hdc, rgn, (HBRUSH)icvGetClassLongPtr(hwnd, CV_HBRBACKGROUND));

            DeleteObject(rgn);
            DeleteObject(rgn1);
            DeleteObject(rgn2);
        }
        return 1;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


static LRESULT CALLBACK HighGUIProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CvWindow* window = icvWindowByHWND(hwnd);
    if( !window )
        // This window is not mentioned in HighGUI storage
        // Actually, this should be error except for the case of calls to CreateWindow
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    // Process the message
    switch(uMsg)
    {
    case WM_WINDOWPOSCHANGING:
        {
            LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
            RECT rect = icvCalcWindowRect(window);
            pos->x = rect.left;
            pos->y = rect.top;
            pos->cx = rect.right - rect.left + 1;
            pos->cy = rect.bottom - rect.top + 1;
        }
        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
        if( window->on_mouse )
        {
            POINT pt;
            RECT rect;
            SIZE size = {0,0};

            int flags = (wParam & MK_LBUTTON ? CV_EVENT_FLAG_LBUTTON : 0)|
                        (wParam & MK_RBUTTON ? CV_EVENT_FLAG_RBUTTON : 0)|
                        (wParam & MK_MBUTTON ? CV_EVENT_FLAG_MBUTTON : 0)|
                        (wParam & MK_CONTROL ? CV_EVENT_FLAG_CTRLKEY : 0)|
                        (wParam & MK_SHIFT ? CV_EVENT_FLAG_SHIFTKEY : 0)|
                        (GetKeyState(VK_MENU) < 0 ? CV_EVENT_FLAG_ALTKEY : 0);
            int event = uMsg == WM_LBUTTONDOWN ? CV_EVENT_LBUTTONDOWN :
                        uMsg == WM_RBUTTONDOWN ? CV_EVENT_RBUTTONDOWN :
                        uMsg == WM_MBUTTONDOWN ? CV_EVENT_MBUTTONDOWN :
                        uMsg == WM_LBUTTONUP ? CV_EVENT_LBUTTONUP :
                        uMsg == WM_RBUTTONUP ? CV_EVENT_RBUTTONUP :
                        uMsg == WM_MBUTTONUP ? CV_EVENT_MBUTTONUP :
                        uMsg == WM_LBUTTONDBLCLK ? CV_EVENT_LBUTTONDBLCLK :
                        uMsg == WM_RBUTTONDBLCLK ? CV_EVENT_RBUTTONDBLCLK :
                        uMsg == WM_MBUTTONDBLCLK ? CV_EVENT_MBUTTONDBLCLK :
                                                   CV_EVENT_MOUSEMOVE;
            if( uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN )
                SetCapture( hwnd );
            if( uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONUP || uMsg == WM_MBUTTONUP )
                ReleaseCapture();

            pt.x = LOWORD( lParam );
            pt.y = HIWORD( lParam );

            GetClientRect( window->hwnd, &rect );
            icvGetBitmapData( window, &size, 0, 0 );

            window->on_mouse( event, pt.x*size.cx/MAX(rect.right - rect.left,1),
                                     pt.y*size.cy/MAX(rect.bottom - rect.top,1), flags,
                                     window->on_mouse_param );
        }
        break;

    case WM_PAINT:
        if(window->image != 0)
        {
            int nchannels = 3;
            SIZE size = {0,0};
            PAINTSTRUCT paint;
            HDC hdc;
            RGBQUAD table[256];

            // Determine the bitmap's dimensions
            icvGetBitmapData( window, &size, &nchannels, 0 );

            hdc = BeginPaint(hwnd, &paint);
            SetStretchBltMode(hdc, COLORONCOLOR);

            if( nchannels == 1 )
            {
                int i;
                for(i = 0; i < 256; i++)
                {
                    table[i].rgbBlue = (unsigned char)i;
                    table[i].rgbGreen = (unsigned char)i;
                    table[i].rgbRed = (unsigned char)i;
                }
                SetDIBColorTable(window->dc, 0, 255, table);
            }

            if(window->flags & CV_WINDOW_AUTOSIZE)
            {
                BitBlt( hdc, 0, 0, size.cx, size.cy, window->dc, 0, 0, SRCCOPY );
            }
            else
            {
                RECT rect;
                GetClientRect(window->hwnd, &rect);
                StretchBlt( hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                            window->dc, 0, 0, size.cx, size.cy, SRCCOPY );
            }
            //DeleteDC(hdc);
            EndPaint(hwnd, &paint);
        }
        else
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        return 0;

    case WM_ERASEBKGND:
        if(window->image)
            return 0;
        break;

    case WM_DESTROY:

        icvRemoveWindow(window);
        // Do nothing!!!
        //PostQuitMessage(0);
        break;

    case WM_SETCURSOR:
        SetCursor((HCURSOR)icvGetClassLongPtr(hwnd, CV_HCURSOR));
        return 0;

    case WM_KEYDOWN:
        window->last_key = (int)wParam;
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


static LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    LRESULT ret;

    if( hg_on_preprocess )
    {
        int was_processed = 0;
        int ret = hg_on_preprocess(hwnd, uMsg, wParam, lParam, &was_processed);
        if( was_processed )
            return ret;
    }
    ret = HighGUIProc(hwnd, uMsg, wParam, lParam);

    if(hg_on_postprocess)
    {
        int was_processed = 0;
        int ret = hg_on_postprocess(hwnd, uMsg, wParam, lParam, &was_processed);
        if( was_processed )
            return ret;
    }

    return ret;
}


static void icvUpdateTrackbar( CvTrackbar* trackbar, int pos )
{
    const int max_name_len = 10;
    const char* suffix = "";
    char pos_text[32];
    int name_len;

    if( trackbar->data )
        *trackbar->data = pos;

    if( trackbar->pos != pos )
    {
        trackbar->pos = pos;
        if( trackbar->notify )
            trackbar->notify(pos);

        name_len = (int)strlen(trackbar->name);

        if( name_len > max_name_len )
        {
            int start_len = max_name_len*2/3;
            int end_len = max_name_len - start_len - 2;
            memcpy( pos_text, trackbar->name, start_len );
            memcpy( pos_text + start_len, "...", 3 );
            memcpy( pos_text + start_len + 3, trackbar->name + name_len - end_len, end_len + 1 );
        }
        else
        {
            memcpy( pos_text, trackbar->name, name_len + 1);
        }

        sprintf( pos_text + strlen(pos_text), "%s: %d\n", suffix, pos );

        WCHAR    wpos_text[32];
        LPWSTR  lppos_text = wpos_text;
        MultiByteToWideChar(CP_ACP, 0, pos_text, -1, lppos_text, 32);

        SetWindowText( trackbar->buddy, (LPCSTR)lppos_text );
    }
}


static LRESULT CALLBACK HGToolbarProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CvWindow* window = icvWindowByHWND( hwnd );
    if(!window)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    // Control messages processing
    switch(uMsg)
    {
    // Slider processing
    case WM_HSCROLL:
        {
            HWND slider = (HWND)lParam;
            int pos = (int)SendMessage(slider, TBM_GETPOS, 0, 0);
            CvTrackbar* trackbar = icvTrackbarByHWND( slider );

            if( trackbar )
            {
                if( trackbar->pos != pos )
                    icvUpdateTrackbar( trackbar, pos );
            }

            SetFocus( window->hwnd );
            return 0;
        }

    case WM_NCCALCSIZE:
        {
            LRESULT ret = CallWindowProc(window->toolbar.toolBarProc, hwnd, uMsg, wParam, lParam);
            int rows = (int)SendMessage(hwnd, TB_GETROWS, 0, 0);

            if(window->toolbar.rows != rows)
            {
                SendMessage(window->toolbar.toolbar, TB_BUTTONCOUNT, 0, 0);
                CvTrackbar* trackbar = window->toolbar.first;

                for( ; trackbar != 0; trackbar = trackbar->next )
                {
                    RECT rect;
                    SendMessage(window->toolbar.toolbar, TB_GETITEMRECT,
                               (WPARAM)trackbar->id, (LPARAM)&rect);
                    MoveWindow(trackbar->hwnd, rect.left + HG_BUDDY_WIDTH, rect.top,
                               rect.right - rect.left - HG_BUDDY_WIDTH,
                               rect.bottom - rect.top, FALSE);
                    MoveWindow(trackbar->buddy, rect.left, rect.top,
                               HG_BUDDY_WIDTH, rect.bottom - rect.top, FALSE);
                }
                window->toolbar.rows = rows;
            }
            return ret;
        }
    }

    return CallWindowProc(window->toolbar.toolBarProc, hwnd, uMsg, wParam, lParam);
}


CV_IMPL void
cvDestroyAllWindows(void)
{
    CvWindow* window = hg_windows;

    while( window )
    {
        HWND mainhWnd = window->frame;
        HWND hwnd = window->hwnd;
        window = window->next;

        SendMessage( hwnd, WM_CLOSE, 0, 0 );
        SendMessage( mainhWnd, WM_CLOSE, 0, 0 );
    }
}


CV_IMPL int
cvWaitKey( int delay )
{
    int time0 = GetTickCount();

    for(;;)
    {
        CvWindow* window;
        MSG message;
        int is_processed = 0;

        if( (delay > 0 && abs((int)(GetTickCount() - time0)) >= delay) || hg_windows == 0 )
            return -1;

        if( delay <= 0 )
            GetMessage(&message, 0, 0, 0);
        else if( PeekMessage(&message, 0, 0, 0, PM_REMOVE) == FALSE )
        {
            Sleep(1);
            continue;
        }

        for( window = hg_windows; window != 0 && is_processed == 0; window = window->next )
        {
            if( window->hwnd == message.hwnd || window->frame == message.hwnd )
            {
                is_processed = 1;
                switch(message.message)
                {
                case WM_DESTROY:
                case WM_CHAR:
                    DispatchMessage(&message);
                    return (int)message.wParam;

                case WM_KEYDOWN:
                    TranslateMessage(&message);
                default:
                    DispatchMessage(&message);
                    is_processed = 1;
                    break;
                }
            }
        }

        if( !is_processed )
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }
}


static CvTrackbar*
icvFindTrackbarByName( const CvWindow* window, const char* name )
{
    CvTrackbar* trackbar = window->toolbar.first;

    for( ; trackbar != 0 && strcmp( trackbar->name, name ) != 0; trackbar = trackbar->next )
        ;

    return trackbar;
}


typedef struct
{
    UINT cbSize;
    DWORD dwMask;
    int idCommand;
    int iImage;
    BYTE fsState;
    BYTE fsStyle;
    WORD cx;
    DWORD lParam;
    LPSTR pszText;
    int cchText;
}
ButtonInfo;


CV_IMPL int
cvCreateTrackbar( const char* trackbar_name, const char* window_name,
                  int* val, int count, CvTrackbarCallback on_notify )
{
//     int result = 0;
// 
//     CV_FUNCNAME( "cvCreateTrackbar" );
// 
//     __BEGIN__;
// 
//     char slider_name[32];
//     CvWindow* window = 0;
//     CvTrackbar* trackbar = 0;
//     int pos = 0;
// 
//     if( !window_name || !trackbar_name )
//         CV_ERROR( CV_StsNullPtr, "NULL window or trackbar name" );
// 
//     if( count <= 0 )
//         CV_ERROR( CV_StsOutOfRange, "Bad trackbar maximal value" );
// 
//     window = icvFindWindowByName(window_name);
//     if( !window )
//         EXIT;
// 
//     trackbar = icvFindTrackbarByName(window,trackbar_name);
//     if( !trackbar )
//     {
//         TBBUTTON tbs;
//         ButtonInfo tbis;
//         RECT rect;
//         int bcount;
//         int len = (int)strlen( trackbar_name );
// 
//         // create toolbar if it is not created yet
//         if( !window->toolbar.toolbar )
//         {
//             const int default_height = 30;
// 
//             window->toolbar.toolbar = CreateToolbarEx(
//                     window->frame, WS_CHILD | CCS_TOP | TBSTYLE_WRAPABLE,
//                     1, 0, 0, 0, 0, 0, 16, 20, 16, 16, sizeof(TBBUTTON));
//             GetClientRect(window->frame, &rect);
//             MoveWindow( window->toolbar.toolbar, 0, 0,
//                         rect.right - rect.left, default_height, TRUE);
//             SendMessage(window->toolbar.toolbar, TB_AUTOSIZE, 0, 0);
//             ShowWindow(window->toolbar.toolbar, SW_SHOW);
// 
//             window->toolbar.first = 0;
//             window->toolbar.pos = 0;
//             window->toolbar.rows = 0;
//             window->toolbar.toolBarProc =
//                 (WNDPROC)icvGetWindowLongPtr(window->toolbar.toolbar, CV_WNDPROC);
// 
//             icvUpdateWindowPos(window);
// 
//             // Subclassing from toolbar
//             icvSetWindowLongPtr(window->toolbar.toolbar, CV_WNDPROC, HGToolbarProc);
//             icvSetWindowLongPtr(window->toolbar.toolbar, CV_USERDATA, window);
//         }
// 
//         /* Retrieve current buttons count */
//         bcount = (int)SendMessage(window->toolbar.toolbar, TB_BUTTONCOUNT, 0, 0);
// 
//         if(bcount > 1)
//         {
//             /* If this is not the first button then we need to
//             separate it from the previous one */
//             tbs.iBitmap = 0;
//             tbs.idCommand = bcount; // Set button id to it's number
//             tbs.iString = 0;
//             tbs.fsStyle = TBSTYLE_SEP;
//             tbs.fsState = TBSTATE_ENABLED;
//             SendMessage(window->toolbar.toolbar, TB_ADDBUTTONS, 1, (LPARAM)&tbs);
// 
//             // Retrieve current buttons count
//             bcount = (int)SendMessage(window->toolbar.toolbar, TB_BUTTONCOUNT, 0, 0);
//         }
// 
//         /* Add a button which we're going to cover with the slider */
//         tbs.iBitmap = 0;
//         tbs.idCommand = bcount; // Set button id to it's number
//         tbs.fsState = TBSTATE_ENABLED;
// #if 0/*!defined WIN64 && !defined EM64T*/
//         tbs.fsStyle = 0;
//         tbs.iString = 0;
// #else
// #ifndef TBSTYLE_AUTOSIZE
// #define TBSTYLE_AUTOSIZE        0x0010
// #define TBSTYLE_GROUP           0x0004
// #endif
//         //tbs.fsStyle = TBSTYLE_AUTOSIZE;
//         tbs.fsStyle = TBSTYLE_GROUP;
//         tbs.iString = (INT_PTR)trackbar_text;
// #endif
//         SendMessage(window->toolbar.toolbar, TB_ADDBUTTONS, 1, (LPARAM)&tbs);
// 
//         /* Adjust button size to the slider */
//         tbis.cbSize = sizeof(tbis);
//         tbis.dwMask = TBIF_SIZE;
// 
//         GetClientRect(window->hwnd, &rect);
//         tbis.cx = (unsigned short)(rect.right - rect.left);
// 
//         SendMessage(window->toolbar.toolbar, TB_SETBUTTONINFO,
//             (WPARAM)tbs.idCommand, (LPARAM)&tbis);
// 
//         /* Get button position */
//         SendMessage(window->toolbar.toolbar, TB_GETITEMRECT,
//             (WPARAM)tbs.idCommand, (LPARAM)&rect);
// 
//         /* Create a slider */
//         trackbar = (CvTrackbar*)cvAlloc( sizeof(CvTrackbar) + len + 1 );
//         trackbar->signature = CV_TRACKBAR_MAGIC_VAL;
//         trackbar->notify = 0;
//         trackbar->parent = window;
//         trackbar->pos = 0;
//         trackbar->data = 0;
//         trackbar->id = bcount;
//         trackbar->next = window->toolbar.first;
//         trackbar->name = (char*)(trackbar + 1);
//         memcpy( trackbar->name, trackbar_name, len + 1 );
//         window->toolbar.first = trackbar;
// 
//         sprintf(slider_name, "Trackbar%p", val);
//         trackbar->hwnd = CreateWindowEx(0, TRACKBAR_CLASS, slider_name,
//                             WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS |
//                             TBS_FIXEDLENGTH | TBS_HORZ | TBS_BOTTOM,
//                             rect.left + HG_BUDDY_WIDTH, rect.top,
//                             rect.right - rect.left - HG_BUDDY_WIDTH,
//                             rect.bottom - rect.top, window->toolbar.toolbar,
//                             (HMENU)(size_t)bcount, hg_hinstance, 0);
// 
//         sprintf(slider_name,"Buddy%p", val);
//         trackbar->buddy = CreateWindowEx(0, "STATIC", slider_name,
//                             WS_CHILD | SS_RIGHT,
//                             rect.left, rect.top,
//                             HG_BUDDY_WIDTH, rect.bottom - rect.top,
//                             window->toolbar.toolbar, 0, hg_hinstance, 0);
// 
//         icvSetWindowLongPtr( trackbar->hwnd, CV_USERDATA, trackbar );
// 
//         /* Minimize the number of rows */
//         SendMessage( window->toolbar.toolbar, TB_SETROWS,
//                      MAKEWPARAM(1, FALSE), (LPARAM)&rect );
//     }
//     else
//     {
//         trackbar->data = 0;
//         trackbar->notify = 0;
//     }
// 
//     trackbar->maxval = count;
// 
//     /* Adjust slider parameters */
//     SendMessage(trackbar->hwnd, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, count));
//     SendMessage(trackbar->hwnd, TBM_SETTICFREQ, (WPARAM)1, (LPARAM)0 );
//     if( val )
//         pos = *val;
// 
//     SendMessage(trackbar->hwnd, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pos );
//     SendMessage(window->toolbar.toolbar, TB_AUTOSIZE, 0, 0);
// 
//     trackbar->pos = -1;
//     icvUpdateTrackbar( trackbar, pos );
//     ShowWindow( trackbar->buddy, SW_SHOW );
//     ShowWindow( trackbar->hwnd, SW_SHOW );
// 
//     trackbar->notify = on_notify;
//     trackbar->data = val;
// 
//     /* Resize the window to reflect the toolbar resizing*/
//     icvUpdateWindowPos(window);
// 
//     result = 1;
// 
//     __END__;

  //  return result;
return 0;
}


CV_IMPL void
cvSetMouseCallback( const char* window_name, CvMouseCallback on_mouse, void* param )
{
    CV_FUNCNAME( "cvSetMouseCallback" );

    __BEGIN__;

    CvWindow* window = 0;

    if( !window_name )
        CV_ERROR( CV_StsNullPtr, "NULL window name" );

    window = icvFindWindowByName(window_name);
    if( !window )
        EXIT;

    window->on_mouse = on_mouse;
    window->on_mouse_param = param;

    __END__;
}


CV_IMPL int cvGetTrackbarPos( const char* trackbar_name, const char* window_name )
{
    int pos = -1;

    CV_FUNCNAME( "cvGetTrackbarPos" );

    __BEGIN__;

    CvWindow* window;
    CvTrackbar* trackbar = 0;

    if( trackbar_name == 0 || window_name == 0 )
        CV_ERROR( CV_StsNullPtr, "NULL trackbar or window name" );

    window = icvFindWindowByName( window_name );
    if( window )
        trackbar = icvFindTrackbarByName( window, trackbar_name );

    if( trackbar )
        pos = trackbar->pos;

    __END__;

    return pos;
}


CV_IMPL void cvSetTrackbarPos( const char* trackbar_name, const char* window_name, int pos )
{
    CV_FUNCNAME( "cvSetTrackbarPos" );

    __BEGIN__;

    CvWindow* window;
    CvTrackbar* trackbar = 0;

    if( trackbar_name == 0 || window_name == 0 )
        CV_ERROR( CV_StsNullPtr, "NULL trackbar or window name" );

    window = icvFindWindowByName( window_name );
    if( window )
        trackbar = icvFindTrackbarByName( window, trackbar_name );

    if( trackbar )
    {
        if( pos < 0 )
            pos = 0;

        if( pos > trackbar->maxval )
            pos = trackbar->maxval;

        SendMessage( trackbar->hwnd, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pos );
        icvUpdateTrackbar( trackbar, pos );
    }

    __END__;
}


CV_IMPL void* cvGetWindowHandle( const char* window_name )
{
    void* hwnd = 0;

    CV_FUNCNAME( "cvGetWindowHandle" );

    __BEGIN__;

    CvWindow* window;

    if( window_name == 0 )
        CV_ERROR( CV_StsNullPtr, "NULL window name" );

    window = icvFindWindowByName( window_name );
    if( window )
        hwnd = (void*)window->hwnd;

    __END__;

    return hwnd;
}


CV_IMPL const char* cvGetWindowName( void* window_handle )
{
    const char* window_name = "";

    CV_FUNCNAME( "cvGetWindowName" );

    __BEGIN__;

    CvWindow* window;

    if( window_handle == 0 )
        CV_ERROR( CV_StsNullPtr, "NULL window" );

    window = icvWindowByHWND( (HWND)window_handle );
    if( window )
        window_name = window->name;

    __END__;

    return window_name;
}



// CV_IMPL void
// cvSetPreprocessFuncWin32(int (__cdecl *on_preprocess)(HWND, UINT, WPARAM, LPARAM, int*))
// {
//     if(on_preprocess)
//         hg_on_preprocess = on_preprocess;
//     else
//         assert(on_preprocess);
// }

CV_IMPL void
cvSetPostprocessFuncWin32(int (__cdecl *on_postprocess)(HWND, UINT, WPARAM, LPARAM, int*))
{
    if(on_postprocess)
        hg_on_postprocess = on_postprocess;
    else
        assert(on_postprocess);
}

#endif //WIN32

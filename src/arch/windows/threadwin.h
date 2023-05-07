#ifndef __THREADWIN_H__
#define __THREADWIN_H__

#include "pgcore.h"
#include "svg.h"

#if 0	

unsigned WINAPI _monitor_msp_thread(LPVOID lpData)
{
	DWORD dwRet;
	HWND hWndUI = (HWND)lpData;
	ATLASSERT(::IsWindow(hWndUI));

	g_kaSignal[1] = FindFirstChangeNotification(g_filepath, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);

	if(INVALID_HANDLE_VALUE == g_kaSignal[1]) {
		MessageBox(NULL, g_filepath, _T("Cannot Monitor"), MB_OK);
		return 0;
	}

	InterlockedIncrement(&g_threadCount);

	dwRet = MsgWaitForMultipleObjects(2, g_kaSignal, FALSE, INFINITE, QS_ALLINPUT);

	if(WAIT_OBJECT_0 + 1 == dwRet)
	{
		//MessageBox(NULL, g_filepath, _T("MB_OK"), MB_OK);
		PostMessage(hWndUI, WM_UI_NOTIFY, UI_NOTIFY_MONITOR, 0);
	}

	InterlockedDecrement(&g_threadCount);

	MessageBox(NULL, _T("Monitoring thead is quiting!"), _T("MB_OK"), MB_OK);

	return 0;
}
#endif 

static MemoryContext renderCxt = NULL;

unsigned WINAPI open_mspfile_thread(LPVOID lpData)
{
	int   fd;
	unsigned char *p;
    unsigned char  png_magic[8] = { 0 };

	unsigned int size, align_size, bytes, ret;
	fileType ft = fileUnKnown;

	HWND hWndUI = (HWND)lpData;
	ATLASSERT(::IsWindow(hWndUI));

	if (0 != _tsopen_s(&fd, g_filepath, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0)) 
	{
		return 0;
	}
	
	size = (unsigned int)_lseek(fd, 0, SEEK_END); /* get the video file size */
	if (size > MAX_BUF_LEN) 
	{
		_close(fd);
		return 0;
	}

    _lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    if(size > 8) /* try to detect PNG header */
    {
        bytes = (unsigned int)_read(fd, png_magic, 8);
        if(8 != bytes) 
        {
            _close(fd);
            return 0;
        }
        /* check PNG magic number: 89 50 4e 47 0d 0a 1a 0a */
        p = (unsigned char*)png_magic;
        
        if((0x89 == p[0]) && (0x50 == p[1]) && (0x4e == p[2]) && (0x47 == p[3]) && 
           (0x0d == p[4]) && (0x0a == p[5]) && (0x1a == p[6]) && (0x0a == p[7]))
        {
            ft = filePNG;
        }
    }

	align_size = MSP_ALIGN_PAGE(size); /* 4K page align for performance */

	_lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    if(filePNG == ft) 
    {
        if(NULL != renderCxt) 
        {
            EnterCriticalSection(&(d2d.cs));
                d2d.pData = NULL;
            LeaveCriticalSection(&(d2d.cs));

            MemoryContextDelete(renderCxt);
            renderCxt = NULL;
        }

        renderCxt = AllocSetContextCreate(TopMemoryContext,
                                        "renderCxt",
                                        ALLOCSET_DEFAULT_SIZES);
        if(NULL == renderCxt)
        {
            _close(fd);
            return 0;
        }
        
        p = (unsigned char*)palloc(align_size);
        
        if(NULL == p)
        {
            _close(fd);
            return 0;
        }

        bytes = (unsigned int)_read(fd, p, size); /* read the entire PNG file into the buffer */

        if (bytes != size) /* read error, since bytes != size */
        {
            MemoryContextDelete(renderCxt);
            renderCxt = NULL;
            _close(fd);
            return 0;
        }
        _close(fd);

        RenderNode n = (RenderNode)palloc(sizeof(RenderNodeData));
        if(NULL == n)
        {
            MemoryContextDelete(renderCxt);
            renderCxt = NULL;
            return 0;
        }

        n->flag = SO_TYPE_IMAGE;
        n->next = NULL;
        n->data	= p;
        n->len  = size;

        EnterCriticalSection(&(d2d.cs));
            d2d.pData = n;
        LeaveCriticalSection(&(d2d.cs));

        PostMessage(hWndUI, WM_UI_NOTIFY, UI_NOTIFY_FILEOPN, ft);
        return 0;
    }

	_close(fd);
    return 0;

#if 0
	if(NULL != g_buffer) { free(g_buffer); }

	g_buffer = (char*)malloc(align_size);
	if (NULL == g_buffer)
	{
		_close(fd);
		return 0;
	}


	bytes = (unsigned int)_read(fd, g_buffer, size); /* read the entire file into g_buffer */
	if (bytes != size) /* read error, since bytes != size */
	{
		free(g_buffer);
		g_buffer = NULL;
		_close(fd);
		return 0;
	}
	_close(fd);

	if(fileUnKnown == ft) 
	{
		free(g_buffer);
		g_buffer = NULL;
	}

	PostMessage(hWndUI, WM_UI_NOTIFY, UI_NOTIFY_FILEOPN, ft);
	return 0;
#endif     
}

#endif /* __THREADWIN_H__ */
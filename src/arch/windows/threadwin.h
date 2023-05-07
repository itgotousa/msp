#ifndef __THREADWIN_H__
#define __THREADWIN_H__

#include "pgcore.h"
#include "svg.h"

//static MemoryContext CurrRenderCxt = NULL;
//static MemoryContext NewRenderCxt  = NULL;
static unsigned char* file_buffer  = NULL;

static void ReleaseD2DResource(D2DRenderNode n)
{
    while(NULL != n)
    {
        if(NULL != n->pConverter)
        {
            n->pConverter->Release();
            n->pConverter = NULL;
        }
        if(NULL != n->pFrame)
        {
            n->pFrame->Release();
            n->pFrame = NULL;
        }
        if(NULL != n->pDecoder)
        {
            n->pDecoder->Release();
            n->pDecoder = NULL;
        }
        if(NULL != n->pStream)
        {
            n->pStream->Release();
            n->pStream = NULL;
        }
        n = (D2DRenderNode)n->std.next;
    }
}

unsigned WINAPI open_mspfile_thread(LPVOID lpData)
{
	int   fd;
	unsigned char *p;
    unsigned char  png_magic[8] = { 0 };
	unsigned int size, bytes, ret;
	fileType ft = fileUnKnown;
    WPARAM wp = 0;
    LPARAM lp = 0;
    D2DRenderNode m = NULL;
    MemoryContext mcxt = NULL;

	HWND hWndUI = (HWND)lpData;
	ATLASSERT(::IsWindow(hWndUI));

	if (0 != _tsopen_s(&fd, g_filepath, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0)) 
	{
        wp = UI_NOTIFY_FILEFAIL;
        lp = 1;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);
		return 0;
	}
	
	size = (unsigned int)_lseek(fd, 0, SEEK_END); /* get the video file size */
	if (size > MAX_BUF_LEN) 
	{
        wp = UI_NOTIFY_FILEFAIL;
        lp = 2;
        goto Quit_open_mspfile_thread;
	}

    _lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    if(size > 8) /* try to detect PNG header */
    {
        bytes = (unsigned int)_read(fd, png_magic, 8);
        if(8 != bytes) 
        {
            wp = UI_NOTIFY_FILEFAIL;
            lp = 3;
            goto Quit_open_mspfile_thread;
        }
        /* check PNG magic number: 89 50 4e 47 0d 0a 1a 0a */
        p = (unsigned char*)png_magic;
        
        if((0x89 == p[0]) && (0x50 == p[1]) && (0x4e == p[2]) && (0x47 == p[3]) && 
           (0x0d == p[4]) && (0x0a == p[5]) && (0x1a == p[6]) && (0x0a == p[7]))
        {
            ft = filePNG;
        }
    }

	_lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    if(filePNG == ft) 
    {
        mcxt = AllocSetContextCreate(TopMemoryContext,
                                    "renderCxt",
                                    ALLOCSET_DEFAULT_SIZES);
        if(NULL == mcxt)
        {
            wp = UI_NOTIFY_FILEFAIL;
            lp = 5;
            goto Quit_open_mspfile_thread;
        }
        
        MemoryContextSwitchTo(mcxt);
        p = (unsigned char*)palloc(size);
        
        if(NULL == p)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }

        bytes = (unsigned int)_read(fd, p, size); /* read the entire PNG file into the buffer */

        if (bytes != size) /* read error, since bytes != size */
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 7;
            goto Quit_open_mspfile_thread;
        }

        D2DRenderNode n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
        if(NULL == n)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 8;
            goto Quit_open_mspfile_thread;
        }

        n->std.flag = SO_TYPE_IMAGE;
        n->std.next = NULL;
        n->std.data	= p;
        n->std.len  = size;

        d2d.pIWICFactory->CreateStream(&(n->pStream));
        if(NULL == n->pStream)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 7;
            goto Quit_open_mspfile_thread;
        }

        n->pStream->InitializeFromMemory((WICInProcPointer)(n->std.data), n->std.len);
        d2d.pIWICFactory->CreateDecoderFromStream(n->pStream, NULL, WICDecodeMetadataCacheOnLoad, 
                                                &(n->pDecoder));

        if(NULL == n->pDecoder)
        {
            n->pStream->Release();
            n->pStream = NULL;
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 7;
            goto Quit_open_mspfile_thread;
        }

        n->pDecoder->GetFrame(0, &(n->pFrame));
        if(NULL == n->pFrame)
        {
            n->pStream->Release();
            n->pStream = NULL;
            n->pDecoder->Release();
            n->pDecoder = NULL;
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 7;
            goto Quit_open_mspfile_thread;
        }

        d2d.pIWICFactory->CreateFormatConverter(&(n->pConverter));
        if(NULL == n->pConverter)
        {
            n->pStream->Release();
            n->pStream = NULL;
            n->pDecoder->Release();
            n->pDecoder = NULL;
            n->pFrame->Release();
            n->pFrame = NULL;
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 7;
            goto Quit_open_mspfile_thread;
        }

        n->pConverter->Initialize(n->pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, 
                                NULL, 0.0, WICBitmapPaletteTypeMedianCut);

        m = d2d.pData;
        EnterCriticalSection(&(d2d.cs));
            d2d.pData = n;
        LeaveCriticalSection(&(d2d.cs));
        
        ReleaseD2DResource(m);

        if(NULL != m)
        {
            mcxt = *(MemoryContext *) (((char *) m) - sizeof(void *)); 
            MemoryContextDelete(mcxt);
        }

        wp = UI_NOTIFY_FILEOPEN;
        lp = (LPARAM)ft;
    }

Quit_open_mspfile_thread:
	_close(fd);
    PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);

    return 0;
}

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

#endif /* __THREADWIN_H__ */
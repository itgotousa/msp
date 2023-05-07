#include "stdafx.h"
#include "pgcore.h"
#include "svg.h"
#include "resource.h"
#include "mspwin.h"

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
    HRESULT hr = S_OK;
    ThreadParam* t = (ThreadParam*)lpData;

    if(NULL == t) return 0;
	HWND hWndUI = t->hWnd;
	ATLASSERT(::IsWindow(hWndUI));

	if (0 != _tsopen_s(&fd, t->pfilePath, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0)) 
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
        if((0xff == p[0]) && (0xd8 == p[1]))
        {
            ft = fileJPG;
        }
        if((0x47 == p[0]) && (0x49 == p[1]) && (0x46 == p[2]) && (0x38 == p[3]) && 
           (0x39 == p[4]) && (0x61 == p[5]))
        {
            ft = fileGIF;
        }

    }

	_lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    if(filePNG == ft || fileJPG == ft || fileGIF == ft) 
    {
        mcxt = AllocSetContextCreate(TopMemoryContext,
                                    "ImageCxt",
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
        if(fileJPG == ft) /* we still need to check the last two bytes */
        {
            if((0xff != p[size-2]) || (0xd9 != p[size-1]))
            {
                MemoryContextDelete(mcxt);
                wp = UI_NOTIFY_FILEFAIL;
                lp = 7;
                goto Quit_open_mspfile_thread;
            }
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

        hr = d2d.pIWICFactory->CreateStream(&(n->pStream));
        if(FAILED(hr) || NULL == n->pStream)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 7;
            goto Quit_open_mspfile_thread;
        }

        hr = n->pStream->InitializeFromMemory((WICInProcPointer)(n->std.data), n->std.len);
        if(FAILED(hr))
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 7;
            goto Quit_open_mspfile_thread;
        }

        hr = d2d.pIWICFactory->CreateDecoderFromStream(n->pStream, NULL, WICDecodeMetadataCacheOnLoad, 
                                                &(n->pDecoder));

        if(FAILED(hr) || NULL == n->pDecoder)
        {
            n->pStream->Release();
            n->pStream = NULL;
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 7;
            goto Quit_open_mspfile_thread;
        }

        hr = n->pDecoder->GetFrame(0, &(n->pFrame));
        if(FAILED(hr) || NULL == n->pFrame)
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

        hr = d2d.pIWICFactory->CreateFormatConverter(&(n->pConverter));
        if(FAILED(hr) || NULL == n->pConverter)
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

        hr = n->pConverter->Initialize(n->pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, 
                                NULL, 0.0, WICBitmapPaletteTypeMedianCut);

        if(FAILED(hr))
        {
            n->pStream->Release();
            n->pStream = NULL;
            n->pDecoder->Release();
            n->pDecoder = NULL;
            n->pFrame->Release();
            n->pFrame = NULL;
            n->pConverter->Release();
            n->pConverter = NULL;

            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 7;
            goto Quit_open_mspfile_thread;
        }

        d2d.ft = ft;        
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
        goto Quit_open_mspfile_thread;
    }

    ft = fileSVG;
    if(fileSVG == ft)
    {
        mcxt = AllocSetContextCreate(TopMemoryContext,
                                    "SVGCxt",
                                    ALLOCSET_DEFAULT_SIZES);
        if(NULL == mcxt)
        {
            wp = UI_NOTIFY_FILEFAIL;
            lp = 5;
            goto Quit_open_mspfile_thread;
        }
        
        MemoryContextSwitchTo(mcxt);

        D2DRenderNode n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
        if(NULL == n)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 8;
            goto Quit_open_mspfile_thread;
        }

        hr = d2d.pFactory->CreatePathGeometry(&(n->pGeometry));
        if(FAILED(hr) || NULL == n->pGeometry)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 7;
            goto Quit_open_mspfile_thread;
        }

        n->std.flag = SO_TYPE_GRAPHIC;

        ID2D1GeometrySink *pSink = NULL;
        hr = n->pGeometry->Open(&pSink);
        if (SUCCEEDED(hr))
        {
            pSink->SetFillMode(D2D1_FILL_MODE_WINDING);
            pSink->BeginFigure(
                D2D1::Point2F(10,10),
                D2D1_FIGURE_BEGIN_HOLLOW
                );
            D2D1_POINT_2F points[] = {
                D2D1::Point2F(110, 10),
                D2D1::Point2F(110, 110),
                };
            pSink->AddLines(points, ARRAYSIZE(points));
            pSink->EndFigure(D2D1_FIGURE_END_OPEN);    
        }

        hr = pSink->Close();
        pSink->Release();
        pSink = NULL;

        d2d.ft = ft;        
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
        goto Quit_open_mspfile_thread;
    }

Quit_open_mspfile_thread:
	_close(fd);
    PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);

    return 0;
}


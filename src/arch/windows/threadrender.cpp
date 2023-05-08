#include "stdafx.h"
#include "pgcore.h"
#include "svg.h"
#include "resource.h"
#include "mspwin.h"

static HRESULT GetBackgroundColor(IWICMetadataQueryReader*, 
                        IWICBitmapDecoder*, D2D1_COLOR_F*);
static Animation GetAnimationMetaData(IWICBitmapDecoder*);

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

        hr = d2d.pIWICFactory->CreateDecoderFromStream(n->pStream, NULL,
                                WICDecodeMetadataCacheOnLoad, &(n->pDecoder));

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

        if(fileGIF == ft) {  n->am = GetAnimationMetaData(n->pDecoder); }

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

/******************************************************************
*                                                                 *
*  GetBackgroundColor()                                           *
*                                                                 *
*  Reads and stores the background color for gif.                 *
*                                                                 *
******************************************************************/
static HRESULT GetBackgroundColor(IWICMetadataQueryReader* qr, 
                            IWICBitmapDecoder* de, D2D1_COLOR_F* bc)
{
    HRESULT hr = S_OK;
    DWORD dwBGColor;
    BYTE backgroundIndex = 0;
    WICColor rgColors[256];
    UINT cColorsCopied = 0;
    PROPVARIANT propVariant;
    PropVariantInit(&propVariant);
    IWICPalette *pWicPalette = NULL;

    // If we have a global palette, get the palette and background color
    hr = qr->GetMetadataByName(L"/logscrdesc/GlobalColorTableFlag", &propVariant);
    if (SUCCEEDED(hr))
    {
        hr = (propVariant.vt != VT_BOOL || !propVariant.boolVal) ? E_FAIL : S_OK;
        PropVariantClear(&propVariant);
    }
    if (SUCCEEDED(hr))
    {
        // Background color index
        hr = qr->GetMetadataByName(L"/logscrdesc/BackgroundColorIndex", &propVariant);
        if (SUCCEEDED(hr))
        {
            hr = (propVariant.vt != VT_UI1) ? E_FAIL : S_OK;
            if (SUCCEEDED(hr))
            {
                backgroundIndex = propVariant.bVal;
            }
            PropVariantClear(&propVariant);
        }
    }
    // Get the color from the palette
    if (SUCCEEDED(hr))
    {
        hr = d2d.pIWICFactory->CreatePalette(&pWicPalette);
    }
    if (SUCCEEDED(hr))
    {
        // Get the global palette
        hr = de->CopyPalette(pWicPalette);
    }
    if (SUCCEEDED(hr))
    {
        hr = pWicPalette->GetColors(ARRAYSIZE(rgColors), rgColors, &cColorsCopied);
    }
    if (SUCCEEDED(hr))
    {
        // Check whether background color is outside range 
        hr = (backgroundIndex >= cColorsCopied) ? E_FAIL : S_OK;
    }
    if (SUCCEEDED(hr))
    {
        // Get the color in ARGB format
        dwBGColor = rgColors[backgroundIndex];

        // The background color is in ARGB format, and we want to 
        // extract the alpha value and convert it in FLOAT
        FLOAT alpha = (dwBGColor >> 24) / 255.f;
        *bc = D2D1::ColorF(dwBGColor, alpha);
    }

    pWicPalette->Release();
    pWicPalette = NULL;
    return hr;
}

/******************************************************************
*                                                                 *
*  GetAnimationMetaData()                                         *
*                                                                 *
*  Retrieves global metadata which pertains to the entire image.  *
*                                                                 *
******************************************************************/

static Animation GetAnimationMetaData(IWICBitmapDecoder* de)
{
    UINT frames;
    PROPVARIANT propValue;
    PropVariantInit(&propValue);
    IWICMetadataQueryReader *pMetadataQueryReader = NULL;
    HRESULT hr = S_OK;

    hr = de->GetFrameCount(&frames);
    if(1 == frames) return NULL;
    Animation am = (Animation)palloc0(sizeof(AnimationData));
    if(NULL == am) return NULL;
    am->frameCount = frames;

    // Create a MetadataQueryReader from the decoder
    hr = de->GetMetadataQueryReader(&pMetadataQueryReader);
    if(FAILED(hr)) return NULL;

    // Get background color
    if(FAILED(GetBackgroundColor(pMetadataQueryReader, de, &(am->color_bkg))))
    {
        // Default to transparent if failed to get the color
        am->color_bkg = D2D1::ColorF(0, 0.f);
    }

    // Get global frame size
    if (SUCCEEDED(hr))
    {
        // Get width
        hr = pMetadataQueryReader->GetMetadataByName(L"/logscrdesc/Width", &propValue);
        if (SUCCEEDED(hr))
        {
            hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
            if (SUCCEEDED(hr))
            {
                am->cxImage = propValue.uiVal;
            }
            PropVariantClear(&propValue);
        }
    }

    if (SUCCEEDED(hr))
    {
        // Get height
        hr = pMetadataQueryReader->GetMetadataByName(L"/logscrdesc/Height", &propValue);
        if (SUCCEEDED(hr))
        {
            hr = (propValue.vt == VT_UI2 ? S_OK : E_FAIL);
            if (SUCCEEDED(hr))
            {
                am->cyImage = propValue.uiVal;
            }
            PropVariantClear(&propValue);
        }
    }

    if (SUCCEEDED(hr))
    {
        // Get pixel aspect ratio
        hr = pMetadataQueryReader->GetMetadataByName(L"/logscrdesc/PixelAspectRatio", &propValue);
        if (SUCCEEDED(hr))
        {
            hr = (propValue.vt == VT_UI1 ? S_OK : E_FAIL);
            if (SUCCEEDED(hr))
            {
                UINT uPixelAspRatio = propValue.bVal;
                if (uPixelAspRatio != 0)
                {
                    // Need to calculate the ratio. The value in uPixelAspRatio 
                    // allows specifying widest pixel 4:1 to the tallest pixel of 
                    // 1:4 in increments of 1/64th
                    FLOAT pixelAspRatio = (uPixelAspRatio + 15.f) / 64.f;

                    // Calculate the image width and height in pixel based on the
                    // pixel aspect ratio. Only shrink the image.
                    if (pixelAspRatio > 1.f)
                    {
                        am->cxImagePixel = am->cxImage;
                        am->cyImagePixel = static_cast<UINT>(am->cyImage / pixelAspRatio);
                    }
                    else
                    {
                        am->cxImagePixel = static_cast<UINT>(am->cxImage * pixelAspRatio);
                        am->cyImagePixel = am->cyImage;
                    }
                }
                else
                {
                    // The value is 0, so its ratio is 1
                    am->cxImagePixel = am->cxImage;
                    am->cyImagePixel = am->cyImage;
                }
            }
            PropVariantClear(&propValue);
        }
    }

    // Get looping information
    if (SUCCEEDED(hr))
    {
        // First check to see if the application block in the Application Extension
        // contains "NETSCAPE2.0" and "ANIMEXTS1.0", which indicates the gif animation
        // has looping information associated with it.
        // 
        // If we fail to get the looping information, loop the animation infinitely.
        if (SUCCEEDED(pMetadataQueryReader->GetMetadataByName(L"/appext/application", &propValue)) &&
                propValue.vt == (VT_UI1 | VT_VECTOR) &&
                propValue.caub.cElems == 11 &&  // Length of the application block
                (!memcmp(propValue.caub.pElems, "NETSCAPE2.0", propValue.caub.cElems) ||
                !memcmp(propValue.caub.pElems, "ANIMEXTS1.0", propValue.caub.cElems)))
        {
            PropVariantClear(&propValue);

            hr = pMetadataQueryReader->GetMetadataByName(L"/appext/data", &propValue);
            if (SUCCEEDED(hr))
            {
                //  The data is in the following format:
                //  byte 0: extsize (must be > 1)
                //  byte 1: loopType (1 == animated gif)
                //  byte 2: loop count (least significant byte)
                //  byte 3: loop count (most significant byte)
                //  byte 4: set to zero
                if (propValue.vt == (VT_UI1 | VT_VECTOR) &&
                    propValue.caub.cElems >= 4 &&
                    propValue.caub.pElems[0] > 0 &&
                    propValue.caub.pElems[1] == 1)
                {
                    am->totalLoopCount = MAKEWORD(propValue.caub.pElems[2], propValue.caub.pElems[3]);
                    
                    // If the total loop count is not zero, we then have a loop count
                    // If it is 0, then we repeat infinitely
                    if (am->totalLoopCount != 0) 
                    {
                        am->hasLoop = TRUE;
                    }
                }
            }
        }
    }

    PropVariantClear(&propValue);
    pMetadataQueryReader->Release();
    pMetadataQueryReader = NULL;
    return am;
}

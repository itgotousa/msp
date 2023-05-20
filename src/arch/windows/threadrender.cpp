#include "stdafx.h"
#include "pgcore.h"
#include "svg.h"
#include "resource.h"
#include "mspwin.h"
#include "lunasvg.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace lunasvg;

static HRESULT _create_device_independance_D2D(D2DRenderNode n)
{
    HRESULT hr = E_FAIL;

    if (NULL != n)
    {
        hr = d2d.pIWICFactory->CreateStream(&(n->pStream));
        if (FAILED(hr) || NULL == n->pStream) return hr;

        hr = n->pStream->InitializeFromMemory((WICInProcPointer)(n->std.data), n->std.length);
        if (FAILED(hr))
        {
            SAFERELEASE(n->pStream);
            return hr;
        }

        hr = d2d.pIWICFactory->CreateDecoderFromStream(n->pStream, NULL, WICDecodeMetadataCacheOnLoad, &(n->pDecoder));
        if (FAILED(hr) || NULL == n->pDecoder)
        {
            SAFERELEASE(n->pStream);
            return hr;
        }
        hr = n->pDecoder->GetFrame(0, &(n->pFrame));
        if (FAILED(hr) || NULL == n->pFrame)
        {
            SAFERELEASE(n->pStream);
            SAFERELEASE(n->pDecoder);
            return hr;
        }
        hr = d2d.pIWICFactory->CreateFormatConverter(&(n->pConverter));
        if (FAILED(hr) || NULL == n->pConverter)
        {
            SAFERELEASE(n->pStream);
            SAFERELEASE(n->pDecoder);
            SAFERELEASE(n->pFrame);
            return hr;
        }
        hr = n->pConverter->Initialize(n->pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
            NULL, 0.0, WICBitmapPaletteTypeMedianCut);

        if (FAILED(hr))
        {
            SAFERELEASE(n->pStream);
            SAFERELEASE(n->pDecoder);
            SAFERELEASE(n->pFrame);
            SAFERELEASE(n->pConverter);
            return hr;
        }
    }
    return hr;
}

static D2DRenderNode _read_pic_file(TCHAR* path)
{
	int             fd;
    MemoryContext   mcxt = NULL;
    D2DRenderNode   n = NULL;
	unsigned char  *p;
    unsigned char   png_header[24] = { 0 };
	unsigned int    size, bytes, w = 0, h = 0;
    fileType ft = fileUnKnown;
    HRESULT hr = S_OK;

    if(NULL == path) return NULL;
	if (0 != _tsopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0)) return NULL;

	size = (unsigned int)_lseek(fd, 0, SEEK_END); /* get the file size */
	if (size > MAX_BUF_LEN || size < 24) 
	{
	    _close(fd); 
		return NULL;
	}

    _lseek(fd, 0, SEEK_SET); /* go to the begin of the file */
    
    bytes = (unsigned int)_read(fd, png_header, 24);  /* try to detect PNG header */
    if(24 != bytes) 
    {
	    _close(fd); 
		return NULL;
    }
    /* check PNG magic number: 89 50 4e 47 0d 0a 1a 0a */
    p = (unsigned char*)png_header;
    if((0x89 == p[0]) && (0x50 == p[1]) && (0x4e == p[2]) && (0x47 == p[3]) && 
        (0x0d == p[4]) && (0x0a == p[5]) && (0x1a == p[6]) && (0x0a == p[7]))
    {
        if ('I' == p[12] && 'H' == p[13] && 'D' == p[14] && 'R' == p[15])
        {
            ft = filePNG;
            w = p[19] + (p[18] << 8) + (p[17] << 16) + (p[16] << 24);
            h = p[23] + (p[22] << 8) + (p[21] << 16) + (p[20] << 24);
        }
    }
#if 0
    if((0xff == p[0]) && (0xd8 == p[1])) /* JPG head magic */
    {
        ft = fileJPG;
    }


    if((0x42 == p[0]) && (0x4d == p[1])) /* BMP head magic */
    {
        bytes = *((unsigned int*)(p+2));
        if(size == bytes) ft = fileBMP;
    }

    if((0x47 == p[0]) && (0x49 == p[1]) && (0x46 == p[2]) && (0x38 == p[3]) && 
        (0x39 == p[4]) && (0x61 == p[5]))  /* GIF head magic */
    {
        ft = fileGIF;
    }
#endif 
	_lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    //if(filePNG != ft && fileJPG != ft && fileBMP != ft && fileGIF != ft)
    if (filePNG != ft)
    {
	    _close(fd); 
		return NULL;
    }

    mcxt = AllocSetContextCreate(TopMemoryContext, "ImageCxt", ALLOCSET_DEFAULT_SIZES);
    if(NULL == mcxt)
    {
        _close(fd); 
        return NULL;
    }
    MemoryContextSwitchTo(mcxt);
    
    p = (unsigned char*)palloc(size);
    if(NULL == p)
    {
        MemoryContextDelete(mcxt);
        _close(fd); 
        return NULL;
    }

    bytes = (unsigned int)_read(fd, p, size); /* read the entire picture file into the buffer p */
    if (bytes != size) /* read error, since bytes != size */
    {
        MemoryContextDelete(mcxt);
        _close(fd); 
        return NULL;
    }

    _close(fd);

#if 0
    if(fileJPG == ft) /* we still need to check the last two bytes */
    {
        if((0xff != p[size-2]) || (0xd9 != p[size-1]))
        {
            MemoryContextDelete(mcxt);
            _close(fd); 
            return NULL;
        }
    }
#endif
    n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
    if(NULL == n)
    {
        MemoryContextDelete(mcxt);
        return NULL;
    }

    n->std.flag     = SO_TYPE_IMAGE;
    n->std.next     = NULL;
    n->std.data	    = p;
    n->std.length   = size;
    n->std.width    = w;
    n->std.height   = h;

    hr = _create_device_independance_D2D(n);
    if (FAILED(hr))
    {
        MemoryContextDelete(mcxt);
        return NULL;
    }

    return n;
}

void render_svg_logo(const char* logoSVG)
{
    d2d.pDataDefault = NULL;
#if 0    
    plutovg_surface_t* surface;
    
    size_t size = strlen(logoSVG);

    surface = plutosvg_load_from_memory(logoSVG, size, NULL, 0, 0, 96.0);
    if(NULL == surface) return;

    D2DRenderNode n = _compose_png_data(surface);

    plutovg_surface_destroy(surface);

    if(NULL == n) return;
    d2d.ft = filePNG;
    d2d.pDataDefault = n;
   
    ReleaseD2DResource(d2d.pData);
#endif 

}

#if 0
static HRESULT GetBackgroundColor(IWICMetadataQueryReader*, 
                        IWICBitmapDecoder*, D2D1_COLOR_F*);
static Animation GetAnimationMetaData(IWICBitmapDecoder*);
#endif

static int utf82unicode(unsigned char* input, int length, LPWSTR output, int max)
{
    wmemset(output, 0, max);
    return MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)input, length, output, max);
}

unsigned WINAPI open_mspfile_thread(LPVOID lpData)
{
    int   fd;
    BOOL  isOpened = FALSE;
    unsigned char *p;
    unsigned char* q;
    unsigned char  png_magic[8] = { 0 };
    unsigned int size, bytes, i;
    fileType ft = fileUnKnown;
    WPARAM wp = 0;
    LPARAM lp = 0;
    MemoryContext mcxt = NULL;
    D2DRenderNode n, m;
    HRESULT hr = S_OK;
    ThreadParam* tp = (ThreadParam*)lpData;

    if(NULL == tp) return 0;

    HWND hWndUI = tp->hWnd;
    ATLASSERT(::IsWindow(hWndUI));

    n = _read_pic_file(tp->pfilePath);

    if(NULL != n) /* it is one PNG or JPG/GIF file */
    {
        d2d.ft = filePNG;
        m = d2d.pData0;

        EnterCriticalSection(&(d2d.cs));
            d2d.pData0 = n;
        LeaveCriticalSection(&(d2d.cs));
        wp = UI_NOTIFY_FILEOPEN;
        lp = (LPARAM)filePNG;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);

        ReleaseD2DResource(m);

        return 0;
    }

    /* now check if it is a md or svg file */
    if (0 != _tsopen_s(&fd, tp->pfilePath, _O_RDONLY | _O_TEXT, _SH_DENYWR, 0))
    {
        wp = UI_NOTIFY_FILEFAIL;
        lp = 1;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);
	    return 0;
    }
	
    isOpened = TRUE;

    size = (unsigned int)_lseek(fd, 0, SEEK_END); /* get the file size */
    if (size > MAX_BUF_LEN || 0 == size) /* even it is one byte, it is still a valid md file */
    {
        wp = UI_NOTIFY_FILEFAIL;
        lp = 2;
        goto Quit_open_mspfile_thread;
    }
    _lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    p = (unsigned char*)malloc(size);
    if(NULL == p)
    {
        wp = UI_NOTIFY_FILEFAIL;
        lp = 6;
        goto Quit_open_mspfile_thread;
    }

    bytes = (unsigned int)_read(fd, p, size); /* read the entire PNG file into the buffer */

    if (bytes > size) /* bytes should be <= size */
    {
        wp = UI_NOTIFY_FILEFAIL;
        lp = 6;
        goto Quit_open_mspfile_thread;
    }

    /* scan the buffer to decide the file type */
    i = 0; q = p;
    while(i < bytes) /* skip the space characters */
    {
        if (0x20 != *q && '\t' != *q && '\r' != *q && '\n' != *q) break;
        q++; i++; 
    }
    q = p + i;
    if(i < bytes - 7)  /* <svg></svg> ||  <?xml ?> */
    {
        if(('<' == q[0]) && ('s' == q[1]) && ('v' == q[2]) && ('g' == q[3])) ft = fileSVG;
        else if(('<' == q[0]) && ('?' == q[1]) && ('x' == q[2]) && ('m' == q[3]) && ('l' == q[4])) ft =fileSVG;

        if(fileSVG == ft) goto handle_svg;
    }

    unsigned int charlen, k;
    while(i < bytes && *q) /* get all UTF-8 characters until we meed a none-UTF8 chararcter */
    {
        if (0 == (0x80 & *q))           charlen = 1;  /* 1-byte character */
        else if (0xE0 == (0xF0 & *q))   charlen = 3;  /* 3-byte character */
        else if (0xC0 == (0xE0 & *q))   charlen = 2;  /* 2-byte character */
        else if (0xF0 == (0xF8 & *q))   charlen = 4;  /* 4-byte character */
        else goto handle_markdown;  /* it is not UTF-8 character anymore */

        if (i > bytes - charlen) { goto handle_markdown; }
        for (k = 1; k < charlen; k++)
        {
            if (0x80 != (0xC0 & *(q + k))) { goto handle_markdown; }
        }
        q += charlen; i += charlen;
    }

handle_markdown:
    if(i > 0) ft = fileMD; /* any legal UTF8 encoding stream is a legal markdown file */
    if (fileMD == ft)
    {
#if 0
        q = (unsigned char*)palloc(i + 1);
        if (NULL == q)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }
        memcpy(q, p, i);
        q[i] = 0;
#endif
        mcxt = AllocSetContextCreate(TopMemoryContext, "Markdown-Cxt", ALLOCSET_DEFAULT_SIZES);
        if (NULL == mcxt) 
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }

        MemoryContextSwitchTo(mcxt);
        
        uint32_t wlen = sizeof(WCHAR) * (i + 1);
        WCHAR* w = (WCHAR*)palloc0(wlen);
        MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)p, i, w, wlen-2);
        UINT32 len = (UINT32)wcsnlen_s(w, wlen);

        free(p); p = NULL;
        _close(fd); isOpened = FALSE;

        n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
        n->std.flag = SO_TYPE_TEXT;
        n->std.next = NULL;
        n->std.data = w;
        n->std.length = len;
        hr = d2d.pDWriteFactory->CreateTextLayout(w, len, d2d.pTextFormat, tm.width, 1, &(n->pTextLayout));
        
        if (FAILED(hr))
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }

        DWRITE_TEXT_RANGE tr = { 0 };
        tr.startPosition = 2; tr.length = 2;
        n->pTextLayout->SetFontSize(36, tr);

        tr.startPosition = 8; tr.length = 2;
        n->pTextLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, tr);

        tr.startPosition = 12; tr.length = 4;
        n->pTextLayout->SetUnderline(TRUE, tr);

        tr.startPosition = 18; tr.length = 5;
        n->pTextLayout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, tr);

        tr.startPosition = 22; tr.length = 10;
        n->pTextLayout->SetStrikethrough(TRUE, tr);

        DWRITE_TEXT_METRICS textMetrics;
        HRESULT hr = n->pTextLayout->GetMetrics(&textMetrics);
        if (SUCCEEDED(hr))
        {
            n->std.width = std::max(textMetrics.layoutWidth, textMetrics.left + textMetrics.width);
            n->std.height = std::max(textMetrics.layoutHeight, textMetrics.height);
        }

        d2d.ft = fileMD;
        m = d2d.pData0;

        EnterCriticalSection(&(d2d.cs));
            d2d.pData0 = n;
        LeaveCriticalSection(&(d2d.cs));

        wp = UI_NOTIFY_FILEOPEN;
        lp = (LPARAM)fileMD;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);

        ReleaseD2DResource(m);
        return 0;
    }

handle_svg:
    if(fileSVG == ft)
    {
        std::uint32_t width = 0, height = 0;
        std::uint32_t bgColor = 0x00000000;
        auto document = Document::loadFromData((const char*)p, (std::size_t)bytes);
        if(!document)
        {
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }

        free(p); p = NULL;
        _close(fd); isOpened = FALSE;

        auto bitmap = document->renderToBitmap(width, height, bgColor);
        if(!bitmap.valid())
        {
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }

        bitmap.convertToRGBA();

        mcxt = AllocSetContextCreate(TopMemoryContext, "SVG2PNG-Cxt", ALLOCSET_DEFAULT_SIZES);
        if(NULL == mcxt) return NULL;
        MemoryContextSwitchTo(mcxt);
        n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
        n->std.flag     = SO_TYPE_IMAGE;
        n->std.next     = NULL;
        n->std.width    = int(bitmap.width());
        n->std.height   = int(bitmap.height());

        int len;
        unsigned char *png = stbi_write_png_to_mem(bitmap.data(), 0, n->std.width, n->std.height, 4, &len);
        if(NULL == png)
        {
            MemoryContextDelete(mcxt);        
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }
        n->std.length  = len;
        n->std.data = (unsigned char* )palloc(len);
        if(NULL == n->std.data)
        {
            free(png);
            MemoryContextDelete(mcxt);        
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }
        memcpy(n->std.data, png, len);
        free(png);

        HRESULT hr = _create_device_independance_D2D(n);
        if (FAILED(hr))
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }

        d2d.ft = filePNG;
        m = d2d.pData0;

        EnterCriticalSection(&(d2d.cs));
            d2d.pData0 = n;
        LeaveCriticalSection(&(d2d.cs));
        wp = UI_NOTIFY_FILEOPEN;
        lp = (LPARAM)fileBMP;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);

        ReleaseD2DResource(m);
        return 0;
    }

Quit_open_mspfile_thread:
    if(NULL != p) { free(p); p = NULL; }
    if(isOpened) { _close(fd); isOpened = FALSE; }
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
#if 0
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
#endif 

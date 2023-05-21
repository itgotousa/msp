#include "stdafx.h"
#include "pgcore.h"
#include "msp.h"
#include "resource.h"
#include "mspwin.h"
#include "libnsgif.h"
#include "spng.h"
#include "lunasvg.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace lunasvg;

#define ALLOCSET_IMAGE_MINSIZE   0
#define ALLOCSET_IMAGE_INITSIZE  (8 * 1024)
#define ALLOCSET_IMAGE_MAXSIZE   (32 * 1024 * 1024)

static HRESULT GetBackgroundColor(IWICMetadataQueryReader*,
    IWICBitmapDecoder*, D2D1_COLOR_F*);
static BOOL GetAnimationMetaData(D2DRenderNode n);

static D2DRenderNode _decode_png_data(unsigned char* buf, unsigned int length, unsigned int w, unsigned int h)
{
    D2DRenderNode n = NULL;

    size_t out_size = 0;
    unsigned char* p = NULL;

    spng_ctx* ctx = spng_ctx_new(0);
    if (NULL == ctx) return NULL;
    
    if (0 != spng_set_png_buffer(ctx, buf, length))
    {
        spng_ctx_free(ctx);
        return NULL;
    }

    if(0 != spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &out_size))
    {
        spng_ctx_free(ctx);
        return NULL;
    }

    MemoryContext mcxt = AllocSetContextCreate(TopMemoryContext, "Image-PNG-Cxt", 0, MAXALIGN(out_size + 1024), ALLOCSET_IMAGE_MAXSIZE);
    if (NULL == mcxt)
    {
        spng_ctx_free(ctx);
        return NULL;
    }

    MemoryContextSwitchTo(mcxt);
    p = (unsigned char*)palloc(out_size);
    if (NULL == p)
    {
        spng_ctx_free(ctx);
        MemoryContextDelete(mcxt);
        return NULL;
    }

    if (0 != spng_decode_image(ctx, p, out_size, SPNG_FMT_RGBA8, 0))
    {
        spng_ctx_free(ctx);
        MemoryContextDelete(mcxt);
        return NULL;
    }
    spng_ctx_free(ctx);

    n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
    if (NULL == n)
    {
        spng_ctx_free(ctx);
        MemoryContextDelete(mcxt);
        return NULL;
    }

    n->std.type = MSP_TYPE_IMAGE;
    n->std.next = NULL;
    n->std.data = p;
    n->std.length = out_size;
    n->std.width = w;
    n->std.height = h;

    n->std.flag |= MSP_HINT_PNG;
    return n;
}

#if 1
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

        GetAnimationMetaData(n);
#if 0
        n->am = GetAnimationMetaData(n->pDecoder);
        n->am.frameCount = 1;
        hr = n->pDecoder->GetFrameCount(&(n->am.frameCount));
        if (FAILED(hr)) { n->am.frameCount = 1; }
#endif
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
#endif 

#define PIC_HEADER_SIZE     32

static D2DRenderNode _read_pic_file(TCHAR* path)
{
	int             fd;
    D2DRenderNode   n = NULL;
	unsigned char  *p;
    unsigned char   pic_header[PIC_HEADER_SIZE] = { 0 };
	unsigned int    size, bytes, w = 0, h = 0;
    fileType ft = fileUnKnown;
    HRESULT hr = S_OK;

    if(NULL == path) return NULL;
	if (0 != _tsopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0)) return NULL;

	size = (unsigned int)_lseek(fd, 0, SEEK_END); /* get the file size */
	if (size > MAX_BUF_LEN || size < 32) 
	{
	    _close(fd); 
		return NULL;
	}

    _lseek(fd, 0, SEEK_SET); /* go to the begin of the file */
    
    bytes = (unsigned int)_read(fd, pic_header, PIC_HEADER_SIZE);  /* try to detect PNG header */
    if(PIC_HEADER_SIZE != bytes)
    {
	    _close(fd); 
		return NULL;
    }
    /* check PNG magic number: 89 50 4e 47 0d 0a 1a 0a */
    p = (unsigned char*)pic_header;
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
        w = p[18] + (p[19] << 8) + (p[20] << 16) + (p[21] << 24);
        h = p[22] + (p[23] << 8) + (p[24] << 16) + (p[25] << 24);
    }

    if((0x47 == p[0]) && (0x49 == p[1]) && (0x46 == p[2]) && (0x38 == p[3]) && 
        (0x39 == p[4]) && (0x61 == p[5]))  /* GIF head magic */
    {
        ft = fileGIF;
        w = p[6] + (p[7] << 8);
        h = p[8] + (p[9] << 8);
    }
#endif 
	_lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    if(filePNG != ft && fileJPG != ft && fileBMP != ft && fileGIF != ft)
    {
	    _close(fd); 
		return NULL;
    }

    p = (unsigned char*)malloc(size);
    if (NULL == p)
    {
        _close(fd);
        return NULL;
    }
    bytes = (unsigned int)_read(fd, p, size); /* read the entire picture file into the buffer p */
    if (bytes != size) /* read error, since bytes != size */
    {
        _close(fd);
        return NULL;
    }

    if (filePNG == ft)
    {
        n = _decode_png_data(p, bytes, w, h);
        if (NULL == n)
        {
            free(p);
            _close(fd);
            return NULL;
        }
    }

    free(p);
    _close(fd);
    return n;

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
}

void render_svg_logo(const char* logoSVG)
{
    d2d.pDataDefault = NULL;
  
    size_t size = strlen(logoSVG);

    std::uint32_t width = 0, height = 0;
    std::uint32_t bgColor = 0x00000000;
    auto document = Document::loadFromData((const char*)logoSVG, (std::size_t)size);
    if (!document) return;

    auto bitmap = document->renderToBitmap(width, height, bgColor);
    if (bitmap.valid())
    {
        bitmap.convertToRGBA();

        MemoryContext mcxt = AllocSetContextCreate(TopMemoryContext, "SVGLogo-Cxt", ALLOCSET_DEFAULT_SIZES);
        if (NULL != mcxt)
        {
            MemoryContextSwitchTo(mcxt);
            D2DRenderNode n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
            n->std.type = MSP_TYPE_IMAGE;
            n->std.next = NULL;
            n->std.width = int(bitmap.width());
            n->std.height = int(bitmap.height());

            int len;
            unsigned char* png = stbi_write_png_to_mem(bitmap.data(), 0, n->std.width, n->std.height, 4, &len);
            if (NULL != png)
            {
                n->std.flag |= MSP_HINT_PNG;
                n->std.length = len;
                n->std.data = (unsigned char*)palloc(len);
                if (NULL != n->std.data)
                {
                    memcpy(n->std.data, png, len);
                    HRESULT hr = _create_device_independance_D2D(n);
                    if (SUCCEEDED(hr))
                    {
                        d2d.pDataDefault = n;
                    }
                    else
                    {
                        MemoryContextDelete(mcxt);
                        d2d.pDataDefault = NULL;
                    }
                }
                free(png);
            }
            else
            {
                MemoryContextDelete(mcxt);
                d2d.pDataDefault = NULL;
            }
        }
    }
}


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
        n->std.type = MSP_TYPE_TEXT;
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
        n->std.type     = MSP_TYPE_IMAGE;
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

//                           Gif Animation Overview
// In order to play a gif animation, raw frames (which are compressed frames 
// directly retrieved from the image file) and image metadata are loaded 
// and used to compose the frames that are actually displayed in the animation 
// loop (which we call composed frames in this sample).  Composed frames have 
// the same sizes as the global gif image size, while raw frames can have their own sizes.
//
// At the highest level, a gif animation contains a fixed or infinite number of animation
// loops, in which the animation will be displayed repeatedly frame by frame; once all 
// loops are displayed, the animation will stop and the last frame will be displayed 
// from that point.
//
// In each loop, first the entire composed frame will be initialized with the background 
// color retrieved from the image metadata.  The very first raw frame then will be loaded 
// and directly overlaid onto the previous composed frame (i.e. in this case, the frame 
// cleared with background color) to produce the first  composed frame, and this frame 
// will then be displayed for a period that equals its delay.  For any raw frame after 
// the first raw frame (if there are any), the composed frame will first be disposed based 
// on the disposal method associated with the previous raw frame. Then the next raw frame 
// will be loaded and overlaid onto the result (i.e. the composed frame after disposal).  
// These two steps (i.e. disposing the previous frame and overlaying the current frame) together 
// 'compose' the next frame to be displayed.  The composed frame then gets displayed.  
// This process continues until the last frame in a loop is reached.
//
// An exception is the zero delay intermediate frames, which are frames with 0 delay 
// associated with them.  These frames will be used to compose the next frame, but the 
// difference is that the composed frame will not be displayed unless it's the last frame 
// in the loop (i.e. we move immediately to composing the next composed frame).
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

    SAFERELEASE(pWicPalette);
    return hr;
}

/******************************************************************
*                                                                 *
*  GetAnimationMetaData()                                         *
*                                                                 *
*  Retrieves global metadata which pertains to the entire image.  *
*                                                                 *
******************************************************************/

static BOOL GetAnimationMetaData(D2DRenderNode n)
{
    UINT frames;
    PROPVARIANT propValue;
    PropVariantInit(&propValue);
    IWICMetadataQueryReader *pMetadataQueryReader = NULL;
    HRESULT hr = S_OK;

    IWICBitmapDecoder* de = n->pDecoder;
    if (NULL == de) return FALSE;
    
    hr = de->GetFrameCount(&frames);
    if (FAILED(hr)) return FALSE;

    if(1 == frames) return FALSE;
    Animation am = &(n->am);

    am->frameCount = frames;

    // Create a MetadataQueryReader from the decoder
    hr = de->GetMetadataQueryReader(&pMetadataQueryReader);
    if (FAILED(hr))
    {
        am->frameCount = 1;
        return FALSE;
    }

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
    SAFERELEASE(pMetadataQueryReader);
    return TRUE;
}


#include "stdafx.h"
#include "pgcore.h"
#include "msp.h"
#include "resource.h"
#include "mspwin.h"
#include "libnsgif.h"
#include "unicode.h"
#include "font.h"
#include "spng.h"
#include "lunasvg.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace lunasvg;

#define ALLOCSET_IMAGE_MINSIZE   0
#define ALLOCSET_IMAGE_INITSIZE  (8 * 1024)
#define ALLOCSET_IMAGE_MAXSIZE   (32 * 1024 * 1024)

#define BYTES_PER_PIXEL 4
#define MAX_IMAGE_BYTES (48 * 1024 * 1024)

static void* bitmap_create(int width, int height)
{
    /* ensure a stupidly large bitmap is not created */
    if (((long long)width * (long long)height) > (MAX_IMAGE_BYTES / BYTES_PER_PIXEL)) {
        return NULL;
    }
    return calloc(width * height, BYTES_PER_PIXEL);
}


static void bitmap_set_opaque(void* bitmap, bool opaque)
{
    (void)opaque;  /* unused */
    (void)bitmap;  /* unused */
    //assert(bitmap);
}

static bool bitmap_test_opaque(void* bitmap)
{
    (void)bitmap;  /* unused */
    //assert(bitmap);
    return false;
}

static unsigned char* bitmap_get_buffer(void* bitmap)
{
    //assert(bitmap);
    return (unsigned char*)bitmap;
}

static void bitmap_destroy(void* bitmap)
{
    //assert(bitmap);
    free(bitmap);
}

static void bitmap_modified(void* bitmap)
{
    (void)bitmap;  /* unused */
    //assert(bitmap);
    return;
}

static D2DRenderNode _decode_gif_data(unsigned char* data, unsigned int length, unsigned int w, unsigned int h)
{
    D2DRenderNode n = NULL;
    gif_bitmap_callback_vt bitmap_callbacks = {
            bitmap_create,
            bitmap_destroy,
            bitmap_get_buffer,
            bitmap_set_opaque,
            bitmap_test_opaque,
            bitmap_modified
    };
    gif_animation gif;
    gif_result code;

    /* create our gif animation */
    gif_create(&gif, &bitmap_callbacks);

    /* begin decoding */
    do 
    {
        code = gif_initialise(&gif, length, data);
        if (code != GIF_OK && code != GIF_WORKING) 
        {
            gif_finalise(&gif);
            return NULL;
        }
    } while (code != GIF_OK);

    code = gif_decode_frame(&gif, 0);
    if (GIF_OK == code)
    {
        unsigned int out_size = gif.height * gif.width * 4;
        MemoryContext mcxt = AllocSetContextCreate(TopMemoryContext, "Image-GIF-Cxt", 0, MAXALIGN(out_size + 1024), ALLOCSET_IMAGE_MAXSIZE);
        if (NULL == mcxt)
        {
            gif_finalise(&gif);
            return NULL;
        }
        //image = (unsigned char*)gif.frame_image;
        MemoryContextSwitchTo(mcxt);
        unsigned char* p = (unsigned char*)palloc(out_size);
        if(NULL == p)
        {
            gif_finalise(&gif);
            MemoryContextDelete(mcxt);
            return NULL;
        }
        n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
        if (NULL == n)
        {
            gif_finalise(&gif);
            MemoryContextDelete(mcxt);
            return NULL;
        }
        memcpy(p, (unsigned char*)gif.frame_image, out_size);
        n->std.type = MSP_TYPE_IMAGE;
        n->std.next = NULL;
        n->std.image = p;
        n->std.image_length = out_size;
        n->std.width = w;
        n->std.height = h;
        n->std.flag |= MSP_HINT_GIF;
    }
    /* clean up */
    gif_finalise(&gif);

    return n;
}

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
    n->std.image = p;
    n->std.image_length = out_size;
    n->std.width = w;
    n->std.height = h;

    n->std.flag |= MSP_HINT_PNG;
    return n;
}

#define PIC_HEADER_SIZE     32

static D2DRenderNode _read_pic_file(TCHAR* path)
{
	int             fd;
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

    if((0x47 == p[0]) && (0x49 == p[1]) && (0x46 == p[2]) && (0x38 == p[3]) && 
        (0x39 == p[4]) && (0x61 == p[5]))  /* GIF head magic */
    {
        ft = fileGIF;
        w = p[6] + (p[7] << 8);
        h = p[8] + (p[9] << 8);
    }

    _lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    if(filePNG != ft && fileGIF != ft)
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
    
    D2DRenderNode n = NULL;
    switch (ft)
    {
    case filePNG    :
        n = _decode_png_data(p, bytes, w, h);
        break;
    case fileGIF    :
        n = _decode_gif_data(p, bytes, w, h);
        break;
    default         :
        break;
    }

    free(p);
    _close(fd);
    return n;
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
            n->std.image_length = (n->std.width * n->std.height) << 2;
            n->std.image = palloc(n->std.image_length);
            if (NULL == n->std.image)
            {
                MemoryContextDelete(mcxt);
                return;
            }
            memcpy(n->std.image, bitmap.data(), n->std.image_length);
            d2d.pDataDefault = n;
        }
    }
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
#endif
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
        if(NULL == mcxt)
        {
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }

        MemoryContextSwitchTo(mcxt);

        n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
        if(NULL == n)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }
        n->std.type     = MSP_TYPE_IMAGE;
        n->std.next     = NULL;
        n->std.width    = int(bitmap.width());
        n->std.height   = int(bitmap.height());
        n->std.image_length   = (n->std.width * n->std.height) << 2;
        n->std.image     = palloc(n->std.image_length);
        if (NULL == n->std.image)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }
        memcpy(n->std.image, bitmap.data(), n->std.image_length);

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

Quit_open_mspfile_thread:
    if(NULL != p) { free(p); p = NULL; }
    if(isOpened) { _close(fd); isOpened = FALSE; }
    PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);

    return 0;
}


#include "stdafx.h"
#include "pgcore.h"
#include "msp.h"
#include "resource.h"
#include "mspwin.h"
#include "libnsgif.h"
#include "unicode.h"
#include "md.h"
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

static RenderRoot DecodeGifData(U8* data, U32 length, U32 w, U32 h)
{
    RenderRoot root = NULL;
   
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
        U8* p = (U8*)palloc(out_size);
        if(NULL == p)
        {
            gif_finalise(&gif);
            MemoryContextDelete(mcxt);
            return NULL;
        }

        root = (RenderRoot)palloc0(sizeof(RenderRootData));
        if (NULL == root)
        {
            gif_finalise(&gif);
            MemoryContextDelete(mcxt);
            return NULL;
        }
        D2DRenderNode node = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
        if (NULL == node)
        {
            gif_finalise(&gif);
            MemoryContextDelete(mcxt);
            return NULL;
        }
        memcpy(p, (unsigned char*)gif.frame_image, out_size);
        node->std.type = MSP_TYPE_IMAGE;
        node->std.next = NULL;
        node->std.image = p;
        node->std.image_length = out_size;
        node->std.width = w;
        node->std.height = h;
        node->std.flag |= MSP_HINT_GIF;

        root->count = 1;
        root->width = w;
        root->height = h;
        root->node = (RenderNode)node;
    }
    /* clean up */
    gif_finalise(&gif);

    return root;
}

static RenderRoot DecodePngData(U8* buf, U32 length, U32 w, U32 h)
{
    RenderRoot root = NULL;

    size_t out_size = 0;

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

    U8* p = (U8*)palloc(out_size);
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

    root = (RenderRoot)palloc0(sizeof(RenderRootData));
    if (NULL == root)
    {
        spng_ctx_free(ctx);
        MemoryContextDelete(mcxt);
        return NULL;
    }
    D2DRenderNode node = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
    if (NULL == node)
    {
        spng_ctx_free(ctx);
        MemoryContextDelete(mcxt);
        return NULL;
    }
    node->std.type = MSP_TYPE_IMAGE;
    node->std.next = NULL;
    node->std.image = p;
    node->std.image_length = out_size;
    node->std.width = w;
    node->std.height = h;
    node->std.flag |= MSP_HINT_PNG;

    root->count = 1;
    root->width = w;
    root->height = h;
    root->node = (RenderNode)node;

    return root;
}

#define PIC_HEADER_SIZE     32

static RenderRoot ReadPicFile(wchar_t* path)
{
	int     fd;
    U8      pic_header[PIC_HEADER_SIZE] = { 0 };
	U32     size, bytes, w = 0, h = 0;
    fileType ft = fileUnKnown;
    HRESULT hr = S_OK;

    if(NULL == path) return NULL;
	if (0 != _tsopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0)) return NULL;

	size = (unsigned int)_lseek(fd, 0, SEEK_END); /* get the file size */
	if (size > MAX_BUF_LEN || size < PIC_HEADER_SIZE)
	{
	    _close(fd); 
		return NULL;
	}
    _lseek(fd, 0, SEEK_SET); /* go to the begin of the file */
    
    bytes = (U32)_read(fd, pic_header, PIC_HEADER_SIZE);  /* try to detect PNG header */
    if(PIC_HEADER_SIZE != bytes)
    {
	    _close(fd); 
		return NULL;
    }
    /* check PNG magic number: 89 50 4e 47 0d 0a 1a 0a */
    U8* p = (U8*)pic_header;
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

    p = (U8*)malloc(size);
    if (NULL == p)
    {
        _close(fd);
        return NULL;
    }
    bytes = (U32)_read(fd, p, size); /* read the entire picture file into the buffer p */
    if (bytes != size) /* read error, since bytes != size */
    {
        free(p);
        _close(fd);
        return NULL;
    }
    _close(fd);

    RenderRoot root = NULL;
    switch (ft)
    {
    case filePNG    :
        root = DecodePngData(p, bytes, w, h);
        break;
    case fileGIF    :
        root = DecodeGifData(p, bytes, w, h);
        break;
    default         :
        break;
    }
    free(p);

    return root;
}

void render_svg_logo(const char* logoSVG)
{
    RenderRoot root = NULL;
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
            root = (RenderRoot)palloc0(sizeof(RenderRootData));
            if (NULL == root)
            {
                MemoryContextDelete(mcxt);
                return;
            }
            D2DRenderNode node = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
            if (NULL == node)
            {
                MemoryContextDelete(mcxt);
                return;
            }
            node->std.type = MSP_TYPE_IMAGE;
            node->std.next = NULL;
            node->std.width = int(bitmap.width());
            node->std.height = int(bitmap.height());
            node->std.image_length = (node->std.width * node->std.height) << 2;
            node->std.image = palloc(node->std.image_length);
            if (NULL == node->std.image)
            {
                MemoryContextDelete(mcxt);
                return;
            }
            memcpy(node->std.image, bitmap.data(), node->std.image_length);

            root->count = 1;
            root->width = node->std.width;
            root->height = node->std.height;
            root->node = (RenderNode)node;

            d2d.pDataDefault = root;
        }
    }
}

#define SVG_MINIMAL_SIZE     32
unsigned WINAPI open_mspfile_thread(LPVOID lpData)
{
    int         fd;
    BOOL        isOpened = FALSE;
    fileType    ft = fileUnKnown;
    WPARAM      wp = 0;
    LPARAM      lp = 0;
    RenderRoot  root;
    ThreadParam* tp = (ThreadParam*)lpData;

    if(NULL == tp) return 0;

    HWND hWndUI = tp->hWnd;
    ATLASSERT(::IsWindow(hWndUI));

    root = ReadPicFile(tp->pfilePath);
    RenderRoot old;
    if(NULL != root) /* it is one PNG or JPG/GIF file */
    {
        EnterCriticalSection(&(d2d.cs));
            old = d2d.pData0;
            d2d.pData0 = root;
        LeaveCriticalSection(&(d2d.cs));
        
        wp = UI_NOTIFY_FILEOPEN;
        lp = (LPARAM)filePNG;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);

        ReleaseD2DResource(old);
        return 0;
    }

    /* this file is not picture file, so now check if it is a md or svg file */
    if (0 != _tsopen_s(&fd, tp->pfilePath, _O_RDONLY | _O_TEXT, _SH_DENYWR, 0))
    {
        wp = UI_NOTIFY_FILEFAIL; lp = 1;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);
	    return 0;
    }
    isOpened = TRUE;

    U32 size = (U32)_lseek(fd, 0, SEEK_END); /* get the file size */
    if (size > MAX_BUF_LEN || 0 == size) /* even it is one byte, it is still a valid md file */
    {
        wp = UI_NOTIFY_FILEFAIL; lp = 1;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);
    }
    _lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    U8* textbuffer = (U8*)malloc(size);
    if(NULL == textbuffer)
    {
        wp = UI_NOTIFY_FILEFAIL; lp = 6;
        goto Quit_open_mspfile_thread;
    }
    /* read the entire file into the buffer */
    U32 bytes = (U32)_read(fd, textbuffer, size);
    if (bytes > size) /* bytes should be <= size */
    {
        wp = UI_NOTIFY_FILEFAIL; lp = 6;
        goto Quit_open_mspfile_thread;
    }
    _close(fd); isOpened = FALSE;

    /* scan the buffer to decide the file type */
    U32 i = 0; 
    U8* p = textbuffer;
    while(i < bytes) /* skip the leading space characters */
    {
        if (0x20 != *p && '\t' != *p && '\r' != *p && '\n' != *p) break;
        p++; i++; 
    }

    if(i < bytes - SVG_MINIMAL_SIZE)  /* <svg></svg> ||  <?xml ?> */
    {
        if(('<' == *p) && ('s' == *(p+1)) && ('v' == *(p+2)) && ('g' == *(p+3))) 
            ft = fileSVG;
        else if(('<' == *p) && ('?' == *(p+1)) && ('x' == *(p+2)) && ('m' == *(p+3)) && ('l' == *(p+4)))
            ft =fileSVG;

        if(fileSVG == ft) goto handle_svg;
    }
    
    U32 characters, surrogate, utf8bytes;
    /* extract the legal UTF8 string from textbuffer */
    utf8bytes = verify_utf8_string(textbuffer, bytes, &characters, &surrogate);

    /* any legal UTF8 encoding stream is a legal markdown file */
    if(utf8bytes > 0) 
    {
        ft = fileMD;
        /* parse the textbuffer to get the parsered tree */
        md_parse_buffer(textbuffer, utf8bytes);
        /* generate the render tree from the above parserd tree */
        MemoryContext mcxt = AllocSetContextCreate(TopMemoryContext, "Markdown-Cxt", ALLOCSET_DEFAULT_SIZES);
        if (NULL == mcxt) 
        {
            wp = UI_NOTIFY_FILEFAIL;  lp = 6;
            goto Quit_open_mspfile_thread;
        }

        MemoryContextSwitchTo(mcxt);
        U32 utf16_len = (characters << 1) + surrogate; /* in bytes */

        U16* utf16_buffer = (U16*)palloc0(utf16_len);
        if(NULL == utf16_buffer)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;  lp = 6;
            goto Quit_open_mspfile_thread;
        }

        U32 words = UTF8toUTF16(textbuffer, utf8bytes, utf16_buffer);
        //MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)p, i, w, wlen-2);
        //UINT32 len = (UINT32)wcsnlen_s(w, wlen);
        free(textbuffer); textbuffer = NULL;

        root = (RenderRoot)palloc0(sizeof(RenderRootData));
        if (NULL == root)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;  lp = 6;
            goto Quit_open_mspfile_thread;
        }
        D2DRenderNode node = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
        if (NULL == node)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;  lp = 6;
            goto Quit_open_mspfile_thread;
        }
        node->std.type = MSP_TYPE_TEXT;
        node->std.next = NULL;
        node->std.text = utf16_buffer;
        node->std.text_length = utf16_len;
        HRESULT hr = d2d.pDWriteFactory->CreateTextLayout((WCHAR*)utf16_buffer, (utf16_len>>1), d2d.pDefaultTextFormat, tm.width, 1, &(node->pTextLayout));
        if (FAILED(hr))
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;  lp = 6;
            goto Quit_open_mspfile_thread;
        }

        DWRITE_TEXT_RANGE tr = { 0 };
        tr.startPosition = 0; tr.length = 2;
        node->pTextLayout->SetFontSize(36, tr);
#if 0
        tr.startPosition = 8; tr.length = 2;
        n->pTextLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, tr);

        tr.startPosition = 12; tr.length = 4;
        n->pTextLayout->SetUnderline(TRUE, tr);

        tr.startPosition = 18; tr.length = 5;
        n->pTextLayout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, tr);

        tr.startPosition = 22; tr.length = 10;
        n->pTextLayout->SetStrikethrough(TRUE, tr);
#endif
        DWRITE_TEXT_METRICS textMetrics;
        hr = node->pTextLayout->GetMetrics(&textMetrics);
        if (SUCCEEDED(hr))
        {
            node->std.width = tm.width; // std::max(textMetrics.layoutWidth, textMetrics.left + textMetrics.width);
            node->std.height = std::max(textMetrics.layoutHeight, textMetrics.height);
        }
        root->count = 1;
        root->node = (RenderNode)node;
#if 0
        if (NULL != d2d.pDataDefault)
        {
            node->std.next = d2d.pDataDefault->node;

            if (NULL != d2d.pDataDefault->node)
            {
                D2DRenderNode m = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
                if (NULL != m)
                {
                    d2d.pDataDefault->node->next = (RenderNode)m;
                    m->std.type = MSP_TYPE_TEXT;
                    m->std.next = NULL;
                    m->std.text = node->std.text;
                    m->std.text_length = node->std.text_length;
                    hr = d2d.pDWriteFactory->CreateTextLayout((WCHAR*)(m->std.text), (m->std.text_length >> 1),
                        tm.pblockTextFormat, tm.width, 1, &(m->pTextLayout));
                    if (SUCCEEDED(hr))
                    {
                        hr = m->pTextLayout->GetMetrics(&textMetrics);
                        if (SUCCEEDED(hr))
                        {
                            m->std.width = std::max(textMetrics.layoutWidth, textMetrics.left + textMetrics.width);
                            m->std.height = std::max(textMetrics.layoutHeight, textMetrics.height);
                        }
                    }
                }
            }
        }
#endif
        RenderNode n = root->node;
        root->width = 0; root->height = 0;
        while (NULL != n)
        {
            n->top = tm.top_margin + root->height;
            if (n->width > root->width) root->width = n->width;
            root->height += n->height;
            n = n->next;
        }

        RenderRoot old;
        EnterCriticalSection(&(d2d.cs));
            old = d2d.pData0;
            d2d.pData0 = root;
        LeaveCriticalSection(&(d2d.cs));

        wp = UI_NOTIFY_FILEOPEN;
        lp = (LPARAM)fileMD;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);

        ReleaseD2DResource(old);

        return 0;
    }

handle_svg:
    if(fileSVG == ft)
    {
        std::uint32_t width = 0, height = 0;
        std::uint32_t bgColor = 0x00000000;
        auto document = Document::loadFromData((const char*)textbuffer, (std::size_t)bytes);
        if(!document)
        {
            wp = UI_NOTIFY_FILEFAIL; lp = 6;
            goto Quit_open_mspfile_thread;
        }
        free(textbuffer); textbuffer = NULL;

        auto bitmap = document->renderToBitmap(width, height, bgColor);
        if(!bitmap.valid())
        {
            wp = UI_NOTIFY_FILEFAIL; lp = 6;
            goto Quit_open_mspfile_thread;
        }
        bitmap.convertToRGBA();

        MemoryContext mcxt = AllocSetContextCreate(TopMemoryContext, "SVG2PNG-Cxt", ALLOCSET_DEFAULT_SIZES);
        if(NULL == mcxt)
        {
            wp = UI_NOTIFY_FILEFAIL; lp = 6;
            goto Quit_open_mspfile_thread;
        }
        MemoryContextSwitchTo(mcxt);

        root = (RenderRoot)palloc0(sizeof(RenderRootData));
        if (NULL == root)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;  lp = 6;
            goto Quit_open_mspfile_thread;
        }
        D2DRenderNode node = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
        if(NULL == node)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;  lp = 6;
            goto Quit_open_mspfile_thread;
        }
        node->std.type     = MSP_TYPE_IMAGE;
        node->std.next     = NULL;
        node->std.width    = int(bitmap.width());
        node->std.height   = int(bitmap.height());
        node->std.image_length   = (node->std.width * node->std.height) << 2;
        node->std.image     = palloc(node->std.image_length);
        if (NULL == node->std.image)
        {
            MemoryContextDelete(mcxt);
            wp = UI_NOTIFY_FILEFAIL;  lp = 6;
            goto Quit_open_mspfile_thread;
        }
        memcpy(node->std.image, bitmap.data(), node->std.image_length);

        root->count = 1;
        root->width = node->std.width;
        root->height = node->std.height;
        root->node = (RenderNode)node;

        RenderRoot old;
        EnterCriticalSection(&(d2d.cs));
            old = d2d.pData0;
            d2d.pData0 = root;
        LeaveCriticalSection(&(d2d.cs));

        wp = UI_NOTIFY_FILEOPEN;
        lp = (LPARAM)filePNG;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);

        ReleaseD2DResource(old);
        return 0;
    }

Quit_open_mspfile_thread:

    PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp); /* notify the UI thread */
    if(NULL != textbuffer)
    { 
        free(textbuffer); 
        textbuffer = NULL; 
    }
    if(isOpened) 
    { 
        _close(fd); 
        isOpened = FALSE; 
    }
    return 0;
}


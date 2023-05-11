#include "stdafx.h"
#include "pgcore.h"
#include "stb_image_write.h"
#include "plutovg-private.h"
//#include "plutovg.h"
#include "plutosvg.h"
#include "svg.h"
#include "resource.h"
#include "mspwin.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned char *stbi_write_png_to_mem(const unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);

#ifdef __cplusplus
}
#endif

static HRESULT _create_device_independance_D2D(D2DRenderNode n)
{
    HRESULT hr = E_FAIL;
    MemoryContext mcxt = NULL;

    if(NULL == n) return E_FAIL;
    mcxt = *(MemoryContext *) (((char *) n) - sizeof(void *)); 

    hr = d2d.pIWICFactory->CreateStream(&(n->pStream));
    if(FAILED(hr) || NULL == n->pStream)
    {
        MemoryContextDelete(mcxt);
        return hr;
    }
    hr = n->pStream->InitializeFromMemory((WICInProcPointer)(n->std.data), n->std.len);
    if(FAILED(hr))
    {
        n->pStream->Release();
        n->pStream = NULL;
        MemoryContextDelete(mcxt);
        return hr;
    }
    hr = d2d.pIWICFactory->CreateDecoderFromStream(n->pStream, NULL,
                            WICDecodeMetadataCacheOnLoad, &(n->pDecoder));

    if(FAILED(hr) || NULL == n->pDecoder)
    {
        n->pStream->Release();
        n->pStream = NULL;
        MemoryContextDelete(mcxt);
        return hr;
    }
    hr = n->pDecoder->GetFrame(0, &(n->pFrame));
    if(FAILED(hr) || NULL == n->pFrame)
    {
        n->pStream->Release();
        n->pStream = NULL;
        n->pDecoder->Release();
        n->pDecoder = NULL;
        MemoryContextDelete(mcxt);
        return hr;
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
        return hr;
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
        return hr;
    }

    return S_OK;
}

static D2DRenderNode _read_png_file(TCHAR* path)
{
	int             fd;
    MemoryContext   mcxt = NULL;
    D2DRenderNode   n = NULL;
	unsigned char  *p;
    unsigned char   png_magic[8] = { 0 };
	unsigned int    size, bytes;
    fileType ft = fileUnKnown;
    HRESULT hr = S_OK;

    if(NULL == path) return NULL;
	if (0 != _tsopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0)) return NULL;

	size = (unsigned int)_lseek(fd, 0, SEEK_END); /* get the file size */
	if (size > MAX_BUF_LEN || size < 8) 
	{
	    _close(fd); 
		return NULL;
	}

    _lseek(fd, 0, SEEK_SET); /* go to the begin of the file */
    
    bytes = (unsigned int)_read(fd, png_magic, 8);  /* try to detect PNG header */
    if(8 != bytes) 
    {
	    _close(fd); 
		return NULL;
    }
    /* check PNG magic number: 89 50 4e 47 0d 0a 1a 0a */
    p = (unsigned char*)png_magic;
    if((0x89 == p[0]) && (0x50 == p[1]) && (0x4e == p[2]) && (0x47 == p[3]) && 
        (0x0d == p[4]) && (0x0a == p[5]) && (0x1a == p[6]) && (0x0a == p[7]))
    {
        ft = filePNG;
    }

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

	_lseek(fd, 0, SEEK_SET); /* go to the begin of the file */

    if(filePNG != ft && fileJPG != ft && fileBMP != ft && fileGIF != ft)
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

    if(fileJPG == ft) /* we still need to check the last two bytes */
    {
        if((0xff != p[size-2]) || (0xd9 != p[size-1]))
        {
            MemoryContextDelete(mcxt);
            _close(fd); 
            return NULL;
        }
    }

    n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
    if(NULL == n)
    {
        MemoryContextDelete(mcxt);
        _close(fd); 
        return NULL;
    }
    
    _close(fd); 

    n->std.flag = SO_TYPE_IMAGE;
    n->std.next = NULL;
    n->std.data	= p;
    n->std.len  = size;

    hr = _create_device_independance_D2D(n);
    if(SUCCEEDED(hr)) 
    {
        return n;
    }
 
    return NULL;
 
#if 0    
    //mcxt = *(MemoryContext *) (((char *) n) - sizeof(void *)); 

    hr = d2d.pIWICFactory->CreateStream(&(n->pStream));
    if(FAILED(hr) || NULL == n->pStream)
    {
        MemoryContextDelete(mcxt);
        return NULL;
    }
    hr = n->pStream->InitializeFromMemory((WICInProcPointer)(n->std.data), n->std.len);
    if(FAILED(hr))
    {
        n->pStream->Release();
        n->pStream = NULL;
        MemoryContextDelete(mcxt);
        return NULL;
    }
    hr = d2d.pIWICFactory->CreateDecoderFromStream(n->pStream, NULL,
                            WICDecodeMetadataCacheOnLoad, &(n->pDecoder));

    if(FAILED(hr) || NULL == n->pDecoder)
    {
        n->pStream->Release();
        n->pStream = NULL;
        MemoryContextDelete(mcxt);
        return 0;
    }
    hr = n->pDecoder->GetFrame(0, &(n->pFrame));
    if(FAILED(hr) || NULL == n->pFrame)
    {
        n->pStream->Release();
        n->pStream = NULL;
        n->pDecoder->Release();
        n->pDecoder = NULL;
        MemoryContextDelete(mcxt);
        return NULL;
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
        return NULL;
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
        return NULL;
    }
#endif 
    //if(fileGIF == ft) {  n->am = GetAnimationMetaData(n->pDecoder); }
 //   return n;
}

STBIWDEF unsigned char *stbi_write_png_to_mem(const unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);

static D2DRenderNode _compose_png_data(plutovg_surface_t* surface)
{
    D2DRenderNode n = NULL;
    MemoryContext mcxt = NULL;
    unsigned char* data = surface->data;
    int width = surface->width;
    int height = surface->height;
    int stride = surface->stride;
    uint32_t image_size; //, total_size;
    unsigned char* image = NULL;

    mcxt = AllocSetContextCreate(TopMemoryContext, "BmpCxt", ALLOCSET_DEFAULT_SIZES);

    if(NULL == mcxt) return NULL;
    MemoryContextSwitchTo(mcxt);
    
    image_size = stride * height;
    image = (unsigned char* )palloc(image_size);
    if(NULL == image)
    {
        MemoryContextDelete(mcxt);        
        return NULL;
    }

    n = (D2DRenderNode)palloc0(sizeof(D2DRenderNodeData));
    if(NULL == n)
    {
        MemoryContextDelete(mcxt);
        return NULL;
    }

#if 0
    pF = (BITMAPFILEHEADER*)bmp;
    pF->bfType = 0x4d42;
    pF->bfSize = (DWORD)total_size;
    pF->bfReserved1 = 0;
    pF->bfReserved2 = 0;
    pF->bfOffBits   = 0x36;

    pI = (BITMAPINFOHEADER*)(bmp + sizeof(BITMAPFILEHEADER));
    pI->biSize          = 0x28;
    pI->biWidth         = width;
    pI->biHeight        = height;
    pI->biPlanes        = 0x01;
    pI->biBitCount      = 0x20;
    pI->biCompression   = BI_RGB;
    pI->biSizeImage     = total_size - (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
    pI->biXPelsPerMeter = 0;
    pI->biYPelsPerMeter = 0;
    pI->biClrUsed       = 0;
    pI->biClrImportant  = 0;

    image = bmp + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
#endif
    for(int y = 0; y < height; y++)
    {
        const uint32_t* src = (uint32_t*)(data + stride * y);
        uint32_t* dst = (uint32_t*)(image + stride * y);
        for(int x = 0;x < width;x++)
        {
            uint32_t a = src[x] >> 24;
            if(a != 0)
            {
                uint32_t r = (((src[x] >> 16) & 0xff) * 255) / a;
                uint32_t g = (((src[x] >> 8) & 0xff) * 255) / a;
                uint32_t b = (((src[x] >> 0) & 0xff) * 255) / a;

                dst[x] = (a << 24) | (b << 16) | (g << 8) | r;
            }
            else
            {
                dst[x] = 0;
            }
        }
    }

    int len;
    unsigned char *png = stbi_write_png_to_mem((const unsigned char *)image, stride, width, height, 4, &len);
    if (png == NULL)
    {
        MemoryContextDelete(mcxt);
        return NULL;
    }
    pfree(image);
    image = (unsigned char* )palloc(len);
    if(NULL == image)
    {
        free(png);
        MemoryContextDelete(mcxt);        
        return NULL;
    }
    memcpy(image, png, len);
    free(png);
    n->std.flag     = SO_TYPE_IMAGE;
    n->std.next     = NULL;
    n->std.data	    = image;
    n->std.len      = len;
    n->std.width    = width;
    n->std.height   = height;

    HRESULT hr = _create_device_independance_D2D(n);
    if(SUCCEEDED(hr)) return n;

    return NULL;

#if 0    
    hr = d2d.pIWICFactory->CreateStream(&(n->pStream));
    if(FAILED(hr) || NULL == n->pStream)
    {
        MemoryContextDelete(mcxt);
        return NULL;
    }
    hr = n->pStream->InitializeFromMemory((WICInProcPointer)(n->std.data), n->std.len);
    if(FAILED(hr))
    {
        n->pStream->Release();
        n->pStream = NULL;
        MemoryContextDelete(mcxt);
        return NULL;
    }
    hr = d2d.pIWICFactory->CreateDecoderFromStream(n->pStream, NULL,
                            WICDecodeMetadataCacheOnLoad, &(n->pDecoder));

    if(FAILED(hr) || NULL == n->pDecoder)
    {
        n->pStream->Release();
        n->pStream = NULL;
        MemoryContextDelete(mcxt);
        return 0;
    }
    hr = n->pDecoder->GetFrame(0, &(n->pFrame));
    if(FAILED(hr) || NULL == n->pFrame)
    {
        n->pStream->Release();
        n->pStream = NULL;
        n->pDecoder->Release();
        n->pDecoder = NULL;
        MemoryContextDelete(mcxt);
        return NULL;
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
        return NULL;
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
        return NULL;
    }

    return n;
#endif     
}

void render_svg_logo(const char* logoSVG)
{
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
}

#if 0
static HRESULT GetBackgroundColor(IWICMetadataQueryReader*, 
                        IWICBitmapDecoder*, D2D1_COLOR_F*);
static Animation GetAnimationMetaData(IWICBitmapDecoder*);
#endif

unsigned WINAPI open_mspfile_thread(LPVOID lpData)
{
	int   fd;
    BOOL  isOpened = FALSE;
	unsigned char *p;
    unsigned char  png_magic[8] = { 0 };
	unsigned int size, bytes, ret;
    fileType ft = fileUnKnown;
    WPARAM wp = 0;
    LPARAM lp = 0;
    D2DRenderNode n, m;
    HRESULT hr = S_OK;
    ThreadParam* t = (ThreadParam*)lpData;

    if(NULL == t) return 0;

	HWND hWndUI = t->hWnd;
	ATLASSERT(::IsWindow(hWndUI));

    n = _read_png_file(t->pfilePath);

    if(NULL != n) /* it is one PNG or JPG/GIF file */
    {
        d2d.ft = filePNG;
        m = d2d.pData;

        EnterCriticalSection(&(d2d.cs));
            d2d.pData = n;
        LeaveCriticalSection(&(d2d.cs));
        
        ReleaseD2DResource(m);

        wp = UI_NOTIFY_FILEOPEN;
        lp = (LPARAM)filePNG;
        PostMessage(hWndUI, WM_UI_NOTIFY, wp, lp);
        return 0;
    }
    /* now check if it is a md or svg file */
	if (0 != _tsopen_s(&fd, t->pfilePath, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0)) 
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
    if (bytes != size) /* read error, since bytes != size */
    {
        wp = UI_NOTIFY_FILEFAIL;
        lp = 6;
        goto Quit_open_mspfile_thread;
    }

    ft = fileSVG;
    if(fileSVG == ft)
    {
        plutovg_surface_t* surface = plutosvg_load_from_memory((const char*)p, size, NULL, 0, 0, 96.0);
        if(NULL == surface)
        {
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }

        free(p); p = NULL;
        _close(fd); isOpened = FALSE;

        n = _compose_png_data(surface);
        
        plutovg_surface_destroy(surface);
        if(NULL == n)
        {
            wp = UI_NOTIFY_FILEFAIL;
            lp = 6;
            goto Quit_open_mspfile_thread;
        }

        d2d.ft = filePNG;
        m = d2d.pData;

        EnterCriticalSection(&(d2d.cs));
            d2d.pData = n;
        LeaveCriticalSection(&(d2d.cs));
        
        ReleaseD2DResource(m);

        wp = UI_NOTIFY_FILEOPEN;
        lp = (LPARAM)fileBMP;
        goto Quit_open_mspfile_thread;

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

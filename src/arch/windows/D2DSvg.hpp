#ifndef __D2DSVG_HPP__
#define __D2DSVG_HPP__
#include "stdafx.h"
#include <stack>
#include <memory>
#include "svg.hpp"

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        std::string msg = "Operation failed with result: " + std::to_string(hr);
        throw std::runtime_error(msg);
    }
}

template <class T>
void ThrowIfNull(CComPtr<T> inComObject)
{
    if (!inComObject)
    {
        throw std::runtime_error("Object is Null");
    }
}

class D2DSVGPath final : public Path
{
public:
    D2DSVGPath(CComPtr<ID2D1Factory> inD2DFactory)
    {
        inD2DFactory->CreatePathGeometry(&mPath);
        SVG_ASSERT(mPath);
    }
    ~D2DSVGPath()
    {
        SVG_ASSERT(mPath);
        ClosePathSink();
        mPath.Release();
    }

    void Rect(float x, float y, float w, float h) override
    {
        MoveTo(x, y);
        LineTo(x + w, y);
        LineTo(x + w, y + h);
        LineTo(x, y + h);
        ClosePath();
    }

    void RoundedRect(float x, float y, float w, float h, float rx, float ry) override
    {
        MoveTo(x + rx, y);
        LineTo(x + w - rx, y);
        AddArc(x + w, y + ry, rx, ry);
        LineTo(x + w, y + h - ry);
        AddArc(x + w - rx, y + h, rx, ry);
        LineTo(x + rx, y + h);
        AddArc(x, y + h - ry, rx, ry);
        LineTo(x, y + ry);
        AddArc(x + rx, y, rx, ry);
        ClosePath();
    }

    void Ellipse(float cx, float cy, float rx, float ry) override
    {
        MoveTo(cx, cy - ry);
        AddArc(cx, cy + ry, rx, ry);
        AddArc(cx, cy - ry, rx, ry);
        ClosePath();
    }

    void MoveTo(float x, float y) override
    {
        if (mHasOpenFigure)
        {
            SVG_ASSERT(mSink);
            mSink->EndFigure(D2D1_FIGURE_END_OPEN);
            mHasOpenFigure = false;
        }
        if (!mSink)
            mPath->Open(&mSink);
        SVG_ASSERT(mSink);
        mSink->BeginFigure(
                D2D1::Point2F(x, y),
                D2D1_FIGURE_BEGIN_FILLED
            );
        mHasOpenFigure = true;
        mCurrentX = x;
        mCurrentY = y;
    }

    void LineTo(float x, float y) override
    {
        if (!mHasOpenFigure)
            MoveTo(x, y);
        mSink->AddLine(D2D1::Point2F(x, y));
        mCurrentX = x;
        mCurrentY = y;
    }

    void CurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override
    {
        SVG_ASSERT(mSink);
        if (!mHasOpenFigure)
            MoveTo(x1, y1);
        mSink->AddBezier(
            D2D1::BezierSegment(
                D2D1::Point2F(x1, y1),
                D2D1::Point2F(x2, y2),
                D2D1::Point2F(x3, y3))
            );
        mCurrentX = x3;
        mCurrentY = y3;
    }

    void CurveToV(float x2, float y2, float x3, float y3) override
    {
        float cx1 = static_cast<float>(mCurrentX + 2.0 / 3.0 * (x2 - mCurrentX));
        float cy1 = static_cast<float>(mCurrentY + 2.0 / 3.0 * (y2 - mCurrentY));
        float cx2 = static_cast<float>(x3 + 2.0 / 3.0 * (x2 - x3));
        float cy2 = static_cast<float>(y3 + 2.0 / 3.0 * (y2 - y3));
        CurveTo(cx1, cy1, cx2, cy2, x3, y3);
    }

    void ClosePath() override
    {
        if (mHasOpenFigure)
        {
            SVG_ASSERT(mSink);
            mSink->EndFigure(D2D1_FIGURE_END_OPEN);
            mHasOpenFigure = false;
        }
        if (mSink)
        {
            mSink->Close();
            mSink.Release();
        }
    }

    CComPtr<ID2D1PathGeometry> GetGraphicsPath()
    {
        ClosePathSink();
        return mPath;
    }

private:
    void AddArc(float x, float y, float rx, float ry)
    {
        SVG_ASSERT(mSink);
        mSink->AddArc(
            D2D1::ArcSegment(
                D2D1::Point2F(x, y),
                D2D1::SizeF(rx, ry),
                0,
                D2D1_SWEEP_DIRECTION_CLOCKWISE,
                D2D1_ARC_SIZE_SMALL
            )
        );
    }

    void ClosePathSink()
    {
        if (mHasOpenFigure)
        {
            SVG_ASSERT(mSink);
            mSink->EndFigure(D2D1_FIGURE_END_OPEN);
            mHasOpenFigure = false;
        }
        if (mSink)
        {
            mSink->Close();
            mSink.Release();
            mSink = NULL;
        }
    }

private:
    CComPtr<ID2D1PathGeometry> mPath;
    CComPtr<ID2D1GeometrySink> mSink;
    bool mHasOpenFigure{ false };
    float mCurrentX{};
    float mCurrentY{};
};


class D2DSVGTransform final : public Transform
{
public:
    D2DSVGTransform(float a, float b, float c, float d, float tx, float ty)
    {
        mTransform = D2D1::Matrix3x2F{a, b, c, d, tx, ty};
    }

    void Set(float a, float b, float c, float d, float tx, float ty) override
    {
        mTransform = D2D1::Matrix3x2F{ a, b, c, d, tx, ty };
    }

    void Rotate(float r) override
    {
        mTransform = D2D1::Matrix3x2F::Rotation(r) * mTransform;
    }

    void Translate(float tx, float ty) override
    {
        mTransform = D2D1::Matrix3x2F::Translation(tx, ty) * mTransform;
    }

    void Scale(float sx, float sy) override
    {
        mTransform = D2D1::Matrix3x2F::Scale(sx, sy) * mTransform;
    }

    void Concat(float a, float b, float c, float d, float tx, float ty) override
    {
        mTransform = D2D1::Matrix3x2F{a, b, c, d, tx, ty} * mTransform;
    }

    const D2D1::Matrix3x2F& GetMatrix() const { return mTransform; }

private:
    D2D1::Matrix3x2F mTransform;
};

class D2DSVGImageData final : public ImageData
{
public:
    D2DSVGImageData(CComPtr<IWICBitmapSource> bitmapSource) : mBitmapSource(bitmapSource)
    {
        if (mBitmapSource)
        {
            UINT width = 0;
            UINT height = 0;
            ThrowIfFailed(mBitmapSource->GetSize(&width, &height));
            mWidth = static_cast<float>(width);
            mHeight = static_cast<float>(height);
        }
    }

    ~D2DSVGImageData() {}

    float Width() const override { return mWidth; }
    float Height() const override { return mHeight; }

    CComPtr<IWICBitmapSource> GetBitmapSource() const { return mBitmapSource; }

private:
    float mWidth{};
    float mHeight{};
    CComPtr<IWICBitmapSource> mBitmapSource;
};

inline D2D1_CAP_STYLE D2DCapStyle(LineCap lineCap)
{
    switch (lineCap)
    {
    case LineCap::kRound:
        return D2D1_CAP_STYLE_ROUND;
    case LineCap::kSquare:
        return D2D1_CAP_STYLE_SQUARE;
    case LineCap::kButt:
    default:
        return D2D1_CAP_STYLE_FLAT;
    }
}

inline D2D1_LINE_JOIN D2DLineJoin(LineJoin lineJoin)
{
    switch (lineJoin)
    {
    case LineJoin::kRound:
        return D2D1_LINE_JOIN_ROUND;
    case LineJoin::kBevel:
        return D2D1_LINE_JOIN_BEVEL;
    case LineJoin::kMiter:
    default:
        return D2D1_LINE_JOIN_MITER;
    }
}

//#if 0
class SVG_IMP_EXP D2DSVGRenderer final : public SVGRenderer
{
public:
    D2DSVGRenderer() {}

    virtual ~D2DSVGRenderer() {}
    
#if 0
    std::unique_ptr<ImageData> CreateImageData(const std::string& base64, ImageEncoding encoding) override
    {
        std::string imageString = base64_decode(base64);
        CComPtr<IStream> stream{ SHCreateMemStream((const BYTE*)imageString.c_str(), (UINT)imageString.size()) };
        ThrowIfNull(stream);

        CComPtr<IWICBitmapDecoder> imgDecoder;
        ThrowIfFailed(
            mWICFactory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &imgDecoder));

        CComPtr<IWICBitmapFrameDecode> frame;
        ThrowIfFailed(imgDecoder->GetFrame(0, &frame));

        // Convert to a format that Direct2D can use (pre-multiplied BRGA is best)
        CComPtr<IWICFormatConverter> converter;
        ThrowIfFailed(mWICFactory->CreateFormatConverter(&converter));
        ThrowIfFailed(converter->Initialize(
            frame,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeMedianCut));

        // Keep a BitmapSource and not an ID2D1Bitmap because the latter is tied to a specific ID2D1RenderTarget.
        CComPtr<IWICBitmapSource> bitmapSource = converter;
        return std::unique_ptr<D2DSVGImageData>(new D2DSVGImageData(bitmapSource));

    }
#endif 
    std::unique_ptr<Path> CreatePath() override
    {
        return std::unique_ptr<D2DSVGPath>(new D2DSVGPath{mD2DFactory});
    }

    std::unique_ptr<Transform> CreateTransform(
        float a = 1.0, float b = 0.0, float c = 0.0, float d = 1.0, float tx = 0.0, float ty = 0.0) override
    {
        return std::unique_ptr<D2DSVGTransform>(new D2DSVGTransform(a, b, c, d, tx, ty));        
    }

    void Save(const GraphicStyle& graphicStyle) override
    {
        CComPtr<ID2D1Geometry> maskPath;
        D2D1_MATRIX_3X2_F maskTransform = D2D1::IdentityMatrix();
        if (graphicStyle.clippingPath)
        {
            if (graphicStyle.clippingPath->transform)
                maskTransform = dynamic_cast<D2DSVGTransform*>(graphicStyle.clippingPath->transform.get())->GetMatrix();
            const auto constPath = dynamic_cast<const D2DSVGPath*>(graphicStyle.clippingPath->path.get());
            maskPath = const_cast<D2DSVGPath*>(constPath)->GetGraphicsPath();
        }

        CComPtr<ID2D1Layer> layer;
        mContext->CreateLayer(&layer);
        mContext->PushLayer(
            D2D1::LayerParameters(
                D2D1::InfiniteRect(),
                maskPath,
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                maskTransform,
                graphicStyle.opacity
            ),
            layer);

        // FIXME: May need to get applied before creating the layer to apply to clipping path.
        D2D1_MATRIX_3X2_F transform;
        mContext->GetTransform(&transform);
        mContextTransform.push(transform);
        if (graphicStyle.transform)
        {
            D2D1_MATRIX_3X2_F gsTransform = dynamic_cast<D2DSVGTransform*>(graphicStyle.transform.get())->GetMatrix();
            mContext->SetTransform(gsTransform * transform);
        }
    }

    void Restore() override
    {
        mContext->SetTransform(mContextTransform.top());
        mContextTransform.pop();
        mContext->PopLayer();
    }

    void DrawPath(const Path& renderPath, const GraphicStyle& graphicStyle, const FillStyle& fillStyle, const StrokeStyle& strokeStyle) override
    {
        SVG_ASSERT(mContext);

        Save(graphicStyle);

        const auto& constPath = dynamic_cast<const D2DSVGPath&>(renderPath);
        auto path = const_cast<D2DSVGPath&>(constPath).GetGraphicsPath();
        if (fillStyle.hasFill)
        {
            auto brush = CreateBrush(fillStyle.paint);
            mContext->FillGeometry(path, brush, nullptr);
            brush.Release();
        }
        if (strokeStyle.hasStroke)
        {
            auto lineCap = D2DCapStyle(strokeStyle.lineCap);
            auto lineJoin = D2DLineJoin(strokeStyle.lineJoin);

            auto brush = CreateBrush(strokeStyle.paint);

            SVG_ASSERT(mD2DFactory);
            CComPtr<ID2D1StrokeStyle> d2dStrokeStyle;
            mD2DFactory->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(
                    lineCap,
                    lineCap,
                    lineCap,
                    lineJoin,
                    strokeStyle.miterLimit,
                    D2D1_DASH_STYLE_CUSTOM,
                    strokeStyle.dashOffset),
                strokeStyle.dashArray.data(),
                static_cast<UINT32>(strokeStyle.dashArray.size()),
                &d2dStrokeStyle);
            
            mContext->DrawGeometry(
                path,
                brush,
                strokeStyle.lineWidth,
                d2dStrokeStyle);

            d2dStrokeStyle.Release();
            brush.Release();
        }
        Restore();
    }

    void DrawImage(const ImageData& image, const GraphicStyle& graphicStyle, const Rect& clipArea, const Rect& fillArea) override
    {
        const D2DSVGImageData& imageData(dynamic_cast<const D2DSVGImageData&>(image));
        CComPtr<IWICBitmapSource> bitmapSource = imageData.GetBitmapSource();
        if (bitmapSource)
        {
            CComPtr<ID2D1Bitmap> bitmap;
            ThrowIfFailed(mContext->CreateBitmapFromWicBitmap(bitmapSource, &bitmap));

            SVG_ASSERT(mContext);
            Save(graphicStyle);
            D2D1_RECT_F clipRect{};
            if (clipArea.width < fillArea.width || clipArea.height < fillArea.height)
            {
                clipRect = D2D1::RectF(clipArea.x, clipArea.y, clipArea.x + clipArea.width, clipArea.y + clipArea.height);
                CComPtr<ID2D1Layer> layer;
                mContext->CreateLayer(&layer);
                mContext->PushLayer(
                    D2D1::LayerParameters(clipRect),
                    layer
                );
            }

            D2D1_RECT_F drawRect = D2D1::RectF(fillArea.x, fillArea.y, fillArea.x + fillArea.width, fillArea.y + fillArea.height);
            mContext->DrawBitmap(bitmap, drawRect);

            if (clipRect.right > clipRect.left)
            {
                mContext->PopLayer();
            }

            Restore();
        }
    }

    void SetGraphicsContext(IWICImagingFactory* pWICFactory, ID2D1Factory* pD2DFactory, ID2D1RenderTarget* renderTarget)
    {
        mWICFactory = pWICFactory;
        mD2DFactory = pD2DFactory;
        mContext = renderTarget;
    }

    void ReleaseGraphicsContext()
    {
        mD2DFactory.Release(); mD2DFactory = NULL;
        mWICFactory.Release(); mWICFactory = NULL;
    }

private:
    CComPtr<ID2D1Brush> D2DSVGRenderer::CreateBrush(const Paint& paint)
    {

        SVG_ASSERT(mContext);
        CComPtr<ID2D1Brush> brush = NULL;
#if 0        
        if (paint.type() == typeid(Color))
        {
            const auto& color = boost::get<Color>(paint);
            CComPtr<ID2D1SolidColorBrush> solidColorBrush;
            mContext->CreateSolidColorBrush({color[0], color[1], color[2], color[3]}, &solidColorBrush);
            solidColorBrush->QueryInterface(&brush);
        }
        else if (paint.type() == typeid(Gradient))
        {
            const auto& gradient = boost::get<Gradient>(paint);
            std::vector<D2D1_GRADIENT_STOP> colorsStops;
            for (const auto& stop : gradient.colorStops)
            {
                const auto& color = stop.second;
                colorsStops.push_back({ stop.first, { color[0], color[1], color[2], color[3] } });
            }
            CComPtr<ID2D1GradientStopCollection> gradientStopCollection;
            mContext->CreateGradientStopCollection(
                colorsStops.data(),
                static_cast<UINT32>(colorsStops.size()),
                D2D1_GAMMA_2_2,
                D2DSpreadMethod(gradient.method),
                &gradientStopCollection);
            if (gradient.type == GradientType::kLinearGradient)
            {
                CComPtr<ID2D1LinearGradientBrush> linearGradientBrush;
                mContext->CreateLinearGradientBrush(
                    D2D1::LinearGradientBrushProperties(
                        D2D1::Point2F(gradient.x1, gradient.y1),
                        D2D1::Point2F(gradient.x2, gradient.y2)),
                    gradientStopCollection,
                    &linearGradientBrush);
                if (gradient.transform)
                    linearGradientBrush->SetTransform(std::static_pointer_cast<D2DSVGTransform>(gradient.transform)->GetMatrix());
                linearGradientBrush->QueryInterface(&brush);
            }
            else
            {
                CComPtr<ID2D1RadialGradientBrush> radialGradientBrush;
                mContext->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties(
                        D2D1::Point2F(gradient.cx, gradient.cy),
                        D2D1::Point2F(gradient.fx, gradient.fy),
                        gradient.r,
                        gradient.r),
                    gradientStopCollection,
                    &radialGradientBrush);
                if (gradient.transform)
                    radialGradientBrush->SetTransform(std::static_pointer_cast<D2DSVGTransform>(gradient.transform)->GetMatrix());
                radialGradientBrush->QueryInterface(&brush);
            }
        }
        else
        {
            SVG_ASSERT_MSG(false, "Unknown paint type");
        }
#endif 
        return brush;

    }

    CComPtr<ID2D1RenderTarget> mContext;
    CComPtr<ID2D1Factory> mD2DFactory;
    CComPtr<IWICImagingFactory> mWICFactory;

    std::stack<D2D1_MATRIX_3X2_F> mContextTransform;
};

//#endif 
#endif /* __D2DSVG_HPP__ */
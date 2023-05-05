#ifndef __SVG_HPP__
#define __SVG_HPP__

#include <array>
//#include <boost/variant.hpp>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "rect.hpp"

#define SVG_IMP_EXP     /* empty */
#define SVG_ASSERT(x)   do {} while(0);

// Attributes
constexpr const char* kIdAttr{"id"};
constexpr const char* kXAttr{"x"};
constexpr const char* kYAttr{"y"};
constexpr const char* kWidthAttr{"width"};
constexpr const char* kHeightAttr{"height"};
constexpr const char* kRxAttr{"rx"};
constexpr const char* kRyAttr{"ry"};
constexpr const char* kRAttr{"r"};
constexpr const char* kDAttr{"d"};
constexpr const char* kCxAttr{"cx"};
constexpr const char* kCyAttr{"cy"};
constexpr const char* kFxAttr{"fx"};
constexpr const char* kFyAttr{"fy"};
constexpr const char* kX1Attr{"x1"};
constexpr const char* kY1Attr{"y1"};
constexpr const char* kX2Attr{"x2"};
constexpr const char* kY2Attr{"y2"};
constexpr const char* kPointsAttr{"points"};
constexpr const char* kHrefAttr{"href"};
constexpr const char* kTransformAttr{"transform"};
constexpr const char* kGradientTransformAttr{"gradientTransform"};
constexpr const char* kViewBoxAttr{"viewBox"};
constexpr const char* kSpreadMethodAttr{"spreadMethod"};
constexpr const char* kOffsetAttr{"offset"};
#ifdef SVG_DEBUG
constexpr const char* kDataNameAttr{"data-name"};
#endif

// Properties
constexpr const char* kColorProp{"color"};
constexpr const char* kClipRuleProp{"clip-rule"};
constexpr const char* kFillProp{"fill"};
constexpr const char* kFillRuleProp{"fill-rule"};
constexpr const char* kFillOpacityProp{"fill-opacity"};
constexpr const char* kStrokeProp{"stroke"};
constexpr const char* kStrokeDasharrayProp{"stroke-dasharray"};
constexpr const char* kStrokeDashoffsetProp{"stroke-dashoffset"};
constexpr const char* kStrokeLinecapProp{"stroke-linecap"};
constexpr const char* kStrokeLinejoinProp{"stroke-linejoin"};
constexpr const char* kStrokeMiterlimitProp{"stroke-miterlimit"};
constexpr const char* kStrokeOpacityProp{"stroke-opacity"};
constexpr const char* kStrokeWidthProp{"stroke-width"};
constexpr const char* kVisibilityProp{"visibility"};
constexpr const char* kClipPathProp{"clip-path"};
constexpr const char* kDisplayProp{"display"};
constexpr const char* kOpacityProp{"opacity"};
constexpr const char* kStopOpacityProp{"stop-opacity"};
constexpr const char* kStopColorProp{"stop-color"};
constexpr const char* kPreserveAspectRatioAttr{"preserveAspectRatio"};

// Elements
constexpr const char* kLineElem{"line"};
constexpr const char* kRectElem{"rect"};
constexpr const char* kPathElem{"path"};
constexpr const char* kPolygonElem{"polygon"};
constexpr const char* kPolylineElem{"polyline"};
constexpr const char* kEllipseElem{"ellipse"};
constexpr const char* kCircleElem{"circle"};
constexpr const char* kGElem{"g"};
constexpr const char* kClipPathElem{"clipPath"};
constexpr const char* kSymbolElem{"symbol"};
constexpr const char* kStyleElem{"style"};
constexpr const char* kLinearGradientElem{"linearGradient"};
constexpr const char* kRadialGradientElem{"radialGradient"};
constexpr const char* kDefsElem{"defs"};
constexpr const char* kUseElem{"use"};
constexpr const char* kImageElem{"image"};
constexpr const char* kStopElem{"stop"};
constexpr const char* kSvgElem{"svg"};

// Values
constexpr const char* kDataUrlPngVal{"data:image/png;base64,"};
//constexpr const char* kDataUrlJpgVal{"data:image/jpg;base64,"};
//constexpr const char* kDataUrlJpegVal{"data:image/jpeg;base64,"};
constexpr const char* kSliceVal{"slice"};
constexpr const char* kXMinYMinVal{"xMinYMin"};
constexpr const char* kXMidYMinVal{"xMidYMin"};
constexpr const char* kXMaxYMinVal{"xMaxYMin"};
constexpr const char* kXMinYMidVal{"xMinYMid"};
constexpr const char* kXMaxYMidVal{"xMaxYMid"};
constexpr const char* kXMinYMaxVal{"xMinYMax"};
constexpr const char* kXMidYMaxVal{"xMidYMax"};
constexpr const char* kXMaxYMaxVal{"xMaxYMax"};
constexpr const char* kEvenoddVal{"evenodd"};
constexpr const char* kNonzeroVal{"nonzero"};
constexpr const char* kHiddenVal{"hidden"};
constexpr const char* kCollapseVal{"collapse"};
constexpr const char* kVisibleVal{"visible"};
constexpr const char* kRoundVal{"round"};
constexpr const char* kSquareVal{"square"};
constexpr const char* kBevelVal{"bevel"};
constexpr const char* kUrlVal{"url(#"};
constexpr const char* kNoneVal{"none"};
constexpr const char* kPadVal{"pad"};
constexpr const char* kReflectVal{"reflect"};
constexpr const char* kRepeatVal{"repeat"};

// Others
constexpr const char* kXlinkNS{"xlink"};

/**
 * Supported image encoding formats are PNG and JPEG.
 * The assumed encoding format based on the base64 string.
 */
enum class ImageEncoding
{
    kPNG
    //,kJPEG 
};

/**
 * Line caps as described in:
 * https://www.w3.org/TR/SVG2/painting.html#LineCaps
 */
enum class LineCap
{
    kButt,
    kRound,
    kSquare
};

/**
 * Line joins as described in:
 * https://www.w3.org/TR/SVG2/painting.html#LineJoin
 */
enum class LineJoin
{
    kMiter,
    kRound,
    kBevel
};

/**
 * Winding rules as described in:
 * https://www.w3.org/TR/SVG2/painting.html#WindingRule
 */
enum class WindingRule
{
    kNonZero,
    kEvenOdd
};

/**
 * Gradient type. SVG Native supports the 2 gradient types
 * * linear gradient and
 * * radial gradient.
 */
enum class GradientType
{
    kLinearGradient,
    kRadialGradient
};

/**
 * Gradient spread method.
 * * pad
 * * reflect
 * * repeat
 *
 * @note See https://www.w3.org/TR/SVG11/pservers.html#LinearGradientElementSpreadMethodAttribute
 */
enum class SpreadMethod
{
    kPad,
    kReflect,
    kRepeat
};

struct Gradient;
class Transform;
class Path;

using Color = std::array<float, 4>;
//using Paint = boost::variant<Color, Gradient>;
using Paint = Color;
using ColorStop = std::pair<float, Color>;
using ColorMap = std::map<std::string, Color>;

/**
 * Representation of a linear gradient paint server.
 */
struct Gradient
{
    GradientType type = GradientType::kLinearGradient;
    SpreadMethod method = SpreadMethod::kPad;
    std::vector<ColorStop> colorStops; /** Color stops with offset-color pairs **/
    float x1 = std::numeric_limits<float>::quiet_NaN(); /** x1 for linearGradient **/
    float y1 = std::numeric_limits<float>::quiet_NaN(); /** y1 for linearGradient **/
    float x2 = std::numeric_limits<float>::quiet_NaN(); /** x2 for linearGradient **/
    float y2 = std::numeric_limits<float>::quiet_NaN(); /** y2 for linearGradient **/
    float cx = std::numeric_limits<float>::quiet_NaN(); /** cx for radialGradient **/
    float cy = std::numeric_limits<float>::quiet_NaN(); /** cy for radialGradient **/
    float fx = std::numeric_limits<float>::quiet_NaN(); /** fx for radialGradient **/
    float fy = std::numeric_limits<float>::quiet_NaN(); /** fy for radialGradient **/
    float r = std::numeric_limits<float>::quiet_NaN(); /** r for radialGradient **/
    std::shared_ptr<Transform> transform; /** Joined transformation matrix based to the "transform" attribute. **/
};

/**
 * Stroke style information.
 */
struct StrokeStyle
{
    bool hasStroke = false;
    float strokeOpacity = 1.0;
    float lineWidth = 1.0;
    LineCap lineCap = LineCap::kButt;
    LineJoin lineJoin = LineJoin::kMiter;
    float miterLimit = 4.0;
    std::vector<float> dashArray;
    float dashOffset = 0.0;
    Paint paint = Color{{0, 0, 0, 1.0}};
};

/**
 * Fill style information.
 */
struct FillStyle
{
    bool hasFill = true;
    WindingRule fillRule = WindingRule::kNonZero;
    float fillOpacity = 1.0;
    Paint paint = Color{{0, 0, 0, 1.0}};
};

/**
 * Representation of a 2D affine transform with 6 values.
 */
class Transform
{
public:
    virtual ~Transform() = default;

    virtual void Set(float a, float b, float c, float d, float tx, float ty) = 0;
    virtual void Rotate(float r) = 0;
    virtual void Translate(float tx, float ty) = 0;
    virtual void Scale(float sx, float sy) = 0;
    virtual void Concat(float a, float b, float c, float d, float tx, float ty) = 0;
};

struct ClippingPath
{
    ClippingPath(bool aHasClipContent, WindingRule aClipRule, std::shared_ptr<Path> aPath, std::shared_ptr<Transform> aTransform)
        : hasClipContent{aHasClipContent}
        , clipRule{aClipRule}
        , path{aPath}
        , transform{aTransform}
    {}

    bool hasClipContent = false;
    WindingRule clipRule = WindingRule::kNonZero;
    std::shared_ptr<Path> path; /** Clipping path. **/
    std::shared_ptr<Transform> transform; /** Joined transformation matrix based to the "transform" attribute. **/
};

/**
 * All compositing related properties. With the exception of the
 */
struct GraphicStyle
{
    // Add blend modes and other graphic style options here.
    float opacity = 1.0; /** Corresponds to the "opacity" CSS property. **/
    std::shared_ptr<Transform> transform; /** Joined transformation matrix based to the "transform" attribute. **/
    std::shared_ptr<ClippingPath> clippingPath;
};

/**
 * A presentation of a path.
 */
class Path
{
public:
    virtual ~Path() = default;

    virtual void Rect(float x, float y, float width, float height) = 0;
    virtual void RoundedRect(float x, float y, float width, float height, float cornerRadiusX, float cornerRadiusY) = 0;
    virtual void Ellipse(float cx, float cy, float rx, float ry) = 0;

    virtual void MoveTo(float x, float y) = 0;
    virtual void LineTo(float x, float y) = 0;
    virtual void CurveTo(float x1, float y1, float x2, float y2, float x3, float y3) = 0;
    virtual void CurveToV(float x2, float y2, float x3, float y3) = 0;
    virtual void ClosePath() = 0;
};

/**
 * An image object generated from a base64 string.
 * The port needs to decode the Base64 string and provide
 * information about the dimensions of the image.
 **/
class ImageData
{
public:
    virtual ~ImageData() = default;

    virtual float Width() const = 0;
    virtual float Height() const = 0;
};


/**
 * Base class for deriving, platform dependent renderer classes with immediate
 * graphic library calls.
 */
class SVGRenderer
{
public:
    virtual ~SVGRenderer() = default;

    virtual std::unique_ptr<ImageData> CreateImageData(const std::string& base64, ImageEncoding) = 0;
    virtual std::unique_ptr<Path> CreatePath() = 0;
    virtual std::unique_ptr<Transform> CreateTransform(
        float a = 1.0, float b = 0.0, float c = 0.0, float d = 1.0, float tx = 0.0, float ty = 0.0) = 0;

    virtual void Save(const GraphicStyle& graphicStyle) = 0;
    virtual void Restore() = 0;

    virtual void DrawPath(
        const Path& path, const GraphicStyle& graphicStyle, const FillStyle& fillStyle, const StrokeStyle& strokeStyle) = 0;
    virtual void DrawImage(const ImageData& image, const GraphicStyle& graphicStyle, const Rect& clipArea, const Rect& fillArea) = 0;
    virtual Rect GetBounds(const Path& path, const GraphicStyle& graphicStyle, const FillStyle& fillStyle, const StrokeStyle& strokeStyle)
    {
      throw "Bound calculation functionality not implemented in this port";
      return Rect{0, 0, 0, 0};
    }
};

class SaveRestoreHelper
{
public:
    SaveRestoreHelper(std::weak_ptr<SVGRenderer> renderer, const GraphicStyle& graphicStyle)
        : mRenderer{renderer}
    {
        if (auto renderer = mRenderer.lock())
            renderer->Save(graphicStyle);
    }

    ~SaveRestoreHelper()
    {
        if (auto renderer = mRenderer.lock())
            renderer->Restore();
    }
private:
    std::weak_ptr<SVGRenderer> mRenderer{};
};


#endif /* __SVG_HPP__ */

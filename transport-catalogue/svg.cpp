#include "svg.h"

namespace svg {

using namespace std;

// Оператор вывода для Color
ostream& operator<<(ostream& out, const Color& color) {
    visit([&out](const auto& value) {
        using T = decay_t<decltype(value)>;
        if constexpr (is_same_v<T, monostate>) {
            out << "none";
        } else if constexpr (is_same_v<T, string>) {
            out << value;
        } else if constexpr (is_same_v<T, Rgb>) {
            out << "rgb(" << static_cast<int>(value.red) << ","
                << static_cast<int>(value.green) << ","
                << static_cast<int>(value.blue) << ")";
        } else if constexpr (is_same_v<T, Rgba>) {
            out << "rgba(" << static_cast<int>(value.red) << ","
                << static_cast<int>(value.green) << ","
                << static_cast<int>(value.blue) << ","
                << value.alpha << ")";
        }
    }, color);
    return out;
}

// ---------- Object ----------
void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    RenderObject(context);
    context.out << endl;
}

// ---------- Stroke enums ----------
ostream& operator<<(ostream& out, StrokeLineCap cap) {
    switch (cap) {
        case StrokeLineCap::BUTT: return out << "butt";
        case StrokeLineCap::ROUND: return out << "round";
        case StrokeLineCap::SQUARE: return out << "square";
    }
    return out;
}

ostream& operator<<(ostream& out, StrokeLineJoin join) {
    switch (join) {
        case StrokeLineJoin::ARCS: return out << "arcs";
        case StrokeLineJoin::BEVEL: return out << "bevel";
        case StrokeLineJoin::MITER: return out << "miter";
        case StrokeLineJoin::MITER_CLIP: return out << "miter-clip";
        case StrokeLineJoin::ROUND: return out << "round";
    }
    return out;
}

// ---------- Circle ----------
Circle& Circle::SetCenter(Point center) { center_ = center; return *this; }
Circle& Circle::SetRadius(double radius) { radius_ = radius; return *this; }

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\"" << center_.x << "\" cy=\"" << center_.y
        << "\" r=\"" << radius_ << "\"";
    RenderAttrs(out);
    out << " />";
}

// ---------- Polyline ----------
Polyline& Polyline::AddPoint(Point point) { points_.push_back(point); return *this; }

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\"";
    for (size_t i = 0; i < points_.size(); ++i) {
        if (i > 0) out << " ";
        out << points_[i].x << "," << points_[i].y;
    }
    out << "\"";
    RenderAttrs(out);
    out << "/>";
}

// ---------- Text ----------
static string Escape(const string& s) {
    string r;
    for (char c : s) {
        switch (c) {
            case '"': r += "&quot;"; break;
            case '\'': r += "&apos;"; break;
            case '<': r += "&lt;"; break;
            case '>': r += "&gt;"; break;
            case '&': r += "&amp;"; break;
            default: r += c;
        }
    }
    return r;
}

Text& Text::SetPosition(Point p) { pos_ = p; return *this; }
Text& Text::SetOffset(Point o) { offset_ = o; return *this; }
Text& Text::SetFontSize(uint32_t s) { font_size_ = s; return *this; }
Text& Text::SetFontFamily(string f) { font_family_ = move(f); return *this; }
Text& Text::SetFontWeight(string w) { font_weight_ = move(w); return *this; }
Text& Text::SetData(string d) { data_ = move(d); return *this; }

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text";
    RenderAttrs(out);
    out << " x=\"" << pos_.x << "\" y=\"" << pos_.y
        << "\" dx=\"" << offset_.x << "\" dy=\"" << offset_.y
        << "\" font-size=\"" << font_size_ << "\"";
    if (!font_family_.empty()) out << " font-family=\"" << font_family_ << "\"";
    if (!font_weight_.empty()) out << " font-weight=\"" << font_weight_ << "\"";
    out << ">" << Escape(data_) << "</text>";
}

// ---------- Document ----------
void Document::AddPtr(unique_ptr<Object>&& obj) { objects_.push_back(move(obj)); }

void Document::Render(ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
    RenderContext ctx(out, 2, 2);
    for (const auto& obj : objects_) obj->Render(ctx);
    out << "</svg>";
}

}  // namespace svg
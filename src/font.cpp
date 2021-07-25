//
// Created by hungr on 2021/07/12.
//
#include <canto/font.h>

#include <algorithm>
#include <list>
#include <map>
#include <unordered_map>

#include FT_OUTLINE_H

#include <gl++/vertex_buffer.h>


namespace {
    struct outline_t {
        glm::vec2 vertex;
        glm::vec2 control;
    };

    struct offset_t {
        std::ptrdiff_t outline_offset;
        std::size_t num_outline_points;
        std::ptrdiff_t character_index;
    };

    struct path {
        FT_BBox box;
        std::vector<outline_t> outer;
        std::list<std::vector<glm::vec2>> inner;
        std::optional<glm::vec2> move;
        void add_outer_point1(const FT_Vector& p);
        void add_outer_point2(const FT_Vector& p);
        void add_outer_point2(const glm::vec2& p);
        void add_outer_point3(const FT_Vector& p);
        void add_outer_point3(const glm::vec2& p);
        void new_inner();
        void add_inner_point(const FT_Vector& p);
        void add_inner_point(const glm::vec2& p);
        void replicate_back();
        void move_to(const FT_Vector& p);
        glm::vec2& back();
        [[nodiscard]] glm::vec2 to_vec2(const FT_Vector& v) const;
    };

    FT_Outline_Funcs funcs {
            +[](const FT_Vector* to, void* data) {
                auto p = reinterpret_cast<path*>(data);
                p->new_inner();
                p->add_inner_point(*to);
                p->move_to(*to); return 0;
            },
            +[](const FT_Vector* to, void* data) {
                auto p = reinterpret_cast<path*>(data);
                p->add_inner_point(*to);
                p->move_to(*to); return 0;
            },
            funcs.conic_to = +[](const FT_Vector* ctl, const FT_Vector* to, void* data) {
                auto p = reinterpret_cast<path*>(data);
                p->replicate_back();
                p->add_outer_point2(*ctl);
                p->add_outer_point3(*to);
                p->add_inner_point(*to);
                return 0;
            },

            funcs.cubic_to = +[](const FT_Vector* ctl1, const FT_Vector* ctl2, const FT_Vector* to, void* data) {
                auto p = reinterpret_cast<path*>(data);
                p->replicate_back();
                auto& ap1 = p->back();
                auto ap3 = p->to_vec2(*to);
                auto cp1 = 0.25f * (ap1 + 3.0f * p->to_vec2(*ctl1));
                auto cp2 = 0.25f * (ap3 + 3.0f * p->to_vec2(*ctl2));
                auto ap2 = 0.5f * (cp1 + cp2);
                p->add_outer_point2(cp1);
                p->add_outer_point3(ap2);
                p->replicate_back();
                p->add_outer_point2(cp2);
                p->add_outer_point3(ap3);
                p->add_inner_point(ap2);
                p->add_inner_point(ap3);
                return 0;
            }
    };

    std::map<std::string, FT_Long> index_pool;

    FT_Long get_index(const std::string& path);

    std::tuple<std::vector<outline_t>, std::vector<glm::vec2>, float> load_char(char16_t c, canto::face& face);
}

struct canto::font::font_resource {
    std::unordered_map<char16_t, offset_t> offsets;
    gl::vertex_buffer<gl::buffer_trait<outline_t, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW>> outline_vbo;
    gl::vertex_buffer<gl::buffer_trait<glm::vec2, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW>> box_vbo;
    std::vector<float> width;

    explicit font_resource(const std::tuple<std::vector<outline_t>, std::vector<glm::vec2>, std::vector<float>, std::unordered_map<char16_t, offset_t>>& data);
    explicit font_resource(canto::face& face);
private:
    static std::tuple<std::vector<outline_t>, std::vector<glm::vec2>, std::vector<float>, std::unordered_map<char16_t, offset_t>> load_default_chars(canto::face& face);
};

canto::font::font(const std::string &path) : m_face(*canto::library::get_library(), path, get_index(path)), resource(std::make_unique<font_resource>(m_face)) {

}

canto::font::font_resource::font_resource(const std::tuple<std::vector<outline_t>, std::vector<glm::vec2>, std::vector<float>, std::unordered_map<char16_t, offset_t>>& data) :
                                          offsets(std::get<3>(data)),
                                          outline_vbo(std::get<0>(data).begin(), std::get<0>(data).end()),
                                          box_vbo(std::get<1>(data).begin(), std::get<1>(data).end()),
                                          width(std::get<2>(data)) {

}

canto::font::font_resource::font_resource(canto::face& face) : font_resource(load_default_chars(face)) {

}

std::tuple<std::vector<outline_t>, std::vector<glm::vec2>, std::vector<float>, std::unordered_map<char16_t, offset_t>>
canto::font::font_resource::load_default_chars(canto::face& face) {
    std::unordered_map<char16_t, offset_t> offsets_;
    std::vector<outline_t> outlines;
    std::vector<glm::vec2> boxes;
    std::vector<float> widths;
    for(char16_t c = 0; c < 128; c++) {
        if(!std::isprint(c) && c == ' ') continue;

        auto [outline, box, width] = load_char(c, face);
        offsets_.emplace(c, offset_t { static_cast<ptrdiff_t>(outlines.size()), outline.size(), static_cast<ptrdiff_t>(offsets_.size()) });
        outlines.insert(outlines.end(), outline.begin(), outline.end());
        boxes.insert(boxes.end(), box.begin(), box.end());
        widths.push_back(width);
    }
    return std::make_tuple(outlines, boxes, widths, offsets_);
}

namespace {
    FT_Long get_index(const std::string& path) {
        if(index_pool.find(path) == index_pool.end()) {
            auto index = static_cast<FT_Long>(index_pool.size());
            return index_pool[path] = index;
        }
        return index_pool[path];
    }

    void path::add_outer_point1(const FT_Vector &p) {
        move.reset();
        outer.push_back(outline_t { to_vec2(p), { 0.0, 0.0 } });
    }

    void path::add_outer_point2(const FT_Vector &p) {
        move.reset();
        outer.push_back(outline_t { to_vec2(p), { 0.5, 0.0 } });
    }

    void path::add_outer_point3(const FT_Vector &p) {
        move.reset();
        outer.push_back(outline_t { to_vec2(p), { 1.0, 1.0 } });
    }

    void path::add_outer_point2(const glm::vec2 &p) {
        move.reset();
        outer.push_back(outline_t { p, { 0.5, 0.0 } });
    }

    void path::add_outer_point3(const glm::vec2 &p) {
        move.reset();
        outer.push_back(outline_t { p, { 1.0, 2.0 } });
    }

    void path::add_inner_point(const FT_Vector &p) {
        inner.back().emplace_back(to_vec2(p));
    }

    void path::add_inner_point(const glm::vec2 &p) {
        inner.back().emplace_back(p);
    }

    void path::new_inner() {
        inner.emplace_back();
    }

    void path::replicate_back() {
        if(move.has_value()) {
            auto m = move.value();
            move.reset();
            outer.push_back(outline_t { m, { 0.0, 0.0 } });
        } else {
            outer.push_back(outline_t { outer.back().vertex, { 0.0, 0.0 } });
        }
    }

    void path::move_to(const FT_Vector &p) {
        move = to_vec2(p);
    }

    glm::vec2 path::to_vec2(const FT_Vector &v) const {
        return glm::vec2 {
                float(v.x) / float(box.yMax - box.yMin),
                float(v.y) / float(box.yMax)
        };
    }

    glm::vec2 &path::back() {
        return outer.back().vertex;
    }

    std::tuple<std::vector<outline_t>, std::vector<glm::vec2>, float> load_char(char16_t c, canto::face& face) {
        std::vector<outline_t> vertexes;
        std::vector<glm::vec2> box;

        FT_Load_Glyph(*face, FT_Get_Char_Index(*face, c), FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP);
        auto& outline = face->glyph->outline;
        std::ptrdiff_t start = vertexes.size();
        path p { face->bbox };
        FT_Outline_Decompose(&outline, &funcs, &p);
        vertexes.insert(vertexes.end(), p.outer.begin(), p.outer.end());
        for(auto& u : p.inner) {
            for(std::size_t i = 1; i < u.size() - 1; i++) {
                vertexes.push_back(outline_t { u[0], { 0.5, 0.5 } });
                vertexes.push_back(outline_t { u[i], { 0.5, 0.5 } });
                vertexes.push_back(outline_t { u[i + 1], { 0.5, 0.5 } });
            }
        }
        auto max_height = static_cast<float>(face->bbox.yMax - face->bbox.yMin);
        auto xmin = std::min_element(vertexes.begin() + start / 2, vertexes.begin() + vertexes.size() / 2, [](auto& p1, auto& p2) { return p1.vertex.x < p2.vertex.x; })->vertex.x;
        auto xmax = std::max_element(vertexes.begin() + start / 2, vertexes.begin() + vertexes.size() / 2, [](auto& p1, auto& p2) { return p1.vertex.x < p2.vertex.x; })->vertex.x;
        auto ymin = std::min_element(vertexes.begin() + start / 2, vertexes.begin() + vertexes.size() / 2, [](auto& p1, auto& p2) { return p1.vertex.y < p2.vertex.y; })->vertex.y;
        auto ymax = std::max_element(vertexes.begin() + start / 2, vertexes.begin() + vertexes.size() / 2, [](auto& p1, auto& p2) { return p1.vertex.y < p2.vertex.y; })->vertex.y;

        box.emplace_back(xmin, ymax);
        box.emplace_back(xmax, ymax);
        box.emplace_back(xmin, ymin);
        box.emplace_back(xmax, ymin);

        float width = static_cast<float>(face->glyph->metrics.horiAdvance) / max_height;

        return std::make_tuple(std::move(vertexes), std::move(box), width);
    }
}


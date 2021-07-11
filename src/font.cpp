//
// Created by hungr on 2021/07/12.
//
#include <canto/font.h>

#include <map>

#include <gl++/vertex_buffer.h>
#include <unordered_map>

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

    std::map<std::string, FT_Long> index_pool;

    FT_Long get_index(const std::string& path);
}

struct canto::font::font_resource {
    gl::vertex_buffer<gl::buffer_trait<outline_t, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW>> outline;
    gl::vertex_buffer<gl::buffer_trait<glm::vec2, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW>> box;
    std::unordered_map<char16_t, offset_t> offsets;

    explicit font_resource(const std::tuple<std::vector<outline_t>, std::vector<glm::vec2>>& data);
    explicit font_resource(const std::string& path);
    std::tuple<std::vector<outline_t>, std::vector<glm::vec2>> load_default_chars(const std::string& path);
};

canto::font::font(const std::string &path) : m_face(*library::get_library(), path, get_index(path)), resource(std::make_unique<font_resource>(path)) {

}

canto::font::font_resource::font_resource(const std::tuple<std::vector<outline_t>, std::vector<glm::vec2>>& data) :
                                          outline(std::get<0>(data).begin(), std::get<0>(data).end()),
                                          box(std::get<1>(data).begin(), std::get<1>(data).end()) {

}

canto::font::font_resource::font_resource(const std::string &path) : font_resource(load_default_chars(path)) {

}

std::tuple<std::vector<outline_t>, std::vector<glm::vec2>>
canto::font::font_resource::load_default_chars(const std::string &path) {
    return std::tuple<std::vector<outline_t>, std::vector<glm::vec2>>();
}

namespace {
    FT_Long get_index(const std::string& path) {
        if(index_pool.find(path) == index_pool.end()) {
            auto index = static_cast<FT_Long>(index_pool.size());
            return index_pool[path] = index;
        }
        return index_pool[path];
    }
}


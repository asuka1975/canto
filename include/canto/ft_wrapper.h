//
// Created by hungr on 2021/07/12.
//

#ifndef CANTO_FT_WRAPPER_H
#define CANTO_FT_WRAPPER_H

#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace canto {
    struct library : std::unique_ptr<FT_Library, void(*)(FT_Library*)> {
        FT_Library operator->();
        static library& get_library() noexcept;
    private:
        library();
    };

    struct face : std::unique_ptr<FT_Face, void(*)(FT_Face*)> {
        face(FT_Library& library, const std::string& font_file, FT_Long index);
        FT_Face operator->();
    };
}

#endif //CANTO_FT_WRAPPER_H

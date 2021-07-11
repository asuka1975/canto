//
// Created by hungr on 2021/07/12.
//
#include "canto/ft_wrapper.h"

FT_Library canto::library::operator->() {
    return *get();
}

canto::library &canto::library::get_library() noexcept {
    static library lib{};
    return lib;
}

canto::library::library() : std::unique_ptr<FT_Library, void(*)(FT_Library *)>(new FT_Library, +[](FT_Library* ptr) {
    FT_Done_FreeType(*ptr);
    delete ptr;
}) {
    FT_Init_FreeType(get());
}

canto::face::face(FT_Library &library, const std::string &font_file, FT_Long index) : std::unique_ptr<FT_Face, void(*)(FT_Face *)>(new FT_Face, +[](FT_Face* ptr) {
    FT_Done_Face(*ptr);
    delete ptr;
}) {
    FT_New_Face(library, font_file.c_str(), index, get());
}

FT_Face canto::face::operator->() {
    return *get();
}

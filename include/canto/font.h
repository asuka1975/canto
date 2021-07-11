//
// Created by hungr on 2021/07/12.
//

#ifndef CANTO_FONT_H
#define CANTO_FONT_H

#include <memory>
#include <string>

#include "canto/ft_wrapper.h"

namespace canto {
    class font {
    public:
        explicit font(const std::string& path);
    private:
        canto::face m_face;
        struct font_resource;
        std::unique_ptr<font_resource> resource;
    };
}

#endif //CANTO_FONT_H

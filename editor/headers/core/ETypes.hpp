//
// Created by drhaz on 15.07.2026.
//

#ifndef SMB_ETYPES_HPP
#define SMB_ETYPES_HPP
#include <string>

namespace AetherEditor::core {
    struct EditorError {
        int code;
        std::string msg;
    };
}


#endif //SMB_ETYPES_HPP

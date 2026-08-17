#ifndef SMB_SECURITY_ASSETVALIDATION_HPP
#define SMB_SECURITY_ASSETVALIDATION_HPP

#include <cstdint>
#include <vector>

#include <bgfx/bgfx.h>

namespace AetherEngine::security {
    [[nodiscard]] auto validateShaderContainer(
        const std::vector<std::uint8_t>& bytes,
        char expectedStage,
        bgfx::RendererType::Enum renderer
    ) -> bool;

    [[nodiscard]] auto validateKtxPayload(const std::vector<std::uint8_t>& bytes) -> bool;
}

#endif // SMB_SECURITY_ASSETVALIDATION_HPP

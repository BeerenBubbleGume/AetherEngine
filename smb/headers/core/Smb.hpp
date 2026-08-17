//
// Created by drhaz on 22.06.2026.
//

#ifndef SMB_SMB_HPP
#define SMB_SMB_HPP

#include "core/IApplication.hpp"

namespace smb {
    class SMB final : public AetherEngine::core::IApplication {
    public:
        struct SMBDeleter {
            void operator()(const SMB* ptr) const { delete ptr; }
        };
        using SMBPtr = std::unique_ptr<SMB, SMBDeleter>;
        SMB(const SMB&) = delete;
        SMB& operator=(const SMB&) = delete;
        SMB(SMB&&) = delete;
        SMB& operator=(SMB&&) = delete;

        auto init(AetherEngine::core::EngineContext& ctx) -> std::expected<void, AetherEngine::core::EngineError> override;
        auto update(float dt, AetherEngine::core::EngineContext& ctx) -> void override;
        auto render(AetherEngine::core::EngineContext& ctx) -> void override;

        auto run() -> void;

        static auto createSMB() -> SMBPtr;

    private:
        SMB() = default;
        ~SMB() override;

        std::optional<AetherEngine::scene::Entity> playerEntity {std::nullopt};
    };
} // smb

#endif //SMB_SMB_HPP

//
// Created by drhaz on 03.07.2026.
//

#ifndef SMB_PHYSICSSYSTEM_HPP
#define SMB_PHYSICSSYSTEM_HPP
#include <expected>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <span>
#include <vector>

#include <entt/entt.hpp>

#include "components/PhysicsComponents.hpp"
#include "components/TransformComponent.hpp"
#include "physics/PhysicsTypes.hpp"
#include "scene/Scene.hpp"

namespace AetherEngine::physics {
    class IPhysicsBackend;
}
namespace AetherEngine::logging { class Logger; }

namespace AetherEngine::systems {
    using PhysicsError = physics::PhysicsError;
    struct PhysicsBodyError {
        scene::EntityId entityId;
        std::string entityName;
        PhysicsError error;
    };

    class PhysicsSystem final {
    public:
        struct PhysicsSystemDeleter {
            void operator()(PhysicsSystem* system) const;
        };
        using PhysicsSystemPtr = std::unique_ptr<PhysicsSystem, PhysicsSystemDeleter>;

        [[nodiscard]] static PhysicsSystemPtr createPhysicsSystem(logging::Logger* logger = nullptr);
        [[nodiscard]] auto init() const -> std::expected<void, PhysicsError>;
        auto shutdown() -> void;

        auto fixedUpdate(scene::Scene& scene, float fixedDelta) -> void;
        auto clearSceneBodies() noexcept -> void;
        [[nodiscard]] auto errors() const noexcept -> std::span<const PhysicsBodyError> { return m_errors; }
    private:
        auto syncActorsFromScene(scene::Scene& scene) -> void;
        auto syncSceneFromActors(scene::Scene& scene) -> void;

        auto createMissingActors(scene::Scene& scene) -> void;
        auto createActorForEntity(scene::Scene& scene, scene::Entity entity) -> void;
        auto destroyRemovedActors(scene::Scene& scene) -> void;
        auto reportCreationFailure(scene::Scene& scene, scene::Entity entity, const physics::BodyDesc& desc, PhysicsError error) -> void;

        std::unique_ptr<physics::IPhysicsBackend> m_backend;
        std::unordered_map<entt::entity, physics::BodyHandle> m_actors;
        struct FailedBody {
            physics::BodyDesc description;
            PhysicsBodyError diagnostic;
        };
        std::unordered_map<entt::entity, FailedBody> m_failedBodies;
        std::vector<PhysicsBodyError> m_errors;
        logging::Logger* m_logger{};

        explicit PhysicsSystem(std::unique_ptr<physics::IPhysicsBackend> backend, logging::Logger* logger);
        ~PhysicsSystem();
    };
} // systems
// AetherEngine

#endif //SMB_PHYSICSSYSTEM_HPP

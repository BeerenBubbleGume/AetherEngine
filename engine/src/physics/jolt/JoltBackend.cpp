//
// Created by Codex on 04.08.2026.
//

#include "JoltBackend.hpp"

#include <array>
#include <cmath>
#include <mutex>
#include <thread>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace engine::physics {
    namespace {
        constexpr JPH::ObjectLayer NonMovingLayer = 0;
        constexpr JPH::ObjectLayer MovingLayer = 1;
        constexpr JPH::ObjectLayer LayerCount = 2;

        const JPH::BroadPhaseLayer NonMovingBroadPhaseLayer{0};
        const JPH::BroadPhaseLayer MovingBroadPhaseLayer{1};
        constexpr JPH::uint BroadPhaseLayerCount = 2;

        constexpr JPH::uint MaxBodies = 1024;
        constexpr JPH::uint NumBodyMutexes = 0;
        constexpr JPH::uint MaxBodyPairs = 1024;
        constexpr JPH::uint MaxContactConstraints = 1024;
        constexpr std::uint32_t TempAllocatorSize = 10 * 1024 * 1024;
        constexpr float MaxColliderDimension = 100000.0f;

        [[nodiscard]] auto isFinite(math::Vec3 value) -> bool {
            return std::isfinite(value.x) &&
                std::isfinite(value.y) &&
                std::isfinite(value.z);
        }

        [[nodiscard]] auto isValidRotation(math::Quat value) -> bool {
            if (!std::isfinite(value.w) ||
                !std::isfinite(value.x) ||
                !std::isfinite(value.y) ||
                !std::isfinite(value.z)) {
                return false;
            }

            const auto lengthSquared = value.w * value.w +
                value.x * value.x +
                value.y * value.y +
                value.z * value.z;
            return std::abs(lengthSquared - 1.0f) <= 1.0e-3f;
        }

        [[nodiscard]] auto isValidTransform(const math::Transform& transform) -> bool {
            return isFinite(transform.position) && isValidRotation(transform.rotation);
        }

        [[nodiscard]] auto toJoltPosition(math::Vec3 value) -> JPH::RVec3 {
            return {value.x, value.y, value.z};
        }

        [[nodiscard]] auto toJoltRotation(math::Quat value) -> JPH::Quat {
            return {value.x, value.y, value.z, value.w};
        }

        [[nodiscard]] auto fromJoltPosition(JPH::RVec3Arg value) -> math::Vec3 {
            return {
                static_cast<float>(value.GetX()),
                static_cast<float>(value.GetY()),
                static_cast<float>(value.GetZ())
            };
        }

        [[nodiscard]] auto fromJoltRotation(JPH::QuatArg value) -> math::Quat {
            return {value.GetW(), value.GetX(), value.GetY(), value.GetZ()};
        }

        [[nodiscard]] auto validDimension(float value) -> bool {
            return std::isfinite(value) &&
                value > 0.0f &&
                value <= MaxColliderDimension;
        }

        auto registerDefaultAllocatorOnce() -> void {
            static std::once_flag allocatorRegistration;
            std::call_once(allocatorRegistration, [] {
                JPH::RegisterDefaultAllocator();
            });
        }
    }

    class JoltBackend::BroadPhaseLayerInterface final
        : public JPH::BroadPhaseLayerInterface {
    public:
        BroadPhaseLayerInterface() {
            m_objectToBroadPhase[NonMovingLayer] = NonMovingBroadPhaseLayer;
            m_objectToBroadPhase[MovingLayer] = MovingBroadPhaseLayer;
        }

        [[nodiscard]] auto GetNumBroadPhaseLayers() const -> JPH::uint override {
            return BroadPhaseLayerCount;
        }

        [[nodiscard]] auto GetBroadPhaseLayer(
            JPH::ObjectLayer layer
        ) const -> JPH::BroadPhaseLayer override {
            return layer < LayerCount
                ? m_objectToBroadPhase[layer]
                : NonMovingBroadPhaseLayer;
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        [[nodiscard]] auto GetBroadPhaseLayerName(
            JPH::BroadPhaseLayer layer
        ) const -> const char* override {
            if (layer == NonMovingBroadPhaseLayer) {
                return "NON_MOVING";
            }
            if (layer == MovingBroadPhaseLayer) {
                return "MOVING";
            }
            return "INVALID";
        }
#endif

    private:
        std::array<JPH::BroadPhaseLayer, LayerCount> m_objectToBroadPhase;
    };

    class JoltBackend::ObjectVsBroadPhaseLayerFilter final
        : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        [[nodiscard]] auto ShouldCollide(
            JPH::ObjectLayer objectLayer,
            JPH::BroadPhaseLayer broadPhaseLayer
        ) const -> bool override {
            return objectLayer == NonMovingLayer
                ? broadPhaseLayer == MovingBroadPhaseLayer
                : objectLayer == MovingLayer;
        }
    };

    class JoltBackend::ObjectLayerPairFilter final
        : public JPH::ObjectLayerPairFilter {
    public:
        [[nodiscard]] auto ShouldCollide(
            JPH::ObjectLayer first,
            JPH::ObjectLayer second
        ) const -> bool override {
            return first == NonMovingLayer
                ? second == MovingLayer
                : first == MovingLayer;
        }
    };

    JoltBackend::JoltBackend() = default;

    JoltBackend::~JoltBackend() {
        shutdown();
    }

    auto JoltBackend::init() -> std::expected<void, PhysicsError> {
        if (m_physicsSystem) {
            return {};
        }

        registerDefaultAllocatorOnce();
        if (!JPH::Factory::sInstance) {
            JPH::Factory::sInstance = new JPH::Factory();
            JPH::RegisterTypes();
            m_ownsFactory = true;
        }

        m_broadPhaseLayerInterface = std::make_unique<BroadPhaseLayerInterface>();
        m_objectVsBroadPhaseLayerFilter =
            std::make_unique<ObjectVsBroadPhaseLayerFilter>();
        m_objectLayerPairFilter = std::make_unique<ObjectLayerPairFilter>();
        m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(TempAllocatorSize);

        const auto availableThreads = std::thread::hardware_concurrency();
        const auto workerThreads = availableThreads > 1 ? availableThreads - 1 : 1;
        m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            static_cast<int>(workerThreads)
        );

        m_physicsSystem = std::make_unique<JPH::PhysicsSystem>();
        m_physicsSystem->Init(
            MaxBodies,
            NumBodyMutexes,
            MaxBodyPairs,
            MaxContactConstraints,
            *m_broadPhaseLayerInterface,
            *m_objectVsBroadPhaseLayerFilter,
            *m_objectLayerPairFilter
        );
        m_physicsSystem->SetGravity({0.0f, -9.81f, 0.0f});

        return {};
    }

    auto JoltBackend::shutdown() -> void {
        if (m_physicsSystem) {
            auto& bodyInterface = m_physicsSystem->GetBodyInterface();
            for (const auto& [handle, bodyId] : m_bodies) {
                static_cast<void>(handle);
                bodyInterface.RemoveBody(bodyId);
                bodyInterface.DestroyBody(bodyId);
            }
        }
        m_bodies.clear();

        m_physicsSystem.reset();
        m_jobSystem.reset();
        m_tempAllocator.reset();
        m_objectLayerPairFilter.reset();
        m_objectVsBroadPhaseLayerFilter.reset();
        m_broadPhaseLayerInterface.reset();

        if (m_ownsFactory) {
            JPH::UnregisterTypes();
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
            m_ownsFactory = false;
        }

        m_nextBodyHandle = 1;
    }

    auto JoltBackend::createBody(const BodyDesc& desc) -> BodyHandle {
        if (!m_physicsSystem || !isValidTransform(desc.transform)) {
            return {};
        }
        if (desc.dynamic && (!std::isfinite(desc.mass) || desc.mass <= 0.0f)) {
            return {};
        }

        const auto shape = createShape(desc.collider);
        if (!shape) {
            return {};
        }

        JPH::BodyCreationSettings settings{
            shape,
            toJoltPosition(desc.transform.position),
            toJoltRotation(desc.transform.rotation),
            desc.dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
            desc.dynamic ? MovingLayer : NonMovingLayer
        };
        settings.mFriction = 0.5f;
        settings.mRestitution = 0.5f;
        settings.mIsSensor = desc.collider.trigger;
        settings.mGravityFactor = desc.useGravity ? 1.0f : 0.0f;
        if (desc.dynamic) {
            settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
            settings.mMassPropertiesOverride.mMass = desc.mass;
        }

        auto& bodyInterface = m_physicsSystem->GetBodyInterface();
        const auto bodyId = bodyInterface.CreateAndAddBody(
            settings,
            desc.dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate
        );
        if (bodyId.IsInvalid()) {
            return {};
        }

        const auto handle = allocateBodyHandle();
        if (!handle.isValid()) {
            bodyInterface.RemoveBody(bodyId);
            bodyInterface.DestroyBody(bodyId);
            return {};
        }

        m_bodies.emplace(handle.value, bodyId);
        return handle;
    }

    auto JoltBackend::destroyBody(BodyHandle handle) -> void {
        const auto bodyIt = m_bodies.find(handle.value);
        if (!m_physicsSystem || bodyIt == m_bodies.end()) {
            return;
        }

        auto& bodyInterface = m_physicsSystem->GetBodyInterface();
        bodyInterface.RemoveBody(bodyIt->second);
        bodyInterface.DestroyBody(bodyIt->second);
        m_bodies.erase(bodyIt);
    }

    auto JoltBackend::setTransform(
        BodyHandle handle,
        const math::Transform& transform
    ) -> void {
        const auto bodyIt = m_bodies.find(handle.value);
        if (!m_physicsSystem ||
            bodyIt == m_bodies.end() ||
            !isValidTransform(transform)) {
            return;
        }

        m_physicsSystem->GetBodyInterface().SetPositionAndRotation(
            bodyIt->second,
            toJoltPosition(transform.position),
            toJoltRotation(transform.rotation),
            JPH::EActivation::Activate
        );
    }

    auto JoltBackend::getTransform(BodyHandle handle) const -> math::Transform {
        const auto bodyIt = m_bodies.find(handle.value);
        if (!m_physicsSystem || bodyIt == m_bodies.end()) {
            return {};
        }

        JPH::RVec3 position;
        JPH::Quat rotation;
        m_physicsSystem->GetBodyInterface().GetPositionAndRotation(
            bodyIt->second,
            position,
            rotation
        );

        math::Transform transform;
        transform.position = fromJoltPosition(position);
        transform.rotation = fromJoltRotation(rotation);
        return transform;
    }

    auto JoltBackend::simulate(float fixedDelta) -> void {
        if (!m_physicsSystem ||
            !m_tempAllocator ||
            !m_jobSystem ||
            !std::isfinite(fixedDelta) ||
            fixedDelta <= 0.0f) {
            return;
        }

        constexpr int CollisionSteps = 1;
        m_physicsSystem->Update(
            fixedDelta,
            CollisionSteps,
            m_tempAllocator.get(),
            m_jobSystem.get()
        );
    }

    auto JoltBackend::createShape(
        const components::ColliderComponent& collider
    ) -> JPH::ShapeRefC {
        JPH::ShapeSettings::ShapeResult result;
        switch (collider.type) {
            case components::ColliderType::Box:
                if (!validDimension(collider.size.x) ||
                    !validDimension(collider.size.y) ||
                    !validDimension(collider.size.z)) {
                    return {};
                }
                result = JPH::BoxShapeSettings{
                    {collider.size.x, collider.size.y, collider.size.z}
                }.Create();
                break;

            case components::ColliderType::Sphere:
                if (!validDimension(collider.radius)) {
                    return {};
                }
                result = JPH::SphereShapeSettings{collider.radius}.Create();
                break;

            case components::ColliderType::Capsule:
                if (!validDimension(collider.radius) ||
                    !validDimension(collider.height)) {
                    return {};
                }
                result = JPH::CapsuleShapeSettings{
                    collider.height * 0.5f,
                    collider.radius
                }.Create();
                break;

            default:
                return {};
        }

        return result.HasError() ? JPH::ShapeRefC{} : result.Get();
    }

    auto JoltBackend::allocateBodyHandle() -> BodyHandle {
        if (m_nextBodyHandle == 0) {
            m_nextBodyHandle = 1;
        }

        while (m_bodies.contains(m_nextBodyHandle)) {
            ++m_nextBodyHandle;
            if (m_nextBodyHandle == 0) {
                m_nextBodyHandle = 1;
            }
        }

        return BodyHandle{m_nextBodyHandle++};
    }
}

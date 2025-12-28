
#include "jolt_impl.hpp"
#include <thread>
#include <algorithm>
#include "../physics.hpp" // For PhysicsDebugDrawer

// Jolt Debug Renderer Implementation
class JoltDebugRendererImpl : public JPH::DebugRenderer
{
public:
    JoltDebugRendererImpl() {
        Initialize();
    }

    virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
        glm::vec3 p1(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
        glm::vec3 p2(inTo.GetX(), inTo.GetY(), inTo.GetZ());
        glm::vec3 color(inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f);
        PhysicsDebugDrawer::DrawLine(p1, p2, color);
    }

    virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off) override {
        glm::vec3 p1(inV1.GetX(), inV1.GetY(), inV1.GetZ());
        glm::vec3 p2(inV2.GetX(), inV2.GetY(), inV2.GetZ());
        glm::vec3 p3(inV3.GetX(), inV3.GetY(), inV3.GetZ());
        glm::vec3 color(inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f);
        PhysicsDebugDrawer::DrawTriangle(p1, p2, p3, color);
    }

    virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor, float inHeight = 0.5f) override {
        // Not implemented yet
    }

    class BatchImpl : public JPH::RefTargetVirtual
    {
    public:
        virtual void AddRef() override { mRefCount++; }
        virtual void Release() override { if (--mRefCount == 0) delete this; }

        BatchImpl(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount)
        {
            mTriangles.assign(inTriangles, inTriangles + inTriangleCount);
        }

        std::vector<JPH::DebugRenderer::Triangle> mTriangles;

    private:
        std::atomic<uint32> mRefCount = 0;
    };

    virtual Batch CreateTriangleBatch(const Triangle *inTriangles, int inTriangleCount) override {
        return new BatchImpl(inTriangles, inTriangleCount);
    }

    virtual Batch CreateTriangleBatch(const Vertex *inVertices, int inVertexCount, const uint32 *inIndices, int inIndexCount) override {
        std::vector<Triangle> triangles;
        triangles.reserve(inIndexCount / 3);
        for (int i = 0; i < inIndexCount; i += 3) {
            Triangle t;
            t.mV[0] = inVertices[inIndices[i]];
            t.mV[1] = inVertices[inIndices[i+1]];
            t.mV[2] = inVertices[inIndices[i+2]];
            triangles.push_back(t);
        }
        return new BatchImpl(triangles.data(), (int)triangles.size());
    }

    virtual void DrawGeometry(JPH::RMat44Arg inModelMatrix, const AABox &inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef &inGeometry, ECullMode inCullMode = ECullMode::CullBackFace, ECastShadow inCastShadow = ECastShadow::Off, EDrawMode inDrawMode = EDrawMode::Solid) override {
        // Get the geometry
        const Geometry *geo = inGeometry.GetPtr();

        // Simple LOD selection: just pick the highest detail (LOD 0)
        // In a real implementation we would check inLODScaleSq against geo->mLODs[i].mDistanceSq
        if (geo->mLODs.empty()) return;

        const LOD &lod = geo->mLODs[0];

        // Get our batch implementation
        const BatchImpl* batch = static_cast<const BatchImpl*>(lod.mTriangleBatch.GetPtr());

        glm::vec3 color(inModelColor.r / 255.0f, inModelColor.g / 255.0f, inModelColor.b / 255.0f);

        for (const auto& tri : batch->mTriangles) {
            // Transform vertices manually to be safe
            auto transform = [&](const JPH::Float3& v) -> glm::vec3 {
                // Convert Float3 to Vec3 for multiplication
                JPH::Vec3 jv(v.x, v.y, v.z);
                JPH::RVec3 rv = inModelMatrix * jv;
                return glm::vec3(rv.GetX(), rv.GetY(), rv.GetZ());
            };

            glm::vec3 v1 = transform(tri.mV[0].mPosition);
            glm::vec3 v2 = transform(tri.mV[1].mPosition);
            glm::vec3 v3 = transform(tri.mV[2].mPosition);

            PhysicsDebugDrawer::DrawTriangle(v1, v2, v3, color);
        }
    }
};

Jolt_Impl::Jolt_Impl()
	: m_bootstrap()
	, m_temp_allocator_impl(10 * 1024 * 1024)
	, m_job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers,
				   std::max(1u, std::thread::hardware_concurrency() > 0
								 ? std::thread::hardware_concurrency() - 1
								 : 1))
{
}

void Jolt_Impl::Init() {
     // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodies = 1024;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	const uint cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodyPairs = 1024;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
	const uint cMaxContactConstraints = 1024;

	// Now we can create the actual physics system.
	m_physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_broad_phase_layer_interface, m_object_vs_broadphase_layer_filter, m_object_vs_object_layer_filter);

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	m_physics_system.SetBodyActivationListener(&m_body_activation_listener);

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	m_physics_system.SetContactListener(&m_contact_listener);

    // Initialize Debug Renderer
    m_debug_renderer = new JoltDebugRendererImpl();
}

void Jolt_Impl::Destroy() {
    // Unregisters all types with the factory and cleans up the default material
    // Note: This is handled by JoltBootstrap destructor in this implementation
    if (m_debug_renderer) {
        delete m_debug_renderer;
        m_debug_renderer = nullptr;
    }
}

void Jolt_Impl::Step(float dt) {
	// We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
	// const float cDeltaTime = 1.0f / 60.0f;

	// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
	const int cCollisionSteps = 1;

	// Step the world
	m_physics_system.Update(dt, cCollisionSteps, &m_temp_allocator_impl, &m_job_system);
}

void Jolt_Impl::DrawDebug() {
    if (m_debug_renderer) {
        JPH::BodyManager::DrawSettings settings;
        settings.mDrawShape = true;
        settings.mDrawBoundingBox = true; // Optional: Draw AABBs
        m_physics_system.DrawBodies(settings, m_debug_renderer);
    }
}

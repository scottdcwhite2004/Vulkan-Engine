#pragma once
#include <glm/glm.hpp>
#include <Mesh.h>
#include <Material.h>
#include <textureManager.h>

// Non-copyable interface base for scene objects sharing Mesh/Material lifecycle and rendering.
class IWorldObject
{
    glm::vec3 _position{};
    Material _material{ glm::vec4(1.0f), 0.0f, 1.0f, nullptr };
    Mesh _mesh{ glm::vec3(0.0f), _material, "" };

    textureManager* _textureMgr{ nullptr };
    Texture* _texture{ nullptr };

    // NEW: flag to mark this object as a post-process target (rendered into a mask)
    bool _usePostProcess{ false };

protected:
    // Derived classes pass position, material, and modelPath to wire up mesh
    explicit IWorldObject(const glm::vec3& position,
        const std::string& modelPath,
        textureManager* textureMgr,
        const char* textureKeyOrNull)
        : _position(position)
        , _material(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 1.0f, nullptr)
        , _mesh(position,
            Material(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 1.0f,
                (textureMgr != nullptr && textureKeyOrNull != nullptr)
                ? textureMgr->getTexture(textureKeyOrNull)
                : nullptr),
            modelPath)
        , _textureMgr(textureMgr)
        , _texture((textureMgr != nullptr && textureKeyOrNull != nullptr)
            ? textureMgr->getTexture(textureKeyOrNull)
            : nullptr)
    {
        _material.setTexture(_texture);
        _mesh.create();
    }

    // Copy support for derived classes
    IWorldObject(const IWorldObject& other)
        : _position(other._position)
        , _material(other._material)
        , _mesh(other._position, other._material, other._mesh.getModelPath())
        , _textureMgr(other._textureMgr)
        , _texture(other._texture)
        , _usePostProcess(other._usePostProcess)
    {
        _material.setTexture(_texture);
        _mesh.create();
    }

    IWorldObject& operator=(const IWorldObject& other)
    {
        if (this == &other) return *this;
        _position = other._position;

        const Material materialCopy = other._material;
        _material = materialCopy;

        _mesh = Mesh(other._position, materialCopy, other._mesh.getModelPath());

        _textureMgr = other._textureMgr;
        _texture = other._texture;

        _usePostProcess = other._usePostProcess;

        _material.setTexture(_texture);
        _mesh.create();
        return *this;
    }

    IWorldObject(IWorldObject&& other) noexcept
        : _position(std::move(other._position))
        , _material(std::move(other._material))
        , _mesh(other._position, other._material, other._mesh.getModelPath())
        , _textureMgr(other._textureMgr)
        , _texture(other._texture)
        , _usePostProcess(other._usePostProcess)
    {
        other._textureMgr = nullptr;
        other._texture = nullptr;
        other._usePostProcess = false;
    }

    IWorldObject& operator=(IWorldObject&& other) noexcept
    {
        if (this == &other) return *this;

        _position = std::move(other._position);
        Material materialMoved = std::move(other._material);
        _material = std::move(materialMoved);

        _mesh = Mesh(_position, _material, other._mesh.getModelPath());

        _textureMgr = other._textureMgr;
        _texture = other._texture;

        _usePostProcess = other._usePostProcess;

        _material.setTexture(_texture);

        other._textureMgr = nullptr;
        other._texture = nullptr;
        other._usePostProcess = false;
        return *this;
    }

public:
    virtual ~IWorldObject();

    // Common rendering lifecycle
    virtual void draw(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout layout, uint32_t currentFrame);

    virtual void upload(const RenderContext& ctx, uint32_t framesInFlight,
        VkImageView textureImageView, VkSampler textureSampler,
        const std::vector<VkDescriptorBufferInfo>& lightingBufferInfos);

    virtual void destroy(const RenderContext& ctx);

    virtual void updateUniformBuffer(uint32_t frameIndex,
        const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj) const;

    // Optional hook for per-frame updates
    virtual void update(float& /*deltaTime*/);

    // Accessors
    const glm::vec3 position() const { return _position; }
    void setPosition(const glm::vec3& pos) { _position = pos; }

    const Mesh mesh() const { return _mesh; }
    const Material material() const { return _material; }
    Texture* texture() const { return _texture; }
    textureManager* textureMgr() const { return _textureMgr; }

    // NEW: post-process usage flag
    bool usesPostProcess() const { return _usePostProcess; }
    void setUsesPostProcess(bool enable) { _usePostProcess = enable; }
};


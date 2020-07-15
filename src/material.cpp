#include <glm/glm.hpp>

// TODO: Refactor
struct MaterialBufferObject
{
    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;
    alignas(4) glm::vec1 shininess;

    MaterialBufferObject()
    {
        ambient = glm::vec3(1.0f, 0.5f, 0.31f);
        diffuse = glm::vec3(1.0f, 0.5f, 0.31);
        specular = glm::vec3(0.5f, 0.5f, 0.5);
        shininess = glm::vec1(8.0f);
    }
};

class Material
{
  private:
    std::unique_ptr<core::Buffer> buffer;

  public:
    Material() {}

    Material(std::shared_ptr<core::Device> device)
    {
        buffer = std::make_unique<core::Buffer>(device, sizeof(MaterialBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);
    }

    Material(std::shared_ptr<core::Device> device, MaterialBufferObject material)
    {
        buffer = std::make_unique<core::Buffer>(device, sizeof(MaterialBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);
        update(material);
    }

    void update(MaterialBufferObject material)
    {
        buffer->map(0, sizeof(material));
        buffer->copy(&material, sizeof(material));
        buffer->unmap();
    }

    // TODO: Not used. Remove ?
    vk::Buffer getBuffer()
    {
        return buffer->buffer.get();
    }

    vk::DescriptorBufferInfo getBufferDescriptor()
    {
        return buffer->getDescriptor();
    }
};

#ifndef AGPU_VULKAN_SHADER_RESOURCE_BINDING_HPP
#define AGPU_VULKAN_SHADER_RESOURCE_BINDING_HPP

#include "device.hpp"
#include "shader_signature_builder.hpp"

namespace AgpuVulkan
{

class AVkShaderResourceBinding : public agpu::shader_resource_binding
{
public:
    AVkShaderResourceBinding(const agpu::device_ref &device);
    ~AVkShaderResourceBinding();

    static agpu::shader_resource_binding_ref create(const agpu::device_ref &device, const agpu::shader_signature_ref &signature, agpu_uint elementIndex, VkDescriptorSet descriptorSet, const ShaderSignatureElementDescription &elementDescription);

    virtual agpu_error bindUniformBuffer(agpu_int location, const agpu::buffer_ref &uniform_buffer) override;
    virtual agpu_error bindUniformBufferRange(agpu_int location, const agpu::buffer_ref &uniform_buffer, agpu_size offset, agpu_size size) override;
    virtual agpu_error bindStorageBuffer(agpu_int location, const agpu::buffer_ref &storage_buffer) override;
    virtual agpu_error bindStorageBufferRange(agpu_int location, const agpu::buffer_ref &storage_buffer, agpu_size offset, agpu_size size) override;
	virtual agpu_error bindSampledTextureView(agpu_int location, const agpu::texture_view_ref &view) override;
	virtual agpu_error bindStorageImageView(agpu_int location, const agpu::texture_view_ref &view) override;
	virtual agpu_error bindSampler(agpu_int location, const agpu::sampler_ref &sampler) override;

    agpu::device_ref device;
    agpu::shader_signature_ref signature;
    agpu_uint elementIndex;
    VkDescriptorSet descriptorSet;
    const ShaderSignatureElementDescription *bindingDescription;
};

} // End of namespace AgpuVulkan

#endif //AGPU_VULKAN_SHADER_RESOURCE_BINDING_HPP

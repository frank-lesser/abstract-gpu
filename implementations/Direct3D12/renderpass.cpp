#include "renderpass.hpp"
#include "texture_formats.hpp"

namespace AgpuD3D12
{

ADXRenderPass::ADXRenderPass(const agpu::device_ref &cdevice)
    : device(cdevice), hasDepth(false), hasStencil(false)
{
    memset(&depthStencilAttachment, 0, sizeof(depthStencilAttachment));
}

ADXRenderPass::~ADXRenderPass()
{
}

agpu::renderpass_ref ADXRenderPass::create(const agpu::device_ref &device, agpu_renderpass_description *description)
{
    if (!description)
        return agpu::renderpass_ref();

    auto result = agpu::makeObject<ADXRenderPass> (device);
    auto adxRenderpass = result.as<ADXRenderPass> ();

    adxRenderpass->colorAttachments.reserve(description->color_attachment_count);
    for (size_t i = 0; i < description->color_attachment_count; ++i)
    {
        adxRenderpass->colorAttachments.push_back(description->color_attachments[i]);
    }

    if (description->depth_stencil_attachment)
    {
        adxRenderpass->depthStencilAttachment = *description->depth_stencil_attachment;
        adxRenderpass->hasDepth = hasDepthComponent(adxRenderpass->depthStencilAttachment.format);
        adxRenderpass->hasStencil = hasStencilComponent(adxRenderpass->depthStencilAttachment.format);
    }

    return result;
}

agpu_error ADXRenderPass::setDepthStencilClearValue(agpu_depth_stencil_value value)
{
    depthStencilAttachment.clear_value = value;
    return AGPU_OK;
}

agpu_error ADXRenderPass::setColorClearValue(agpu_uint attachment_index, agpu_color4f value)
{
    if (attachment_index >= colorAttachments.size())
        return AGPU_OUT_OF_BOUNDS;

    colorAttachments[attachment_index].clear_value = value;
    return AGPU_OK;
}

agpu_error ADXRenderPass::setColorClearValueFrom(agpu_uint attachment_index, agpu_color4f* value)
{
    CHECK_POINTER(value);
    return setColorClearValue(attachment_index, *value);
}

agpu_error ADXRenderPass::getColorAttachmentFormats(agpu_uint* color_attachment_count, agpu_texture_format* formats)
{
    CHECK_POINTER(color_attachment_count);
    if(formats)
    {
        if(*color_attachment_count < this->colorAttachments.size())
            return AGPU_INVALID_PARAMETER;

        for(agpu_uint i = 0; i < this->colorAttachments.size(); ++i)
            formats[i] = this->colorAttachments[i].format;
    }

    *color_attachment_count = this->colorAttachments.size();
    return AGPU_OK;
}

agpu_texture_format ADXRenderPass::getDepthStencilAttachmentFormat()
{
    return depthStencilAttachment.format;
}

} // End of namespace AgpuD3D12

#include "command_allocator.hpp"
#include "command_list.hpp"
#include "common_commands.hpp"
#include "pipeline_state.hpp"
#include "buffer.hpp"
#include "vertex_binding.hpp"
#include "shader_signature.hpp"
#include "shader_resource_binding.hpp"
#include "framebuffer.hpp"
#include "renderpass.hpp"
#include "texture.hpp"

namespace AgpuD3D12
{

inline D3D_PRIMITIVE_TOPOLOGY mapPrimitiveTopology(agpu_primitive_topology topology)
{
    switch (topology)
    {
    case AGPU_POINTS: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case AGPU_LINES: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case AGPU_LINES_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
    case AGPU_LINE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case AGPU_LINE_STRIP_ADJACENCY:return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
    case AGPU_TRIANGLES: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case AGPU_TRIANGLES_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
    case AGPU_TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    case AGPU_TRIANGLE_STRIP_ADJACENCY: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
    default:
        abort();
    }
}

ADXCommandList::ADXCommandList(const agpu::device_ref &cdevice)
    : device(cdevice)
{
}

ADXCommandList::~ADXCommandList()
{
}

agpu::command_list_ref ADXCommandList::create(const agpu::device_ref &device, agpu_command_list_type type, const agpu::command_allocator_ref &allocator, const agpu::pipeline_state_ref &initialState)
{
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ID3D12PipelineState *state = nullptr;
    if (initialState)
    {
        state = initialState.as<ADXPipelineState> ()->state.Get();
    }

    if (FAILED(deviceForDX->d3dDevice->CreateCommandList(0, mapCommandListType(type), allocator.as<ADXCommandAllocator> ()->allocator.Get(), state, IID_PPV_ARGS(&commandList))))
        return agpu::command_list_ref();;

    if (initialState)
    {
        auto adxInitialState = initialState.as<ADXPipelineState> ();
        commandList->IASetPrimitiveTopology(mapPrimitiveTopology(adxInitialState->primitiveTopology));
    }

    auto result = agpu::makeObject<ADXCommandList> (device);
    auto adxList = result.as<ADXCommandList> ();
    adxList->type = type;
    adxList->commandList = commandList;
    if (adxList->setCommonState() < 0)
    {
        return agpu::command_list_ref();
    }

    return result;
}

agpu_error ADXCommandList::setShaderSignature(const agpu::shader_signature_ref &signature)
{
    CHECK_POINTER(signature);
    ID3D12DescriptorHeap *heaps[2];
    int heapCount = 0;
    auto adxSignature = signature.as<ADXShaderSignature> ();
    commandList->SetGraphicsRootSignature(adxSignature->rootSignature.Get());

    if (adxSignature->shaderResourceViewHeap)
        heaps[heapCount++] = adxSignature->shaderResourceViewHeap.Get();
    if (adxSignature->samplerHeap)
        heaps[heapCount++] = adxSignature->samplerHeap.Get();
	if (heapCount > 0)
	{
		commandList->SetDescriptorHeaps(heapCount, heaps);
		for (size_t i = 0; i < adxSignature->nullDescriptorTables.size(); ++i)
			commandList->SetGraphicsRootDescriptorTable(i, adxSignature->nullDescriptorTables[i]);
	}

    return AGPU_OK;
}

agpu_error ADXCommandList::setCommonState()
{
    currentFramebuffer.reset();

    return AGPU_OK;
}

agpu_error ADXCommandList::setViewport(agpu_int x, agpu_int y, agpu_int w, agpu_int h)
{
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = (FLOAT)x;
    viewport.TopLeftY = (FLOAT)y;
    viewport.Width = (FLOAT)w;
    viewport.Height = (FLOAT)h;
    viewport.MinDepth = 0.0;
    viewport.MaxDepth = 1.0;
    commandList->RSSetViewports(1, &viewport);
    return AGPU_OK;
}

agpu_error ADXCommandList::setScissor(agpu_int x, agpu_int y, agpu_int w, agpu_int h)
{
    RECT rect;
    rect.top = x;
    rect.left = y;
    rect.right = x + w;
    rect.bottom = y + h;
    commandList->RSSetScissorRects(1, &rect);
    return AGPU_OK;
}

agpu_error ADXCommandList::usePipelineState(const agpu::pipeline_state_ref &pipeline)
{
    CHECK_POINTER(pipeline);
    auto adxPipeline = pipeline.as<ADXPipelineState> ();
    commandList->SetPipelineState(adxPipeline->state.Get());
    commandList->IASetPrimitiveTopology(mapPrimitiveTopology(adxPipeline->primitiveTopology));

    return AGPU_OK;
}

agpu_error ADXCommandList::useVertexBinding(const agpu::vertex_binding_ref &vertex_binding)
{
    CHECK_POINTER(vertex_binding);
    auto adxVertexBinding = vertex_binding.as<ADXVertexBinding> ();
    commandList->IASetVertexBuffers(0, (UINT)adxVertexBinding->vertexBuffers.size(), &adxVertexBinding->vertexBufferViews[0]);
    return AGPU_OK;
}

agpu_error ADXCommandList::useIndexBuffer(const agpu::buffer_ref &index_buffer)
{
    CHECK_POINTER(index_buffer);
    auto adxIndexBuffer = index_buffer.as<ADXBuffer> ();
    return useIndexBufferAt(index_buffer, 0, adxIndexBuffer->description.stride);
}

agpu_error ADXCommandList::useIndexBufferAt(const agpu::buffer_ref &index_buffer, agpu_size offset, agpu_size index_size)
{
    CHECK_POINTER(index_buffer);
    auto adxIndexBuffer = index_buffer.as<ADXBuffer> ();
    if ((adxIndexBuffer->description.binding & AGPU_ELEMENT_ARRAY_BUFFER) == 0)
        return AGPU_INVALID_PARAMETER;

    D3D12_INDEX_BUFFER_VIEW view = {};
    auto error = adxIndexBuffer->createIndexBufferView(&view, offset, index_size);
    if(error) return error;

    commandList->IASetIndexBuffer(&view);
    return AGPU_OK;
}

agpu_error ADXCommandList::useDrawIndirectBuffer(const agpu::buffer_ref &draw_buffer)
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error ADXCommandList::useComputeDispatchIndirectBuffer(const agpu::buffer_ref & buffer)
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error ADXCommandList::useShaderResources(const agpu::shader_resource_binding_ref &binding)
{
    CHECK_POINTER(binding);

    auto adxBinding = binding.as<ADXShaderResourceBinding> ();
	commandList->SetGraphicsRootDescriptorTable(adxBinding->bankIndex, adxBinding->gpuDescriptorTableHandle);
    return AGPU_OK;
}

agpu_error ADXCommandList::useComputeShaderResources(const agpu::shader_resource_binding_ref &binding)
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error ADXCommandList::drawArrays(agpu_uint vertex_count, agpu_uint instance_count, agpu_uint first_vertex, agpu_uint base_instance)
{
    commandList->DrawInstanced(vertex_count, instance_count, first_vertex, base_instance);
    return AGPU_OK;
}

agpu_error ADXCommandList::drawArraysIndirect(agpu_size offset, agpu_size drawcount)
{
    return AGPU_OK;
}

agpu_error ADXCommandList::drawElements(agpu_uint index_count, agpu_uint instance_count, agpu_uint first_index, agpu_int base_vertex, agpu_uint base_instance)
{
    commandList->DrawIndexedInstanced(index_count, instance_count, first_index, base_vertex, base_instance);
    return AGPU_OK;
}

agpu_error ADXCommandList::drawElementsIndirect(agpu_size offset, agpu_size drawcount)
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error ADXCommandList::dispatchCompute(agpu_uint group_count_x, agpu_uint group_count_y, agpu_uint group_count_z)
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error ADXCommandList::dispatchComputeIndirect(agpu_size offset)
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error ADXCommandList::setStencilReference(agpu_uint reference)
{
    commandList->OMSetStencilRef(reference);
    return AGPU_OK;
}

agpu_error ADXCommandList::executeBundle(const agpu::command_list_ref &bundle)
{
    CHECK_POINTER(bundle);
    auto adxBundle = bundle.as<ADXCommandList> ();
    if (adxBundle->type != AGPU_COMMAND_LIST_TYPE_BUNDLE)
        return AGPU_INVALID_PARAMETER;

    commandList->ExecuteBundle(adxBundle->commandList.Get());
    return AGPU_OK;
}

agpu_error ADXCommandList::close()
{
    ERROR_IF_FAILED(commandList->Close());
    return AGPU_OK;
}

agpu_error ADXCommandList::reset(const agpu::command_allocator_ref &allocator, const agpu::pipeline_state_ref &initial_pipeline_state)
{
    CHECK_POINTER(allocator);

    ID3D12PipelineState *state = nullptr;
    if (initial_pipeline_state)
        state = initial_pipeline_state.as<ADXPipelineState> ()->state.Get();

    ERROR_IF_FAILED(commandList->Reset(allocator.as<ADXCommandAllocator> ()->allocator.Get(), state));

    if (initial_pipeline_state)
        commandList->IASetPrimitiveTopology(mapPrimitiveTopology(initial_pipeline_state.as<ADXPipelineState> ()->primitiveTopology));

    return setCommonState();
}

agpu_error ADXCommandList::resetBundle(const agpu::command_allocator_ref & allocator, const agpu::pipeline_state_ref & initial_pipeline_state, agpu_inheritance_info* inheritance_info)
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error ADXCommandList::beginRenderPass(const agpu::renderpass_ref &renderpass, const agpu::framebuffer_ref &framebuffer, agpu_bool secondaryContent)
{
    CHECK_POINTER(renderpass);
    CHECK_POINTER(framebuffer);
    currentFramebuffer = framebuffer;
    if (!currentFramebuffer)
        return AGPU_OK;

    // TODO: Use a more proper state depending if this is used as a texture or not.
    auto adxFramebuffer = framebuffer.as<ADXFramebuffer> ();
    auto adxRenderpass = renderpass.as<ADXRenderPass> ();
    D3D12_RESOURCE_STATES prevState = adxFramebuffer->swapChainBuffer ? D3D12_RESOURCE_STATE_PRESENT : D3D12_RESOURCE_STATE_GENERIC_READ;

    // Perform the resource transitions
    for (size_t i = 0; i < adxFramebuffer->getColorBufferCount(); ++i)
    {
        auto colorBuffer = adxFramebuffer->colorBuffers[i];
        if (!colorBuffer)
            return AGPU_ERROR;

        auto barrier = resourceTransitionBarrier(colorBuffer.as<ADXTexture> ()->gpuResource.Get(), prevState, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &barrier);
    }

    if (adxFramebuffer->depthStencilView)
    {
        auto desc = adxFramebuffer->getDepthStencilCpuHandle();
        commandList->OMSetRenderTargets((UINT)adxFramebuffer->colorBufferDescriptors.size(), &adxFramebuffer->colorBufferDescriptors[0], FALSE, &desc);
    }
    else
    {
        commandList->OMSetRenderTargets((UINT)adxFramebuffer->colorBufferDescriptors.size(), &adxFramebuffer->colorBufferDescriptors[0], FALSE, nullptr);
    }

    // Clear the color buffers
    for (size_t i = 0; i < adxFramebuffer->getColorBufferCount(); ++i)
    {
        if (!adxFramebuffer->colorBuffers[i])
            return AGPU_ERROR;

        auto handle = adxFramebuffer->getColorBufferCpuHandle(i);
        auto &attachment = adxRenderpass->colorAttachments[i];
        if(attachment.begin_action == AGPU_ATTACHMENT_CLEAR)
            commandList->ClearRenderTargetView(handle, reinterpret_cast<FLOAT*> (&attachment.clear_value), 0, nullptr);
    }

    if ((adxRenderpass->hasDepth || adxRenderpass->hasStencil) && adxRenderpass->depthStencilAttachment.begin_action == AGPU_ATTACHMENT_CLEAR)
    {
        D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAGS(0);
        if (adxRenderpass->hasDepth)
            flags |= D3D12_CLEAR_FLAG_DEPTH;
        if (adxRenderpass->hasStencil)
            flags |= D3D12_CLEAR_FLAG_STENCIL;

        auto clearDepth = adxRenderpass->depthStencilAttachment.clear_value.depth;
        auto clearStencil = adxRenderpass->depthStencilAttachment.clear_value.stencil;
        commandList->ClearDepthStencilView(adxFramebuffer->getDepthStencilCpuHandle(), flags, clearDepth, clearStencil, 0, nullptr);
    }

    return AGPU_OK;
}

agpu_error ADXCommandList::endRenderPass()
{
    if (!currentFramebuffer)
        return AGPU_OK;

    auto adxFramebuffer = currentFramebuffer.as<ADXFramebuffer> ();
    commandList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);

    // TODO: Use a more proper state depending if this is used as a texture or not.
    D3D12_RESOURCE_STATES newState = adxFramebuffer->swapChainBuffer ? D3D12_RESOURCE_STATE_PRESENT : D3D12_RESOURCE_STATE_GENERIC_READ;

    // Perform the resource transitions
    for (size_t i = 0; i < adxFramebuffer->getColorBufferCount(); ++i)
    {
        auto &colorBuffer = adxFramebuffer->colorBuffers[i];
        if (!colorBuffer)
            return AGPU_ERROR;

        auto barrier = resourceTransitionBarrier(colorBuffer.as<ADXTexture> ()->gpuResource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, newState);
        commandList->ResourceBarrier(1, &barrier);
    }

    currentFramebuffer.reset();
    return AGPU_OK;
}

agpu_error ADXCommandList::resolveFramebuffer(const agpu::framebuffer_ref &destFramebuffer, const agpu::framebuffer_ref &sourceFramebuffer)
{
    CHECK_POINTER(destFramebuffer);
    CHECK_POINTER(sourceFramebuffer);

    auto adxDestFramebuffer = destFramebuffer.as<ADXFramebuffer> ();
    auto adxSourceFramebuffer = sourceFramebuffer.as<ADXFramebuffer> ();
    if (destFramebuffer == sourceFramebuffer ||
        adxDestFramebuffer->getColorBufferCount() != adxSourceFramebuffer->getColorBufferCount())
        return AGPU_INVALID_PARAMETER;

    D3D12_RESOURCE_STATES destState = adxDestFramebuffer->swapChainBuffer ? D3D12_RESOURCE_STATE_PRESENT : D3D12_RESOURCE_STATE_GENERIC_READ;
    D3D12_RESOURCE_STATES sourceState = adxSourceFramebuffer->swapChainBuffer ? D3D12_RESOURCE_STATE_PRESENT : D3D12_RESOURCE_STATE_GENERIC_READ;

    // Go to resolve state
    for (size_t i = 0; i < adxDestFramebuffer->getColorBufferCount(); ++i)
    {
        auto &destColorBuffer = adxDestFramebuffer->colorBuffers[i];
        auto &sourceColorBuffer = adxSourceFramebuffer->colorBuffers[i];
        if (!destColorBuffer || !sourceColorBuffer)
            return AGPU_ERROR;

        {
            D3D12_RESOURCE_BARRIER barriers[2] = {
                resourceTransitionBarrier(destColorBuffer.as<ADXTexture> ()->gpuResource.Get(), destState, D3D12_RESOURCE_STATE_RESOLVE_DEST),
                resourceTransitionBarrier(sourceColorBuffer.as<ADXTexture>()->gpuResource.Get(), sourceState, D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
            };
            commandList->ResourceBarrier(2, barriers);
        }

        commandList->ResolveSubresource(destColorBuffer.as<ADXTexture>()->gpuResource.Get(), 0, sourceColorBuffer.as<ADXTexture>()->gpuResource.Get(), 0, (DXGI_FORMAT)sourceColorBuffer.as<ADXTexture>()->description.format);

        {
            D3D12_RESOURCE_BARRIER barriers[2] = {
                resourceTransitionBarrier(destColorBuffer.as<ADXTexture>()->gpuResource.Get(), D3D12_RESOURCE_STATE_RESOLVE_DEST, destState),
                resourceTransitionBarrier(sourceColorBuffer.as<ADXTexture>()->gpuResource.Get(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, sourceState)
            };
            commandList->ResourceBarrier(2, barriers);
        }
    }

    return AGPU_OK;
}

agpu_error ADXCommandList::resolveTexture(const agpu::texture_ref & sourceTexture, agpu_uint sourceLevel, agpu_uint sourceLayer, const agpu::texture_ref & destTexture, agpu_uint destLevel, agpu_uint destLayer, agpu_uint levelCount, agpu_uint layerCount, agpu_texture_aspect aspect)
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error ADXCommandList::pushConstants(agpu_uint offset, agpu_uint size, agpu_pointer values)
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error ADXCommandList::memoryBarrier(agpu_pipeline_stage_flags source_stage, agpu_pipeline_stage_flags dest_stage, agpu_access_flags source_accesses, agpu_access_flags dest_accesses)
{
    return AGPU_UNIMPLEMENTED;
}

} // End of namespace AgpuD3D12

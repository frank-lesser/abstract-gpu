#include "command_list.hpp"
#include "command_allocator.hpp"
#include "command_queue.hpp"

_agpu_command_list::_agpu_command_list(agpu_device *device)
    : device(device)
{
    allocator = nullptr;
    buffer = nil;
}

void _agpu_command_list::lostReferences()
{
    if(allocator)
        allocator->release();
}

agpu_command_list* _agpu_command_list::create ( agpu_device* device, agpu_command_list_type type, agpu_command_allocator* allocator, agpu_pipeline_state* initial_pipeline_state )
{
    if(!allocator)
        return nullptr;

    auto result = new agpu_command_list(device);
    result->type = type;
    auto error = result->reset(allocator, initial_pipeline_state);
    if(error != AGPU_OK)
    {
        result->release();
        return nullptr;
    }

    return result;
}

agpu_error _agpu_command_list::setShaderSignature ( agpu_shader_signature* signature )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::setViewport ( agpu_int x, agpu_int y, agpu_int w, agpu_int h )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::setScissor ( agpu_int x, agpu_int y, agpu_int w, agpu_int h )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::setClearColor ( agpu_float r, agpu_float g, agpu_float b, agpu_float a )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::setClearDepth ( agpu_float depth )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::setClearStencil ( agpu_int value )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::clear ( agpu_bitfield buffers )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::usePipelineState ( agpu_pipeline_state* pipeline )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::useVertexBinding ( agpu_vertex_binding* vertex_binding )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::useIndexBuffer ( agpu_buffer* index_buffer )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::setPrimitiveTopology ( agpu_primitive_topology topology )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::useDrawIndirectBuffer ( agpu_buffer* draw_buffer )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::useShaderResources ( agpu_shader_resource_binding* binding )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::drawArrays ( agpu_uint vertex_count, agpu_uint instance_count, agpu_uint first_vertex, agpu_uint base_instance )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::drawElements ( agpu_uint index_count, agpu_uint instance_count, agpu_uint first_index, agpu_int base_vertex, agpu_uint base_instance )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::drawElementsIndirect ( agpu_size offset )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::multiDrawElementsIndirect ( agpu_size offset, agpu_size drawcount )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::setStencilReference ( agpu_uint reference )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::executeBundle ( agpu_command_list* bundle )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::close (  )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::reset ( agpu_command_allocator* allocator, agpu_pipeline_state* initial_pipeline_state )
{
    CHECK_POINTER(allocator);

    // Store the new allocator.
    allocator->retain();
    if(this->allocator)
        this->allocator->release();
    this->allocator = allocator;

    // Create the buffer.
    if(!buffer)
        buffer = [allocator->queue->handle commandBuffer];

    return AGPU_OK;
}

agpu_error _agpu_command_list::beginFrame ( agpu_framebuffer* framebuffer, agpu_bool bundle_content )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::endFrame (  )
{
    return AGPU_UNIMPLEMENTED;
}

agpu_error _agpu_command_list::resolveFramebuffer ( agpu_framebuffer* destFramebuffer, agpu_framebuffer* sourceFramebuffer )
{
    return AGPU_UNIMPLEMENTED;
}

// The exported C interface
AGPU_EXPORT agpu_error agpuAddCommandListReference ( agpu_command_list* command_list )
{
    CHECK_POINTER(command_list);
    return command_list->retain();
}

AGPU_EXPORT agpu_error agpuReleaseCommandList ( agpu_command_list* command_list )
{
    CHECK_POINTER(command_list);
    return command_list->release();
}

AGPU_EXPORT agpu_error agpuSetShaderSignature ( agpu_command_list* command_list, agpu_shader_signature* signature )
{
    CHECK_POINTER(command_list);
    return command_list->setShaderSignature(signature);
}

AGPU_EXPORT agpu_error agpuSetViewport ( agpu_command_list* command_list, agpu_int x, agpu_int y, agpu_int w, agpu_int h )
{
    CHECK_POINTER(command_list);
    return command_list->setViewport(x, y, w, h);
}

AGPU_EXPORT agpu_error agpuSetScissor ( agpu_command_list* command_list, agpu_int x, agpu_int y, agpu_int w, agpu_int h )
{
    CHECK_POINTER(command_list);
    return command_list->setScissor(x, y, w, h);
}

AGPU_EXPORT agpu_error agpuSetClearColor ( agpu_command_list* command_list, agpu_float r, agpu_float g, agpu_float b, agpu_float a )
{
    CHECK_POINTER(command_list);
    return command_list->setClearColor(r, g, b, a);
}

AGPU_EXPORT agpu_error agpuSetClearDepth ( agpu_command_list* command_list, agpu_float depth )
{
    CHECK_POINTER(command_list);
    return command_list->setClearDepth(depth);
}

AGPU_EXPORT agpu_error agpuSetClearStencil ( agpu_command_list* command_list, agpu_int value )
{
    CHECK_POINTER(command_list);
    return command_list->setClearStencil(value);
}

AGPU_EXPORT agpu_error agpuClear ( agpu_command_list* command_list, agpu_bitfield buffers )
{
    CHECK_POINTER(command_list);
    return command_list->clear(buffers);
}

AGPU_EXPORT agpu_error agpuUsePipelineState ( agpu_command_list* command_list, agpu_pipeline_state* pipeline )
{
    CHECK_POINTER(command_list);
    return command_list->usePipelineState(pipeline);
}

AGPU_EXPORT agpu_error agpuUseVertexBinding ( agpu_command_list* command_list, agpu_vertex_binding* vertex_binding )
{
    CHECK_POINTER(command_list);
    return command_list->useVertexBinding(vertex_binding);
}

AGPU_EXPORT agpu_error agpuUseIndexBuffer ( agpu_command_list* command_list, agpu_buffer* index_buffer )
{
    CHECK_POINTER(command_list);
    return command_list->useIndexBuffer(index_buffer);
}

AGPU_EXPORT agpu_error agpuSetPrimitiveTopology ( agpu_command_list* command_list, agpu_primitive_topology topology )
{
    CHECK_POINTER(command_list);
    return command_list->setPrimitiveTopology(topology);
}

AGPU_EXPORT agpu_error agpuUseDrawIndirectBuffer ( agpu_command_list* command_list, agpu_buffer* draw_buffer )
{
    CHECK_POINTER(command_list);
    return command_list->useDrawIndirectBuffer(draw_buffer);
}

AGPU_EXPORT agpu_error agpuUseShaderResources ( agpu_command_list* command_list, agpu_shader_resource_binding* binding )
{
    CHECK_POINTER(command_list);
    return command_list->useShaderResources(binding);
}

AGPU_EXPORT agpu_error agpuDrawArrays ( agpu_command_list* command_list, agpu_uint vertex_count, agpu_uint instance_count, agpu_uint first_vertex, agpu_uint base_instance )
{
    CHECK_POINTER(command_list);
    return command_list->drawArrays(vertex_count, instance_count, first_vertex, base_instance);
}

AGPU_EXPORT agpu_error agpuDrawElements ( agpu_command_list* command_list, agpu_uint index_count, agpu_uint instance_count, agpu_uint first_index, agpu_int base_vertex, agpu_uint base_instance )
{
    CHECK_POINTER(command_list);
    return command_list->drawElements(index_count, instance_count, first_index, base_vertex, base_instance);
}

AGPU_EXPORT agpu_error agpuDrawElementsIndirect ( agpu_command_list* command_list, agpu_size offset )
{
    CHECK_POINTER(command_list);
    return command_list->drawElementsIndirect(offset);
}

AGPU_EXPORT agpu_error agpuMultiDrawElementsIndirect ( agpu_command_list* command_list, agpu_size offset, agpu_size drawcount )
{
    CHECK_POINTER(command_list);
    return command_list->multiDrawElementsIndirect(offset, drawcount);
}

AGPU_EXPORT agpu_error agpuSetStencilReference ( agpu_command_list* command_list, agpu_uint reference )
{
    CHECK_POINTER(command_list);
    return command_list->setStencilReference(reference);
}

AGPU_EXPORT agpu_error agpuExecuteBundle ( agpu_command_list* command_list, agpu_command_list* bundle )
{
    CHECK_POINTER(command_list);
    return command_list->executeBundle(bundle);
}

AGPU_EXPORT agpu_error agpuCloseCommandList ( agpu_command_list* command_list )
{
    CHECK_POINTER(command_list);
    return command_list->close();
}

AGPU_EXPORT agpu_error agpuResetCommandList ( agpu_command_list* command_list, agpu_command_allocator* allocator, agpu_pipeline_state* initial_pipeline_state )
{
    CHECK_POINTER(command_list);
    return command_list->reset(allocator, initial_pipeline_state);
}

AGPU_EXPORT agpu_error agpuBeginFrame ( agpu_command_list* command_list, agpu_framebuffer* framebuffer, agpu_bool bundle_content )
{
    CHECK_POINTER(command_list);
    return command_list->beginFrame(framebuffer, bundle_content);
}

AGPU_EXPORT agpu_error agpuEndFrame ( agpu_command_list* command_list )
{
    CHECK_POINTER(command_list);
    return command_list->endFrame();
}

AGPU_EXPORT agpu_error agpuResolveFramebuffer ( agpu_command_list* command_list, agpu_framebuffer* destFramebuffer, agpu_framebuffer* sourceFramebuffer )
{
    CHECK_POINTER(command_list);
    return command_list->resolveFramebuffer(destFramebuffer, sourceFramebuffer);
}

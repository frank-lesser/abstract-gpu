#include "shader_signature.hpp"


_agpu_shader_signature::_agpu_shader_signature(agpu_device *device)
    : device(device)
{

}

void _agpu_shader_signature::lostReferences()
{

}

agpu_shader_signature *_agpu_shader_signature::create(agpu_device *device, agpu_shader_signature_builder *builder)
{
    return new agpu_shader_signature(device);
}

// The exported C interface
AGPU_EXPORT agpu_error agpuAddShaderSignature ( agpu_shader_signature* shader_signature )
{
    CHECK_POINTER(shader_signature);
    return shader_signature->retain();
}

AGPU_EXPORT agpu_error agpuReleaseShaderSignature ( agpu_shader_signature* shader_signature )
{
    CHECK_POINTER(shader_signature);
    return shader_signature->release();
}

AGPU_EXPORT agpu_shader_resource_binding* agpuCreateShaderResourceBinding ( agpu_shader_signature* shader_signature, agpu_uint element )
{
    return nullptr;
}
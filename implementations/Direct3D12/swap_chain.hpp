#ifndef AGPU_D3D12_SWAP_CHAIN_HPP
#define AGPU_D3D12_SWAP_CHAIN_HPP

#include <vector>
#include "device.hpp"

namespace AgpuD3D12
{

class ADXSwapChain : public agpu::swap_chain
{
public:
    ADXSwapChain(const agpu::device_ref &cdevice);
    ~ADXSwapChain();

    static agpu::swap_chain_ref create(const agpu::device_ref &device, const agpu::command_queue_ref &queue, agpu_swap_chain_create_info *createInfo);

    virtual agpu_error swapBuffers() override;
    virtual agpu::framebuffer_ptr getCurrentBackBuffer() override;
    virtual agpu_size getCurrentBackBufferIndex() override;
    virtual agpu_size getFramebufferCount() override;

public:
    agpu::device_ref device;

    HWND window;

    // Frame buffers
    ComPtr<IDXGISwapChain3> swapChain;
    std::vector<agpu::framebuffer_ref> framebuffers;

    size_t frameIndex;
    size_t frameCount;
    uint32_t windowWidth, windowHeight;
};

} // End of namespace AgpuD3D12

#endif //AGPU_D3D12_VERTEX_LAYOUT_HPP

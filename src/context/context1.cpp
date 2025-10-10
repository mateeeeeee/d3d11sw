#include "context1.h"

MarsDeviceContext1::MarsDeviceContext1(ID3D11Device* device)
    : MarsDeviceContext(device)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::CopySubresourceRegion1(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox, UINT CopyFlags)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::UpdateSubresource1(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::DiscardResource(ID3D11Resource* pResource)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::DiscardView(ID3D11View* pResourceView)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::VSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::HSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::DSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::GSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::PSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::CSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::VSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::HSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::DSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::GSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::PSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::CSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::SwapDeviceContextState(ID3DDeviceContextState* pState, ID3DDeviceContextState** ppPreviousState)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::ClearView(ID3D11View* pView, const FLOAT Color[4], const D3D11_RECT* pRect, UINT NumRects)
{
}

void STDMETHODCALLTYPE MarsDeviceContext1::DiscardView1(ID3D11View* pResourceView, const D3D11_RECT* pRects, UINT NumRects)
{
}

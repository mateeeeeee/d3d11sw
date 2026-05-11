#include "d3d9/state_block/state_block.h"
#include "d3d9/device/device.h"
#include "d3d9/resources/index_buffer.h"
#include "d3d9/resources/texture.h"
#include "d3d9/resources/vertex_buffer.h"
#include "d3d9/shaders/pixel_shader.h"
#include "d3d9/shaders/vertex_declaration.h"
#include "d3d9/shaders/vertex_shader.h"
#include "core/common/trace.h"
#include <cstring>

namespace d3dsw {

D3D9StateBlockSW::D3D9StateBlockSW(D3D9DeviceSW* device)
    : _device(device)
{
    if (_device) { _device->AddRef(); }
}

D3D9StateBlockSW::~D3D9StateBlockSW()
{
    ReleaseRefs();
    if (_device) { _device->Release(); }
}

void D3D9StateBlockSW::ReleaseRefs()
{
    for (UINT s = 0; s < kMaxStages; ++s)
    {
        if (_textures[s]) { _textures[s]->Release(); _textures[s] = nullptr; }
    }
    for (UINT s = 0; s < kMaxStreams; ++s)
    {
        if (_streams[s].buffer) { _streams[s].buffer->Release(); _streams[s].buffer = nullptr; }
    }
    if (_vs)         { _vs->Release();         _vs         = nullptr; }
    if (_ps)         { _ps->Release();         _ps         = nullptr; }
    if (_vertexDecl) { _vertexDecl->Release(); _vertexDecl = nullptr; }
    if (_indices)    { _indices->Release();    _indices    = nullptr; }
}

void D3D9StateBlockSW::Capture(D3D9DeviceSW* device)
{
    ReleaseRefs();

    // POD state
    std::memcpy(_rs,            device->_rs,            sizeof(_rs));
    std::memcpy(_transforms,    device->_transforms,    sizeof(_transforms));
    std::memcpy(_samplerStates, device->_samplerStates, sizeof(_samplerStates));
    std::memcpy(_tss,           device->_tss,           sizeof(_tss));
    _material     = device->_material;
    std::memcpy(_lights,        device->_lights,        sizeof(_lights));
    std::memcpy(_lightEnabled,  device->_lightEnabled,  sizeof(_lightEnabled));
    std::memcpy(_vsConstF,      device->_vsConstF,      sizeof(_vsConstF));
    std::memcpy(_vsConstI,      device->_vsConstI,      sizeof(_vsConstI));
    std::memcpy(_vsConstB,      device->_vsConstB,      sizeof(_vsConstB));
    std::memcpy(_psConstF,      device->_psConstF,      sizeof(_psConstF));
    std::memcpy(_psConstI,      device->_psConstI,      sizeof(_psConstI));
    std::memcpy(_psConstB,      device->_psConstB,      sizeof(_psConstB));
    _fvf              = device->_fvf;
    _viewport         = device->_viewport;
    _scissor          = device->_scissor;
    _scissorEnabled   = device->_scissorEnabled;
    std::memcpy(_clipPlanes,    device->_clipPlanes,    sizeof(_clipPlanes));
    _clipStatus       = device->_clipStatus;

    // COM objects (AddRef each)
    for (UINT s = 0; s < kMaxStages; ++s)
    {
        _textures[s] = device->_textures[s];
        if (_textures[s]) { _textures[s]->AddRef(); }
    }
    for (UINT s = 0; s < kMaxStreams; ++s)
    {
        _streams[s].buffer = device->_streams[s].buffer;
        _streams[s].offset = device->_streams[s].offset;
        _streams[s].stride = device->_streams[s].stride;
        _streamFreq[s]     = device->_streamFreq[s];
        if (_streams[s].buffer) { _streams[s].buffer->AddRef(); }
    }
    _vs         = device->_vs;         if (_vs)         { _vs->AddRef(); }
    _ps         = device->_ps;         if (_ps)         { _ps->AddRef(); }
    _vertexDecl = device->_vertexDecl; if (_vertexDecl) { _vertexDecl->AddRef(); }
    _indices    = device->_indices;    if (_indices)    { _indices->AddRef(); }

    _valid = true;
}

HRESULT STDMETHODCALLTYPE D3D9StateBlockSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) { return E_POINTER; }
    if (riid == IID_IUnknown || riid == IID_IDirect3DStateBlock9)
    {
        *ppv = static_cast<IDirect3DStateBlock9*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE D3D9StateBlockSW::GetDevice(IDirect3DDevice9** ppDevice)
{
    if (!ppDevice) { return D3DERR_INVALIDCALL; }
    *ppDevice = static_cast<IDirect3DDevice9*>(_device);
    if (_device) { _device->AddRef(); }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9StateBlockSW::Capture()
{
    D3DSW_TRACE_STATE("IDirect3DStateBlock9::Capture");
    if (!_device) { return D3DERR_INVALIDCALL; }
    this->Capture(_device);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9StateBlockSW::Apply()
{
    D3DSW_TRACE_STATE("IDirect3DStateBlock9::Apply");
    if (!_device || !_valid) { return D3DERR_INVALIDCALL; }

    // Restore render states
    for (UINT i = 0; i < 210; ++i)
    {
        _device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(i), _rs[i]);
    }
    // Restore transforms
    for (UINT i = 0; i < 512; ++i)
    {
        _device->SetTransform(static_cast<D3DTRANSFORMSTATETYPE>(i), &_transforms[i]);
    }
    // Restore texture stage states + textures
    for (UINT s = 0; s < kMaxStages; ++s)
    {
        _device->SetTexture(s, _textures[s]);
        for (UINT t = 1; t < 33; ++t)
        {
            _device->SetTextureStageState(s, static_cast<D3DTEXTURESTAGESTATETYPE>(t), _tss[s][t]);
        }
        for (UINT t = 1; t < 14; ++t)
        {
            _device->SetSamplerState(s, static_cast<D3DSAMPLERSTATETYPE>(t), _samplerStates[s][t]);
        }
    }
    // Restore material & lights
    _device->SetMaterial(&_material);
    for (UINT i = 0; i < kMaxLights; ++i)
    {
        _device->SetLight(i, &_lights[i]);
        _device->LightEnable(i, _lightEnabled[i]);
    }
    // Restore shader constants
    _device->SetVertexShaderConstantF(0, &_vsConstF[0][0], 256);
    _device->SetVertexShaderConstantI(0, &_vsConstI[0][0], 16);
    _device->SetPixelShaderConstantF (0, &_psConstF[0][0], 224);
    _device->SetPixelShaderConstantI (0, &_psConstI[0][0], 16);
    // Restore shaders & IA
    _device->SetVertexShader(static_cast<IDirect3DVertexShader9*>(_vs));
    _device->SetPixelShader (static_cast<IDirect3DPixelShader9*> (_ps));
    _device->SetVertexDeclaration(_vertexDecl);
    _device->SetFVF(_fvf);
    _device->SetIndices(_indices);
    for (UINT s = 0; s < kMaxStreams; ++s)
    {
        _device->SetStreamSource(s,
            static_cast<IDirect3DVertexBuffer9*>(_streams[s].buffer),
            _streams[s].offset, _streams[s].stride);
        _device->SetStreamSourceFreq(s, _streamFreq[s]);
    }
    // Restore viewport, scissor, clip planes, clip status
    _device->SetViewport(&_viewport);
    _device->SetScissorRect(&_scissor);
    _device->SetRenderState(D3DRS_SCISSORTESTENABLE, _scissorEnabled);
    for (UINT i = 0; i < kMaxClipPl; ++i)
    {
        _device->SetClipPlane(i, _clipPlanes[i]);
    }
    _device->SetClipStatus(&_clipStatus);
    return S_OK;
}

}

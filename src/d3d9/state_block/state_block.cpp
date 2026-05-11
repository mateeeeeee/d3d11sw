#include <cstring>
#include "d3d9/state_block/state_block.h"
#include "d3d9/device/device.h"
#include "d3d9/resources/index_buffer.h"
#include "d3d9/resources/texture.h"
#include "d3d9/resources/vertex_buffer.h"
#include "d3d9/shaders/pixel_shader.h"
#include "d3d9/shaders/vertex_declaration.h"
#include "d3d9/shaders/vertex_shader.h"
#include "core/common/trace.h"

namespace d3dsw {

D3D9StateBlockSW::D3D9StateBlockSW(D3D9DeviceSW* device)
    : _device(device)
{
    if (_device)
    {
        _device->AddRef();
    }
}

D3D9StateBlockSW::~D3D9StateBlockSW()
{
    ReleaseRefs();
    if (_device)
    {
        _device->Release();
    }
}

void D3D9StateBlockSW::ReleaseRefs()
{
    for (Uint s = 0; s < MaxStages; ++s)
    {
        if (_textures[s])
        {
            _textures[s]->Release();
            _textures[s] = nullptr;
        }
    }
    for (Uint s = 0; s < MaxStreams; ++s)
    {
        if (_streams[s].buffer)
        {
            _streams[s].buffer->Release();
            _streams[s].buffer = nullptr;
        }
    }
    if (_vs)
    {
        _vs->Release();
        _vs = nullptr;
    }
    if (_ps)
    {
        _ps->Release();
        _ps = nullptr;
    }
    if (_vertexDecl)
    {
        _vertexDecl->Release();
        _vertexDecl = nullptr;
    }
    if (_indices)
    {
        _indices->Release();
        _indices = nullptr;
    }
}

void D3D9StateBlockSW::CaptureAll(D3D9DeviceSW* device)
{
    ReleaseRefs();

    std::memcpy(_rs,            device->_rs,            sizeof(_rs));
    std::memcpy(_transforms,    device->_transforms,    sizeof(_transforms));
    std::memcpy(_samplerStates, device->_samplerStates, sizeof(_samplerStates));
    std::memcpy(_tss,           device->_tss,           sizeof(_tss));
    _material = device->_material;
    std::memcpy(_lights,        device->_lights,        sizeof(_lights));
    std::memcpy(_lightEnabled,  device->_lightEnabled,  sizeof(_lightEnabled));
    std::memcpy(_vsConstF,      device->_vsConstF,      sizeof(_vsConstF));
    std::memcpy(_vsConstI,      device->_vsConstI,      sizeof(_vsConstI));
    std::memcpy(_vsConstB,      device->_vsConstB,      sizeof(_vsConstB));
    std::memcpy(_psConstF,      device->_psConstF,      sizeof(_psConstF));
    std::memcpy(_psConstI,      device->_psConstI,      sizeof(_psConstI));
    std::memcpy(_psConstB,      device->_psConstB,      sizeof(_psConstB));
    _fvf      = device->_fvf;
    _viewport = device->_viewport;
    _scissor  = device->_scissor;
    std::memcpy(_clipPlanes, device->_clipPlanes, sizeof(_clipPlanes));

    for (Uint s = 0; s < MaxStages; ++s)
    {
        _textures[s] = device->_textures[s];
        if (_textures[s])
        {
            _textures[s]->AddRef();
        }
    }
    for (Uint s = 0; s < MaxStreams; ++s)
    {
        _streams[s].buffer = device->_streams[s].buffer;
        _streams[s].offset = static_cast<Uint>(device->_streams[s].offset);
        _streams[s].stride = static_cast<Uint>(device->_streams[s].stride);
        _streamFreq[s]     = device->_streamFreq[s];
        if (_streams[s].buffer)
        {
            _streams[s].buffer->AddRef();
        }
    }
    _vs = device->_vs;
    if (_vs)
    {
        _vs->AddRef();
    }
    _ps = device->_ps;
    if (_ps)
    {
        _ps->AddRef();
    }
    _vertexDecl = device->_vertexDecl;
    if (_vertexDecl)
    {
        _vertexDecl->AddRef();
    }
    _indices = device->_indices;
    if (_indices)
    {
        _indices->AddRef();
    }

    _valid = true;
}

void D3D9StateBlockSW::MarkAllChanged()
{
    std::memset(_changedRS, 0xFF, sizeof(_changedRS));
    std::memset(_changedTransforms, 0xFF, sizeof(_changedTransforms));
    std::memset(_changedSamplerState, 0xFF, sizeof(_changedSamplerState));
    std::memset(_changedTSS, 0xFF, sizeof(_changedTSS));
    _changedTexture     = 0xFF;
    _changedLight       = 0xFF;
    _changedLightEnable = 0xFF;
    std::memset(_changedVSConstF, 0xFF, sizeof(_changedVSConstF));
    std::memset(_changedPSConstF, 0xFF, sizeof(_changedPSConstF));
    _changedVSConstI    = 0xFFFF;
    _changedVSConstB    = 0xFFFF;
    _changedPSConstI    = 0xFFFF;
    _changedPSConstB    = 0xFFFF;
    _changedStreams     = 0xFFFF;
    _changedStreamFreq  = 0xFFFF;
    _changedClipPlane   = 0x3F;
    _changedViewport    = true;
    _changedScissor     = true;
    _changedMaterial    = true;
    _changedFVF         = true;
    _changedIndices     = true;
    _changedVertexDecl  = true;
    _changedVS          = true;
    _changedPS          = true;
}

// --- Recording methods ---

void D3D9StateBlockSW::RecordRenderState(D3DRENDERSTATETYPE state, DWORD value)
{
    Uint idx = static_cast<Uint>(state);
    _rs[idx] = value;
    _changedRS[idx / 32] |= (1u << (idx % 32));
    _valid = true;
}

void D3D9StateBlockSW::RecordTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix)
{
    Uint idx = static_cast<Uint>(state);
    _transforms[idx] = *matrix;
    _changedTransforms[idx / 32] |= (1u << (idx % 32));
    _valid = true;
}

void D3D9StateBlockSW::RecordTexture(DWORD stage, IDirect3DBaseTexture9* texture)
{
    if (texture)
    {
        texture->AddRef();
    }
    if (_textures[stage])
    {
        _textures[stage]->Release();
    }
    _textures[stage] = texture;
    _changedTexture |= (1u << stage);
    _valid = true;
}

void D3D9StateBlockSW::RecordTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type, DWORD value)
{
    _tss[stage][type] = value;
    _changedTSS[stage] |= (1ull << type);
    _valid = true;
}

void D3D9StateBlockSW::RecordSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE type, DWORD value)
{
    _samplerStates[sampler][type] = value;
    _changedSamplerState[sampler] |= (1u << type);
    _valid = true;
}

void D3D9StateBlockSW::RecordVertexShader(D3D9VertexShaderSW* vs)
{
    if (vs)
    {
        vs->AddRef();
    }
    if (_vs)
    {
        _vs->Release();
    }
    _vs = vs;
    _changedVS = true;
    _valid = true;
}

void D3D9StateBlockSW::RecordPixelShader(D3D9PixelShaderSW* ps)
{
    if (ps)
    {
        ps->AddRef();
    }
    if (_ps)
    {
        _ps->Release();
    }
    _ps = ps;
    _changedPS = true;
    _valid = true;
}

void D3D9StateBlockSW::RecordVertexDeclaration(D3D9VertexDeclarationSW* decl)
{
    if (decl)
    {
        decl->AddRef();
    }
    if (_vertexDecl)
    {
        _vertexDecl->Release();
    }
    _vertexDecl = decl;
    _changedVertexDecl = true;
    _valid = true;
}

void D3D9StateBlockSW::RecordFVF(DWORD fvf)
{
    _fvf = fvf;
    _changedFVF = true;
    _valid = true;
}

void D3D9StateBlockSW::RecordIndices(D3D9IndexBufferSW* ib)
{
    if (ib)
    {
        ib->AddRef();
    }
    if (_indices)
    {
        _indices->Release();
    }
    _indices = ib;
    _changedIndices = true;
    _valid = true;
}

void D3D9StateBlockSW::RecordStreamSource(UINT stream, D3D9VertexBufferSW* buffer, UINT offset, UINT stride)
{
    if (buffer)
    {
        buffer->AddRef();
    }
    if (_streams[stream].buffer)
    {
        _streams[stream].buffer->Release();
    }
    _streams[stream].buffer = buffer;
    _streams[stream].offset = offset;
    _streams[stream].stride = stride;
    _changedStreams |= (1u << stream);
    _valid = true;
}

void D3D9StateBlockSW::RecordStreamSourceFreq(UINT stream, UINT freq)
{
    _streamFreq[stream] = freq;
    _changedStreamFreq |= (1u << stream);
    _valid = true;
}

void D3D9StateBlockSW::RecordViewport(const D3DVIEWPORT9* viewport)
{
    _viewport = *viewport;
    _changedViewport = true;
    _valid = true;
}

void D3D9StateBlockSW::RecordScissorRect(const RECT* rect)
{
    _scissor = *rect;
    _changedScissor = true;
    _valid = true;
}

void D3D9StateBlockSW::RecordMaterial(const D3DMATERIAL9* material)
{
    _material = *material;
    _changedMaterial = true;
    _valid = true;
}

void D3D9StateBlockSW::RecordLight(DWORD index, const D3DLIGHT9* light)
{
    _lights[index] = *light;
    _changedLight |= (1u << index);
    _valid = true;
}

void D3D9StateBlockSW::RecordLightEnable(DWORD index, BOOL enable)
{
    _lightEnabled[index] = enable;
    _changedLightEnable |= (1u << index);
    _valid = true;
}

void D3D9StateBlockSW::RecordClipPlane(DWORD index, const float* plane)
{
    std::memcpy(_clipPlanes[index], plane, 4 * sizeof(float));
    _changedClipPlane |= (1u << index);
    _valid = true;
}

void D3D9StateBlockSW::RecordVSConstantF(UINT reg, const float* data, UINT count)
{
    std::memcpy(&_vsConstF[reg][0], data, count * 4 * sizeof(Float));
    for (Uint i = reg; i < reg + count; ++i)
    {
        _changedVSConstF[i / 32] |= (1u << (i % 32));
    }
    _valid = true;
}

void D3D9StateBlockSW::RecordVSConstantI(UINT reg, const int* data, UINT count)
{
    std::memcpy(&_vsConstI[reg][0], data, count * 4 * sizeof(Int32));
    for (Uint i = reg; i < reg + count; ++i)
    {
        _changedVSConstI |= (1u << i);
    }
    _valid = true;
}

void D3D9StateBlockSW::RecordVSConstantB(UINT reg, const BOOL* data, UINT count)
{
    for (Uint i = 0; i < count; ++i)
    {
        _vsConstB[reg + i] = data[i] ? 1u : 0u;
    }
    for (Uint i = reg; i < reg + count; ++i)
    {
        _changedVSConstB |= (1u << i);
    }
    _valid = true;
}

void D3D9StateBlockSW::RecordPSConstantF(UINT reg, const float* data, UINT count)
{
    std::memcpy(&_psConstF[reg][0], data, count * 4 * sizeof(Float));
    for (Uint i = reg; i < reg + count; ++i)
    {
        _changedPSConstF[i / 32] |= (1u << (i % 32));
    }
    _valid = true;
}

void D3D9StateBlockSW::RecordPSConstantI(UINT reg, const int* data, UINT count)
{
    std::memcpy(&_psConstI[reg][0], data, count * 4 * sizeof(Int32));
    for (Uint i = reg; i < reg + count; ++i)
    {
        _changedPSConstI |= (1u << i);
    }
    _valid = true;
}

void D3D9StateBlockSW::RecordPSConstantB(UINT reg, const BOOL* data, UINT count)
{
    for (Uint i = 0; i < count; ++i)
    {
        _psConstB[reg + i] = data[i] ? 1u : 0u;
    }
    for (Uint i = reg; i < reg + count; ++i)
    {
        _changedPSConstB |= (1u << i);
    }
    _valid = true;
}

// --- COM interface ---

HRESULT STDMETHODCALLTYPE D3D9StateBlockSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }
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
    if (!ppDevice)
    {
        return D3DERR_INVALIDCALL;
    }
    *ppDevice = static_cast<IDirect3DDevice9*>(_device);
    if (_device)
    {
        _device->AddRef();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9StateBlockSW::Capture()
{
    D3DSW_TRACE_STATE("IDirect3DStateBlock9::Capture");
    if (!_device)
    {
        return D3DERR_INVALIDCALL;
    }

    for (Uint i = 0; i < 210; ++i)
    {
        if (_changedRS[i / 32] & (1u << (i % 32)))
        {
            _rs[i] = _device->_rs[i];
        }
    }
    for (Uint i = 0; i < 512; ++i)
    {
        if (_changedTransforms[i / 32] & (1u << (i % 32)))
        {
            _transforms[i] = _device->_transforms[i];
        }
    }
    for (Uint s = 0; s < MaxStages; ++s)
    {
        if (_changedTexture & (1u << s))
        {
            if (_textures[s])
            {
                _textures[s]->Release();
            }
            _textures[s] = _device->_textures[s];
            if (_textures[s])
            {
                _textures[s]->AddRef();
            }
        }
        for (Uint t = 1; t < 33; ++t)
        {
            if (_changedTSS[s] & (1ull << t))
            {
                _tss[s][t] = _device->_tss[s][t];
            }
        }
        for (Uint t = 1; t < 14; ++t)
        {
            if (_changedSamplerState[s] & (1u << t))
            {
                _samplerStates[s][t] = _device->_samplerStates[s][t];
            }
        }
    }
    if (_changedMaterial)
    {
        _material = _device->_material;
    }
    for (Uint i = 0; i < MaxLights; ++i)
    {
        if (_changedLight & (1u << i))
        {
            _lights[i] = _device->_lights[i];
        }
        if (_changedLightEnable & (1u << i))
        {
            _lightEnabled[i] = _device->_lightEnabled[i];
        }
    }
    for (Uint i = 0; i < 256; ++i)
    {
        if (_changedVSConstF[i / 32] & (1u << (i % 32)))
        {
            std::memcpy(_vsConstF[i], _device->_vsConstF[i], 4 * sizeof(Float));
        }
    }
    for (Uint i = 0; i < 16; ++i)
    {
        if (_changedVSConstI & (1u << i))
        {
            std::memcpy(_vsConstI[i], _device->_vsConstI[i], 4 * sizeof(Int32));
        }
        if (_changedVSConstB & (1u << i))
        {
            _vsConstB[i] = _device->_vsConstB[i];
        }
    }
    for (Uint i = 0; i < 224; ++i)
    {
        if (_changedPSConstF[i / 32] & (1u << (i % 32)))
        {
            std::memcpy(_psConstF[i], _device->_psConstF[i], 4 * sizeof(Float));
        }
    }
    for (Uint i = 0; i < 16; ++i)
    {
        if (_changedPSConstI & (1u << i))
        {
            std::memcpy(_psConstI[i], _device->_psConstI[i], 4 * sizeof(Int32));
        }
        if (_changedPSConstB & (1u << i))
        {
            _psConstB[i] = _device->_psConstB[i];
        }
    }
    if (_changedVS)
    {
        if (_vs)
        {
            _vs->Release();
        }
        _vs = _device->_vs;
        if (_vs)
        {
            _vs->AddRef();
        }
    }
    if (_changedPS)
    {
        if (_ps)
        {
            _ps->Release();
        }
        _ps = _device->_ps;
        if (_ps)
        {
            _ps->AddRef();
        }
    }
    if (_changedVertexDecl)
    {
        if (_vertexDecl)
        {
            _vertexDecl->Release();
        }
        _vertexDecl = _device->_vertexDecl;
        if (_vertexDecl)
        {
            _vertexDecl->AddRef();
        }
    }
    if (_changedFVF)
    {
        _fvf = _device->_fvf;
    }
    if (_changedIndices)
    {
        if (_indices)
        {
            _indices->Release();
        }
        _indices = _device->_indices;
        if (_indices)
        {
            _indices->AddRef();
        }
    }
    for (Uint s = 0; s < MaxStreams; ++s)
    {
        if (_changedStreams & (1u << s))
        {
            if (_streams[s].buffer)
            {
                _streams[s].buffer->Release();
            }
            _streams[s].buffer = _device->_streams[s].buffer;
            _streams[s].offset = static_cast<Uint>(_device->_streams[s].offset);
            _streams[s].stride = static_cast<Uint>(_device->_streams[s].stride);
            if (_streams[s].buffer)
            {
                _streams[s].buffer->AddRef();
            }
        }
        if (_changedStreamFreq & (1u << s))
        {
            _streamFreq[s] = _device->_streamFreq[s];
        }
    }
    if (_changedViewport)
    {
        _viewport = _device->_viewport;
    }
    if (_changedScissor)
    {
        _scissor = _device->_scissor;
    }
    for (Uint i = 0; i < MaxClipPlanes; ++i)
    {
        if (_changedClipPlane & (1u << i))
        {
            std::memcpy(_clipPlanes[i], _device->_clipPlanes[i], 4 * sizeof(Float));
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D9StateBlockSW::Apply()
{
    D3DSW_TRACE_STATE("IDirect3DStateBlock9::Apply");
    if (!_device || !_valid)
    {
        return D3DERR_INVALIDCALL;
    }

    for (Uint i = 0; i < 210; ++i)
    {
        if (_changedRS[i / 32] & (1u << (i % 32)))
        {
            _device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(i), _rs[i]);
        }
    }
    for (Uint i = 0; i < 512; ++i)
    {
        if (_changedTransforms[i / 32] & (1u << (i % 32)))
        {
            _device->SetTransform(static_cast<D3DTRANSFORMSTATETYPE>(i), &_transforms[i]);
        }
    }
    for (Uint s = 0; s < MaxStages; ++s)
    {
        if (_changedTexture & (1u << s))
        {
            _device->SetTexture(s, _textures[s]);
        }
        for (Uint t = 1; t < 33; ++t)
        {
            if (_changedTSS[s] & (1ull << t))
            {
                _device->SetTextureStageState(s, static_cast<D3DTEXTURESTAGESTATETYPE>(t), _tss[s][t]);
            }
        }
        for (Uint t = 1; t < 14; ++t)
        {
            if (_changedSamplerState[s] & (1u << t))
            {
                _device->SetSamplerState(s, static_cast<D3DSAMPLERSTATETYPE>(t), _samplerStates[s][t]);
            }
        }
    }
    if (_changedMaterial)
    {
        _device->SetMaterial(&_material);
    }
    for (Uint i = 0; i < MaxLights; ++i)
    {
        if (_changedLight & (1u << i))
        {
            _device->SetLight(i, &_lights[i]);
        }
        if (_changedLightEnable & (1u << i))
        {
            _device->LightEnable(i, _lightEnabled[i]);
        }
    }
    for (Uint i = 0; i < 256; ++i)
    {
        if (_changedVSConstF[i / 32] & (1u << (i % 32)))
        {
            _device->SetVertexShaderConstantF(i, _vsConstF[i], 1);
        }
    }
    for (Uint i = 0; i < 16; ++i)
    {
        if (_changedVSConstI & (1u << i))
        {
            _device->SetVertexShaderConstantI(i, _vsConstI[i], 1);
        }
        if (_changedVSConstB & (1u << i))
        {
            BOOL b = _vsConstB[i] ? TRUE : FALSE;
            _device->SetVertexShaderConstantB(i, &b, 1);
        }
    }
    for (Uint i = 0; i < 224; ++i)
    {
        if (_changedPSConstF[i / 32] & (1u << (i % 32)))
        {
            _device->SetPixelShaderConstantF(i, _psConstF[i], 1);
        }
    }
    for (Uint i = 0; i < 16; ++i)
    {
        if (_changedPSConstI & (1u << i))
        {
            _device->SetPixelShaderConstantI(i, _psConstI[i], 1);
        }
        if (_changedPSConstB & (1u << i))
        {
            BOOL b = _psConstB[i] ? TRUE : FALSE;
            _device->SetPixelShaderConstantB(i, &b, 1);
        }
    }
    if (_changedVS)
    {
        _device->SetVertexShader(static_cast<IDirect3DVertexShader9*>(_vs));
    }
    if (_changedPS)
    {
        _device->SetPixelShader(static_cast<IDirect3DPixelShader9*>(_ps));
    }
    if (_changedVertexDecl)
    {
        _device->SetVertexDeclaration(_vertexDecl);
    }
    if (_changedFVF)
    {
        _device->SetFVF(_fvf);
    }
    if (_changedIndices)
    {
        _device->SetIndices(_indices);
    }
    for (Uint s = 0; s < MaxStreams; ++s)
    {
        if (_changedStreams & (1u << s))
        {
            _device->SetStreamSource(s, static_cast<IDirect3DVertexBuffer9*>(_streams[s].buffer), _streams[s].offset, _streams[s].stride);
        }
        if (_changedStreamFreq & (1u << s))
        {
            _device->SetStreamSourceFreq(s, _streamFreq[s]);
        }
    }
    if (_changedViewport)
    {
        _device->SetViewport(&_viewport);
    }
    if (_changedScissor)
    {
        _device->SetScissorRect(&_scissor);
    }
    for (Uint i = 0; i < MaxClipPlanes; ++i)
    {
        if (_changedClipPlane & (1u << i))
        {
            _device->SetClipPlane(i, _clipPlanes[i]);
        }
    }

    return S_OK;
}

}

# d3dsw

A software CPU implementation of the Direct3D family of APIs.

## API Status

| API | Status |
|---|---|
| **Direct3D 11** | Feature-complete core |
| **Direct3D 9** | Work in progress |
| **Direct3D 7/8** | Planned |

## Feature list

<details>
<summary><b>Direct3D 11</b></summary>

### Shaders
- Vertex, Pixel, Compute, Geometry, Hull, Domain shaders
- JIT pipeline: DXBC bytecode → C++20 → clang++/MSVC → native .dylib/.dll
- Full SM4.0 / SM5.0 instruction set (arithmetic, integer, bitwise, control flow, atomics, derivatives, bit manipulation)
- All shader stages share the same JIT, codegen, and runtime — frontend is pluggable

### Rasterizer
- Tiled rasterizer with 28.4 fixed-point edge functions
- 2×2 quad pixel shader execution (correct derivatives, auto-LOD)
- MSAA: 1×/2×/4×/8×/16×, per-sample coverage/depth/stencil, per-sample shading (`SV_SampleIndex`), `SV_Coverage` input/output, `LD_MS`, `ResolveSubresource`
- Early-Z with optional Hi-Z acceleration; late-Z when PS uses `discard`, writes `SV_Depth`, or uses UAVs
- `SV_ViewportArrayIndex`, `SV_RenderTargetArrayIndex`

### Geometry / Tessellation
- Geometry Shader, stream output, adjacency topologies, `DrawAuto`
- Hull Shader + Domain Shader (tri/quad/isoline domains, all partition modes)
- Instanced geometry shaders (`DCL_GS_INSTANCE_COUNT`)

### Textures & Sampling
- 1D / 2D / 3D / Cube textures, mipmap chains, `GenerateMips`
- Point, bilinear, trilinear filtering; all address modes (wrap, mirror, clamp, border, mirror-once)
- Anisotropic filtering (correct per-pixel footprint, up to 16×)
- `SampleLevel`, `SampleGrad`, `SampleBias`, `SampleCmp`, `SampleCmpLevelZero`, `Gather`, `GatherCmp`, `GatherPO`
- BC compressed textures: BC1, BC2, BC3, BC4, BC5, BC7 (BC6H not yet decoded — returns black)
- sRGB read/write

### Output Merger
- Depth/stencil: all comparison functions, all stencil ops, Hi-Z
- Blending: all blend factors/ops, dual-source blending, logic ops
- Multi-render-target (up to 8), per-RT write masks, clip/cull distances

### Compute
- Thread group shared memory (TGSM), `GroupMemoryBarrierWithGroupSync`
- Work-stealing thread pool for TGSM-free shaders; barrier-synchronised group pool for shaders with TGSM
- Append/consume buffers, structured buffers, raw buffers, typed UAVs
- Atomic operations (32-bit int/uint, compare-exchange, exchange)

### Draw & Dispatch
- Indexed, instanced, indirect draw and dispatch (`DrawInstancedIndirect`, `DispatchIndirect`)
- All primitive topologies including adjacency and patch lists

### Queries
- `D3D11_QUERY_EVENT` — CPU/GPU sync point
- `D3D11_QUERY_TIMESTAMP` / `D3D11_QUERY_TIMESTAMP_DISJOINT` — timing
- `D3D11_QUERY_PIPELINE_STATISTICS` — VS/PS/etc invocation counts
- `D3D11_QUERY_OCCLUSION` / `D3D11_QUERY_OCCLUSION_PREDICATE`
- `D3D11_QUERY_SO_STATISTICS` / `D3D11_QUERY_SO_OVERFLOW_PREDICATE`

### Missing / not implemented
The following return `E_NOTIMPL` or produce no-op results:

- **BC6H texture decoding** — `DXGI_FORMAT_BC6H_UF16 / SF16` texels decode to black. Requires a full BC6H bit-field decoder
- **Deferred contexts** — `CreateDeferredContext`, `CreateDeferredContext1/2/3`. Would require a full command-list record/replay path
- **`ID3DDeviceContextState`** (`CreateDeviceContextState`) — D3D11.1 context snapshot/swap; not implemented
- **Tiled resources** — `UpdateTileMappings`, `CopyTileMappings`, `CopyTiles`, `ResizeTilePool` (D3D11.2 sparse resources)
- **Fences** — `CreateFence`, `OpenSharedFence` (D3D11.4 / D3D12-style timeline semaphores)
- **Cross-process shared resources** — `OpenSharedResource`, `OpenSharedResource1`, `OpenSharedResourceByName`
- **Class linkage / shader subroutines** — `CreateClassLinkage`; the `INTERFACE_CALL` SM5 opcode is not handled
- **Performance counters** — `CreateCounter`, `CheckCounter`, `CheckCounterInfo`.
- **Predicates** — `CreatePredicate` (GPU predicated rendering)
- **`MSAD4`** — the SM5 masked sum-of-absolute-differences instruction
- **`EVAL_SNAPPED` / `EVAL_SAMPLE_INDEX` / `EVAL_CENTROID`** — SM5 PS interpolation-mode override opcodes
- **DXGI factory extras** — `CreateSwapChainForCoreWindow`, `CreateSwapChainForComposition`, `GetWindowAssociation`, `CreateSoftwareAdapter`, stereo/occlusion status events
- **DXGI device extras** — `CreateSurface` (DXGI surface sharing), `QueryResourceResidency`, `SetGPUThreadPriority`

</details>

<details>
<summary><b>Direct3D 9 (work in progress)</b></summary>

### Shaders
- **SM2 and SM3** (vs_2_0 / ps_2_0 / vs_3_0 / ps_3_0): same JIT pipeline as D3D11 — token stream → SM4-shaped IR → C++20 → native code
- Broad SM3 opcode coverage: MOV/ADD/MUL/MAD/DP3/DP4/MIN/MAX, NRM, POW, SINCOS, ABS, LRP, CMP, SLT/SGE, LIT, DST, SGN, CRS, DP2ADD, TEXLD/TEXLDL/TEXLDB/TEXLDP/TEXLDD, TEXKILL, REP/ENDREP, LOOP/ENDLOOP, CALL/CALLNZ/LABEL (subroutine inlining), MOVA, matrix macros (M3x2–M4x4), all source/dest modifiers
- Predicated flow control: SETP, IFC, BREAKC/BREAKP (predicate register p0 mapped to temp)
- SM2 output registers: `oPos` (RASTOUT[0]) → SV_Position, `oD0/oD1` (ATTROUT) → COLOR, `oT0–oT7` (TEXCRDOUT) → TEXCOORD — inferred from write scan when no DCL is present
- `D3DSPR_MISCTYPE`: vPos (→ SV_Position screen-space) and vFace (→ ±1.0 front/back via MOVC)
- Compile-time DEF/DEFI constant folding; sampler/texture binding reconstruction from DCL
- Implicit-LOD SAMPLE dispatches to `sw_sample_3d_grad` / `sw_sample_cube_grad` / `sw_sample_2d_grad` per texture type; screen-space derivatives computed from all UV components across the 2×2 quad

### Fixed-Function Pipeline
- **VS**: WVP transform; normal transform + normalize; full Blinn-Phong lighting (ambient, diffuse, specular) for up to 8 directional/point/spot lights; full material colour source routing (`D3DRS_COLORVERTEX`, `D3DMCS_COLOR1/2/MATERIAL` per component); vertex fog (LINEAR/EXP/EXP2); `D3DTS_TEXTURE[i]` transform (COUNT1–4, PROJECTED)
- **PS**: all 8 TSS stages chained; color/alpha ops: SELECTARG1/2, MODULATE, MODULATE2X, ADD, ADDSIGNED, SUBTRACT, LERP; args: D3DTA_TEXTURE (white fallback), D3DTA_DIFFUSE, D3DTA_CURRENT, D3DTA_COMPLEMENT, D3DTA_ALPHAREPLICATE
- Custom VS + FF PS and FF VS + custom PS correctly coexist

### Fog
- Vertex fog (`D3DRS_FOGVERTEXMODE`): LINEAR/EXP/EXP2 computed from eye-space Z in the FF VS; custom SM3 VS writes fog factor to `oFog` (RASTOUT[1])
- Table fog (`D3DRS_FOGTABLEMODE`): computed per-pixel from eye-space depth (`1/perspW`) in the rasterizer
- Applied to all active render targets after the PS executes: `lerp(fogColor, color, fogFactor)`

### Device State
- `SetRenderState`: depth/stencil, blend, cull, fill, scissor, alpha test, separate alpha blend, per-RT write masks, two-sided stencil, slope depth bias, lighting/specular/ambient, fog
- `SetTexture` / `SetSamplerState` (stages 0–7)
- `SetTransform` / `MultiplyTransform` (512 slots: world, view, projection, texture, bone matrices)
- `SetTextureStageState` (all 8 stages), `SetMaterial`, `SetLight` / `LightEnable` (up to 8 lights)
- `SetClipPlane` (6 planes), `SetClipStatus`
- `SetVertexDeclaration`, `SetFVF`, `SetStreamSource` (16 slots), `SetStreamSourceFreq` (GPU instancing), `SetIndices`
- `SetVertexShaderConstantF/I/B`, `SetPixelShaderConstantF/I/B`
- `CreateStateBlock` / `BeginStateBlock` / `EndStateBlock`

### Instancing
- `SetStreamSourceFreq` with `D3DSTREAMSOURCE_INDEXEDDATA | N` (stream 0) and `D3DSTREAMSOURCE_INSTANCEDATA | stepRate` (per-instance streams)
- Vertex declaration elements on per-instance streams automatically classified as `PerInstanceData` with the declared step rate

### Draw Calls
- `DrawPrimitive`, `DrawIndexedPrimitive`, `DrawPrimitiveUP`, `DrawIndexedPrimitiveUP`
- All D3D9 primitive topologies including `D3DPT_TRIANGLEFAN`
- `Clear` (colour, depth, stencil), `Present`
- `UpdateSurface`, `UpdateTexture` (pixel copy; all resources are system memory)

### Resources
- `IDirect3DTexture9`: mipmap levels, `LockRect` / `UnlockRect`, `D3DFMT_R8G8B8` 24-bpp expand
- `IDirect3DCubeTexture9`: 6-face flat backing buffer, `GetCubeMapSurface`, cube-map sampling
- `IDirect3DVolumeTexture9` / `IDirect3DVolume9`: contiguous mip storage, `LockBox` / `UnlockBox`, 3D texture sampling
- `IDirect3DVertexBuffer9`, `IDirect3DIndexBuffer9`
- `IDirect3DSurface9` (render target, depth-stencil, offscreen)
- Private data (`SetPrivateData` / `GetPrivateData` / `FreePrivateData`) on all resources
- Broad `D3DFORMAT` coverage: BGRA/RGBA families, depth formats, DXT/BC compressed, float HDR, signed bump-map formats, luminance formats, `D3DFMT_R3G3B2` (3+3+2-bit packed, expanded on upload)

### Factory and Device
- `Direct3DCreate9` and `Direct3DCreate9Ex` — both return a full `IDirect3D9Ex` implementation
- `IDirect3DDevice9Ex`: `CheckDeviceState` (returns S_OK — no lost device), `ResetEx`, `PresentEx`, `Create*Ex` surface variants

### Adapter Enumeration
- `GetAdapterCount` (1 software adapter), `GetAdapterIdentifier`, `GetAdapterModeCount` / `EnumAdapterModes`, `GetAdapterModeCountEx` / `EnumAdapterModesEx`, `GetDeviceCaps`, `ValidateDevice`, `CheckDeviceType/Format`

### Queries
- `CreateQuery`: EVENT (always ready), OCCLUSION (returns 0), TIMESTAMP (returns 0)

### Missing / deferred
- Clip plane enable in FF VS/PS
- `DOTPRODUCT3`, `BLENDDIFFUSEALPHA`, `BUMPENVMAP*` TSS ops
- `ProcessVertices`
- macOS/Win32 interactive example

</details>

## Screenshots

<details><summary><b>D3D11</b></summary>

**Triangle**
<p align="center">
    <img width="49%" src="examples/screenshots/RedTriangle.png" alt="Triangle"/>
</p>

**Textured Cube**
<p align="center">
    <img width="49%" src="examples/screenshots/TexturedCube.png" alt="Textured Cube"/>
&nbsp;
    <img width="49%" src="examples/screenshots/ProceduralTexturedCube.png" alt="Procedural Textured Cube"/>
</p>

**Instanced Cubes**
<p align="center">
    <img width="49%" src="examples/screenshots/InstancedCubes.png" alt="Instanced Cubes"/>
</p>

**Floor — Aliased vs Mipmapped**
<p align="center">
    <img width="49%" src="examples/screenshots/AliasedFloor.png" alt="Aliased Floor"/>
&nbsp;
    <img width="49%" src="examples/screenshots/AntialiasedFloor.png" alt="Mipmapped Floor"/>
</p>

**DirectX SDK Samples — Tutorial 10**
<p align="center">
    <img width="49%" src="examples/screenshots/DirectX-SDK-Samples-Tutorial10.png" alt="DirectX SDK Samples Tutorial 10"/>
</p>

**DirectX SDK Samples — DecalTessellation11**
<p align="center">
    <img width="49%" src="examples/screenshots/DirectX-SDK-Samples-DecalTesselation11_Tiger.png" alt="Tiger"/>
&nbsp;
    <img width="49%" src="examples/screenshots/DirectX-SDK-Samples-DecalTesselation11_Tiger2.png" alt="Wireframe Tiger"/>
</p>

</details>

## Tests

750+ tests across three categories:
- **Unit tests** — device, resources, views, states, formats, shader compilation, draw and compute pipelines, SM3 translator
- **Golden tests** — pixel-exact comparison against reference images for D3D11 and D3D9 rendering paths
- **Perf tests** — draw and compute benchmarks

## References

- [D3D11.3 Functional Spec](https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm) — rasterization rules, fixed-point precision, LOD calculation, texture filtering
- [D3D11 API Reference](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/) — API contracts, parameter rules, struct/enum definitions
- [Parsing DXBC](https://timjones.io/blog/archive/2015/09/02/parsing-direct3d-shader-bytecode) — DXBC container layout
- `d3d11TokenizedProgramFormat.hpp` (Windows SDK) — opcode definitions, operand encoding, token layout for SM4/SM5 bytecode
- [DirectX9 Graphics Reference](https://learn.microsoft.com/en-us/windows/win32/direct3d9/dx9-graphics-reference-d3d)

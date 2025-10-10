#include "misc/command_list.h"

namespace d3d11sw {


Direct3D11CommandListSW::Direct3D11CommandListSW(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE Direct3D11CommandListSW::GetContextFlags()
{
    return 0;
}

}

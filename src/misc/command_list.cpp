#include "misc/command_list.h"

MarsCommandList::MarsCommandList(ID3D11Device* device)
    : DeviceChildImpl(device) {}

UINT STDMETHODCALLTYPE MarsCommandList::GetContextFlags()
{
    return 0;
}

#include "misc/multithread.h"

namespace d3d11sw {


void STDMETHODCALLTYPE D3D11MultithreadSW::Enter()
{
}

void STDMETHODCALLTYPE D3D11MultithreadSW::Leave()
{
}

BOOL STDMETHODCALLTYPE D3D11MultithreadSW::SetMultithreadProtected(BOOL bMTProtect)
{
    return FALSE;
}

BOOL STDMETHODCALLTYPE D3D11MultithreadSW::GetMultithreadProtected()
{
    return FALSE;
}

}

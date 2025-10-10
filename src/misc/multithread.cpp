#include "misc/multithread.h"

namespace d3d11sw {


void STDMETHODCALLTYPE Direct3D11MultithreadSW::Enter()
{
}

void STDMETHODCALLTYPE Direct3D11MultithreadSW::Leave()
{
}

BOOL STDMETHODCALLTYPE Direct3D11MultithreadSW::SetMultithreadProtected(BOOL bMTProtect)
{
    return FALSE;
}

BOOL STDMETHODCALLTYPE Direct3D11MultithreadSW::GetMultithreadProtected()
{
    return FALSE;
}

}

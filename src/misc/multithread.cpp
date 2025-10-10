#include "misc/multithread.h"

void STDMETHODCALLTYPE MarsMultithread::Enter()
{
}

void STDMETHODCALLTYPE MarsMultithread::Leave()
{
}

BOOL STDMETHODCALLTYPE MarsMultithread::SetMultithreadProtected(BOOL bMTProtect)
{
    return FALSE;
}

BOOL STDMETHODCALLTYPE MarsMultithread::GetMultithreadProtected()
{
    return FALSE;
}

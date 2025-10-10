#pragma once

#include "common/unknown_impl.h"

class MarsMultithread : public UnknownImpl<ID3D11Multithread>
{
public:
    void STDMETHODCALLTYPE Enter() override;
    void STDMETHODCALLTYPE Leave() override;
    BOOL STDMETHODCALLTYPE SetMultithreadProtected(BOOL bMTProtect) override;
    BOOL STDMETHODCALLTYPE GetMultithreadProtected() override;
};

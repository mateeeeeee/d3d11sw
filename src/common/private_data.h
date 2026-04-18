#pragma once
#include "common.h"
#include <vector>
#include <cstring>

namespace d3d11sw {

enum class PrivateDataType : Uint32
{
    None,
    Data,
    Iface,
};

class PrivateDataEntry
{
public:
    PrivateDataEntry() = default;

    PrivateDataEntry(REFGUID guid, UINT size, const void* data)
        : _guid(guid), _type(PrivateDataType::Data), _size(size)
    {
        if (size > 0 && data)
        {
            _data = new Uint8[size];
            std::memcpy(_data, data, size);
        }
    }

    PrivateDataEntry(REFGUID guid, const IUnknown* iface)
        : _guid(guid), _type(PrivateDataType::Iface)
    {
        _iface = const_cast<IUnknown*>(iface);
        if (_iface)
        {
            _iface->AddRef();
        }
    }

    ~PrivateDataEntry() { Destroy(); }

    PrivateDataEntry(PrivateDataEntry&& other) noexcept
        : _guid(other._guid), _type(other._type), _size(other._size),
          _data(other._data), _iface(other._iface)
    {
        other._type  = PrivateDataType::None;
        other._size  = 0;
        other._data  = nullptr;
        other._iface = nullptr;
    }

    PrivateDataEntry& operator=(PrivateDataEntry&& other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            _guid  = other._guid;
            _type  = other._type;
            _size  = other._size;
            _data  = other._data;
            _iface = other._iface;
            other._type  = PrivateDataType::None;
            other._size  = 0;
            other._data  = nullptr;
            other._iface = nullptr;
        }
        return *this;
    }

    D3D11SW_NONCOPYABLE(PrivateDataEntry)

    REFGUID Guid() const { return _guid; }
    Bool HasGuid(REFGUID guid) const { return _guid == guid; }

    HRESULT Get(UINT* pSize, void* pData) const
    {
        if (!pSize)
        {
            return E_INVALIDARG;
        }

        if (_type == PrivateDataType::Iface)
        {
            UINT needed = sizeof(IUnknown*);
            if (!pData)
            {
                *pSize = needed;
                return S_OK;
            }
            if (*pSize < needed)
            {
                *pSize = needed;
                return DXGI_ERROR_MORE_DATA;
            }
            *pSize = needed;
            if (_iface)
            {
                _iface->AddRef();
            }
            *static_cast<IUnknown**>(pData) = _iface;
            return S_OK;
        }

        if (!pData)
        {
            *pSize = _size;
            return S_OK;
        }

        if (*pSize < _size)
        {
            *pSize = _size;
            return DXGI_ERROR_MORE_DATA;
        }

        *pSize = _size;
        if (_size > 0 && _data)
        {
            std::memcpy(pData, _data, _size);
        }
        return S_OK;
    }

private:
    GUID             _guid  = {};
    PrivateDataType  _type  = PrivateDataType::None;
    UINT             _size  = 0;
    void*            _data  = nullptr;
    IUnknown*        _iface = nullptr;

private:
    void Destroy()
    {
        if (_type == PrivateDataType::Data)
        {
            delete[] static_cast<Uint8*>(_data);
        }
        else if (_type == PrivateDataType::Iface && _iface)
        {
            _iface->Release();
        }

        _type  = PrivateDataType::None;
        _size  = 0;
        _data  = nullptr;
        _iface = nullptr;
    }
};

class PrivateDataStore
{
public:
    HRESULT SetData(REFGUID guid, UINT size, const void* data)
    {
        InsertEntry(PrivateDataEntry(guid, size, data));
        return S_OK;
    }

    HRESULT SetInterface(REFGUID guid, const IUnknown* iface)
    {
        InsertEntry(PrivateDataEntry(guid, iface));
        return S_OK;
    }

    HRESULT GetData(REFGUID guid, UINT* pSize, void* pData)
    {
        PrivateDataEntry* entry = FindEntry(guid);
        if (!entry)
        {
            if (pSize)
            {
                *pSize = 0;
            }
            return DXGI_ERROR_NOT_FOUND;
        }
        return entry->Get(pSize, pData);
    }

private:
    PrivateDataEntry* FindEntry(REFGUID guid)
    {
        for (auto& e : _entries)
        {
            if (e.HasGuid(guid))
            {
                return &e;
            }
        }
        return nullptr;
    }

    void InsertEntry(PrivateDataEntry&& entry)
    {
        for (auto& e : _entries)
        {
            if (e.HasGuid(entry.Guid()))
            {
                e = std::move(entry);
                return;
            }
        }
        _entries.push_back(std::move(entry));
    }

    std::vector<PrivateDataEntry> _entries;
};

class PrivateDataMixin
{
protected:
    HRESULT GetPrivateDataImpl(REFGUID guid, UINT* pDataSize, void* pData)
    {
        return _privateData.GetData(guid, pDataSize, pData);
    }

    HRESULT SetPrivateDataImpl(REFGUID guid, UINT dataSize, const void* pData)
    {
        return _privateData.SetData(guid, dataSize, pData);
    }

    HRESULT SetPrivateDataInterfaceImpl(REFGUID guid, const IUnknown* pData)
    {
        return _privateData.SetInterface(guid, pData);
    }

private:
    PrivateDataStore _privateData;
};

}

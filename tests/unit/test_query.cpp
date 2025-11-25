#include <gtest/gtest.h>
#include <d3d11_4.h>
#include "misc/query.h"

using namespace d3d11sw;

struct QueryTests : ::testing::Test
{
    ID3D11Device*        device  = nullptr;
    ID3D11DeviceContext* context = nullptr;

    void SetUp() override
    {
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &device,
            &featureLevel,
            &context);
        ASSERT_TRUE(SUCCEEDED(hr));
        ASSERT_NE(device, nullptr);
        ASSERT_NE(context, nullptr);
    }

    void TearDown() override
    {
        if (context) { context->Release(); context = nullptr; }
        if (device)  { device->Release();  device  = nullptr; }
    }
};

TEST_F(QueryTests, CreateTimestampQuery)
{
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_TIMESTAMP;

    ID3D11Query* query = nullptr;
    HRESULT hr = device->CreateQuery(&desc, &query);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(query, nullptr);

    EXPECT_EQ(query->GetDataSize(), sizeof(UINT64));

    D3D11_QUERY_DESC outDesc = {};
    query->GetDesc(&outDesc);
    EXPECT_EQ(outDesc.Query, D3D11_QUERY_TIMESTAMP);

    query->Release();
}

TEST_F(QueryTests, CreateTimestampDisjointQuery)
{
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

    ID3D11Query* query = nullptr;
    HRESULT hr = device->CreateQuery(&desc, &query);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(query, nullptr);

    EXPECT_EQ(query->GetDataSize(), sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT));

    query->Release();
}

TEST_F(QueryTests, CreateEventQuery)
{
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_EVENT;

    ID3D11Query* query = nullptr;
    HRESULT hr = device->CreateQuery(&desc, &query);
    ASSERT_TRUE(SUCCEEDED(hr));
    ASSERT_NE(query, nullptr);

    EXPECT_EQ(query->GetDataSize(), sizeof(BOOL));

    query->Release();
}

TEST_F(QueryTests, UnsupportedQueryTypeReturnsError)
{
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_OCCLUSION;

    ID3D11Query* query = nullptr;
    HRESULT hr = device->CreateQuery(&desc, &query);
    EXPECT_TRUE(FAILED(hr));
}

TEST_F(QueryTests, ValidationOnlyReturnsS_FALSE)
{
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_TIMESTAMP;

    HRESULT hr = device->CreateQuery(&desc, nullptr);
    EXPECT_EQ(hr, S_FALSE);
}

TEST_F(QueryTests, TimestampEndAndGetData)
{
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_TIMESTAMP;

    ID3D11Query* query = nullptr;
    HRESULT hr = device->CreateQuery(&desc, &query);
    ASSERT_TRUE(SUCCEEDED(hr));

    // Before End, GetData should return S_FALSE
    UINT64 ts = 0;
    hr = context->GetData(query, &ts, sizeof(ts), 0);
    EXPECT_EQ(hr, S_FALSE);

    context->End(query);

    hr = context->GetData(query, &ts, sizeof(ts), 0);
    EXPECT_EQ(hr, S_OK);
    EXPECT_GT(ts, 0u);

    query->Release();
}

TEST_F(QueryTests, TimestampMonotonic)
{
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_TIMESTAMP;

    ID3D11Query* q1 = nullptr;
    ID3D11Query* q2 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateQuery(&desc, &q1)));
    ASSERT_TRUE(SUCCEEDED(device->CreateQuery(&desc, &q2)));

    context->End(q1);
    context->End(q2);

    UINT64 t1 = 0, t2 = 0;
    ASSERT_EQ(context->GetData(q1, &t1, sizeof(t1), 0), S_OK);
    ASSERT_EQ(context->GetData(q2, &t2, sizeof(t2), 0), S_OK);

    EXPECT_GE(t2, t1);

    q1->Release();
    q2->Release();
}

TEST_F(QueryTests, DisjointBracketsTimestamps)
{
    D3D11_QUERY_DESC disjointDesc = {};
    disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

    D3D11_QUERY_DESC tsDesc = {};
    tsDesc.Query = D3D11_QUERY_TIMESTAMP;

    ID3D11Query* disjoint = nullptr;
    ID3D11Query* ts1 = nullptr;
    ID3D11Query* ts2 = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateQuery(&disjointDesc, &disjoint)));
    ASSERT_TRUE(SUCCEEDED(device->CreateQuery(&tsDesc, &ts1)));
    ASSERT_TRUE(SUCCEEDED(device->CreateQuery(&tsDesc, &ts2)));

    context->Begin(disjoint);
    context->End(ts1);
    context->End(ts2);
    context->End(disjoint);

    UINT64 t1 = 0, t2 = 0;
    ASSERT_EQ(context->GetData(ts1, &t1, sizeof(t1), 0), S_OK);
    ASSERT_EQ(context->GetData(ts2, &t2, sizeof(t2), 0), S_OK);
    EXPECT_GE(t2, t1);

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData = {};
    ASSERT_EQ(context->GetData(disjoint, &disjointData, sizeof(disjointData), 0), S_OK);
    EXPECT_EQ(disjointData.Frequency, 1'000'000'000u);
    EXPECT_EQ(disjointData.Disjoint, FALSE);

    disjoint->Release();
    ts1->Release();
    ts2->Release();
}

TEST_F(QueryTests, EventGetData)
{
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_EVENT;

    ID3D11Query* query = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateQuery(&desc, &query)));

    context->End(query);

    BOOL done = FALSE;
    HRESULT hr = context->GetData(query, &done, sizeof(done), 0);
    EXPECT_EQ(hr, S_OK);
    EXPECT_EQ(done, TRUE);

    query->Release();
}

TEST_F(QueryTests, QueryInterfaceToQuery1)
{
    D3D11_QUERY_DESC desc = {};
    desc.Query = D3D11_QUERY_TIMESTAMP;

    ID3D11Query* query = nullptr;
    ASSERT_TRUE(SUCCEEDED(device->CreateQuery(&desc, &query)));

    ID3D11Query1* query1 = nullptr;
    HRESULT hr = query->QueryInterface(__uuidof(ID3D11Query1), reinterpret_cast<void**>(&query1));
    EXPECT_TRUE(SUCCEEDED(hr));
    EXPECT_NE(query1, nullptr);

    if (query1)
    {
        D3D11_QUERY_DESC1 desc1 = {};
        query1->GetDesc1(&desc1);
        EXPECT_EQ(desc1.Query, D3D11_QUERY_TIMESTAMP);
        query1->Release();
    }

    query->Release();
}

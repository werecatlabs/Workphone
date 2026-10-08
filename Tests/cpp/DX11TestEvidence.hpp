#ifndef WORKPHONE_DX11_TEST_EVIDENCE_HPP
#define WORKPHONE_DX11_TEST_EVIDENCE_HPP

#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>

inline bool recordDX11TestDevice( ID3D11Device *device )
{
    if( !device )
        return false;
    IDXGIDevice *dxgiDevice = nullptr;
    if( FAILED( device->QueryInterface( IID_PPV_ARGS( &dxgiDevice ) ) ) )
        return false;
    IDXGIAdapter *adapter = nullptr;
    const auto adapterResult = dxgiDevice->GetAdapter( &adapter );
    dxgiDevice->Release();
    if( FAILED( adapterResult ) )
        return false;
    DXGI_ADAPTER_DESC description{};
    const auto descriptionResult = adapter->GetDesc( &description );
    LARGE_INTEGER driverVersion{};
    const auto driverResult = adapter->CheckInterfaceSupport( __uuidof( IDXGIDevice ), &driverVersion );
    adapter->Release();
    if( FAILED( descriptionResult ) )
        return false;
    char name[512]{};
    if( !WideCharToMultiByte( CP_UTF8, 0, description.Description, -1, name, sizeof( name ), nullptr,
                              nullptr ) )
        return false;
    std::printf( "DX11 device: %s; vendor=%04x device=%04x; featureLevel=%04x\n", name,
                 description.VendorId, description.DeviceId, device->GetFeatureLevel() );
    if( SUCCEEDED( driverResult ) )
        std::printf( "DX11 driver: %u.%u.%u.%u\n", HIWORD( driverVersion.HighPart ),
                     LOWORD( driverVersion.HighPart ), HIWORD( driverVersion.LowPart ),
                     LOWORD( driverVersion.LowPart ) );
    else
        std::printf( "DX11 driver version unavailable: HRESULT=%08lx\n", driverResult );
    return true;
}

#endif

#include "logo.hpp"
#include "resource.h"
#include <shlwapi.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <array>

namespace shaker {
ID3D11ShaderResourceView* createLogoTexture(ID3D11Device* device){
    using Microsoft::WRL::ComPtr;
    const auto module=GetModuleHandleW(nullptr);
    const auto resource=FindResourceW(module,MAKEINTRESOURCEW(IDR_APP_LOGO),RT_RCDATA);
    if(!resource)return nullptr;
    const auto data=LoadResource(module,resource);
    const auto bytes=static_cast<const BYTE*>(LockResource(data));
    if(!bytes)return nullptr;
    const auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    struct ComScope {HRESULT result;~ComScope(){if(SUCCEEDED(result))CoUninitialize();}} com{initialized};
    if(FAILED(initialized)&&initialized!=RPC_E_CHANGED_MODE)return nullptr;
    ComPtr<IStream> stream;stream.Attach(SHCreateMemStream(bytes,SizeofResource(module,resource)));
    ComPtr<IWICImagingFactory> factory;ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICBitmapScaler> scaler;ComPtr<IWICFormatConverter> converter;
    if(!stream||FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)))||
       FAILED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder))||
       FAILED(decoder->GetFrame(0,&frame))||FAILED(factory->CreateBitmapScaler(&scaler))||
       FAILED(scaler->Initialize(frame.Get(),128,128,WICBitmapInterpolationModeFant))||
       FAILED(factory->CreateFormatConverter(&converter))||
       FAILED(converter->Initialize(scaler.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return nullptr;
    std::array<BYTE,128*128*4> pixels{};
    if(FAILED(converter->CopyPixels(nullptr,128*4,UINT(pixels.size()),pixels.data())))return nullptr;
    D3D11_TEXTURE2D_DESC desc{};desc.Width=desc.Height=128;desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial{};initial.pSysMem=pixels.data();initial.SysMemPitch=128*4;
    ComPtr<ID3D11Texture2D> texture;ID3D11ShaderResourceView* view=nullptr;
    if(FAILED(device->CreateTexture2D(&desc,&initial,&texture))||FAILED(device->CreateShaderResourceView(texture.Get(),nullptr,&view)))return nullptr;
    return view;
}
}

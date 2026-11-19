#ifdef _WIN32
#include "earth_video.hpp"
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <d3d11_4.h>
#include <propvarutil.h>
#include <wrl/client.h>
#include <chrono>
#include <cstring>
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"propsys.lib")
#pragma comment(lib,"d3d11.lib")
using Microsoft::WRL::ComPtr;
namespace mcl {
void EarthVideo::start(const std::filesystem::path& file,std::uint32_t width,std::uint32_t height){
 stop();
 std::error_code ec;
 if(!std::filesystem::is_regular_file(file,ec)){failed_=true;return;}
 reqW_=width;reqH_=height;quit_=false;failed_=false;
 worker_=std::thread([this,file]{run(file);});
}
void EarthVideo::stop(){
 quit_=true;
 if(worker_.joinable())worker_.join();
}
bool EarthVideo::latest(std::vector<std::uint32_t>& out,std::uint32_t& w,std::uint32_t& h,std::uint64_t& serial){
 std::lock_guard<std::mutex> lock(m_);
 if(serial_==0||serial_==serial)return false;
 out=frame_;w=w_;h=h_;serial=serial_;return true;
}
void EarthVideo::run(std::filesystem::path file){
 const HRESULT co=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 if(FAILED(MFStartup(MF_VERSION,MFSTARTUP_LITE))){failed_=true;if(SUCCEEDED(co))CoUninitialize();return;}
 [&]{
  // Prefer GPU decoding + GPU scaling through a D3D11 device manager; fall back to software.
  ComPtr<ID3D11Device> device;ComPtr<IMFDXGIDeviceManager> manager;
  {
   const UINT flags=D3D11_CREATE_DEVICE_VIDEO_SUPPORT|D3D11_CREATE_DEVICE_BGRA_SUPPORT;
   if(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,flags,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,nullptr))){
    ComPtr<ID3D11Multithread> mt;if(SUCCEEDED(device.As(&mt)))mt->SetMultithreadProtected(TRUE);
    UINT token=0;
    if(FAILED(MFCreateDXGIDeviceManager(&token,&manager))||FAILED(manager->ResetDevice(device.Get(),token)))manager.Reset();
   }
  }
  ComPtr<IMFSourceReader> reader;
  for(int attempt=0;attempt<2&&!reader;++attempt){
   const bool useGpu=attempt==0&&manager;
   if(attempt==0&&!manager)continue;
   ComPtr<IMFAttributes> attr;
   if(FAILED(MFCreateAttributes(&attr,3)))return void(failed_=true);
   attr->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING,TRUE);
   attr->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS,TRUE);
   if(useGpu)attr->SetUnknown(MF_SOURCE_READER_D3D_MANAGER,manager.Get());
   if(SUCCEEDED(MFCreateSourceReaderFromURL(file.c_str(),attr.Get(),&reader)))hardware_=useGpu;
   else reader.Reset();
  }
  if(!reader)return void(failed_=true);
  const DWORD video=static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
  reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS),FALSE);
  reader->SetStreamSelection(video,TRUE);
  ComPtr<IMFMediaType> type;
  MFCreateMediaType(&type);
  type->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video);
  type->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_RGB32);
  MFSetAttributeSize(type.Get(),MF_MT_FRAME_SIZE,reqW_,reqH_);
  if(FAILED(reader->SetCurrentMediaType(video,nullptr,type.Get()))){
   // Fall back to native size if the processor refuses to scale.
   type->DeleteItem(MF_MT_FRAME_SIZE);
   if(FAILED(reader->SetCurrentMediaType(video,nullptr,type.Get())))return void(failed_=true);
  }
  ComPtr<IMFMediaType> actual;
  if(FAILED(reader->GetCurrentMediaType(video,&actual)))return void(failed_=true);
  UINT32 w=0,h=0;MFGetAttributeSize(actual.Get(),MF_MT_FRAME_SIZE,&w,&h);
  if(!w||!h)return void(failed_=true);
  LONG stride=static_cast<LONG>(w*4);
  {UINT32 s=0;if(SUCCEEDED(actual->GetUINT32(MF_MT_DEFAULT_STRIDE,&s)))stride=static_cast<LONG>(static_cast<INT32>(s));}
  std::vector<std::uint32_t> pixels(static_cast<size_t>(w)*h);
  using clock=std::chrono::steady_clock;
  auto loopStart=clock::now();
  while(!quit_){
   if(paused_){
    const auto pauseStart=clock::now();
    while(paused_&&!quit_)std::this_thread::sleep_for(std::chrono::milliseconds(50));
    loopStart+=clock::now()-pauseStart; // keep pacing continuous after a pause
    continue;
   }
   DWORD stream=0,flags=0;LONGLONG ts=0;ComPtr<IMFSample> sample;
   if(FAILED(reader->ReadSample(video,0,&stream,&flags,&ts,&sample))){failed_=true;return;}
   if(flags&MF_SOURCE_READERF_ENDOFSTREAM){
    PROPVARIANT pos;InitPropVariantFromInt64(0,&pos);
    reader->SetCurrentPosition(GUID_NULL,pos);PropVariantClear(&pos);
    loopStart=clock::now();continue;
   }
   if(!sample)continue;
   ComPtr<IMFMediaBuffer> buffer;
   if(FAILED(sample->ConvertToContiguousBuffer(&buffer)))continue;
   ComPtr<IMF2DBuffer> twoD;BYTE* scan0=nullptr;LONG pitch=stride;BYTE* raw=nullptr;DWORD len=0;
   if(SUCCEEDED(buffer.As(&twoD))&&SUCCEEDED(twoD->Lock2D(&scan0,&pitch))){}
   else if(SUCCEEDED(buffer->Lock(&raw,nullptr,&len))){scan0=pitch<0?raw+static_cast<size_t>(-pitch)*(h-1):raw;}
   else continue;
   for(UINT32 y=0;y<h;++y)std::memcpy(&pixels[static_cast<size_t>(y)*w],scan0+static_cast<ptrdiff_t>(y)*pitch,static_cast<size_t>(w)*4);
   if(twoD&&scan0&&!raw)twoD->Unlock2D();else buffer->Unlock();
   // Pace to the presentation timestamp (100 ns units).
   const auto due=loopStart+std::chrono::microseconds(ts/10);
   while(!quit_&&!paused_&&clock::now()<due)std::this_thread::sleep_for(std::chrono::milliseconds(2));
   std::lock_guard<std::mutex> lock(m_);
   frame_=pixels;w_=w;h_=h;++serial_;
  }
 }();
 MFShutdown();
 if(SUCCEEDED(co))CoUninitialize();
}
}
#endif

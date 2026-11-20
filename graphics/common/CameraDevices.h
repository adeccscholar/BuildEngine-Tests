#pragma once

#include <windows.h>
#include <dshow.h>
#include <oleauto.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

/**
\brief A registered Windows DirectShow video capture source.
\details Registration and availability are separate. Disconnected or busy
   cameras may remain listed; status is determined by explicit capture opening.
*/
struct CameraDevice {
   int iIndex {};
   std::string strName;
   std::string strDevicePath;
};

template<class T>
struct ComRelease {
   void operator()(T* const pObject) const noexcept {
      if(pObject) pObject->Release();
   }
};

inline std::string FromWide(WCHAR const* const szValue) {
   if(!szValue) return {};
   int const iSize=WideCharToMultiByte(CP_UTF8,0,szValue,-1,nullptr,0,nullptr,nullptr);
   if(iSize<=1) return {};
   std::string strText(static_cast<std::size_t>(iSize),'\0');
   WideCharToMultiByte(CP_UTF8,0,szValue,-1,strText.data(),iSize,nullptr,nullptr);
   strText.resize(static_cast<std::size_t>(iSize-1));
   return strText;
}

/**
\brief Lists registered DirectShow video sources without opening them.
\details Uses the Windows System Device Enumerator and the same capture
   category as OpenCV CAP_DSHOW. Device order may change after reconnecting.
\returns Ordered devices including ones that are currently unavailable for capture.
\throw std::runtime_error On fatal device enumeration errors.
\note Windows-disabled devices may be absent from the registry enumeration.
*/
inline std::vector<CameraDevice> EnumerateCameras() {
   HRESULT const iCOM=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
   if(FAILED(iCOM) && iCOM!=RPC_E_CHANGED_MODE)
      throw std::runtime_error("DirectShow COM initialization failed");
   struct COMGuard {
      bool bOwn {};
      ~COMGuard() { if(bOwn) CoUninitialize(); }
   } const theGuard {SUCCEEDED(iCOM)};

   ICreateDevEnum* pRawEnumerator {};
   HRESULT const iCreated=CoCreateInstance(CLSID_SystemDeviceEnum,nullptr,
      CLSCTX_INPROC_SERVER,IID_ICreateDevEnum,
      reinterpret_cast<void**>(&pRawEnumerator));
   if(FAILED(iCreated) || !pRawEnumerator)
      throw std::runtime_error("Cannot create DirectShow system device enumerator");
   std::unique_ptr<ICreateDevEnum,ComRelease<ICreateDevEnum>> const pEnumerator(pRawEnumerator);

   IEnumMoniker* pRawMonikers {};
   HRESULT const iCategory=pEnumerator->CreateClassEnumerator(
      CLSID_VideoInputDeviceCategory,&pRawMonikers,0);
   if(iCategory==S_FALSE) return {};
   if(FAILED(iCategory) || !pRawMonikers)
      throw std::runtime_error("Cannot enumerate DirectShow video capture devices");
   std::unique_ptr<IEnumMoniker,ComRelease<IEnumMoniker>> const pMonikers(pRawMonikers);

   std::vector<CameraDevice> vecDevices;
   IMoniker* pRawMoniker {};
   while(pMonikers->Next(1,&pRawMoniker,nullptr)==S_OK) {
      std::unique_ptr<IMoniker,ComRelease<IMoniker>> const pMoniker(pRawMoniker);
      CameraDevice theDevice;
      theDevice.iIndex=static_cast<int>(vecDevices.size());
      theDevice.strName="Camera "+std::to_string(theDevice.iIndex);

      IPropertyBag* pRawBag {};
      if(SUCCEEDED(pMoniker->BindToStorage(nullptr,nullptr,IID_IPropertyBag,
         reinterpret_cast<void**>(&pRawBag))) && pRawBag) {
         std::unique_ptr<IPropertyBag,ComRelease<IPropertyBag>> const pBag(pRawBag);
         VARIANT theValue {};
         VariantInit(&theValue);
         if(SUCCEEDED(pBag->Read(L"FriendlyName",&theValue,nullptr)) &&
            theValue.vt==VT_BSTR)
            theDevice.strName=FromWide(theValue.bstrVal);
         VariantClear(&theValue);
         VariantInit(&theValue);
         if(SUCCEEDED(pBag->Read(L"DevicePath",&theValue,nullptr)) &&
            theValue.vt==VT_BSTR)
            theDevice.strDevicePath=FromWide(theValue.bstrVal);
         VariantClear(&theValue);
      }
      vecDevices.push_back(std::move(theDevice));
      pRawMoniker=nullptr;
   }
   return vecDevices;
}

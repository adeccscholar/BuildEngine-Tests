// SPDX-License-Identifier: MIT
#include "GeoData.h"
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include "include/core/SkCanvas.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkSurface.h"
#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>
using namespace adecc::geo;
namespace {
struct App {
   Dataset data;
   std::wstring sourcePath;
   bool directorySource=false;
   Bounds bounds{};
   bool loaded=false, parallel=true;
   Mode mode=Mode::Points;
   bool relief=false;
   Method method=Method::OpenMP;
   int threads=8, cell=12;
   int width=1000,height=700;
   double boundMs=0;
   Image image;
   void Render() {
      if(!loaded)return;
      auto const start=std::chrono::steady_clock::now();
      bounds=ProjectBounds(data.points,parallel);
      boundMs=std::chrono::duration<double,std::milli>(
         std::chrono::steady_clock::now()-start).count();
      image=Rasterize(data.points,bounds,width,height,mode,method,threads,cell);
      if(relief) {
         auto terrain=BuildTerrain(data.points,bounds,width,height,
            static_cast<double>(cell),method,threads);
         for(std::size_t i=0;i<image.argb.size();++i) {
            float const h=terrain.peak>0?terrain.heightField[i]/terrain.peak:0;
            auto const value=static_cast<unsigned>(255.0f*std::sqrt(h));
            image.argb[i]=0xff000000u|(value<<16)|((value*3/4)<<8)|(255u-value);
         }
      }
   }
};
App app;
std::string Utf8(std::wstring const& w) {
   if(w.empty())return {};
   int const n=WideCharToMultiByte(CP_UTF8,0,w.data(),static_cast<int>(w.size()),
      nullptr,0,nullptr,nullptr);
   std::string s(static_cast<std::size_t>(n),'\0');
   WideCharToMultiByte(CP_UTF8,0,w.data(),static_cast<int>(w.size()),
      s.data(),n,nullptr,nullptr);
   return s;
}
void LoadFile(HWND window,std::wstring const& path,bool directory=false) {
   try {
      app.data=directory?LoadDirectory(Utf8(path),app.parallel):Load(Utf8(path),app.parallel);
      app.sourcePath=path;
      app.directorySource=directory;
      app.loaded=true;
      app.Render();
      InvalidateRect(window,nullptr,FALSE);
   } catch(std::exception const& e) {
      app.loaded=false;
      MessageBoxA(window,e.what(),"Cannot load address dataset",MB_OK|MB_ICONERROR);
   }
}
void ChooseDirectory(HWND window) {
   // Select any hierarchical street CSV and use its city/unit/district root.
   wchar_t filename[32768]{};
   OPENFILENAMEW dialog{};
   dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;
   dialog.lpstrFile=filename;dialog.nMaxFile=32768;
   dialog.lpstrFilter=L"CompareTest street CSV\0*.csv\0";
   dialog.Flags=OFN_FILEMUSTEXIST;
   if(GetOpenFileNameW(&dialog)) {
      std::filesystem::path root(filename);
      for(int i=0;i<4;++i)root=root.parent_path();
      LoadFile(window,root.wstring(),true);
   }
}
void ChooseFile(HWND window) {
   wchar_t filename[32768]{};
   OPENFILENAMEW dialog{};
   dialog.lStructSize=sizeof(dialog);
   dialog.hwndOwner=window;
   dialog.lpstrFile=filename;
   dialog.nMaxFile=32768;
   dialog.lpstrFilter=L"Address data (*.dat;*.txt)\0*.dat;*.txt\0All files\0*.*\0";
   dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
   if(GetOpenFileNameW(&dialog))LoadFile(window,filename);
}
void Paint(HWND window,HDC dc) {
   RECT rect{};GetClientRect(window,&rect);
   int const w=(std::max)(1,rect.right),h=(std::max)(1,rect.bottom);
   if(app.width!=w||app.height!=h) {
      app.width=w;app.height=h;
      if(app.loaded)app.Render();
   }
   auto surface=SkSurfaces::Raster(SkImageInfo::MakeN32Premul(w,h));
   if(!surface)return;
   SkCanvas* canvas=surface->getCanvas();
   canvas->clear(0xff071525);
   if(app.loaded&&!app.image.argb.empty()) {
      SkPixmap const pixmap(SkImageInfo::MakeN32Premul(w,h),
         app.image.argb.data(),static_cast<std::size_t>(w)*4);
      auto picture=SkImages::RasterFromPixmapCopy(pixmap);
      if(picture)canvas->drawImage(picture,0,0,SkSamplingOptions(),nullptr);
   }
   SkPixmap pixels;
   if(!surface->peekPixels(&pixels))return;
   BITMAPINFO bmi{};
   bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
   bmi.bmiHeader.biWidth=w;bmi.bmiHeader.biHeight=-h;
   bmi.bmiHeader.biPlanes=1;bmi.bmiHeader.biBitCount=32;
   bmi.bmiHeader.biCompression=BI_RGB;
   StretchDIBits(dc,0,0,w,h,0,0,w,h,pixels.addr(),
      &bmi,DIB_RGB_COLORS,SRCCOPY);
   wchar_t title[600];
   if(app.loaded) {
      swprintf_s(title,600,L"GeoSpatial Skia | %zu addresses | %s | %s | load %.1f ms | bounds %.1f ms | prep %.1f ms | cell %d | peak %u",
         app.data.points.size(),app.relief?L"relief":app.mode==Mode::Points?L"points":L"density",
         app.method==Method::Serial?L"serial":app.method==Method::Tbb?L"oneTBB":L"OpenMP",
         app.data.readMs,app.boundMs,app.image.elapsedMs,app.cell,app.image.peak);
   } else wcscpy_s(title,L"GeoSpatial Skia | Ctrl+O to open CompareTest berlin_infos.dat | 1 points / 2 density");
   SetWindowTextW(window,title);
}
LRESULT CALLBACK WindowProc(HWND window,UINT msg,WPARAM wp,LPARAM lp) {
   switch(msg) {
      case WM_CREATE: {
         DragAcceptFiles(window,TRUE);
         RECT r{};GetClientRect(window,&r);
         app.width=(std::max)(1,r.right);app.height=(std::max)(1,r.bottom);
         return 0;
      }
      case WM_DROPFILES: {
         HDROP drop=reinterpret_cast<HDROP>(wp);
         wchar_t path[32768]{};
         if(DragQueryFileW(drop,0,path,32768))LoadFile(window,path);
         DragFinish(drop);return 0;
      }
      case WM_KEYDOWN: {
         bool refresh=true;
         if((GetKeyState(VK_CONTROL)&0x8000)!=0 && wp=='O'){ChooseFile(window);return 0;}
         if((GetKeyState(VK_CONTROL)&0x8000)!=0 && wp=='D'){ChooseDirectory(window);return 0;}
         switch(wp) {
            case '1':app.mode=Mode::Points;app.relief=false;break;
            case '2':app.mode=Mode::Density;app.relief=false;break;
            case '3':app.relief=true;break;
            case 'S':app.method=Method::Serial;break;
            case 'T':app.method=Method::Tbb;break;
            case 'P':app.method=Method::OpenMP;break;
            case VK_OEM_PLUS:app.cell=(std::min)(64,app.cell+2);break;
            case VK_OEM_MINUS:app.cell=(std::max)(2,app.cell-2);break;
            case VK_F5:break;
            case VK_ESCAPE:DestroyWindow(window);return 0;
            default:refresh=false;break;
         }
         if(refresh&&app.loaded){app.Render();InvalidateRect(window,nullptr,FALSE);}
         return 0;
      }
      case WM_SIZE:InvalidateRect(window,nullptr,FALSE);return 0;
      case WM_ERASEBKGND:return 1;
      case WM_PAINT: {
         PAINTSTRUCT ps{};HDC dc=BeginPaint(window,&ps);
         Paint(window,dc);EndPaint(window,&ps);return 0;
      }
      case WM_DESTROY:PostQuitMessage(0);return 0;
   }
   return DefWindowProcW(window,msg,wp,lp);
}
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE,LPSTR,int show) {
   WNDCLASSW klass{};
   klass.hInstance=instance;
   klass.lpfnWndProc=WindowProc;
   klass.lpszClassName=L"adecc.geospatial.skia";
   klass.hCursor=LoadCursor(nullptr,IDC_ARROW);
   if(!RegisterClassW(&klass))return 1;
   HWND window=CreateWindowW(klass.lpszClassName,L"GeoSpatial Skia",WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,CW_USEDEFAULT,1100,760,nullptr,nullptr,instance,nullptr);
   if(!window)return 2;
   ShowWindow(window,show);
   int argc=0;
   LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);
   if(argv) {
      if(argc>1) {
         std::filesystem::path path(argv[1]);
         LoadFile(window,argv[1],std::filesystem::is_directory(path));
      }
      LocalFree(argv);
   }
   MSG message{};
   while(GetMessageW(&message,nullptr,0,0)>0){
      TranslateMessage(&message);DispatchMessageW(&message);
   }
   return static_cast<int>(message.wParam);
}

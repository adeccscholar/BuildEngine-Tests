#pragma once
#include "VectorStage.h"
#include <imgui.h>
#include <format>
/** \brief Export actual native ImGui draw list triangles to a presentation SVG. */
inline void ExportImGui(VectorStage& aStage,ImDrawData const* const pData){
   if(!pData)return;
   for(int iList=0;iList<pData->CmdListsCount;++iList){
      ImDrawList const* const pList=pData->CmdLists[iList];
      for(int i=0;i+2<pList->IdxBuffer.Size;i+=3){
         auto const& a=pList->VtxBuffer[pList->IdxBuffer[i]];
         auto const& b=pList->VtxBuffer[pList->IdxBuffer[i+1]];
         auto const& c=pList->VtxBuffer[pList->IdxBuffer[i+2]];
         unsigned const uColor=a.col,uAlpha=(uColor>>24)&255U;
         if(!uAlpha)continue;
         aStage.Raw(std::format("<polygon points=\"{:.2f},{:.2f} {:.2f},{:.2f} {:.2f},{:.2f}\" fill=\"#{:02x}{:02x}{:02x}\" opacity=\"{:.3f}\"/>\n",
            a.pos.x,a.pos.y,b.pos.x,b.pos.y,c.pos.x,c.pos.y,uColor&255U,(uColor>>8)&255U,(uColor>>16)&255U,uAlpha/255.0));
         }
      }
   }

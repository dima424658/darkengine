#pragma once
#include <charconv>
#include <cstdint>
#include <string_view>
#include <vector>
struct TerrainScale {uint32_t width=0,height=0;};
// Minimal NewDark material metadata, independent of image/storage APIs.
// Unknown material directives are ignored; disabled conditional blocks are not.
inline TerrainScale ParseTerrainScale(std::string_view text,bool (*configEnabled)(std::string_view)=nullptr) {
   TerrainScale result;
   if(text.substr(0,3)=="\xef\xbb\xbf")text.remove_prefix(3);
   std::vector<bool> parents,conditions;bool active=true;
   auto lower=[](std::string_view a,std::string_view b) {
      if(a.size()!=b.size())return false;
      for(size_t i=0;i<a.size();++i){char c=a[i];if(c>='A' && c<='Z')c+=32;if(c!=b[i])return false;}
      return true;
   };
   auto integer=[](std::string_view token,uint32_t &value) {
      if(!token.empty() && token.front()=='+')token.remove_prefix(1);
      const auto converted=std::from_chars(token.data(),token.data()+token.size(),value);
      return converted.ec==std::errc{} && converted.ptr==token.data()+token.size() && value &&
         value<=32767 && !(value&(value-1));
   };
   while(!text.empty()) {
      const auto end=text.find('\n');auto line=text.substr(0,end);
      if(end==std::string_view::npos)text={};else text.remove_prefix(end+1);
      const auto comment=line.find_first_of('#');if(comment!=std::string_view::npos)line=line.substr(0,comment);
      const auto slash=line.find("//");if(slash!=std::string_view::npos)line=line.substr(0,slash);
      std::vector<std::string_view> tokens;
      while(!line.empty()) {
         const auto first=line.find_first_not_of(" \t\r");if(first==std::string_view::npos)break;
         line.remove_prefix(first);const auto last=line.find_first_of(" \t\r");
         tokens.push_back(line.substr(0,last));if(last==std::string_view::npos)break;line.remove_prefix(last);
      }
      if(tokens.empty())continue;
      if(lower(tokens[0],".ifdef") || lower(tokens[0],".ifndef")) {
         bool enabled=false;
         if(tokens.size()==2) {
            enabled=tokens[1]=="1";
            if(tokens[1].front()=='$' && configEnabled)enabled=configEnabled(tokens[1].substr(1));
         }
         if(lower(tokens[0],".ifndef"))enabled=!enabled;
         parents.push_back(active);conditions.push_back(enabled);active=active&&enabled;continue;
      }
      if(lower(tokens[0],".else")) {
         if(!parents.empty()){conditions.back()=!conditions.back();active=parents.back()&&conditions.back();}continue;
      }
      if(lower(tokens[0],".endif")) {
         if(!parents.empty()){active=parents.back();parents.pop_back();conditions.pop_back();}continue;
      }
      if(!active || !lower(tokens[0],"terrain_scale"))continue;
      TerrainScale scale;
      if((tokens.size()==2 || tokens.size()==3) && integer(tokens[1],scale.width)) {
         scale.height=scale.width;
         if(tokens.size()==2 || integer(tokens[2],scale.height)){result=scale;continue;}
      }
      result={};
   }
   return result;
}

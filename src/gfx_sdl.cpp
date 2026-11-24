// SDL3 implementation of gfx.hpp (see there).
#include "gfx.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <tuple>
#include <unordered_map>
namespace fs=std::filesystem;
namespace gfx {

std::string utf8(const std::wstring& s){
 std::string out;out.reserve(s.size());
 for(size_t i=0;i<s.size();++i){
  std::uint32_t c=static_cast<std::uint32_t>(s[i]);
  if(sizeof(wchar_t)==2&&c>=0xD800&&c<0xDC00&&i+1<s.size()){ // UTF-16 surrogate pair (Windows)
   const std::uint32_t lo=static_cast<std::uint32_t>(s[i+1]);
   if(lo>=0xDC00&&lo<0xE000){c=0x10000+((c-0xD800)<<10)+(lo-0xDC00);++i;}
  }
  if(c<0x80)out+=static_cast<char>(c);
  else if(c<0x800){out+=static_cast<char>(0xC0|(c>>6));out+=static_cast<char>(0x80|(c&0x3F));}
  else if(c<0x10000){out+=static_cast<char>(0xE0|(c>>12));out+=static_cast<char>(0x80|((c>>6)&0x3F));out+=static_cast<char>(0x80|(c&0x3F));}
  else{out+=static_cast<char>(0xF0|(c>>18));out+=static_cast<char>(0x80|((c>>12)&0x3F));out+=static_cast<char>(0x80|((c>>6)&0x3F));out+=static_cast<char>(0x80|(c&0x3F));}
 }
 return out;
}

class Image {
public:
 SDL_Texture* tex=nullptr;int w=0,h=0;
 ~Image(){if(tex)SDL_DestroyTexture(tex);}
};
class Font {
public:
 TTF_Font* main=nullptr;std::vector<TTF_Font*> fallbacks;
 ~Font(){if(main)TTF_CloseFont(main);for(auto* f:fallbacks)TTF_CloseFont(f);}
};

namespace {
// System font files (nothing is bundled: Impact and Arial ship with Windows and macOS).
std::vector<fs::path> fontDirs(){
 std::vector<fs::path> d;
#ifdef _WIN32
 const char* win=std::getenv("WINDIR");
 d.push_back(fs::path(win?win:"C:\\Windows")/"Fonts");
#elif defined(__APPLE__)
 d.push_back("/System/Library/Fonts/Supplemental");
 d.push_back("/System/Library/Fonts");
 d.push_back("/Library/Fonts");
#else
 d.push_back("/usr/share/fonts/truetype/msttcorefonts");
 d.push_back("/usr/share/fonts/truetype/dejavu");
#endif
 return d;
}
fs::path findFont(std::initializer_list<const char*> names){
 std::error_code ec;
 for(const auto& dir:fontDirs())for(const char* n:names){const fs::path p=dir/n;if(fs::is_regular_file(p,ec))return p;}
 return {};
}
fs::path familyFile(Family f,bool bold,bool& synthBold){
 synthBold=false;
 if(f==Family::Impact){
  auto p=findFont({"impact.ttf","Impact.ttf"});
  if(!p.empty())return p;
  synthBold=true;return findFont({"arialbd.ttf","Arial Bold.ttf","arial.ttf","Arial.ttf","DejaVuSans-Bold.ttf"});
 }
 if(bold){
  auto p=findFont({"arialbd.ttf","Arial Bold.ttf","DejaVuSans-Bold.ttf"});
  if(!p.empty())return p;
  synthBold=true;
 }
 return findFont({"arial.ttf","Arial.ttf","DejaVuSans.ttf"});
}
// Symbols the menus use (◀ ▶ ★ ☆ ✔ ✕ ○ ♪ ■ □ • ↑↓←→ —) that Impact/Arial may lack.
std::vector<fs::path> fallbackFiles(){
 std::vector<fs::path> v;
 for(auto names:{std::initializer_list<const char*>{"seguisym.ttf","Apple Symbols.ttf","DejaVuSans.ttf"},
                 std::initializer_list<const char*>{"segoeui.ttf","Arial Unicode.ttf"},
                 std::initializer_list<const char*>{"arial.ttf","Arial.ttf"}}){
  const auto p=findFont(names);if(!p.empty())v.push_back(p);
 }
 return v;
}
struct TextKey {
 Font* f;int wrap;int align;std::string s;
 bool operator==(const TextKey& o) const {return f==o.f&&wrap==o.wrap&&align==o.align&&s==o.s;}
};
struct TextKeyHash {
 size_t operator()(const TextKey& k) const {return std::hash<std::string>()(k.s)^(std::hash<void*>()(k.f)*31u)^(static_cast<size_t>(k.wrap)*131u)^static_cast<size_t>(k.align);}
};
struct TextTex { SDL_Texture* tex=nullptr;int w=0,h=0;std::uint64_t used=0; };
}

struct Gfx::Impl {
 SDL_Window* win=nullptr;SDL_Renderer* r=nullptr;
 std::map<std::tuple<int,int,bool,bool>,std::unique_ptr<Font>> fonts;
 std::unordered_map<TextKey,TextTex,TextKeyHash> texts;
 std::uint64_t frame=0;
 std::vector<SDL_Rect> clips;
 bool additive=false;
 ImagePtr bg;float bgKey[5]{};int bgW=0,bgH=0;
 std::vector<SDL_Vertex> verts;std::vector<int> idx;
 ~Impl(){
  for(auto& t:texts)if(t.second.tex)SDL_DestroyTexture(t.second.tex);
  texts.clear();fonts.clear();bg.reset();
  if(r)SDL_DestroyRenderer(r);
  TTF_Quit();
 }
 void blend(){SDL_SetRenderDrawBlendMode(r,additive?SDL_BLENDMODE_ADD:SDL_BLENDMODE_BLEND);}
};

Gfx::Gfx():p_(std::make_unique<Impl>()){}
Gfx::~Gfx()=default;
bool Gfx::init(SDL_Window* window){
 p_->win=window;
 if(!TTF_Init())return false;
 p_->r=SDL_CreateRenderer(window,nullptr);
 if(!p_->r)return false;
 SDL_SetRenderVSync(p_->r,1);
 p_->blend();
 return true;
}
SDL_Renderer* Gfx::renderer() const {return p_->r;}
float Gfx::width() const {int w=1280,h=720;if(p_->r)SDL_GetCurrentRenderOutputSize(p_->r,&w,&h);return static_cast<float>(std::max(1,w));}
float Gfx::height() const {int w=1280,h=720;if(p_->r)SDL_GetCurrentRenderOutputSize(p_->r,&w,&h);return static_cast<float>(std::max(1,h));}
void Gfx::begin(Color c){
 SDL_SetRenderClipRect(p_->r,nullptr);p_->clips.clear();
 p_->additive=false;p_->blend();
 SDL_SetRenderDrawColorFloat(p_->r,c.r,c.g,c.b,c.a);
 SDL_RenderClear(p_->r);
}
void Gfx::end(){
 SDL_RenderPresent(p_->r);
 ++p_->frame;
 // Drop text textures not drawn for a couple of seconds (menus re-render the same strings every frame).
 if(p_->frame%30==0){
  for(auto it=p_->texts.begin();it!=p_->texts.end();){
   if(p_->frame-it->second.used>120){if(it->second.tex)SDL_DestroyTexture(it->second.tex);it=p_->texts.erase(it);}
   else ++it;
  }
 }
}
void Gfx::fill(const Rect& r,Color c,float opacity){
 if(r.right<=r.left||r.bottom<=r.top)return;
 SDL_SetRenderDrawColorFloat(p_->r,c.r,c.g,c.b,c.a*opacity);
 const SDL_FRect f{r.left,r.top,r.right-r.left,r.bottom-r.top};
 SDL_RenderFillRect(p_->r,&f);
}
void Gfx::stroke(const Rect& r,Color c,float w,float opacity){
 const float h=w/2;
 fill({r.left-h,r.top-h,r.right+h,r.top+h},c,opacity);
 fill({r.left-h,r.bottom-h,r.right+h,r.bottom+h},c,opacity);
 fill({r.left-h,r.top+h,r.left+h,r.bottom-h},c,opacity);
 fill({r.right-h,r.top+h,r.right+h,r.bottom-h},c,opacity);
}
void Gfx::setAdditive(bool on){p_->additive=on;p_->blend();}
void Gfx::polyline(const std::vector<Point>& pts,float width,Color c,float opacity){
 if(pts.size()<2||opacity<=0.004f)return;
 float a=std::min(1.f,c.a*opacity);
 if(width<1.f){a*=std::max(0.f,width);width=1.f;}          // hairlines: thinner = fainter
 const float hw=width/2,feather=1.f;
 auto& V=p_->verts;auto& I=p_->idx;V.clear();I.clear();
 const SDL_FColor solid{c.r,c.g,c.b,a},clear{c.r,c.g,c.b,0};
 auto vert=[&](float x,float y,const SDL_FColor& col){V.push_back(SDL_Vertex{{x,y},col,{0,0}});return static_cast<int>(V.size()-1);};
 auto quad=[&](int i0,int i1,int i2,int i3){I.insert(I.end(),{i0,i1,i2,i0,i2,i3});};
 for(size_t i=1;i<pts.size();++i){
  const Point p0=pts[i-1],p1=pts[i];
  const float dx=p1.x-p0.x,dy=p1.y-p0.y,len=std::sqrt(dx*dx+dy*dy);
  if(len<0.01f)continue;
  const float nx=-dy/len,ny=dx/len;
  const float o=hw,f=hw+feather;
  const int a0=vert(p0.x+nx*f,p0.y+ny*f,clear),a1=vert(p0.x+nx*o,p0.y+ny*o,solid),a2=vert(p0.x-nx*o,p0.y-ny*o,solid),a3=vert(p0.x-nx*f,p0.y-ny*f,clear);
  const int b0=vert(p1.x+nx*f,p1.y+ny*f,clear),b1=vert(p1.x+nx*o,p1.y+ny*o,solid),b2=vert(p1.x-nx*o,p1.y-ny*o,solid),b3=vert(p1.x-nx*f,p1.y-ny*f,clear);
  quad(a0,b0,b1,a1);quad(a1,b1,b2,a2);quad(a2,b2,b3,a3);
 }
 // round caps / joins
 const int segs=std::clamp(static_cast<int>(hw*1.2f),8,28);
 for(const auto& p:pts){
  const int centre=vert(p.x,p.y,solid);
  int firstIn=-1,firstOut=-1,prevIn=-1,prevOut=-1;
  for(int k=0;k<=segs;++k){
   const float t=static_cast<float>(k)*6.2831853f/static_cast<float>(segs);
   const float cx=std::cos(t),sy=std::sin(t);
   const int in=k==segs?firstIn:vert(p.x+cx*hw,p.y+sy*hw,solid);
   const int out=k==segs?firstOut:vert(p.x+cx*(hw+feather),p.y+sy*(hw+feather),clear);
   if(k==0){firstIn=in;firstOut=out;}
   else{I.insert(I.end(),{centre,prevIn,in});quad(prevIn,prevOut,out,in);}
   prevIn=in;prevOut=out;
  }
 }
 if(!I.empty())SDL_RenderGeometry(p_->r,nullptr,V.data(),static_cast<int>(V.size()),I.data(),static_cast<int>(I.size()));
}
void Gfx::pushClip(const Rect& r){
 SDL_Rect c{static_cast<int>(std::floor(r.left)),static_cast<int>(std::floor(r.top)),
            static_cast<int>(std::ceil(r.right-r.left)),static_cast<int>(std::ceil(r.bottom-r.top))};
 if(!p_->clips.empty()){SDL_Rect out{};if(!SDL_GetRectIntersection(&c,&p_->clips.back(),&out))out={0,0,0,0};c=out;}
 p_->clips.push_back(c);
 SDL_SetRenderClipRect(p_->r,&c);
}
void Gfx::popClip(){
 if(!p_->clips.empty())p_->clips.pop_back();
 SDL_SetRenderClipRect(p_->r,p_->clips.empty()?nullptr:&p_->clips.back());
}
ImagePtr Gfx::loadImage(const fs::path& file){
 std::error_code ec;
 if(!fs::is_regular_file(file,ec))return nullptr;
 SDL_Texture* t=IMG_LoadTexture(p_->r,file.u8string().c_str());
 if(!t)return nullptr;
 auto img=std::make_shared<Image>();img->tex=t;
 float w=0,h=0;SDL_GetTextureSize(t,&w,&h);img->w=static_cast<int>(w);img->h=static_cast<int>(h);
 SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);
 return img;
}
ImagePtr Gfx::makeImage(int w,int h,bool alpha){
 SDL_Texture* t=SDL_CreateTexture(p_->r,alpha?SDL_PIXELFORMAT_ARGB8888:SDL_PIXELFORMAT_XRGB8888,SDL_TEXTUREACCESS_STREAMING,w,h);
 if(!t)return nullptr;
 SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);
 auto img=std::make_shared<Image>();img->tex=t;img->w=w;img->h=h;return img;
}
void Gfx::upload(Image& img,const std::uint32_t* px,int w,int h){
 if(!img.tex||w!=img.w||h!=img.h)return;
 SDL_UpdateTexture(img.tex,nullptr,px,w*4);
}
int Gfx::imageWidth(const Image* i){return i?i->w:0;}
int Gfx::imageHeight(const Image* i){return i?i->h:0;}
void Gfx::draw(const Image* img,const Rect& dst,float opacity,Filter f,const Rect* src){
 if(!img||!img->tex||opacity<=0.f)return;
 SDL_SetTextureAlphaModFloat(img->tex,std::min(1.f,opacity));
 SDL_SetTextureColorModFloat(img->tex,1,1,1);
 SDL_SetTextureScaleMode(img->tex,f==Filter::Nearest?SDL_SCALEMODE_NEAREST:SDL_SCALEMODE_LINEAR);
 const SDL_FRect d{dst.left,dst.top,dst.right-dst.left,dst.bottom-dst.top};
 if(src){const SDL_FRect s{src->left,src->top,src->right-src->left,src->bottom-src->top};SDL_RenderTexture(p_->r,img->tex,&s,&d);}
 else SDL_RenderTexture(p_->r,img->tex,nullptr,&d);
}
void Gfx::radialBackground(float cx,float cy,float rx,float ry,const std::vector<std::pair<float,Color>>& stops){
 const float W=width(),H=height();
 const float key[5]={cx,cy,rx,ry,W*10000+H};
 if(!p_->bg||!std::equal(key,key+5,p_->bgKey)){
  std::copy(key,key+5,p_->bgKey);
  const int w=320,h=std::max(2,static_cast<int>(320*H/W));
  p_->bg=makeImage(w,h,false);
  if(p_->bg){
   std::vector<std::uint32_t> px(static_cast<size_t>(w)*static_cast<size_t>(h));
   for(int y=0;y<h;++y)for(int x=0;x<w;++x){
    const float X=(static_cast<float>(x)+0.5f)*W/static_cast<float>(w),Y=(static_cast<float>(y)+0.5f)*H/static_cast<float>(h);
    const float t=std::sqrt(((X-cx)/rx)*((X-cx)/rx)+((Y-cy)/ry)*((Y-cy)/ry));
    Color c=stops.empty()?Color{}:stops.back().second;
    for(size_t i=0;i+1<stops.size();++i){
     if(t<=stops[i].first){c=stops[i].second;break;}
     if(t<=stops[i+1].first){const float k=(t-stops[i].first)/std::max(1e-6f,stops[i+1].first-stops[i].first);
      const auto& a=stops[i].second;const auto& b=stops[i+1].second;c={a.r+(b.r-a.r)*k,a.g+(b.g-a.g)*k,a.b+(b.b-a.b)*k,1};break;}
    }
    auto ch=[](float v){return static_cast<std::uint32_t>(std::clamp(v,0.f,1.f)*255.f+0.5f);};
    px[static_cast<size_t>(y)*static_cast<size_t>(w)+static_cast<size_t>(x)]=0xFF000000u|(ch(c.r)<<16)|(ch(c.g)<<8)|ch(c.b);
   }
   upload(*p_->bg,px.data(),w,h);
  }
 }
 if(p_->bg)draw(p_->bg.get(),{0,0,W,H});
}
Font* Gfx::font(Family fam,float px,bool bold,bool italic){
 const auto key=std::make_tuple(static_cast<int>(fam),static_cast<int>(px*4),bold,italic);
 auto it=p_->fonts.find(key);
 if(it!=p_->fonts.end())return it->second.get();
 auto f=std::make_unique<Font>();
 bool synth=false;
 const fs::path file=familyFile(fam,bold,synth);
 const float size=std::max(4.f,px);
 if(!file.empty())f->main=TTF_OpenFont(file.u8string().c_str(),size);
 if(f->main){
  int style=TTF_STYLE_NORMAL;
  if(italic)style|=TTF_STYLE_ITALIC;
  if(synth&&bold)style|=TTF_STYLE_BOLD;
  TTF_SetFontStyle(f->main,static_cast<TTF_FontStyleFlags>(style));
  for(const auto& fb:fallbackFiles()){
   if(fb==file)continue;
   if(TTF_Font* t=TTF_OpenFont(fb.u8string().c_str(),size)){f->fallbacks.push_back(t);TTF_AddFallbackFont(f->main,t);}
  }
 }
 Font* raw=f.get();
 p_->fonts.emplace(key,std::move(f));
 return raw;
}
void Gfx::resetFonts(){
 for(auto& t:p_->texts)if(t.second.tex)SDL_DestroyTexture(t.second.tex);
 p_->texts.clear();
 p_->fonts.clear();
}
float Gfx::textWidth(const std::wstring& s,Font* f){
 if(!f||!f->main||s.empty())return 0;
 const std::string u=utf8(s);int w=0,h=0;
 return TTF_GetStringSize(f->main,u.c_str(),u.size(),&w,&h)?static_cast<float>(w):0.f;
}
float Gfx::text(const std::wstring& s,Font* f,const Rect& r,Color c,const TextStyle& st,float outline,float opacity){
 if(!f||!f->main||s.empty()||opacity<=0.f)return 0;
 const int wrap=st.wrap?std::max(1,static_cast<int>(r.right-r.left)):0;
 TextKey key{f,wrap,static_cast<int>(st.align),utf8(s)};
 auto it=p_->texts.find(key);
 if(it==p_->texts.end()){
  TTF_SetFontWrapAlignment(f->main,st.align==Align::Leading?TTF_HORIZONTAL_ALIGN_LEFT:st.align==Align::Center?TTF_HORIZONTAL_ALIGN_CENTER:TTF_HORIZONTAL_ALIGN_RIGHT);
  TextTex tt;
  if(SDL_Surface* surf=TTF_RenderText_Blended_Wrapped(f->main,key.s.c_str(),key.s.size(),SDL_Color{255,255,255,255},wrap)){
   tt.tex=SDL_CreateTextureFromSurface(p_->r,surf);tt.w=surf->w;tt.h=surf->h;
   SDL_DestroySurface(surf);
   if(tt.tex){SDL_SetTextureBlendMode(tt.tex,SDL_BLENDMODE_BLEND);SDL_SetTextureScaleMode(tt.tex,SDL_SCALEMODE_NEAREST);}
  }
  it=p_->texts.emplace(std::move(key),tt).first;
 }
 TextTex& t=it->second;t.used=p_->frame;
 if(!t.tex)return 0;
 const float w=static_cast<float>(t.w),h=static_cast<float>(t.h);
 float x=st.align==Align::Leading?r.left:st.align==Align::Center?(r.left+r.right-w)/2:r.right-w;
 if(st.wrap&&st.align==Align::Center)x=r.left+((r.right-r.left)-w)/2;
 const float y=st.vcenter&&!st.wrap?(r.top+r.bottom-h)/2:r.top;
 x=std::round(x);const float yy=std::round(y);
 if(st.clip)pushClip(r);
 const float a=std::min(1.f,opacity);
 if(outline>0){
  SDL_SetTextureColorModFloat(t.tex,0,0,0);SDL_SetTextureAlphaModFloat(t.tex,a);
  for(int i=0;i<16;++i){
   const float ang=static_cast<float>(i)*3.14159265f/8.f;
   const SDL_FRect d{x+std::cos(ang)*outline,yy+std::sin(ang)*outline,w,h};
   SDL_RenderTexture(p_->r,t.tex,nullptr,&d);
  }
 }
 SDL_SetTextureColorModFloat(t.tex,c.r,c.g,c.b);SDL_SetTextureAlphaModFloat(t.tex,std::min(1.f,a*c.a));
 const SDL_FRect d{x,yy,w,h};
 SDL_RenderTexture(p_->r,t.tex,nullptr,&d);
 if(st.clip)popClip();
 return st.clip?std::min(h,r.bottom-r.top):h;
}
}

#pragma once
// Portable 2D drawing for the front end (SDL3 renderer + SDL3_ttf + SDL3_image).
// Replaces the Direct2D / DirectWrite / WIC code so the same screens build on Windows and macOS.
// Coordinates are window pixels (on a Retina Mac: real pixels, not points).
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
struct SDL_Window;
struct SDL_Renderer;
namespace gfx {
struct Color { float r=0,g=0,b=0,a=1; };
struct Rect { float left=0,top=0,right=0,bottom=0; };
struct Point { float x=0,y=0; };
inline Color rgba(float r,float g,float b,float a=1.f){return {r,g,b,a};}
inline Rect rect(float l,float t,float r,float b){return {l,t,r,b};}
enum class Align { Leading, Center, Trailing };
enum class Filter { Linear, Nearest };
enum class Family { Impact, Arial };

class Image; // a GPU texture
class Font;  // one face at one pixel size
using ImagePtr=std::shared_ptr<Image>;

// Text box options. Like DirectWrite: by default one line, centred both ways; `wrap` breaks
// lines to the box width and starts at the top. Text outside the box is cut off when `clip`.
struct TextStyle {
 Align align=Align::Center;
 bool vcenter=true;
 bool wrap=false;
 bool clip=false;
};

class Gfx {
public:
 Gfx();
 ~Gfx();
 Gfx(const Gfx&)=delete;
 Gfx& operator=(const Gfx&)=delete;
 bool init(SDL_Window* window); // creates the renderer (vsync on)
 SDL_Renderer* renderer() const;
 float width() const;
 float height() const;
 void begin(Color clear);  // start a frame
 void end();               // present
 // shapes
 void fill(const Rect& r,Color c,float opacity=1.f);
 void stroke(const Rect& r,Color c,float width,float opacity=1.f);
 void polyline(const std::vector<Point>& pts,float width,Color c,float opacity); // round caps + joins, anti-aliased
 void setAdditive(bool on);
 void pushClip(const Rect& r);
 void popClip();
 // images
 ImagePtr loadImage(const std::filesystem::path& file);           // PNG/BMP/JPG, nullptr if missing
 ImagePtr makeImage(int w,int h,bool alpha);                        // updatable (video frames)
 void upload(Image& img,const std::uint32_t* bgrx,int w,int h);    // 32-bit B,G,R,X rows
 void draw(const Image* img,const Rect& dst,float opacity=1.f,Filter f=Filter::Linear,const Rect* src=nullptr);
 static int imageWidth(const Image* img);
 static int imageHeight(const Image* img);
 // Radial gradient filling the whole window (fallback background). Stops are 0..1 along the radius.
 void radialBackground(float cx,float cy,float rx,float ry,const std::vector<std::pair<float,Color>>& stops);
 // text (UTF-16/32 wide strings, as the screens use)
 Font* font(Family f,float pixelSize,bool bold,bool italic);       // owned by Gfx; cleared by resetFonts()
 void resetFonts();
 // Draws `s` in box `r`. `outline` > 0 draws a black outline that thick (16 offset copies).
 // Returns the height of the laid-out text (clamped to the box height when clipping).
 float text(const std::wstring& s,Font* f,const Rect& r,Color c,const TextStyle& st={},float outline=0,float opacity=1.f);
 float textWidth(const std::wstring& s,Font* f);
private:
 struct Impl;
 std::unique_ptr<Impl> p_;
};
std::string utf8(const std::wstring& s);
}

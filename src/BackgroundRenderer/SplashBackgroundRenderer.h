/*
 * Splash Background renderer
 * Render all those things not supported as Image, with Splash
 *
 * Copyright (C) 2012,2013 Lu Wang <coolwanglu@gmail.com>
 */


#ifndef SPLASH_BACKGROUND_RENDERER_H__
#define SPLASH_BACKGROUND_RENDERER_H__

#include <string>

#include <splash/SplashBitmap.h>
#include <SplashOutputDev.h>

#include "pdf2htmlEX-config.h"

#include "Param.h"
#include "HTMLRenderer/HTMLRenderer.h"

namespace pdf2htmlEX {

  using std::cerr;
  using std::endl;

// Based on BackgroundRenderer from poppler
class SplashBackgroundRenderer : public BackgroundRenderer, SplashOutputDev 
{
public:
  static const SplashColor white;

  SplashBackgroundRenderer(HTMLRenderer * html_renderer, const Param & param)
      : SplashOutputDev(splashModeRGB8, 4, gFalse, (SplashColorPtr)(&white), gTrue, gTrue)
      , html_renderer(html_renderer)
      , param(param)
  { }

  virtual ~SplashBackgroundRenderer() { }

  virtual void init(PDFDoc * doc);
  virtual void render_page(PDFDoc * doc, int pageno);
  virtual void embed_image(int pageno);

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  virtual GBool interpretType3Chars() { return !param.process_type3; }

#if POPPLER_OLDER_THAN_0_23_0
  virtual void startPage(int pageNum, GfxState *state);
#else
  virtual void startPage(int pageNum, GfxState *state, XRef *xrefA);
#endif
  
  virtual void drawChar(GfxState *state, double x, double y,
      double dx, double dy,
      double originX, double originY,
      CharCode code, int nBytes, Unicode *u, int uLen);

  virtual void stroke(GfxState *state) {
      if(!html_renderer->can_stroke(state))
          SplashOutputDev::stroke(state);
  }

  virtual void fill(GfxState *state) { 
      if(!html_renderer->can_fill(state))
          SplashOutputDev::fill(state);
  }

  virtual void drawImage(GfxState * state, Object * ref, Stream * str, int width, int height, GfxImageColorMap * colorMap, GBool interpolate, int *maskColors, GBool inlineImg) {}

protected:
  void dump_image(const char * filename, int x1, int y1, int x2, int y2);
  HTMLRenderer * html_renderer;
  const Param & param;
  int BpageNum;
};

} // namespace pdf2htmlEX

#endif // SPLASH_BACKGROUND_RENDERER_H__

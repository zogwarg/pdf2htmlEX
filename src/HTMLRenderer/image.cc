/*
 * image.cc
 *
 * Handling images
 *
 * by WangLu
 * 2012.08.14
 */

#include "HTMLRenderer.h"
#include "util/namespace.h"
#include "util/css_const.h"
#include <goo/ImgWriter.h>
#include <goo/PNGWriter.h>
#include <goo/GooString.h>
#include <sstream>
#include <cmath>
#include <sys/stat.h>

#ifdef ENABLE_LIBPNG
#include <png.h>
#endif

namespace pdf2htmlEX {

using std::cerr;

void HTMLRenderer::drawMaskedImage(GfxState *state, Object *ref, Stream *str,
        int width, int height,
        GfxImageColorMap *colorMap,
        GBool interpolate,
        Stream *maskStr,
        int maskWidth, int maskHeight,
        GBool maskInvert,
        GBool maskInterpolate) 
{
  cerr << "MI{" << endl ;
  maskedFlag = gTrue;
  drawImage(state, ref, str, width, height, colorMap, interpolate, NULL, gFalse);
  maskedFlag = gFalse;
  maskFlag = gTrue;
  image_count--;
  drawImage(state, ref, maskStr, maskWidth, maskHeight, NULL, maskInterpolate, NULL , gFalse);
  maskFlag = gFalse;
  cerr << "}" << endl;
}

void HTMLRenderer::drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
            int width, int height,
            GfxImageColorMap *colorMap,
            GBool interpolate,
            Stream *maskStr,
            int maskWidth, int maskHeight,
            GfxImageColorMap *maskColorMap,
            GBool maskInterpolate) 
{
  cerr << "SMI{" << endl ;
  maskedFlag = gTrue;
  drawImage(state, ref, str, width, height, colorMap, interpolate, NULL, gFalse);
  maskedFlag = gFalse;
  softMaskFlag = gTrue;
  image_count--;
  drawImage(state, ref, maskStr, maskWidth, maskHeight, maskColorMap, maskInterpolate, NULL, gFalse);
  softMaskFlag = gFalse;
  cerr << "}" << endl;
}

void HTMLRenderer::drawImage(GfxState * state, Object * ref, Stream * str, int width, int height, GfxImageColorMap * colorMap, GBool interpolate, int *maskColors, GBool inlineImg)
{
    if (param.process_nontext && param.split_images) {

    if( (width <= 10 || height <= 10) ) return OutputDev::drawImage(state,ref,str,width,height,colorMap,interpolate,maskColors,inlineImg);

    //CTM DETECTION
    double ctm[6];
    memcpy(ctm, state->getCTM(), sizeof(ctm));
    GBool detNull = (ctm[0]*ctm[3]-ctm[1]*ctm[2]==0?gTrue:gFalse);
    if (detNull) return;

    //IMAGE NAME RESOLVING
    std::stringstream sstm;
    const char *filename;
    const char *filepath;

    image_count++;
    
    sstm << hex << (maskFlag||softMaskFlag?"MaskImage-":"Image-") << HTMLRenderer::pageNum << "-" << dec << image_count << ".png";
    filename = sstm.str().c_str();
    sstm.str("");
    sstm.clear();
    sstm << param.dest_dir << "/" << filename;
    filepath = sstm.str().c_str();

    //DEBUG
    cerr << "Page '" << hex <<  HTMLRenderer::pageNum << dec << "' going through draw image " << filename << ":" << endl;
          //<< "\tColorSpace: " << colorMap->getColorSpace()->getMode() << endl
          //<< "\tBlendMode: " << state->getBlendMode() << endl
          //<< "\tStreamKind: " << str->getKind() << endl



    // DRAWING or STREAM COPYING
    // copyStreamToFile(str,filepath);
    // cerr << "\tCopyStream Done." << endl;
    drawPngImage(state, str, width, height ,colorMap ,filepath ,maskFlag);
    cerr << "\tPngWriter Done." << endl;
    
    //DRAW SUCCESS CHECK
    struct stat buffer;   
    if (stat (filepath, &buffer) != 0) return; 
    
    //
    //CSS DUMPING
    //
    auto & all_manager = HTMLRenderer::all_manager;
    
    // THE CURRENT TRANSFORMATION MATRIX goes from A 1x1 px square , is scaled to rectangle, transformed and translated.
    //
    // CTM =  translationXY + scaleX * scaleY * complexTransformation
    // 
    // complexTransformation is often but not always a rotation
    //
    // "CTM" = | ctm0 ctm2 ctm4 | =2D= translation(ctm4,ctm5) + | ctm0 ctm2 |  
    //         | ctm1 ctm3 ctm5 |                               | ctm1 ctm3 |
    //         |   0    0    1  |
    //
    // 
    // 

    double cssWidth,cssHeight;
    double cssLeft = ctm[4] ; 
    double cssBottom = ctm[5] ; 
    ctm[4] = ctm[5] = 0.0;

    if (ctm[0] > 0 && ctm[1] == 0 && ctm[2] == 0 && ctm[3] > 0) {
      cssWidth = ctm[0];
      cssHeight = ctm[3];
      ctm[0] = 1;
      ctm[3] = 1;
    } else if (ctm[0] > 0 && ctm[1] == 0 && ctm[2] == 0 && ctm[3] < 0) {
      cssWidth = ctm[0];
      cssHeight = -ctm[3];
      ctm[0]=1;
      ctm[3]=-1;
    } else {
      cssHeight = 1;
      cssWidth = 1;
    }
    

    (*f_curpage) << "<img class=\"" << CSS::IMAGE_CN
                 << " " << CSS::LEFT_CN             << all_manager.left.install(cssLeft)
                 << " " << CSS::BOTTOM_CN           << all_manager.bottom.install(cssBottom)
                 << " " << CSS::WIDTH_CN            << all_manager.width.install(cssWidth)
                 << " " << CSS::HEIGHT_CN           << all_manager.height.install(cssHeight)
                 << " " << CSS::TRANSFORM_MATRIX_CN << all_manager.transform_matrix.install(ctm)
                 << "\""
                 << " src=\"" << filename << "\""
                 << "/>" ; 
    }
    return OutputDev::drawImage(state,ref,str,width,height,colorMap,interpolate,maskColors,inlineImg);
}
void HTMLRenderer::drawPngImage(GfxState *state, Stream *str, int width, int height,
                                 GfxImageColorMap *colorMap, const char* filepath ,GBool isMask)
{
#ifdef ENABLE_LIBPNG
  FILE *f1;

  if (!colorMap && !isMask) {
    error(errInternal, -1, "Can't have color image without a color map");
    return;
  }
  if( (width > 10 && height > 10) || false) {
  // open the image file
    GooString *fName= new GooString(filepath);
    //fName->append(".png");
    if (!(f1 = fopen(fName->getCString(), "wb"))) {
      error(errIO, -1, "Couldn't open image file '{0:t}'", fName);
      delete fName;
      return;
    }

    cerr << "\tOpened " << filepath << " for writing." << endl;

    PNGWriter *writer = new PNGWriter( isMask ? PNGWriter::MONOCHROME : PNGWriter::RGB );
    // TODO can we calculate the resolution of the image?
    if (!writer->init(f1, width, height, 72, 72)) {
      error(errInternal, -1, "Can't init PNG for image '{0:t}'", fName);
      delete writer;
      fclose(f1);
      return;
    }

    cerr << "\tPNGWriter has an instance." << endl;

    if (!isMask) {
      cerr << "\tIs not Mask." << endl;
      Guchar *p;
      GfxRGB rgb;
      png_byte *row = (png_byte *) gmalloc(3 * width);   // 3 bytes/pixel: RGB
      png_bytep *row_pointer= &row;

      // Initialize the image stream
      ImageStream *imgStr = new ImageStream(str, width,
                          colorMap->getNumPixelComps(), colorMap->getBits());
      imgStr->reset();

      // For each line...
      for (int y = 0; y < height; y++) {

        // Convert into a PNG row
        p = imgStr->getLine();
        for (int x = 0; x < width; x++) {
          colorMap->getRGB(p, &rgb);
          // Write the RGB pixels into the row
          row[3*x]= colToByte(rgb.r);
          row[3*x+1]= colToByte(rgb.g);
          row[3*x+2]= colToByte(rgb.b);
          p += colorMap->getNumPixelComps();
        }

        if (!writer->writeRow(row_pointer)) {
          error(errIO, -1, "Failed to write into PNG '{0:t}'", fName);
          delete writer;
          delete imgStr;
          fclose(f1);
          return;
        }
      }
      gfree(row);
      imgStr->close();
      delete imgStr;
    }
    else { // isMask == true
      cerr << "\tIs Mask." << endl;
      int size = (width + 7)/8;

      // PDF masks use 0 = draw current color, 1 = leave unchanged.
      // We invert this to provide the standard interpretation of alpha
      // (0 = transparent, 1 = opaque). If the colorMap already inverts
      // the mask we leave the data unchanged.
      int invert_bits = 0xff;
      if (colorMap) {
        GfxGray gray;
        Guchar zero = 0;
        colorMap->getGray(&zero, &gray);
        if (colToByte(gray) == 0)
          invert_bits = 0x00;
      }

      str->reset();
      Guchar *png_row = (Guchar *)gmalloc(size);

      for (int ri = 0; ri < height; ++ri)
      {
        for(int i = 0; i < size; i++)
          png_row[i] = str->getChar() ^ invert_bits;

        if (!writer->writeRow( &png_row ))
        {
          error(errIO, -1, "Failed to write into PNG '{0:t}'", fName);
          delete writer;
          fclose(f1);
          gfree(png_row);
          return;
        }
      }
      str->close();
      gfree(png_row);
    }

    str->close();

    writer->close();
    delete writer;
    fclose(f1);

    cerr << "\tImage Writing Sucess." << endl;
    
  } else {
    cerr << "\tImage too small and ignored" << endl ; 
  }
#else
  return;
#endif
}

// void HTMLRenderer::copyStreamToFile(Stream *str, const char * filepath) {
//     GooString *s = new GooString();
//     str->reset();
//     Stream * base = str->getBaseStream() ;
//     base->reset();
//     base->fillGooString(s);
//     FILE * f ;
//     f = fopen(filepath,"wb");

//     int len = s->getLength();
//     for(int i=0;i<len;i++) {
//       fputc(s->getChar(i),f);
//     }
//     base->close();
//     str->close();
//     fclose(f);
// }

} // namespace pdf2htmlEX

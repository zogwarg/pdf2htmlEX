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
#include <goo/ImgWriter.h>
#include <goo/PNGWriter.h>
#include <sstream>

namespace pdf2htmlEX {

using std::cerr;

void HTMLRenderer::drawImage(GfxState * state, Object * ref, Stream * str, int width, int height, GfxImageColorMap * colorMap, GBool interpolate, int *maskColors, GBool inlineImg)
{
    if(maskColors) return;

    unsigned char *row;
    unsigned char *rowp;
    Guchar *p;
    GfxRGB rgb;


    image_count++;
    cerr << "Going through draw image " << image_count << " Format " << colorMap->getBits() <<endl ;
    
    std::stringstream sstm;
    sstm << "Image" << image_count << ".png";
    const char * filename = sstm.str().c_str();
    FILE * f = fopen(filename, "wb");
    
    std::unique_ptr<ImgWriter> writer;
    writer = std::unique_ptr<ImgWriter>(new PNGWriter);
    if(!writer->init(f, width, height, param.h_dpi, param.v_dpi))
        throw "Cannot initialize PNGWriter";
    
    ImageStream * imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
    imgStr->reset();

    row = (unsigned char *) gmallocn(width, sizeof(unsigned int));
    for (int y = 0; y < height; y++) {
        p = imgStr->getLine();
        rowp = row;
        for (int x = 0; x < width; ++x) {
        if (p) {
          colorMap->getRGB(p, &rgb);
          *rowp++ = colToByte(rgb.r);
          *rowp++ = colToByte(rgb.g);
          *rowp++ = colToByte(rgb.b);
          p += colorMap->getNumPixelComps();
        } else {
          *rowp++ = 0;
          *rowp++ = 0;
          *rowp++ = 0;
        }
      }
      writer->writeRow(&row);
    }
    gfree(row);
    imgStr->close();
    delete imgStr;
    writer->close();
    fclose(f);

    return OutputDev::drawImage(state,ref,str,width,height,colorMap,interpolate,maskColors,inlineImg);
}
} // namespace pdf2htmlEX

#if 0
    if(maskColors)
        return;

    rgb8_image_t img(width, height);
    auto imgview = view(img);
    auto loc = imgview.xy_at(0,0);

    ImageStream * img_stream = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
    img_stream->reset();

    for(int i = 0; i < height; ++i)
    {
        auto p = img_stream->getLine();
        for(int j = 0; j < width; ++j)
        {
            GfxRGB rgb;
            colorMap->getRGB(p, &rgb);

            *loc = rgb8_pixel_t(colToByte(rgb.r), colToByte(rgb.g), colToByte(rgb.b));

            p += colorMap->getNumPixelComps();

            ++ loc.x();
        }

        loc = imgview.xy_at(0, i+1);
    }

    png_write_view((format("i%|1$x|.png")%image_count).str(), imgview);
    
    img_stream->close();
    delete img_stream;

    close_line();

    double ctm[6];
    memcpy(ctm, state->getCTM(), sizeof(ctm));
    ctm[4] = ctm[5] = 0.0;
    html_fout << format("<img class=\"i t%2%\" style=\"left:%3%px;bottom:%4%px;width:%5%px;height:%6%px;\" src=\"i%|1$x|.png\" />") % image_count % install_transform_matrix(ctm) % state->getCurX() % state->getCurY() % width % height << endl;


    ++ image_count;
#endif

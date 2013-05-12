#define SLIB_NO_DEFINE_64BIT

#include <algorithm>
#include <CImg.h>
#include <common/types.h>
#include <fftw3.h>
#include <glog/logging.h>
#include "cimgutils.h"

using namespace cimg_library;
using std::max;
using std::min;

namespace slib {
  
  void CImgUtils::PrettyPrintFloatImage(const FloatImage& image) {
    if (image.size() > 100) {
      fprintf(stderr, "Not outputting images larger than 100 values total.\n");
      return;
    }
    cimg_forC(image, c) {
      printf("image(:,:,%d) = \n", c);
      cimg_forY(image, y) {
	printf("\n\t");
	cimg_forX(image, x) {
	  printf("%4.4f\t", image(x,y,c));
	}
      }
      printf("\n\n");
    }
    printf("\n\n");
  }
  
  void CImgUtils::DisplayEigenMatrix(const FloatMatrix& matrix) {
    UInt8Image image(matrix.cols(), matrix.rows(), 1, 1);
    for (int row = 0; row < matrix.rows(); row++) {
      for (int col = 0; col < matrix.cols(); col++) {
	image(col, row) = (char) (matrix(row, col) * 255.0f);
      }
    }
    
    image.map(CImg<unsigned char>::jet_LUT256());
    image.display();
  }
  
  inline int nexthigher(int k) {
    k--;
    for (int i=1; i<32; i<<=1)
      k = k | k >> i;
    return k+1;
  }
  
  FloatImage CImgUtils::FastFilter(const FloatImage& image, const FloatImage& kernel) {    
    FloatImage filtered(image.width(), image.height());
    filtered.fill(0.0f);
    
    int padding = kernel.width();
    int _w = nexthigher(image.width() + padding*2);
    int _h = nexthigher(image.height() + padding*2);
    
    fftwf_complex *imageBuffer=0;
    fftwf_complex *filterBuffer=0;
    fftwf_complex *convBuffer=0;
    
    if (!imageBuffer) {
      imageBuffer = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex)*_w*_h);
      assert(imageBuffer != NULL);
    }
    if (!filterBuffer) {
      filterBuffer = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex)*_w*_h);
      assert(filterBuffer != NULL);
    }
    if (!convBuffer) {
      convBuffer = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex)*_w*_h);
      assert(convBuffer != NULL);
    }
    
    filtered.resize(image.width(), image.height());        
    
    fftwf_plan pf_image = 0;
    fftwf_plan pf_filter = 0;
    fftwf_plan pb_conv = 0;
    
    int dim[2];
    dim[0]=_h; dim[1]=_w;
    
    float lscale=1.0f/((float)_w*_h);
    
    bool import_failed = false;
    char fname[1024];
    FILE *in = 0;
    
    fftwf_forget_wisdom();
    
    import_failed = false;
    sprintf(fname,"%s/.fftw3_wisdom/%d_%d_%d.wisdom", getenv("HOME"), _w, _h, 1);
    in = fopen(fname,"rb");
    if (!in) {
      import_failed = true;
    }
    else {
      if ( 0 == fftwf_import_wisdom_from_file(in) ) {
	LOG(ERROR) << "Unexpected error while attempting to import wisdom.";
	import_failed = true;
      }
      fclose(in);
    }
    
    if (import_failed) {
      LOG(ERROR) << "Failed to locate wisdom file [" << fname << "], will take longer to build plans.";
    }
    
    pf_image = fftwf_plan_many_dft(2, dim, 1,
				   imageBuffer, NULL, 1, _w*_h,   // input 
				   imageBuffer, NULL, 1, _w*_h,   // output (in-place)
				   FFTW_FORWARD, (import_failed ? FFTW_PATIENT : FFTW_ESTIMATE));
    
    pf_filter = fftwf_plan_many_dft(2, dim, 1,
				    filterBuffer, NULL, 1, _w*_h,   // input 
				    filterBuffer, NULL, 1, _w*_h,   // output (in-place)
				    FFTW_FORWARD, (import_failed ? FFTW_PATIENT : FFTW_ESTIMATE));
    
    pb_conv = fftwf_plan_many_dft(2, dim, 1,
				  convBuffer, NULL, 1, _w*_h,   // input 
				  convBuffer, NULL, 1, _w*_h,   // output (in-place)
				  FFTW_BACKWARD, (import_failed ? FFTW_PATIENT : FFTW_ESTIMATE));
    
    if (!pf_image || !pf_filter || !pb_conv) {
      LOG(ERROR) << "Failed to create fftw plans.";
      if (pf_image)
	fftwf_destroy_plan(pf_image);
      if (pf_filter)
	fftwf_destroy_plan(pf_filter);
      if (pb_conv)
	fftwf_destroy_plan(pb_conv);
      
      return filtered;
    }
    
    // prepare image buffer and take FFT
    memset(imageBuffer,0,sizeof(fftwf_complex)*_w*_h);
    for (int y=0; y<image.height()+padding*2; y++) {
      for (int x=0; x<image.width()+padding*2; x++) {
	int xx = x - padding;
	int yy = y - padding;
	
	xx = min(image.width()-1,max(0,xx));
	yy = min(image.height()-1,max(0,yy));
	imageBuffer[y*_w+x][0] = image(xx,yy);
      }
    }
    
    fftwf_execute(pf_image);
    
    // prepare filter buffer and take FFT
    memset(filterBuffer,0,sizeof(fftwf_complex)*_w*_h);
    int w2 = kernel.width()/2;
    int h2 = kernel.height()/2;
    for (int y=-h2; y<=h2; y++) {
      for (int x=-w2; x<=w2; x++) {
	if ( (x+w2 >= kernel.width()) || (y+h2 >= kernel.height()) ) 
	  continue;
	
	int yy = (y<0 ? (_h+y) : y);
	int xx = (x<0 ? (_w+x) : x);
	
	filterBuffer[yy*_w+xx][0] = kernel(x+w2,y+h2);
      }
    }
    
    fftwf_execute(pf_filter);
    
    // compute complex products
    for (int y=0; y<_h; y++) {
      for (int x=0; x<_w; x++) {
	int idx = y*_w+x;
	
	convBuffer[idx][0] = imageBuffer[idx][0]*filterBuffer[idx][0] - imageBuffer[idx][1]*filterBuffer[idx][1];
	convBuffer[idx][1] = imageBuffer[idx][0]*filterBuffer[idx][1] + imageBuffer[idx][1]*filterBuffer[idx][0];
      }
    }
    
    // take IFFT
    fftwf_execute(pb_conv);
    
    for (int y=0; y<image.height(); y++) {
      for (int x=0; x<image.width(); x++) {
	filtered(x,y) = convBuffer[(y+padding)*_w+(x+padding)][0]*lscale;
      }
    }
    
    if (import_failed) {
      FILE *out = fopen(fname,"wb");
      if (!out) {
	LOG(WARNING) << "Failed to open wisdom file for writing: " << fname;
      }
      else {
	fftwf_export_wisdom_to_file(out);
	fclose(out);
	
	LOG(WARNING) << "Wrote wisdom file: " << fname;
      }
    }
    
    // clean up convolution buffers
    fftwf_free(imageBuffer);
    fftwf_free(filterBuffer);
    fftwf_free(convBuffer);
    
    // clean up plans
    fftwf_destroy_plan(pf_image); pf_image=0;
    fftwf_destroy_plan(pf_filter); pf_filter=0;
    fftwf_destroy_plan(pb_conv); pb_conv=0;
    
    // drop wisdom
    fftwf_forget_wisdom();
    
    return filtered;
  }
  
  /**
   * Draws a line between two points p1(p1x,p1y) and p2(p2x,p2y).
   * This function is based on the Bresenham's line algorithm and is highly 
   * optimized to be able to draw lines very quickly. There is no floating point 
   * arithmetic nor multiplications and divisions involved. Only addition, 
   * subtraction and bit shifting are used. 
   *
   * Note that you have to define your own customized setPixel(x,y) function, 
   * which essentially lights a pixel on the screen.
   */  
  void __p_swap(int& x, int& y) {
    const int z = x;
    x = y;
    y = z;
  }

  void CImgUtils::DrawThickLine(const int x1, const int& y1, const int& x2, const int& y2,
				const float& thickness, const float* color, const float& opacity,
				FloatImage* image) {
    int p1x = x1;
    int p1y = y1;
    int p2x = x2;
    int p2y = y2;

    const float t2 = thickness / 2.0f;
    int F, x, y;
    
    if (p1x > p2x)  // Swap points if p1 is on the right of p2
      {
	__p_swap(p1x, p2x);
	__p_swap(p1y, p2y);
      }
    
    // Handle trivial cases separately for algorithm speed up.
    // Trivial case 1: m = +/-INF (Vertical line)
    if (p1x == p2x)
      {
	if (p1y > p2y)  // Swap y-coordinates if p1 is above p2
	  {
	    __p_swap(p1y, p2y);
	  }
	
	x = p1x;
	y = p1y;
	while (y <= p2y)
	  {
	    image->draw_rectangle(x - t2, y - t2, x + t2, y + t2, color, opacity);
	    y++;
	  }
	return;
      }
    // Trivial case 2: m = 0 (Horizontal line)
    else if (p1y == p2y)
      {
	x = p1x;
	y = p1y;
	
	while (x <= p2x)
	  {
	    image->draw_rectangle(x - t2, y - t2, x + t2, y + t2, color, opacity);
	    x++;
	  }
	return;
      }
    
    
    int dy            = p2y - p1y;  // y-increment from p1 to p2
    int dx            = p2x - p1x;  // x-increment from p1 to p2
    int dy2           = (dy << 1);  // dy << 1 == 2*dy
    int dx2           = (dx << 1);
    int dy2_minus_dx2 = dy2 - dx2;  // precompute constant for speed up
    int dy2_plus_dx2  = dy2 + dx2;
    
    
    if (dy >= 0)    // m >= 0
      {
	// Case 1: 0 <= m <= 1 (Original case)
	if (dy <= dx)   
	  {
	    F = dy2 - dx;    // initial F
	    
	    x = p1x;
	    y = p1y;
	    while (x <= p2x)
	      {
		image->draw_rectangle(x - t2, y - t2, x + t2, y + t2, color, opacity);
		if (F <= 0)
		  {
		    F += dy2;
		  }
		else
		  {
		    y++;
		    F += dy2_minus_dx2;
		  }
		x++;
	      }
	  }
	// Case 2: 1 < m < INF (Mirror about y=x line
	// replace all dy by dx and dx by dy)
	else
	  {
	    F = dx2 - dy;    // initial F
	    
	    y = p1y;
	    x = p1x;
	    while (y <= p2y)
	      {
		image->draw_rectangle(x - t2, y - t2, x + t2, y + t2, color, opacity);
		if (F <= 0)
		  {
		    F += dx2;
		  }
		else
		  {
		    x++;
		    F -= dy2_minus_dx2;
		  }
		y++;
	      }
	  }
      }
    else    // m < 0
      {
	// Case 3: -1 <= m < 0 (Mirror about x-axis, replace all dy by -dy)
	if (dx >= -dy)
	  {
	    F = -dy2 - dx;    // initial F
	    
	    x = p1x;
	    y = p1y;
	    while (x <= p2x)
	      {
		image->draw_rectangle(x - t2, y - t2, x + t2, y + t2, color, opacity);
		if (F <= 0)
		  {
		    F -= dy2;
		  }
		else
		  {
		    y--;
		    F -= dy2_plus_dx2;
		  }
		x++;
	      }
	  }
	// Case 4: -INF < m < -1 (Mirror about x-axis and mirror 
	// about y=x line, replace all dx by -dy and dy by dx)
	else    
	  {
	    F = dx2 + dy;    // initial F
	    
	    y = p1y;
	    x = p1x;
	    while (y >= p2y)
	      {
		image->draw_rectangle(x - t2, y - t2, x + t2, y + t2, color, opacity);
		if (F <= 0)
		  {
		    F += dx2;
		  }
		else
		  {
		    x++;
		    F += dy2_plus_dx2;
		  }
		y--;
	      }
	  }
      }
  }

}  // namespace slib

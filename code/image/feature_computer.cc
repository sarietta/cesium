#include "feature_computer.h"

#include "../common/scoped_ptr.h"

namespace slib {
  namespace image {
    
    // Copyright (C) 2008, 2009, 2010 Pedro Felzenszwalb, Ross Girshick
    // Copyright (C) 2007 Pedro Felzenszwalb, Deva Ramanan
    // 
    // Permission is hereby granted, free of charge, to any person obtaining
    // a copy of this software and associated documentation files (the
    // "Software"), to deal in the Software without restriction, including
    // without limitation the rights to use, copy, modify, merge, publish,
    // distribute, sublicense, and/or sell copies of the Software, and to
    // permit persons to whom the Software is furnished to do so, subject to
    // the following conditions:
    // 
    // The above copyright notice and this permission notice shall be
    // included in all copies or substantial portions of the Software.
    // 
    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    // EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    // MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    // NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    // LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    // OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    // WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
    //
    // Modified by Sean M. Arietta - University of California, Berkeley

    // small value, used to avoid division by zero
    #define eps 0.0001
    
    // unit vectors used to compute gradient orientation
    double uu[9] = {1.0000, 
		    0.9397, 
		    0.7660, 
		    0.500, 
		    0.1736, 
		    -0.1736, 
		    -0.5000, 
		    -0.7660, 
		    -0.9397};
    double vv[9] = {0.0000, 
		    0.3420, 
		    0.6428, 
		    0.8660, 
		    0.9848, 
		    0.9848, 
		    0.8660, 
		    0.6428, 
		    0.3420};
    
    static inline double min(double x, double y) { return (x <= y ? x : y); }
    static inline double max(double x, double y) { return (x <= y ? y : x); }
    
    static inline int min(int x, int y) { return (x <= y ? x : y); }
    static inline int max(int x, int y) { return (x <= y ? y : x); }
    
    FloatImage FeatureComputer::ComputeHOGFeatures(const FloatImage& image, const int32& bins) {
      const FloatImage imageT = image.get_transpose();
      const float* im = imageT.data();
      const int dims[] = {
	imageT.width(),
	imageT.height(),
	imageT.spectrum()};
      int sbin = bins;
      
      // memory for caching orientation histograms & their norms
      int blocks[2];
      blocks[0] = (int)round((float)dims[0]/(float)sbin);
      blocks[1] = (int)round((float)dims[1]/(float)sbin);
      scoped_array<double> hist(new double[blocks[0]*blocks[1]*18]);
      // THIS IS INCREDIBLY IMPORTANT!
      for (int i = 0; i < blocks[0]*blocks[1]*18; i ++) {
	hist[i] = 0.0;
      }
      scoped_array<double> norm(new double[blocks[0]*blocks[1]]);
      // THIS IS INCREDIBLY IMPORTANT!
      for (int i = 0; i < blocks[0]*blocks[1]; i++) {
	norm[i] = 0.0;
      }
      
      // memory for HOG features
      int out[3];
      out[0] = max(blocks[0]-2, 0);
      out[1] = max(blocks[1]-2, 0);
      out[2] = 27+4;
      FloatImage features(out[0], out[1], 1, out[2]);
      features.fill(0.0f);
      float* feat = features.data();
      
      int visible[2];
      visible[0] = blocks[0]*sbin;
      visible[1] = blocks[1]*sbin;
      
      for (int x = 1; x < visible[1]-1; x++) {
	for (int y = 1; y < visible[0]-1; y++) {
	  // first color channel
	  const float* s = im + min(x, dims[1]-2)*dims[0] + min(y, dims[0]-2);
	  double dy = (double) (*(s+1) - *(s-1));
	  double dx = (double) (*(s+dims[0]) - *(s-dims[0]));
	  double v = dx*dx + dy*dy;
	  
	  // second color channel
	  s += dims[0]*dims[1];
	  double dy2 = (double) (*(s+1) - *(s-1));
	  double dx2 = (double) (*(s+dims[0]) - *(s-dims[0]));
	  double v2 = dx2*dx2 + dy2*dy2;
	  
	  // third color channel
	  s += dims[0]*dims[1];
	  double dy3 = (double) (*(s+1) - *(s-1));
	  double dx3 = (double) (*(s+dims[0]) - *(s-dims[0]));
	  double v3 = dx3*dx3 + dy3*dy3;
	  
	  // pick channel with strongest gradient
	  if (v2 > v) {
	    v = v2;
	    dx = dx2;
	    dy = dy2;
	  } 
	  if (v3 > v) {
	    v = v3;
	    dx = dx3;
	    dy = dy3;
	  }
	  
	  // snap to one of 18 orientations
	  double best_dot = 0;
	  int best_o = 0;
	  for (int o = 0; o < 9; o++) {
	    double dot = uu[o]*dx + vv[o]*dy;
	    if (dot > best_dot) {
	      best_dot = dot;
	      best_o = o;
	    } else if (-dot > best_dot) {
	      best_dot = -dot;
	      best_o = o+9;
	    }
	  }
	  
	  // add to 4 histograms around pixel using linear interpolation
	  double xp = ((double)x+0.5)/(double)sbin - 0.5;
	  double yp = ((double)y+0.5)/(double)sbin - 0.5;
	  int ixp = (int)floor(xp);
	  int iyp = (int)floor(yp);
	  double vx0 = xp-ixp;
	  double vy0 = yp-iyp;
	  double vx1 = 1.0-vx0;
	  double vy1 = 1.0-vy0;
	  v = sqrt(v);
	  
	  if (ixp >= 0 && iyp >= 0) {
	    *(hist.get() + ixp*blocks[0] + iyp + best_o*blocks[0]*blocks[1]) += 
	      vx1*vy1*v;
	  }
	  
	  if (ixp+1 < blocks[1] && iyp >= 0) {
	    *(hist.get() + (ixp+1)*blocks[0] + iyp + best_o*blocks[0]*blocks[1]) += 
	      vx0*vy1*v;
	  }
	  
	  if (ixp >= 0 && iyp+1 < blocks[0]) {
	    *(hist.get() + ixp*blocks[0] + (iyp+1) + best_o*blocks[0]*blocks[1]) += 
	      vx1*vy0*v;
	  }
	  
	  if (ixp+1 < blocks[1] && iyp+1 < blocks[0]) {
	    *(hist.get() + (ixp+1)*blocks[0] + (iyp+1) + best_o*blocks[0]*blocks[1]) += 
	      vx0*vy0*v;
	  }
	}
      }
      
      // compute energy in each block by summing over orientations
      for (int o = 0; o < 9; o++) {
	double *src1 = hist.get() + o*blocks[0]*blocks[1];
	double *src2 = hist.get() + (o+9)*blocks[0]*blocks[1];
	double *dst = norm.get();
	double *end = norm.get() + blocks[1]*blocks[0];
	while (dst < end) {
	  *(dst++) += (*src1 + *src2) * (*src1 + *src2);
	  src1++;
	  src2++;
	}
      }
      
      // compute features
      for (int x = 0; x < out[1]; x++) {
	for (int y = 0; y < out[0]; y++) {
	  float *dst = feat + x*out[0] + y;      
	  double *src, *p, n1, n2, n3, n4;
	  
	  p = norm.get() + (x+1)*blocks[0] + y+1;
	  n1 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);
	  p = norm.get() + (x+1)*blocks[0] + y;
	  n2 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);
	  p = norm.get() + x*blocks[0] + y+1;
	  n3 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);
	  p = norm.get() + x*blocks[0] + y;      
	  n4 = 1.0 / sqrt(*p + *(p+1) + *(p+blocks[0]) + *(p+blocks[0]+1) + eps);
	  
	  double t1 = 0;
	  double t2 = 0;
	  double t3 = 0;
	  double t4 = 0;
	  
	  // contrast-sensitive features
	  src = hist.get() + (x+1)*blocks[0] + (y+1);
	  for (int o = 0; o < 18; o++) {
	    double h1 = min(*src * n1, 0.2);
	    double h2 = min(*src * n2, 0.2);
	    double h3 = min(*src * n3, 0.2);
	    double h4 = min(*src * n4, 0.2);
	    *dst = (float) (0.5 * (h1 + h2 + h3 + h4));
	    t1 += h1;
	    t2 += h2;
	    t3 += h3;
	    t4 += h4;
	    dst += out[0]*out[1];
	    src += blocks[0]*blocks[1];
	  }
	  
	  // contrast-insensitive features
	  src = hist.get() + (x+1)*blocks[0] + (y+1);
	  for (int o = 0; o < 9; o++) {
	    double sum = *src + *(src + 9*blocks[0]*blocks[1]);
	    double h1 = min(sum * n1, 0.2);
	    double h2 = min(sum * n2, 0.2);
	    double h3 = min(sum * n3, 0.2);
	    double h4 = min(sum * n4, 0.2);
	    *dst = (float) (0.5 * (h1 + h2 + h3 + h4));
	    dst += out[0]*out[1];
	    src += blocks[0]*blocks[1];
	  }
	  
	  // texture features
	  *dst = (float) (0.2357 * t1);
	  dst += out[0]*out[1];
	  *dst = (float) (0.2357 * t2);
	  dst += out[0]*out[1];
	  *dst = (float) (0.2357 * t3);
	  dst += out[0]*out[1];
	  *dst = (float) (0.2357 * t4);
	}
      }
      features.transpose();
      return features;
    }
  }  // namespace image
}  // namespace slib

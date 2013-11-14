#include "colormap.h"

DEFINE_double(gaussian_colormap_shape, -1, "Shape of generalized gaussian");
DEFINE_double(gaussian_colormap_epsilon, 0.4, "Epsilon of generalized gaussian");

namespace slib {
  namespace util {
    
    float ColorMap::GaussianTransformation(const float& max, 
					   const float& min, 
					   const float& value,
					   const float shape,
					   const float epsilon) {
      float val = 0.0f;
      if (shape > 0.0) {
	const float v = (value - min) / (max - min);
	const float p = 1.0f - v;
	val = exp(-pow(p, shape) / (2.0 * epsilon));
      } else {
	val = (value - min) / (max - min);
      }
      val = val < 0 ? 0 : val;
      val = val > 1 ? 1 : val;
      
      return val;
    }
    
    float ColorMap::InverseGaussianTransformation(const float& max, 
						  const float& min, 
						  const float& val,
						  const float shape,
						  const float epsilon) {
      float value = 0.0f;
      if (shape > 0.0) {
	value = min + (max - min) * 
	  (1.0f - pow(-2.0f * epsilon * log(val), 1/shape));
      } else {
	value = min + val * (max - min);
      }
      
      return value;
    }
    
    template <typename T>
    static void OverlayLegend(const std::vector<T>& transformed_values, 
			      const std::vector<T>& original_values,
			      void (*colormap_function)(const float&, float*), 
			      float (*inverse_function)(const float&, const float&, const float&, 
							float, float), 
			      FloatImage* image) {
      const int32 N = transformed_values.size();
      float* transformed_values_ptr = new float[N];
      for (uint32 i = 0; i < N; i++) {
	transformed_values_ptr[i] = static_cast<float>(transformed_values[i]);
      }
      const float max_transformed_value = Max<float>(transformed_values_ptr, N);
      const float min_transformed_value = Min<float>(transformed_values_ptr, N);
      
      const int32 M = original_values.size();
      float* original_values_ptr = new float[N];
      for (uint32 i = 0; i < N; i++) {
	original_values_ptr[i] = static_cast<float>(original_values[i]);
      }
      const float max_original_value = Max<float>(original_values_ptr, N);
      const float min_original_value = Min<float>(original_values_ptr, N);
      
      float max_color[3];
      float min_color[3];
      (*colormap_function)(max_transformed_value, max_color);
      (*colormap_function)(min_transformed_value, min_color);
      
      // Resize the image to accomodate the legend.
      const int32 legend_height = 0.1 * image->height();
      const float color[3] = { 51.0f, 51.0f, 51.0f };
      image->resize(image->width(), image->height() + legend_height,
		    image->depth(), image->spectrum(), 0);
      image->draw_image(0, legend_height, 0, 0, *image);
      image->draw_rectangle(0, 0, image->width(), legend_height, color);
      
      // Draw the colormap on the legend.
      const float white[3] = {255.0f, 255.0f, 255.0f};
      float rgb[3];
      const int32 line_height = legend_height * 0.4;
      const int32 line_y = (0.5f * (float) (legend_height - line_height));
      const int32 colormap_padding = image->width() * 0.03;
      const int32 num_ticks = 10;
      const int32 tick_separation = ceil(((float) (image->width() - 2*colormap_padding)) / ((float) num_ticks));
      const int32 font_height = line_y * 0.5;
      for (int x = colormap_padding; x < image->width()-colormap_padding; x++) {	  
	const float x_norm = static_cast<float>(x-colormap_padding) 
	  / static_cast<float>(image->width()-colormap_padding);
	const float val = x_norm * (max_transformed_value - min_transformed_value) + min_transformed_value;
	(*colormap_function)(val, rgb);
	for (int c = 0; c < 3; c++) {
	  rgb[c] = rgb[c] * 255.0f;
	}
	image->draw_line(x, line_y, x, line_height, rgb);
	
	if ((x - colormap_padding) % tick_separation == 0) {
	  image->draw_line(x, line_y-10, x, line_height+10, white);
	  
	  const float point_value = (*inverse_function)(max_original_value, min_original_value, val, 
							FLAGS_gaussian_colormap_shape, 
							FLAGS_gaussian_colormap_epsilon);
	  
	  char str[255];
	  sprintf(str, "%.2f", point_value);
	  image->draw_text(x - font_height, 
			   line_y + line_height + 10 + 5 - line_y*0.5, 
			   str, 
			   white, 0, 1, font_height);
	}
      }
      
      // The last line.
      const int32 x = image->width() - colormap_padding;
      image->draw_line(x, line_y-10, x, line_height+10, white);	
      const float point_value = max_original_value;
      char str[255];
      sprintf(str, "%.2f", point_value);
      image->draw_text(x - font_height, 
		       line_y + line_height + 10 + 5 - line_y*0.5, 
		       str, 
		       white, 0, 1, font_height);
      
      delete[] transformed_values_ptr;
      delete[] original_values_ptr;
    }
  } // namespace util
} // namespace slib

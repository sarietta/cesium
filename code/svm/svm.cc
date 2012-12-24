#include "svm.h"

#include <common/scoped_ptr.h>
#include <glog/logging.h>
#include <stdio.h>
#include "model.h"
#include <string>
#include <string.h>
#include <util/assert.h>

using std::string;

/******************************************
  BEGIN LIBSVM ROUTINES
******************************************/

void print_null(const char *s) {}

void do_cross_validation() {
#if 0
  int i;
  int total_correct = 0;
  double total_error = 0;
  double sumv = 0, sumy = 0, sumvv = 0, sumyy = 0, sumvy = 0;
  double *target = Malloc(double,prob.l);
  
  svm_cross_validation(&prob,&param,nr_fold,target);
  if(param.svm_type == EPSILON_SVR ||
     param.svm_type == NU_SVR) {
    for(i=0;i<prob.l;i++) {
      double y = prob.y[i];
      double v = target[i];
      total_error += (v-y)*(v-y);
      sumv += v;
      sumy += y;
      sumvv += v*v;
      sumyy += y*y;
      sumvy += v*y;
    }
    printf("Cross Validation Mean squared error = %g\n",total_error/prob.l);
    printf("Cross Validation Squared correlation coefficient = %g\n",
	   ((prob.l*sumvy-sumv*sumy)*(prob.l*sumvy-sumv*sumy))/
	   ((prob.l*sumvv-sumv*sumv)*(prob.l*sumyy-sumy*sumy))
	   );
  }
  else {
    for(i=0;i<prob.l;i++)
      if(target[i] == prob.y[i])
	++total_correct;
    printf("Cross Validation Accuracy = %g%%\n",100.0*total_correct/prob.l);
  }
  free(target);
#else
  LOG(ERROR) << "Cross validation not currently supported";
#endif
}

bool parse_command_line(const string& cmd, svm_parameter* param, int* cross_validation) {
  // default values
  param->svm_type = C_SVC;
  param->kernel_type = RBF;
  param->degree = 3;
  param->gamma = 0;	// 1/num_features
  param->coef0 = 0;
  param->nu = 0.5;
  param->cache_size = 100;
  param->C = 1;
  param->eps = 1e-3;
  param->p = 0.1;
  param->shrinking = 1;
  param->probability = 0;
  param->nr_weight = 0;
  param->weight_label = NULL;
  param->weight = NULL;
  (*cross_validation) = 0;
  
  int argc = 1;
  char cmd_cstr[2048];
  sprintf(cmd_cstr, "%s", cmd.c_str());
  char* argv[1024];
  if((argv[argc] = strtok(cmd_cstr, " ")) != NULL) {
    while((argv[++argc] = strtok(NULL, " ")) != NULL) {
      ;
    }
  }
  // parse options
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') break;
    if (++i >= argc) {
      LOG(ERROR) << "Invalid parameters: " << cmd;
      break;
    }
    switch (argv[i-1][1]) {
    case 's':
      param->svm_type = atoi(argv[i]);
      break;
    case 't':
      param->kernel_type = atoi(argv[i]);
      break;
    case 'd':
      param->degree = atoi(argv[i]);
      break;
    case 'g':
      param->gamma = atof(argv[i]);
      break;
    case 'r':
      param->coef0 = atof(argv[i]);
      break;
    case 'n':
      param->nu = atof(argv[i]);
      break;
    case 'm':
      param->cache_size = atof(argv[i]);
      break;
    case 'c':
      param->C = atof(argv[i]);
      break;
    case 'e':
      param->eps = atof(argv[i]);
      break;
    case 'p':
      param->p = atof(argv[i]);
      break;
    case 'h':
      param->shrinking = atoi(argv[i]);
      break;
    case 'b':
      param->probability = atoi(argv[i]);
      break;
    case 'q':
      i--;
      break;
    case 'v':
#if 0
      (*cross_validation) = 1;
      nr_fold = atoi(argv[i]);
      if (nr_fold < 2) {
	LOG(ERROR) << "n-fold cross validation: n must >= 2";
	return false;
      }
#else
      LOG(WARNING) << "Cross validation not currently supported";
#endif
      break;
    case 'w':
#if 0
      ++param->nr_weight;
      param->weight_label = (int *)realloc(param->weight_label,sizeof(int)*param->nr_weight);
      param->weight = (double *)realloc(param->weight,sizeof(double)*param->nr_weight);
      param->weight_label[param->nr_weight-1] = atoi(&argv[i-1][2]);
      param->weight[param->nr_weight-1] = atof(argv[i]);
      break;
#else
      LOG(ERROR) << "Don't support weighted data points";
      return false;
#endif
    default:
      LOG(ERROR) << "Unknown option: - " << argv[i-1][1];
      return false;
    }
  }
  
  return true;
}

/******************************************
  END LIBSVM ROUTINES
******************************************/

using Eigen::VectorXf;

namespace slib {
  namespace svm {

    void CopyLibSVMModel(const svm_model* libmodel, const int& feature_dimensions, Model* model) {
      if (libmodel->nr_class > 2) {
	LOG(WARNING) << "There were more than 2 classes in the SVM model";
      }

      const int num_weights = feature_dimensions;
      model->num_weights = num_weights;

      model->weights.reset(new float[num_weights]);
      for (int i = 0; i < num_weights; i++) {
	model->weights[i] = 0.0f;
      }
      for (int j = 0; j < libmodel->l; j++) {
	int x_index = 0;
	while (libmodel->SV[j][x_index].index != -1) {
	  const int feature_dimension_index = libmodel->SV[j][x_index].index - 1;
	  model->weights[feature_dimension_index] += libmodel->sv_coef[0][j] * libmodel->SV[j][x_index].value;
	  x_index++;
	}
      }

      model->rho = libmodel->rho[0];
      model->first_label = libmodel->label[0];
    }
    
    bool Train(const VectorXf& labels, const FloatMatrix& samples,
	       const string& flags, Model* model) {
      ASSERT_EQ(labels.size(), samples.rows());
      
      svm_set_print_string_function(&print_null);
      
      svm_parameter param;
      int cross_validation;
      if (!parse_command_line(flags, &param, &cross_validation)) {
	return false;
      }
      
      // Setup the problem form the inputs. This is really the glue
      // between this method and the "external" library.
      svm_problem prob;
      prob.l = samples.rows();
      
      scoped_array<double> y(new double[labels.size()]);
      for (int i = 0; i < labels.size(); i++) {
	y[i] = (double) labels(i);
      }
      prob.y = y.get();
      
      int elements = 0;
      int sc = samples.cols();
      if (param.kernel_type == PRECOMPUTED) {
	elements = prob.l * (sc + 1);
      } else {
	for (int i = 0; i < prob.l; i++) {
	  for (int k = 0; k < sc; k++) {
	    if (samples(i, k) != 0) {
	      elements++;
	    }
	  }
	  // count the '-1' element
	  elements++;
	}
      }
      
      scoped_array<svm_node*> x(new svm_node*[prob.l]);
      scoped_array<svm_node> x_space(new svm_node[elements]);
      
      int max_index = sc;
      int j = 0;
      for (int i = 0; i < prob.l; i++) {
	x[i] = &x_space[j];
	
	for (int k = 0; k < sc; k++) {
	  if (param.kernel_type == PRECOMPUTED || samples(i, k) != 0) {
	    x_space[j].index = k + 1;
	    x_space[j].value = samples(i, k);
	    j++;
	  }
	}
	x_space[j++].index = -1;
      }
      
      if (param.gamma == 0 && max_index > 0) {
	param.gamma = 1.0 / max_index;
      }
      
      if (param.kernel_type == PRECOMPUTED) {
	for (int i = 0; i < prob.l; i++) {
	  if ((int) x[i][0].value <= 0 || (int) x[i][0].value > max_index) {
	    LOG(ERROR) << "Wrong input format: sample_serial_number out of range";
	    return false;
	  }
	}
      }
      
      prob.x = x.get();
      
      const char* error_msg = svm_check_parameter(&prob, &param);
      
      if(error_msg) {
	LOG(ERROR) << "ERROR: " << error_msg;
	return false;
      }
      
      if(cross_validation) {
	do_cross_validation();
      } else {
	svm_model* model_ptr = svm_train(&prob, &param);
	CopyLibSVMModel(model_ptr, samples.cols(), model);
	svm_free_and_destroy_model(&model_ptr);
      }
      svm_destroy_param(&param);

      return true;
    }
  }  // namespace svm
}  // namespace slib

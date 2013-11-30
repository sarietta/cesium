#ifndef __SLIB_MPI_CESIUM_H__
#define __SLIB_MPI_CESIUM_H__

/**
   This is the main framework. It contains all of the code to do checkpointing, etc.
 */

#include <boost/signals2/mutex.hpp>
#include <common/scoped_ptr.h>
#include <gflags/gflags.h>
#include <map>
#include <mpi/mpijob.h>
#include <string>
#include <util/matlab.h>
#include <vector>

#define CESIUM_REGISTER_COMMAND(function) slib::mpi::Cesium::RegisterCommand(#function, function);
#define CESIUM_FINISH_JOB_STRING "__CESIUM_FINISH_JOB__"

#define CESIUM_CACHED_VARIABLES_FIELD "__CESIUM_CACHED_VARIABLES__"

DECLARE_string(cesium_working_directory);
DECLARE_string(cesium_temporary_directory);
DECLARE_bool(cesium_export_log);
DECLARE_int32(cesium_wait_interval);
DECLARE_bool(cesium_checkpoint_variables);
DECLARE_bool(cesium_all_indices_at_once);
DECLARE_bool(cesium_intelligent_parameters);
DECLARE_int32(cesium_partial_variable_chunk_size);
DECLARE_bool(cesium_debug_mode);
DECLARE_int32(cesium_debug_mode_node);
DECLARE_int32(cesium_debug_mode_process_single_index);

#warning "Remove cesium_checkpointed_variables FLAG"
DECLARE_string(cesium_checkpointed_variables);

namespace slib {
  namespace mpi {
    class Kernel;
    typedef void (*Function)(const slib::mpi::JobDescription&, JobOutput* output);
  }
  namespace util {
    class MatlabMatrix;
  }
}

namespace slib {
  namespace mpi {

    enum CesiumNodeType {
      CesiumMasterNode,
      CesiumComputeNode
    };

    struct CesiumExecutionInstance {
      int total_indices;

      // A list of available node ids.
      std::vector<int> available_processors;
      // A list of nodes that have died.
      std::map<int, bool> dead_processors;
      // A list of indices that have been completed.
      std::map<int, bool> completed_indices;
      // A list of indices that are currently being processed.
      std::map<int, bool> pending_indices;
      // A mapping from node to currently processing indices.
      std::map<int, std::vector<int> > node_indices;
      // A list of outputs that will be saved.
      std::map<std::string, slib::util::MatlabMatrix> final_outputs;
      
      // A list of processors that have completed at least one job.
      std::map<int, bool> processors_completed_one;
      
      int partial_output_unique_int;
      // Keeps track of partial outputs.
      std::map<std::string, std::vector<int> > partial_output_indices;
      
      // Keeps track of the number of indices saved for each output (for checkpointing).
      std::map<std::string, int> output_counts;
      // Keeps track of checkpoints.
      std::map<std::string, std::vector<int> > output_indices;
      
      // Synchronizes access to the above resources.
      boost::signals2::mutex job_completion_mutex;

      // Holds the variable types.
      std::map<std::string, VariableType> input_variable_types;
      std::map<std::string, VariableType> output_variable_types;
      
      // For input partial variables. This map allows us to postpone
      // loading each part of the partial variable until we execute
      // the job.
      std::map<std::string, std::pair<slib::util::MatlabMatrix, FILE*> > partial_variables;
    };  // struct CesiumExecutionInstance

    class Cesium {
    public:
      virtual ~Cesium() {}

      static Cesium* GetInstance();

      static void RegisterCommand(const std::string& command, const Function& function);     
      
      // This MUST be called before any other operations. It starts up
      // the framework across the available nodes (automatically
      // gleaned from MPI settings). On non-master nodes, this method
      // will block until the Finish method is called. On the master
      // node, this method immediately returns a value of
      // CesiumMasterNode.
      CesiumNodeType Start();

      // You can set the working directory this way or by modifying
      // the flag cesium_working_directory. The operations are
      // equivalent.
      void SetWorkingDirectory(const std::string& directory);

      // Sets the number of indices that will be processed at one
      // time. Make sure to disable intelligent parameters if you want
      // this to take effect.
      void SetBatchSize(const int& batch_size);

      // This method should be called when all processes are
      // completed.
      void Finish();

      // This is really a helper function because you are not required
      // to load variables via Cesium. You can load them yourself and
      // simply specify them in the JobDescription you pass to
      // Execute*, but this function will take care of loading
      // non-standard VariableTypes for you. This will always look for
      // the input variable in the FLAGS_cesium_working_directory. If
      // you want to load an absolute path use
      // LoadInputVariableWithAbsolutePath instead.
      //
      // You DO NOT need to specify the correct VariableType in the
      // JobDescription for the corresponding variable.
      //
      // TODO(sean): This copies the return value. Should pointer-ize
      // this method for speed.
      slib::util::MatlabMatrix LoadInputVariable(const std::string& variable_name, 
						 const VariableType& type = COMPLETE_VARIABLE);
      slib::util::MatlabMatrix LoadInputVariableWithAbsolutePath(const std::string& variable_name, 
								 const std::string& filename, 
								 const VariableType& type = COMPLETE_VARIABLE);

      // This is similar to the above functions, except that it does
      // not load the variable from disk, it simply marks the variable
      // as being partial so that it can be handled correctly at
      // execution time. You should not use this with non-complete
      // variable types that rely on being able to fseek into files
      // such as the FEATURE_STRIPPED_* variables.
      void SetVariableType(const std::string& variable_name, const slib::util::MatlabMatrix& matrix, 
			   const VariableType& type);

      // This is kind of an odd method that you should not use at all
      // unless you understand the FEATURE_STRIPPED_ROW_VARIABLE
      // variable type. If you use that variable type, you MUST call
      // this with the total number of dimensions in your features so
      // that the system can load them correctly.
      //
      // TODO(sean): Implement a way for users to implement a callback
      // for an arbitrary variable type.
      inline void SetStrippedFeatureDimensions(const int& dimensions) {
	_stripped_feature_dimensions = dimensions;
      }

      bool ExecuteJob(const JobDescription& job, JobOutput* output);
#if 0
      void ExecuteKernel(const Kernel& kernel, const JobDescription& job, JobOutput* output);
      void ExecuteFunction(const Function& function, const JobDescription& job, JobOutput* output);
#endif
    private:
      // You should not construct this object explicitly. Use the
      // singleton accessor Cesium::GetInstance().
      Cesium();
      // This is the function the compute nodes enter once Start() has
      // been executed.
      void ComputeNodeLoop();

      // This allows us to disable nodes that have died. It is called
      // via the __HandleCommunicationErrorWrapper__ method which is
      // set as the error handler for MPI errors via
      // JobController::SetCommunicationErrorHandler.
      void HandleDeadNode(const int& node);
      friend void __HandleCommunicationErrorWrapper__(const int& error_code, const int& node);

      // This is a very important function. It handles all of the
      // merging, etc of job outputs as they complete. This function
      // is handed to the JobController that is created each time you
      // call Cesium::ExecuteJob.
      void HandleJobCompleted(const JobOutput& output, const int& node);
      friend void __HandleJobCompletedWrapper__(const JobOutput& output, const int& node);

      // Logging/Output functions.
      void ExportLog(const int& pid) const;
      void ShowProgress(const std::string& command) const;

      // Sets the batch size and the checkpoint interval automatically.
      void SetParametersIntelligently();

      // Checks the variable types that were specified for the current
      // job via the field variable_types in JobDescription and
      // JobOutput passed to Execute*.
      VariableType GetInputVariableType(const std::string& variable_name) const;
      VariableType GetOutputVariableType(const std::string& variable_name) const;

      // Helper function to save a matrix to a temporary file.
      void SaveTemporaryOutput(const std::string& name, const slib::util::MatlabMatrix& matrix) const;
      // Checkpoints variables if checkpointing is enabled.
      void CheckpointOutputFiles(const JobOutput& output);
      // Handles the loading of checkpointed variables passed in via the 
      // flag cesium_checkpointed_variables.
      void LoadCheckpoint(const std::vector<std::string>& variables, std::vector<int>* all_indices);

      // This function handles a non-COMPLETE_VARIABLE
      // VariableType. It returns a boolean indicating whether it was
      // able to handle the variable. If this function returns false,
      // you should save/merge the variable as you normally would.
      bool HandleSpecialVariable(const JobOutput& output, const slib::util::MatlabMatrix& matrix,
				 const std::string& name, const VariableType& type);

      // This function checks to see if there is an existing instance
      // and if not it creates one. Currently there will only ever be
      // a single instance in use at a time so really the instance
      // member variable should be a singleton but it isn't.
      void InitializeInstance();

      int _rank;
      int _size;
      std::string _hostname;
      scoped_ptr<CesiumExecutionInstance> _instance;

      int _batch_size;
      int _checkpoint_interval;

      int _stripped_feature_dimensions;

      static scoped_ptr<Cesium> _singleton;
      static std::map<std::string, Function> _available_commands;

      static std::map<int, bool> _dead_processors;

      friend class TestCesiumCommunication;
    };  // class Cesium

  }  // namespace mpi
}  // namespace slib

#endif

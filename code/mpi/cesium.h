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

DECLARE_string(cesium_working_directory);
DECLARE_string(cesium_temporary_directory);
DECLARE_bool(cesium_export_log);
DECLARE_int32(cesium_wait_interval);
DECLARE_bool(cesium_checkpoint_variables);
DECLARE_bool(cesium_intelligent_parameters);
DECLARE_int32(cesium_partial_variable_chunk_size);
DECLARE_bool(cesium_debug_mode);
DECLARE_int32(cesium_debug_mode_node);
DECLARE_int32(cesium_debug_mode_process_single_index);

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
      int total_indices = 0;

      // A list of available node ids.
      std::vector<int> available_processors;
      // A list of indices that have been completed.
      std::map<int, bool> completed_indices;
      // A list of indices that are currently being processed.
      std::map<int, bool> pending_indices;
      // A list of outputs that will be saved.
      std::map<std::string, slib::util::MatlabMatrix> final_outputs;
      
      // A list of processors that have completed at least one job.
      std::map<int, bool> processors_completed_one;
      
      int partial_output_unique_int = 0;
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

      // This function handles a non-COMPLETE_VARIABLE
      // VariableType. It returns a boolean indicating whether it was
      // able to handle the variable. If this function returns false,
      // you should save/merge the variable as you normally would.
      bool HandleSpecialVariable(const JobOutput& output, const slib::util::MatlabMatrix& matrix,
				 const std::string& name, const VariableType& type);

      int _rank;
      int _size;
      std::string _hostname;
      scoped_ptr<CesiumExecutionInstance> _instance;

      int _batch_size;
      int _checkpoint_interval;

      static scoped_ptr<Cesium> _singleton;
      static std::map<std::string, Function> _available_commands;
    };  // class Cesium

  }  // namespace mpi
}  // namespace slib

#endif

#include "cesium.h"

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <map>
#include <string>
#include <string/stringutils.h>
#include <svm/detector.h>
#include <util/matlab.h>
#include <util/system.h>
#include <util/timer.h>
#include <vector>

DEFINE_string(cesium_working_directory, "/srv/cluster-fs", 
	      "The root directory where the input files live and the output files will be saved.");
DEFINE_string(cesium_temporary_directory, "/tmp", "Directory to store temp files.");

DEFINE_bool(cesium_export_log, true, "If true, will export the master's log to the working directory.");
DEFINE_int32(cesium_wait_interval, 5, "The number of seconds to wait between checking status of job.");

DEFINE_bool(cesium_checkpoint_variables, true, "Set to false if you don't want to checkpoint variables.");
DEFINE_bool(cesium_intelligent_parameters, true, "Automatically sets batch size and checkpoint interval.");
DEFINE_int32(cesium_partial_variable_chunk_size, 50, 
	     "The size of each chunk of a partial variable. "
	     "If a variable is NxM and is a partial row variable, "
	     "there will a set of partial_variable_chunck_sizexM files that comprise it in the working directory.");

DEFINE_bool(cesium_debug_mode, false, 
	    "Whether the job should run in a debugging mode that will skip inputs, etc based on "
	    "subsequent commands.");
DEFINE_int32(cesium_debug_mode_node, 1, 
	     "The node that is actually going to run.");
DEFINE_int32(cesium_debug_mode_process_single_index, -1, 
	     "Set positive to indicate that only a single index should be computed. "
	     "Useful if a job is failing on a specific index.");

using slib::StringUtils;
using slib::svm::Detector;
using slib::util::MatlabMatrix;
using slib::util::System;
using slib::util::Timer;
using std::make_pair;
using std::map;
using std::pair;
using std::string;
using std::vector;

namespace slib {
  namespace mpi {

    scoped_ptr<Cesium> Cesium::_singleton;
    map<string, Function> Cesium::_available_commands;
    
    Cesium::Cesium() 
      : _rank(-1)
      , _size(-1)
      , _hostname("")
      , _batch_size(5)
      , _checkpoint_interval(-1)
      , _stripped_feature_dimensions(-1) {}
    
    Cesium* Cesium::GetInstance() {
      if (_singleton.get() == NULL) {
	_singleton.reset(new Cesium);
      }

      return _singleton.get();
    }
    
    void Cesium::RegisterCommand(const string& command, const Function& function) {
      if (command == CESIUM_FINISH_JOB_STRING) {
	LOG(ERROR) << "Attempted to register a job under the protected name: " << CESIUM_FINISH_JOB_STRING;
	LOG(ERROR) << "Please rename your function to avoid this collision";
      } else {
	_available_commands[command] = function;
      }
    }

    void Cesium::SetBatchSize(const int& batch_size)  {
      _batch_size = batch_size;
    }

    void Cesium::SetParametersIntelligently() {
      const int total_indices = _instance->total_indices;
      const int num_nodes = _instance->available_processors.size();

      if (total_indices < 15) {
	_batch_size = 3;
      } else if (total_indices < 30) {
	_batch_size = 7;
      } else if (total_indices < 100) {
	_batch_size = 10;
      } else {
	_batch_size = 25;
      }
      
      const int even = (int) (floor(((float) total_indices) / ((float) num_nodes)));
      if (_batch_size > even) {
	_batch_size = even;
      }
      
      if (_batch_size <= 0) {
	_batch_size = 1;
      }
            
      // In general we want about a checkpoint every couple of batches.
      if (num_nodes < 5) {
	_checkpoint_interval = _batch_size * 2;
      } else if (num_nodes < 50) {
	_checkpoint_interval = _batch_size * 10;
      } else if (num_nodes < 100) {
	_checkpoint_interval = _batch_size * 20;
      } else {
	_checkpoint_interval = _batch_size * 50;
      }
      
      LOG(INFO) << "***********************************************";
      LOG(INFO) << "Setting batch size: " << _batch_size;
      if (FLAGS_cesium_checkpoint_variables) {
	LOG(INFO) << "Setting checkpoint interval: " << _checkpoint_interval;
      } else {
	_checkpoint_interval = -1;
      }
      LOG(INFO) << "***********************************************";
    }
    
    CesiumNodeType Cesium::Start() {
      int flag;
      MPI_Initialized(&flag);
      if (!flag) {
	LOG(ERROR) << "Attempted to start a Cesium job before calling MPI_Init. "
		   << "You must call MPI_Init before any Cesium methods.";
      }
      MPI_Comm_rank(MPI_COMM_WORLD, &_rank);
      MPI_Comm_size(MPI_COMM_WORLD, &_size);

      {
	char buf[32];
	gethostname(buf, sizeof(char) * 32);
	_hostname = string(buf);
      }
      
      if (_rank == MPI_ROOT_NODE) {	
	// Create directories as needed.
	System::ExecuteSystemCommand("mkdir -p " + FLAGS_cesium_temporary_directory);
	return CesiumMasterNode;
      } else {
	ComputeNodeLoop();
	return CesiumComputeNode;
      }
    }
    
    void Cesium::Finish() {
      JobController controller;
      
      JobDescription finish;
      finish.command = CESIUM_FINISH_JOB_STRING;
      for (int i = 1; i < _size; i++) {
	controller.StartJobOnNode(finish, i);
      }
    }

    // This is just a wrapper to avoid passing a pointer to a member
    // function to JobController's HandleJobCompleted routine.
    void __HandleJobCompletedWrapper__(const JobOutput& output, const int& node) {
      Cesium::GetInstance()->HandleJobCompleted(output, node);
    }

    void Cesium::InitializeInstance() {
      if (_instance.get() == NULL) {
	_instance.reset(new CesiumExecutionInstance());
	_instance->total_indices = 0;
	_instance->partial_output_unique_int = 0;
      }
    }
    
    bool Cesium::ExecuteJob(const JobDescription& job, JobOutput* output) {
      LOG(INFO) << "Total processors: " << _size - 1;
      if (_size - 1 <= 0) {
	LOG(ERROR) << "This job was started with no workers!";
	return false;
      }

      JobDescription mutable_job = job;

      InitializeInstance();
      for (map<string, VariableType>::const_iterator iter = job.variable_types.begin(); 
	   iter != job.variable_types.end(); iter++) {
	const string name = (*iter).first;
	const VariableType type = (*iter).second;
	_instance->input_variable_types[name] = type;
      }
      for (map<string, VariableType>::const_iterator iter = output->variable_types.begin(); 
	   iter != output->variable_types.end(); iter++) {
	const string name = (*iter).first;
	const VariableType type = (*iter).second;
	_instance->output_variable_types[name] = type;
      }
            
      const int pid = getpid();
      if (FLAGS_logtostderr) {
	FLAGS_cesium_export_log = false;
      }      

      vector<int> all_indices = mutable_job.indices;
      mutable_job.indices.clear();      
      _instance->total_indices = all_indices.size();
#if 0
      // Load the checkpointed variables.
      if (FLAGS_checkpointed_variables != "") {
	const vector<string> variables = StringUtils::Explode(";", FLAGS_checkpointed_variables);
	LoadCheckpoint(variables, &all_indices);
      }
#endif 
      VLOG(1) << "Number of indices: " << all_indices.size();
      
      // This is the main execution loop. The master will loop through
      // all of the indices that need to be computed and will spawn the
      // corresponding jobs on each of the nodes.
      JobController controller;
      controller.SetCompletionHandler(&__HandleJobCompletedWrapper__);
      
      // Setup the available processors information. Do in reverse order
      // in case the job size is less than the number of nodes.
      for (int node = _size - 1; node >= 1; node--) {
	if (FLAGS_cesium_debug_mode) {
	  if (node == FLAGS_cesium_debug_mode_node) {	  
	    _instance->available_processors.push_back(node);
	  }
	} else {
	  _instance->available_processors.push_back(node);
	}
      }
      VLOG(1) << "Available processors: " << _instance->available_processors.size();

      if (FLAGS_cesium_intelligent_parameters) {
	SetParametersIntelligently();
      }
      
      const int last_node = _size - 1;
      
      LOG(INFO) << "***********************************************";
      LOG(INFO) << "Entering Main Computation Loop";
      LOG(INFO) << "***********************************************";
      
      while ((int) _instance->completed_indices.size() < _instance->total_indices) {
	// Synchronizes access with the job completion routine.
	_instance->job_completion_mutex.lock(); {
	  // For each node, set the indices and run the job.
	  for (int i = (int) _instance->available_processors.size() - 1; i >= 0; i--) {
	    const int node = _instance->available_processors.back();
	    
	    if (FLAGS_cesium_debug_mode) {
	      if (node != FLAGS_cesium_debug_mode_node) {
		continue;
	      }
	    }
	    
	    // Set up the JobDescription for this node.
	    vector<int> indices;
	    string indices_list = "[";
	    
	    if (FLAGS_cesium_debug_mode_process_single_index >= 0) {
	      indices.push_back(FLAGS_cesium_debug_mode_process_single_index);
	      indices_list = StringUtils::StringPrintf("%s %d", indices_list.c_str(), indices.back());
	    }
	    
	    // Determine all of the indices that are not completed =
	    // DIFF(all_indices, completed_indices + pending_indices);
	    vector<int> incompleted_indices;
	    for (int j = 0; j < (int) all_indices.size(); j++) {
	      const int index = all_indices[j];
	      if (_instance->completed_indices.find(index) == _instance->completed_indices.end()
		  && _instance->pending_indices.find(index) == _instance->pending_indices.end()) {
		incompleted_indices.push_back(index);
	      }
	    }
	    VLOG(1) << "Number of incompleted indices: " << incompleted_indices.size();
	    
	    const int available_nodes = _instance->available_processors.size();
	    for (int j = 0; j < (int) incompleted_indices.size(); j++) {
	      if (j % available_nodes != (available_nodes - i - 1)) {//(node - 1 - (last_node - available_nodes))) {
		continue;
	      }
	      const int node_index = j;
	      const int index = incompleted_indices[node_index];
	      indices.push_back(index);
	      indices_list = StringUtils::StringPrintf("%s %d", indices_list.c_str(), indices.back());
	      _instance->pending_indices[index] = true;

	      if (_batch_size > 0 && indices.size() >= _batch_size) {
		break;
	      }
	    }
	    
	    indices_list = StringUtils::StringPrintf("%s]", indices_list.c_str());	  
	    
	    // In the case that the node is not assigned any indices,
	    // push it to the bottom of the stack so the next node gets
	    // a chance to run something.
	    if (indices.size() == 0) {
	      VLOG(1) << "No indices available for node: " << node;
	      _instance->available_processors.pop_back();
	      _instance->available_processors.insert(_instance->available_processors.begin(), node);
	      
	      continue;
	    }
	    
	    mutable_job.indices = indices;

	    // Handle partial variables that were loaded via the
	    // LoadInputVariable method.
	    for (map<string, pair<MatlabMatrix, FILE*> >::const_iterator iter = _instance->partial_variables.begin();
		 iter != _instance->partial_variables.end(); iter++) {
	      const string name = (*iter).first;
	      const map<string, VariableType>::const_iterator type_iter = _instance->input_variable_types.find(name);
	      if (type_iter == _instance->input_variable_types.end()) {
		LOG(ERROR) << "Special variable does not have a type defined: " << name;
		continue;
	      }

	      const VariableType type = (*type_iter).second;

	      if (FLAGS_v >= 1) {
		Timer::Start();
	      }

	      if (type == PARTIAL_VARIABLE_ROWS) {
		const MatlabMatrix& variable = _instance->partial_variables[name].first;
		const Pair<int> dimensions = variable.GetDimensions();
		if (dimensions.x <= 1) {
		  LOG(WARNING) << "You specfied variable [" << name << "] as a partial row variable "
			       << "but it has <= 1 rows";
		}

		MatlabMatrix partial(variable.GetMatrixType(), dimensions);
		for (int k = 0; k < (int) indices.size(); k++) {
		  const int index = indices[k];
		  for (int kk = 0; kk < dimensions.y; kk++) {
		    partial.Set(index, kk, variable.Get(index, kk));
		  }
		}

		mutable_job.variables[name] = partial;
	      } else if (type == PARTIAL_VARIABLE_COLS) {
		const MatlabMatrix& variable = _instance->partial_variables[name].first;
		const Pair<int> dimensions = variable.GetDimensions();
		if (dimensions.y <= 1) {
		  LOG(WARNING) << "You specfied variable [" << name << "] as a partial column variable "
			       << "but it has <= 1 columns";
		}

		MatlabMatrix partial(variable.GetMatrixType(), dimensions);
		for (int k = 0; k < (int) indices.size(); k++) {
		  const int index = indices[k];
		  for (int kk = 0; kk < dimensions.x; kk++) {
		    partial.Set(kk, index, variable.Get(kk, index));
		  }
		}

		mutable_job.variables[name] = partial;
	      } else if (type == FEATURE_STRIPPED_ROW_VARIABLE) {
		// TODO(sean): This is WAY too specific to the silicon
		// project. This needs to be made more general in the
		// near future.
		const int32 feature_dimensions = _stripped_feature_dimensions;
		if (feature_dimensions < 0) {
		  LOG(ERROR) << "You specified a FEATURE_STRIPPED_* variable but did not call "
			     << "SetStrippedFeatureDimensions(). You MUST call this function in order "
			     << "to use this variable type.";
		  continue;
		}
		scoped_array<float> data(new float[feature_dimensions]);
		
		mutable_job.variables[name] = _instance->partial_variables[name].first;
		FILE* fid = _instance->partial_variables[name].second;
		if (!fid) {
		  LOG(ERROR) << "Attempted to load a partial variable from a bad file descriptor: " + name;
		  continue;
		}
		fseek(fid, 0, SEEK_SET);
		
		long int seek = 0;	      
		for (int k = 0; k < (int) indices.size(); k++) {
		  const int row = indices[k];
		  for (int col = 0; col < (int) mutable_job.variables[name].GetDimensions().y; col++) {
		    MatlabMatrix cell;
		    mutable_job.variables[name].GetMutableCell(row, col, &cell);
		    for (int kk = 0; kk < cell.GetNumberOfElements(); kk++) {
		      const long int feature_index = (long int) cell.GetStructField("features", kk).GetScalar();
		      fseek(fid, (feature_index - seek) * sizeof(float) * feature_dimensions, SEEK_CUR);
		      fread(data.get(), sizeof(float), feature_dimensions, fid);
		      
		      cell.SetStructField("features", kk, MatlabMatrix(data.get(), 1, feature_dimensions));
		      seek = feature_index + 1;
		    }
		  }
		}
	      }

	      VLOG(1) << "Elapsed time to load partial input [" << name << "]: " << Timer::Stop();
	    }	  
	    
	    // Run the job.
	    LOG(INFO) << "Starting job " << mutable_job.command << " on node " << node << ": " << indices_list;
	    _instance->available_processors.pop_back();
	    controller.StartJobOnNode(mutable_job, node);
	  }
	  ShowProgress(mutable_job.command);
	}
	_instance->job_completion_mutex.unlock();
	
	// Poll the nodes to see if they have completed.
	controller.CheckForCompletion();
	
	// Wait for a little while so we don't overload the output.
	if (FLAGS_cesium_export_log) {
	  ExportLog(pid);
	}
	sleep(FLAGS_cesium_wait_interval);
      }
      
      output->variables = _instance->final_outputs;

      // Save any partial variables that were not completed.
      for (map<string, VariableType>::const_iterator iter = _instance->output_variable_types.begin();
	   iter != _instance->output_variable_types.end(); iter++) {
	const string name = (*iter).first;
	const VariableType type = (*iter).second;

	if (type == slib::mpi::PARTIAL_VARIABLE_ROWS || type == slib::mpi::PARTIAL_VARIABLE_COLS) {
	  if (_instance->partial_output_indices[name].size() > 0) {
	    LOG(INFO) << "***********************************************";
	    LOG(INFO) << "Saving chunk for partial variable: " << name;
	    LOG(INFO) << "***********************************************";

	    System::ExecuteSystemCommand("mkdir -p " + FLAGS_cesium_temporary_directory + "/" + name);
	    SaveTemporaryOutput(StringUtils::StringPrintf("%s/%d", name.c_str(), 
							  _instance->partial_output_unique_int), 
				_instance->final_outputs[name]);

	    _instance->partial_output_indices[name].clear();
	    _instance->final_outputs[name] = MatlabMatrix();
	    _instance->partial_output_unique_int++;
	  }
	}
      }

      // Close the partial variables.
      for (map<string, pair<MatlabMatrix, FILE*> >::const_iterator iter = _instance->partial_variables.begin();
	   iter != _instance->partial_variables.end(); iter++) {
	const string name = (*iter).first;
	FILE* fid = _instance->partial_variables[name].second;
	if (fid != NULL) {
	  fclose(fid);
	}
      }

      // This kills the current instance so that any modifications to
      // an "instance" will effectively create a new one.
      _instance.reset(NULL);

      return true;
    }
    
#if 0
    void Cesium::ExecuteKernel(const Kernel& kernel, const JobDescription& job, JobOutput* output) {
    }
    
    void Cesium::ExecuteFunction(const Function& function, const JobDescription& job, JobOutput* output) {
    }
#endif
    
    void Cesium::ComputeNodeLoop() {
      if (FLAGS_v >= 1) {
	for (map<string, Function>::const_iterator iter = _available_commands.begin();
	     iter != _available_commands.end();
	     iter++) {
	  VLOG(1) << "Available Command: " << (*iter).first;
	}
      }
      while (1) {
	VLOG(1) << "Waiting for a new job...";
	JobDescription job = JobNode::WaitForJobData();
	if (job.command == CESIUM_FINISH_JOB_STRING) {
	  LOG(INFO) << "Node " << _rank << " finishing";
	  break;
	}

	LOG(INFO) << "Received new job: " << job.command;

	// Run the appropriate command.
	JobOutput output;
	output.command = job.command;
	// Determine the function.
	if (_available_commands.find(job.command) == _available_commands.end()) {
	  LOG(ERROR) << "Attempted to execute unknown command: " << job.command;
	} else {
	  const Function& function = _available_commands[job.command];
	  
	  const vector<int> job_indices = job.indices;
	  for (uint32 i = 0; i < job_indices.size(); i++) {
	    job.indices.clear();
	    job.indices.push_back(job_indices[i]);
	    
	    LOG(INFO) << "Running job at index: " << job_indices[i];
	    (*function)(job, &output);
	    google::FlushLogFiles(google::GLOG_INFO);
	  }
	}
	
	VLOG(1) << "Sending completion message";
	JobNode::SendCompletionMessage(MPI_ROOT_NODE);
	VLOG(1) << "Waiting for completion response";
	JobNode::WaitForCompletionResponse(MPI_ROOT_NODE);
	LOG(INFO) << "Send job output to root";
	JobNode::SendJobDataToNode(output, MPI_ROOT_NODE);
      }
    }

    void Cesium::ExportLog(const int& pid) const {
      const string filename = FLAGS_cesium_working_directory + "/master.log";

      const string cmd 
	= StringUtils::StringPrintf("rsync -t %s/*INFO*.%d %s", FLAGS_log_dir.c_str(), pid, filename.c_str());
      System::ExecuteSystemCommand(cmd);
    }

    void Cesium::SetWorkingDirectory(const string& directory) {
      FLAGS_cesium_working_directory = directory;
    }

    void Cesium::ShowProgress(const string& command) const {
      const int running = _size - 1 - (int) _instance->available_processors.size();
      const int total_indices = _instance->total_indices;

      LOG(INFO) << "\nRunning Command: " << command 
		<< "\n\tNumber Pending: " << _instance->pending_indices.size()
		<< "\n\tNumber Completed: " << _instance->completed_indices.size() << " of " << total_indices
		<< "\n\tAvailable Processors: " << _instance->available_processors.size() << " of " << (_size-1)
		<< "\n\tRunning Processors: " << running << " of " << (_size - 1);
      google::FlushLogFiles(google::GLOG_INFO);
    }

    // Helper function for methods below.
    VariableType GetVariableType(const map<string, VariableType>& types, const string& variable_name) {
      const map<string, VariableType>::const_iterator iter = types.find(variable_name);
      if (iter == types.end()) {
	return COMPLETE_VARIABLE;
      } else {
	return (*iter).second;
      }
    }

    VariableType Cesium::GetInputVariableType(const string& variable_name) const {
      return GetVariableType(_instance->input_variable_types, variable_name);
    }

    VariableType Cesium::GetOutputVariableType(const string& variable_name) const {
      return GetVariableType(_instance->output_variable_types, variable_name);
    }

    void Cesium::SaveTemporaryOutput(const string& name, const MatlabMatrix& matrix) const {
      const string local_file = FLAGS_cesium_temporary_directory + "/" + name + ".mat";
      if (!matrix.SaveToFile(local_file)) {
	LOG(ERROR) << "Could not save matrix to temporary file: " << local_file;
      }
    }

    void Cesium::CheckpointOutputFiles(const JobOutput& output) {
      if (_checkpoint_interval < 0) {
	return;
      }

      for (map<string, MatlabMatrix>::iterator iter = _instance->final_outputs.begin();
	   iter != _instance->final_outputs.end(); iter++) {
	const string& name = (*iter).first;
	const VariableType type = GetOutputVariableType(name);
	
	if (type == PARTIAL_VARIABLE_ROWS || type == PARTIAL_VARIABLE_COLS) {
	  return;
	}
	
	if (_instance->output_counts.find(name) == _instance->output_counts.end()) {
	  _instance->output_counts[name] = output.indices.size();
	} else {
	  _instance->output_counts[name] = _instance->output_counts[name] + output.indices.size();
	}
	
	if (_instance->output_indices.find(name) == _instance->output_indices.end()) {
	  _instance->output_indices[name] = output.indices;
	} else {
	  _instance->output_indices[name].insert(_instance->output_indices[name].end(), 
						 output.indices.begin(), 
						 output.indices.end());
	}
	
	if (_instance->output_counts[name] >= _checkpoint_interval) {
	  LOG(INFO) << "***********************************************";
	  LOG(INFO) << "Checkpointing output variable: " << name;
	  LOG(INFO) << "***********************************************";
	  
	  SaveTemporaryOutput(name + "_checkpoint", (*iter).second);
	  SaveTemporaryOutput(name + "_checkpoint_indices", MatlabMatrix(_instance->output_indices[name]));
	  _instance->output_counts[name] = 0;
	}
      }
    }

    void Cesium::HandleJobCompleted(const JobOutput& output, const int& node) {
      LOG(INFO) << "Job completed on node: " << node;

      // Synchronized access with the accessor routines in the main loop
      // below.
      _instance->job_completion_mutex.lock(); {
	string output_indices_list = "[";
	for (uint32 i = 0; i < output.indices.size(); i++) {
	  output_indices_list = StringUtils::StringPrintf("%s %d", output_indices_list.c_str(), output.indices[i]);
	  _instance->completed_indices[output.indices[i]] = true;
	  _instance->pending_indices.erase(output.indices[i]);
	}
	
	LOG(INFO) << "Node " << node << " output indices: " << output_indices_list << "]";
	for (map<string, MatlabMatrix>::const_iterator it = output.variables.begin(); 
	     it != output.variables.end(); 
	     it++) {
	  const string name = (*it).first;
	  const MatlabMatrix matrix = (*it).second;
	  const Pair<int> dimensions = matrix.GetDimensions();
	  VLOG(1) << "Found output: " << name << " (" << dimensions.x << " x " << dimensions.y << ")";

	  if (!HandleSpecialVariable(output, matrix, name, GetOutputVariableType(name))) {
	    _instance->final_outputs[name].Merge(matrix);
	  }
	}
	_instance->available_processors.push_back(node);
	_instance->processors_completed_one[node] = true;

	CheckpointOutputFiles(output);
      } 
      _instance->job_completion_mutex.unlock();
    }

    bool Cesium::HandleSpecialVariable(const JobOutput& output, const MatlabMatrix& matrix,
				       const string& name, const VariableType& type) {
      const Pair<int> dimensions = matrix.GetDimensions();

      if (type == slib::mpi::PARTIAL_VARIABLE_ROWS || type == slib::mpi::PARTIAL_VARIABLE_COLS) {
	// Save current outputs and output indices.
	_instance->final_outputs[name].Merge(matrix);
	_instance->partial_output_indices[name].insert(_instance->partial_output_indices[name].end(), 
						       output.indices.begin(), output.indices.end());

	// Check to see if the current size of the matrix is above the chunk size.
	if (_instance->partial_output_indices[name].size() >= FLAGS_cesium_partial_variable_chunk_size) {
	  LOG(INFO) << "***********************************************";
	  LOG(INFO) << "Saving chunk for partial variable: " << name;
	  LOG(INFO) << "***********************************************";

	  System::ExecuteSystemCommand("mkdir -p " + FLAGS_cesium_temporary_directory + "/" + name);
	  SaveTemporaryOutput(StringUtils::StringPrintf("%s/%d", name.c_str(), _instance->partial_output_unique_int),
			      _instance->final_outputs[name]);

	  _instance->partial_output_indices[name].clear();
	  _instance->final_outputs[name] = MatlabMatrix();
	  _instance->partial_output_unique_int++;
	}
	return true;
      } else if (type == slib::mpi::DSWORK_COLUMN) {
	if (_instance->processors_completed_one.size() == 0) {
	  const string directory = FLAGS_cesium_working_directory + "/" + name;
	  System::ExecuteSystemCommand("rm -rf " + directory);
	  System::ExecuteSystemCommand("mkdir -p " + directory);
	}

	for (int col = 0; col < (int) output.indices.size(); col++) {
	  MatlabMatrix column(slib::util::MATLAB_STRUCT, Pair<int>(1, 1));
	  vector<int> rows;
	  for (int row = 0; row < dimensions.x; row++) {
	    if (output.indices[col] >= dimensions.y) {
	      LOG(WARNING) << "No output data was found in column: " << output.indices[col];
	      continue;
	    }
	    
	    const MatlabMatrix entry = matrix.GetCell(row, output.indices[col]);
	    if (entry.GetMatrixType() != slib::util::MATLAB_NO_TYPE && entry.GetNumberOfElements() > 0) {
	      rows.push_back(row+1);
	      column.SetStructField(StringUtils::StringPrintf("data%d", row+1), entry);
	    }
	  }
	  if (rows.size() == 0) {
	    VLOG(1) << "No output data was found in column: " << output.indices[col];
	    continue;
	  }
	  
	  column.SetStructField("contents", MatlabMatrix(rows, false));

	  const string filename = StringUtils::StringPrintf("%s/%s/%d.mat", 
							    FLAGS_cesium_working_directory.c_str(),
							    name.c_str(), output.indices[col]+1);
	  if (!matrix.SaveToBinaryFile(filename)) {
	    LOG(ERROR) << "Could not save variable piece to file: " << filename;
	  }
	}
	return true;
      }

      return false;
    }

    MatlabMatrix Cesium::LoadInputVariable(const string& variable_name, const VariableType& type) {
      return Cesium::LoadInputVariableWithAbsolutePath(variable_name, 
						       FLAGS_cesium_working_directory + "/" + variable_name + ".mat",
						       type);
    }

    MatlabMatrix Cesium::LoadInputVariableWithAbsolutePath(const string& variable_name, 
							   const string& filename, 
							   const VariableType& type) {
      InitializeInstance();
      
      LOG(INFO) << "Loading input: " << filename;      
      MatlabMatrix input = MatlabMatrix::LoadFromFile(filename);
	
      Pair<int> dimensions = input.GetDimensions();
      VLOG(2) << "Input dimensions: " << dimensions.x << " x " << dimensions.y;
		
      if (input.GetMatrixType() == slib::util::MATLAB_NO_TYPE) {
	LOG(ERROR) << "Could not read input file: " << filename;
	return input;
      }
	
      // If it's a partial variable, wait to load it when the job
      // starts so we don't have to keep the whole thing in memory.
      if (type == slib::mpi::FEATURE_STRIPPED_ROW_VARIABLE) {
	const string feature_filename = StringUtils::Replace(".mat", filename, ".features.bin");
	_instance->partial_variables[variable_name] = make_pair(input, fopen(feature_filename.c_str(),"rb"));
	_instance->input_variable_types[variable_name] = type;
      } else if (type == slib::mpi::PARTIAL_VARIABLE_ROWS || type == slib::mpi::PARTIAL_VARIABLE_COLS) {
	_instance->partial_variables[variable_name] = make_pair(input, (FILE*) NULL);
	_instance->input_variable_types[variable_name] = type;
      }

      return input;
    }

    void Cesium::SetVariableType(const string& variable_name, const MatlabMatrix& input, 
				 const VariableType& type) {
      InitializeInstance();

      if (type == slib::mpi::PARTIAL_VARIABLE_ROWS || type == slib::mpi::PARTIAL_VARIABLE_COLS) {
	_instance->partial_variables[variable_name] = make_pair(input, (FILE*) NULL);
	_instance->input_variable_types[variable_name] = type;
      } else if (type == slib::mpi::COMPLETE_VARIABLE) {
	LOG(INFO) << "You don't need to set the type for complete variables (" << variable_name << ")";
      } else if (type == slib::mpi::FEATURE_STRIPPED_ROW_VARIABLE) {
	LOG(WARNING) << "Use the method LoadInputVariable* for variable type FEATURE_STRIPPED_ROW_VARIABLE "
		     << "(" << variable_name << ")";
      } else {
	LOG(WARNING) << "Don't know how to set the type for variable: " << variable_name;
      }
    }
    
  }  // namespace mpi
}  // namespace slib

#include "cesium.h"

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <map>
#include <string>
#include <string/stringutils.h>
#include <util/matlab.h>
#include <util/system.h>
#include <vector>

DEFINE_string(cesium_working_directory, "/srv/cluster-fs", 
	      "The root directory where the input files live and the output files will be saved.");
DEFINE_bool(cesium_export_log, true, "If true, will export the master's log to the working directory.");

DEFINE_int32(cesium_wait_interval, 5, "The number of seconds to wait between checking status of job.");

DEFINE_bool(cesium_debug_mode, false, 
	    "Whether the job should run in a debugging mode that will skip inputs, etc based on "
	    "subsequent commands.");
DEFINE_int32(cesium_debug_mode_node, 1, 
	     "The node that is actually going to run.");
DEFINE_int32(cesium_debug_mode_process_single_index, -1, 
	     "Set positive to indicate that only a single index should be computed. "
	     "Useful if a job is failing on a specific index.");

using slib::StringUtils;
using slib::util::MatlabMatrix;
using slib::util::System;
using std::map;
using std::string;
using std::vector;

namespace slib {
  namespace mpi {

    scoped_ptr<Cesium> Cesium::_singleton;
    map<string, Function> Cesium::_available_commands;
    
    Cesium::Cesium() {
    }
    
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

    void Cesium::SetParametersIntelligently(const int& total_indices, const int& num_nodes) {
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
      
      // Special consideration for autoclust_mine_negs since we know that
      // it consumes a lot of memory.
      if (FLAGS_command == "autoclust_mine_negs") {
	_batch_size = _batch_size > 5 ? 5 : _batch_size;
      }
      
      // In general we want about a checkpoint every couple of batches.
      if (num_nodes < 5) {
	FLAGS_checkpoint_interval = _batch_size * 2;
      } else if (num_nodes < 50) {
	FLAGS_checkpoint_interval = _batch_size * 10;
      } else if (num_nodes < 100) {
	FLAGS_checkpoint_interval = _batch_size * 20;
      } else {
	FLAGS_checkpoint_interval = _batch_size * 50;
      }
      
      LOG(INFO) << "***********************************************";
      LOG(INFO) << "Setting batch size: " << _batch_size;
      LOG(INFO) << "Setting checkpoint interval: " << FLAGS_checkpoint_interval;
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
    
    bool Cesium::ExecuteJob(const JobDescription& job, JobOutput* output) {
      LOG(INFO) << "Total processors: " << _size - 1;
      if (_size - 1 <= 0) {
	LOG(ERROR) << "This job was started with no workers!";
	return false;
      }

      JobDescription mutable_job = job;

      _instance.reset(new CesiumExecutionInstance());
            
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
#if 0
      if (FLAGS_intelligent_parameters) {
	SetParametersIntelligently(all_indices.size(), size - 1);
      }
#endif
      
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
#if 0
	      if (_batch_size > 0 && indices.size() >= _batch_size) {
		break;
	      }
#endif
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

    void Cesium::ExportLog(const int& pid) {
      const string filename = FLAGS_cesium_working_directory + "/master.log";

      const string cmd 
	= StringUtils::StringPrintf("rsync -t %s/*INFO*.%d %s", FLAGS_log_dir.c_str(), pid, filename.c_str());
      System::ExecuteSystemCommand(cmd);
    }

    void Cesium::SetWorkingDirectory(const string& directory) {
      FLAGS_cesium_working_directory = directory;
    }

    void Cesium::ShowProgress(const string& command) {
      const int running = _size - 1 - (int) _instance->available_processors.size();
      const int total_indices = _instance->total_indices;

      LOG(INFO) << "\nRunning Command: " << command 
		<< "\n\tNumber Pending: " << _instance->pending_indices.size()
		<< "\n\tNumber Completed: " << _instance->completed_indices.size() << " of " << total_indices
		<< "\n\tAvailable Processors: " << _instance->available_processors.size() << " of " << (_size-1)
		<< "\n\tRunning Processors: " << running << " of " << (_size - 1);
      google::FlushLogFiles(google::GLOG_INFO);
    }

    void Cesium::HandleJobCompleted(const JobOutput& output, const int& node) {
      LOG(INFO) << "Job completed on node: " << node;
#if 0
      map<string, VariableType> variable_types = GetOutputVariableTypes(output.command);
#endif
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
#if 0
	  if (variable_types.find(name) != variable_types.end()) {	
	    if (variable_types[name] == slib::mpi::PARTIAL_VARIABLE_ROWS
		|| variable_types[name] == slib::mpi::PARTIAL_VARIABLE_COLS) {
	      // Save current outputs and output indices.
	      final_outputs[name].Merge(matrix);
	      partial_output_indices[name].insert(partial_output_indices[name].end(), 
						  output.indices.begin(), output.indices.end());
	      // Check to see if the current size of the matrix is above the chunk size.
	      if (partial_output_indices[name].size() >= FLAGS_partial_variable_chunk_size) {
		LOG(INFO) << "***********************************************";
		LOG(INFO) << "Saving chunk for partial variable: " << name;
		LOG(INFO) << "***********************************************";
		System::ExecuteSystemCommand("mkdir -p " + FLAGS_temporary_directory + "/" + name);
		SaveOutput(StringUtils::StringPrintf("%s/%d", name.c_str(), partial_output_unique_int),
			   final_outputs[name], true);
		SaveOutputMetadata(StringUtils::StringPrintf("%s/%s/%d.ind", FLAGS_temporary_directory.c_str(),
							     name.c_str(), partial_output_unique_int),
				   partial_output_indices[name]);
		partial_output_indices[name].clear();
		final_outputs[name] = MatlabMatrix();
		partial_output_unique_int++;
	      }
	    } else if (variable_types[name] == slib::mpi::DSWORK_COLUMN) {
	      if (processors_completed_one.size() == 0) {
		const string remote_directory 
		  = StringUtils::Replace(working_host + ":", FLAGS_working_directory + "/" + name, "");
		if (working_host == node_hostname || FLAGS_disable_remote_copy) {
		  System::ExecuteSystemCommand("rm -rf " + remote_directory);
		  System::ExecuteSystemCommand("mkdir -p " + remote_directory);
		} else {
		  System::ExecuteSystemCommand("ssh " + working_host + " rm -rf " + remote_directory);
		  System::ExecuteSystemCommand("ssh " + working_host + " mkdir -p " + remote_directory);
		}
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
		
		if (output.HasInput("binarydetections")) {
		  SaveOutput(StringUtils::StringPrintf("%s/%d", name.c_str(), output.indices[col]+1), 
			     column, false, false, true);
		} else {
#if 0
		  SaveOutput(StringUtils::StringPrintf("%s/%d", name.c_str(), output.indices[col]+1), 
			     column, false, true);
#else
		  SaveOutput(StringUtils::StringPrintf("%s/%d", name.c_str(), output.indices[col]+1), 
			     column, false, false);
#endif
		}
	      }
	    } else {
	      _instance->final_outputs[name].Merge(matrix);
	    }
	  } else {
#endif
	    _instance->final_outputs[name].Merge(matrix);
#if 0
	  }
#endif
	}
	_instance->available_processors.push_back(node);
	_instance->processors_completed_one[node] = true;
#if 0
	CheckpointOutputFiles(output);
#endif
      } 
      _instance->job_completion_mutex.unlock();
}
    
  }  // namespace mpi
}  // namespace slib

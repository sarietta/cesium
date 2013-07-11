#include "mpijob.h"

#include <common/scoped_ptr.h>
#include <glog/logging.h>
#include <map>
#include <util/matlab.h>
#include <map>
#include <mpi.h>
#include <string>
#include <string.h>
#include <vector>

using slib::util::MatlabMatrix;
using std::map;
using std::string;
using std::vector;

namespace slib {
  namespace mpi {

    bool JobNode::_initialized = false;

    MatlabMatrix empty_matrix;
    const MatlabMatrix& JobData::GetInputByName(const string& name) const {
      map<string, MatlabMatrix>::const_iterator iter = variables.find(name);
      if (iter == variables.end()) {
	return empty_matrix;
      } else {
	return iter->second;
      }
    }

    bool JobData::HasInput(const string& name) const {
      map<string, MatlabMatrix>::const_iterator iter = variables.find(name);
      if (iter == variables.end()) {
	return false;
      } else {
	return true;
      }
    }

    void MPIErrorHandler (MPI_Comm* comm, int* err, ...) {
      JobController::PrintMPICommunicationError(*err);
    }
    
    JobController::JobController() : _completion_handler(NULL) {
      int flag;
      MPI_Initialized(&flag);
      if (!flag) {
	LOG(ERROR) << "You tried to create a JobController before calling MPI_Init. Shame on you. Fix it!";
      }
      //MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

      MPI_Comm_create_errhandler(MPIErrorHandler, &_error_handler);
      MPI_Comm_set_errhandler(MPI_COMM_WORLD, _error_handler);
    }

    void JobController::SetCompletionHandler(CompletionHandler handler) {
      _completion_handler = handler;
    }

    void JobController::SendCompletionResponse(const int& node) {
      int message = 1;
      MPI_Send(&message, 1, MPI_INT, node, MPI_COMPLETION_TAG + 1, MPI_COMM_WORLD);
    }

    void JobNode::WaitForCompletionResponse(const int& node) {
      CheckInitialized();
      int message;
      MPI_Recv(&message, 1, MPI_INT, node, MPI_COMPLETION_TAG + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    void JobController::PrintMPICommunicationError(const int& state) {
      char estring[MPI_MAX_ERROR_STRING];

      int eclass, len;
      MPI_Error_class(state, &eclass);
      MPI_Error_string(state, estring, &len);
      LOG(INFO) << "MPI Communication Error: " << estring << " (Error Class :: " << eclass << ")";
    }

    void JobController::CancelPendingRequests() {
      for (RequestIterator iter = _request_handlers.begin(); iter != _request_handlers.end(); iter++) {
	MPI_Request* request = &(iter->second);
	MPI_Status status;
	int flag;
	if (request != NULL && *request != MPI_REQUEST_NULL) {
	  MPI_Cancel(request); 
	  MPI_Wait(request, &status);
	  MPI_Test_cancelled(&status, &flag);
	  if (!flag) {
	    LOG(ERROR) << "Could not cancel pending request to node: " << iter->first;
	  }	  
	}	
      }
      
    }

    void JobController::CheckForCompletion() {
      for (RequestIterator iter = _request_handlers.begin(); iter != _request_handlers.end(); iter++) {
	int flag;
	MPI_Status status;
	MPI_Request* request = &(iter->second);
	if (*request == MPI_REQUEST_NULL) {
	  continue;
	}
	const int state = MPI_Test(request, &flag, &status);
	_request_handlers[iter->first] = *request;

	if (state != MPI_SUCCESS) {
	  const int node = iter->first;
	  LOG(ERROR) << "Communication error with node: " << node;
	  PrintMPICommunicationError(state);
	  continue;
	}

	if (flag == true) {
	  const int node = iter->first;
	  VLOG(1) << "Received a completion response from node: " << node;
	  SendCompletionResponse(node);

	  JobOutput output = JobNode::WaitForJobData(node);
	  if (_completion_handler != NULL) {
	    (*_completion_handler)(output, node);
	  }
	}
      }
    }

    void JobController::StartJobOnNode(const JobDescription& description, const int& node,
				       const map<string, VariableType>& variable_types) {
      // Send the job description over.
      JobNode::SendJobDataToNode(description, node, variable_types);

      // Setup the handler
      if (_completion_handler != NULL) {
	// Check to see if we are already waiting on this node for something.
	RequestIterator iter = _request_handlers.find(node);
	if (iter != _request_handlers.end() && iter->second != MPI_REQUEST_NULL) {
	  LOG(ERROR) << "You cannot start more than one job on a single node";
	  return;
	}

	// Allocate space for the completion status and the request handler.
	if (iter == _request_handlers.end()) {
	  MPI_Request request_handler;
	  _request_handlers[node] = request_handler;
	}

	// Asynchronously receive a completion response from the node.
	MPI_Irecv(&_completion_status, 1, MPI_INT, 
		  node, MPI_COMPLETION_TAG, MPI_COMM_WORLD, &_request_handlers[node]);
      }
    }

    void JobNode::SendStringToNode(const string& message, const int& node) {
      VLOG(3) << "Sending string: " << message << " (receiver node: " << node << ")";
      int length = message.length() + 1;
      scoped_array<char> message_c(new char[length]);
      memcpy(message_c.get(), message.c_str(), sizeof(char) * length);
      MPI_Send(&length, 1, MPI_INT, node, MPI_STRING_MESSAGE_TAG, MPI_COMM_WORLD);
      MPI_Send(message_c.get(), length, MPI_CHAR, node, MPI_STRING_MESSAGE_TAG, MPI_COMM_WORLD);
    }

    string JobNode::WaitForString(const int& node) {
      CheckInitialized();
      VLOG(3) << "Waiting for string from node: " << node;
      int length;
      MPI_Recv(&length, 1, MPI_INT, node, MPI_STRING_MESSAGE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      scoped_array<char> message_c(new char[length]);
      MPI_Recv(message_c.get(), length, MPI_CHAR, node, MPI_STRING_MESSAGE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      VLOG(3) << "Recieved string: " << message_c.get() << " (sending node: " << node << ")";

      return string(message_c.get());
    }

    void JobNode::SendJobDataToNode(const JobData& data, const int& node,
				    const map<string, VariableType>& variable_types) {
      CheckInitialized();
      // Send the command.
      SendStringToNode(data.command, node);

      // Send information about the number of indices and then send
      // over the actual indices.
      {
	int num_indices = data.indices.size();
	scoped_array<int> indices(new int[num_indices]);
	for (int i = 0; i < num_indices; i++) {
	  indices[i] = data.indices[i];
	}
	MPI_Send(&num_indices, 1, MPI_INT, node, 0, MPI_COMM_WORLD);
	MPI_Send(indices.get(), num_indices, MPI_INT, node, 0, MPI_COMM_WORLD);
      }

      // Now we send num_variables worth of string.length information for
      // the variable names (which are the keys in the
      // data.variables map). We also send the byte lengths for
      // each input.
      int num_variables = data.variables.size();
      vector<string> serialized_variables;
      int total_bytes = 0;
      MPI_Send(&num_variables, 1, MPI_INT, node, 0, MPI_COMM_WORLD);
      for (map<string, MatlabMatrix>::const_iterator it = data.variables.begin(); 
	   it != data.variables.end(); 
	   it++) {
	const string input_name = (*it).first;
	SendStringToNode(input_name, node);

	const MatlabMatrix& matrix = (*it).second;
	string serialized = "";
	if ((matrix.GetMatrixType() == slib::util::MATLAB_CELL_ARRAY
	     || matrix.GetMatrixType() == slib::util::MATLAB_STRUCT)
	    && variable_types.find(input_name) != variable_types.end()
	    && variable_types.find(input_name)->second != COMPLETE_VARIABLE) {
	  VLOG(1) << "Found partial input: " << input_name;

	  vector<int> indices = data.indices;
	  sort(indices.begin(), indices.end());

	  const VariableType type = variable_types.find(input_name)->second;
	  if (type == PARTIAL_VARIABLE_ROWS) {
	    MatlabMatrix partial(matrix.GetMatrixType(), matrix.GetDimensions());
	    for (int index = 0; index < (int) indices.size(); index++) {
	      const int row = indices[index];
	      for (int col = 0; col < matrix.GetDimensions().y; col++) {
		partial.Set(row, col, matrix.Get(row, col));
	      }
	    }
	    serialized = partial.Serialize();
	  } else if (type == PARTIAL_VARIABLE_COLS) {
	    MatlabMatrix partial(matrix.GetMatrixType(), matrix.GetDimensions());
	    for (int index = 0; index < (int) indices.size(); index++) {
	      const int col = indices[index];
	      for (int row = 0; row < matrix.GetDimensions().x; row++) {
		partial.Set(row, col, matrix.Get(row, col));
	      }
	    }
	    serialized = partial.Serialize();
	  }
	} else {
	  serialized = matrix.Serialize();
	}
	int byte_length = serialized.length();
	MPI_Send(&byte_length, 1, MPI_INT, node, 0, MPI_COMM_WORLD);
	
	serialized_variables.push_back(serialized);
	total_bytes += byte_length;
      }
	
      // Now comes the big boys. We have to send over the arbitrarily
      // complicated Matlab Matrices thanks to some external
      // dependencies and our avoidance of using the filesystem. These
      // Sends, however, are done asynchronously as they may take a
      // while to complete and we don't want to block on anything else

      // Copy the serialized strings into one big buffer.
      scoped_array<char> serialized_variables_cstr(new char[total_bytes]);
      int byte_offset = 0;
      for (int i = 0; i < num_variables; i++) {
	const int byte_length = serialized_variables[i].length();
	memcpy(serialized_variables_cstr.get() + byte_offset, serialized_variables[i].c_str(), byte_length);
	byte_offset += byte_length;
      }

      scoped_array<MPI_Request> request_handlers(new MPI_Request[num_variables]);
      // Allocate a buffer big enough for all of the variables.
      byte_offset = 0;
      for (int i = 0; i < num_variables; i++) {
	const int byte_length = serialized_variables[i].length();
	MPI_Isend(serialized_variables_cstr.get() + byte_offset, byte_length, MPI_CHAR, 
		  node, i, MPI_COMM_WORLD, &request_handlers[i]);
	byte_offset += byte_length;
      }
      // Wait for them all to finish.
      scoped_array<MPI_Status> request_statuses(new MPI_Status[num_variables]);
      for (int i = 0; i < num_variables; i++) {
	MPI_Wait(&request_handlers[i], &request_statuses[i]);
      }
    }

    JobData JobNode::WaitForJobData(const int& node) {
      CheckInitialized();
      // These routines match the sends above.
      JobData data;

      // Receive the command.
      data.command = WaitForString(node);

      // Receive the number of indices and then the actual indices.
      int num_indices;
      MPI_Recv(&num_indices, 1, MPI_INT, node, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      {
	scoped_array<int> indices(new int[num_indices]);
	MPI_Recv(indices.get(), num_indices, MPI_INT, node, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	for (int i = 0; i < num_indices; i++) {
	  const int index = indices[i];
	  data.indices.push_back(index);
	}
      }

      // Receive the list of input variable names along with their byte lengths.
      int num_variables;
      vector<string> input_names;
      vector<int> input_byte_lengths;
      int total_bytes = 0;
      MPI_Recv(&num_variables, 1, MPI_INT, node, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      for (int i = 0; i < num_variables; i++) {
	input_names.push_back(WaitForString(node));
	int byte_length;
	MPI_Recv(&byte_length, 1, MPI_INT, node, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	VLOG(2) << "Expect Variable: " << input_names[i] << " (length: " << byte_length << ")";
	input_byte_lengths.push_back(byte_length);
	total_bytes += byte_length;
      }
      VLOG(2) << "Expecting a total of " << total_bytes << " bytes worth of variables";

      // Here is where the big data comes. We can handle the set of
      // variables async, but we wait for them all to finish to ensure
      // the JobData is complete.
      scoped_array<MPI_Request> request_handlers(new MPI_Request[num_variables]);
      // Allocate a buffer big enough for all of the variables.
      scoped_array<char> serialized_variables(new char[total_bytes]);
      int byte_offset = 0;
      for (int i = 0; i < num_variables; i++) {
	const int byte_length = input_byte_lengths[i];
	MPI_Irecv(serialized_variables.get() + byte_offset, byte_length, MPI_CHAR, 
		  node, i, MPI_COMM_WORLD, &request_handlers[i]);
	byte_offset += byte_length;
      }
      // Wait for them all to finish.
      scoped_array<MPI_Status> request_statuses(new MPI_Status[num_variables]);
      for (int i = 0; i < num_variables; i++) {
	MPI_Wait(&request_handlers[i], &request_statuses[i]);
      }

      // Now copy the variables into matrices.
      byte_offset = 0;
      for (int i = 0; i < num_variables; i++) {
	const string input_name = input_names[i];
	const int byte_length = input_byte_lengths[i];
	const string serialized_input(serialized_variables.get() + byte_offset, byte_length);
	VLOG(2) << "Read " << serialized_input.length() << " bytes (actual: " << byte_length << ")";

	MatlabMatrix matrix;
	matrix.Deserialize(serialized_input);
	data.variables[input_name] = matrix;

	byte_offset += byte_length;
      }

      return data;
    }

    void JobNode::SendCompletionMessage(const int& node) {
      CheckInitialized();
      int message = 1;
      MPI_Send(&message, 1, MPI_INT, node, MPI_COMPLETION_TAG, MPI_COMM_WORLD);
    }

    bool JobNode::CheckInitialized() {
      if (_initialized) {
	return true;
      } else {
	int flag;
	MPI_Initialized(&flag);
	if (!flag) {
	  LOG(ERROR) << "Attempted to call a JobNode method before calling MPI_Init. "
		     << "You must call MPI_Init before any JobNode methods.";
	  return false;
	}
	_initialized = true;
	return true;
      }
    }

  }  // namespace util
}  // namespace slib

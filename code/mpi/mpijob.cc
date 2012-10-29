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

    MatlabMatrix empty_matrix;
    const MatlabMatrix& JobData::GetInputByName(const string& name) const {
      map<string, MatlabMatrix>::const_iterator iter = variables.find(name);
      if (iter == variables.end()) {
	return empty_matrix;
      } else {
	return iter->second;
      }
    }
    
    JobController::JobController() : _completion_handler(NULL) {}

    void JobController::SetCompletionHandler(CompletionHandler handler) {
      _completion_handler = handler;
    }

    void JobController::SendCompletionResponse(const int& node) {
      int message = 1;
      MPI_Send(&message, 1, MPI_INT, node, MPI_COMPLETION_TAG + 1, MPI_COMM_WORLD);
    }

    void JobNode::WaitForCompletionResponse(const int& node) {
      int message;
      MPI_Recv(&message, 1, MPI_INT, node, MPI_COMPLETION_TAG + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    void JobController::CheckForCompletion() {
      for (RequestIterator iter = _request_handlers.begin(); iter != _request_handlers.end(); iter++) {
	int flag;
	MPI_Status status;
	MPI_Request* request = &(iter->second);
	if (*request == MPI_REQUEST_NULL) {
	  continue;
	}
	MPI_Test(request, &flag, &status);

	_request_handlers[iter->first] = *request;

	if (flag == true) {
	  const int node = iter->first;
	  VLOG(1) << "Received a completion response from node: " << node;
	  SendCompletionResponse(node);

	  JobOutput output = JobNode::WaitForJobData(node);
	  (*_completion_handler)(output, node);
	}
      }
    }

    void JobController::StartJobOnNode(const JobDescription& description, const int& node) {
      // Send the job description over.
      JobNode::SendJobDataToNode(description, node);

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
      scoped_ptr<char> message_c(new char[length]);
      memcpy(message_c.get(), message.c_str(), sizeof(char) * length);
      MPI_Send(&length, 1, MPI_INT, node, MPI_STRING_MESSAGE_TAG, MPI_COMM_WORLD);
      MPI_Send(message_c.get(), length, MPI_CHAR, node, MPI_STRING_MESSAGE_TAG, MPI_COMM_WORLD);
    }

    string JobNode::WaitForString(const int& node) {
      VLOG(3) << "Waiting for string from node: " << node;
      int length;
      MPI_Recv(&length, 1, MPI_INT, node, MPI_STRING_MESSAGE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      scoped_ptr<char> message_c(new char[length]);
      MPI_Recv(message_c.get(), length, MPI_CHAR, node, MPI_STRING_MESSAGE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      VLOG(3) << "Recieved string: " << message_c.get() << " (sending node: " << node << ")";

      return string(message_c.get());
    }

    void JobNode::SendJobDataToNode(const JobData& data, const int& node) {
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

	const MatlabMatrix matrix = (*it).second;
	const string serialized = matrix.Serialize();
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
      scoped_ptr<char> serialized_variables_cstr(new char[total_bytes]);
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

	VLOG(1) << "Expect Variable: " << input_names[i] << " (length: " << byte_length << ")";
	input_byte_lengths.push_back(byte_length);
	total_bytes += byte_length;
      }
      VLOG(1) << "Expecting a total of " << total_bytes << " bytes worth of variables";

      // Here is where the big data comes. We can handle the set of
      // variables async, but we wait for them all to finish to ensure
      // the JobData is complete.
      scoped_array<MPI_Request> request_handlers(new MPI_Request[num_variables]);
      // Allocate a buffer big enough for all of the variables.
      scoped_ptr<char> serialized_variables(new char[total_bytes]);
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
      int message = 1;
      MPI_Send(&message, 1, MPI_INT, node, MPI_COMPLETION_TAG, MPI_COMM_WORLD);
    }

  }  // namespace util
}  // namespace slib

// MPI_Init should be called before any of the methods in this file.

#ifndef __SLIB_UTIL_MPI_H__
#define __SLIB_UTIL_MPI_H__

#include <map>
#include <mpi.h>
#include <string>
#include <vector>

#define MPI_ROOT_NODE 0
#define MPI_COMPLETION_TAG 1025
#define MPI_STRING_MESSAGE_TAG 1026

namespace slib {
  namespace util {
    class MatlabMatrix;
  }
}

namespace slib {
  namespace mpi {

    enum VariableType {
      PARTIAL_VARIABLE_ROWS,  // Rows are partially sent, but all cols are sent
      PARTIAL_VARIABLE_COLS,  // Same as above s/rows/cols
      COMPLETE_VARIABLE,  // A variable that needs all of its data sent
      FEATURE_STRIPPED_ROW_VARIABLE,  // A partial variable that has been stripped of features
      DSWORK_COLUMN  // This variable will be saved directly to disk
		     // on output in the format that dswork
		     // expects. Note that only column vectors can be
		     // saved in this special way.
    };

    struct JobData {
      std::string command;
      std::vector<int> indices;

      std::map<std::string, slib::util::MatlabMatrix> variables;

      const slib::util::MatlabMatrix& GetInputByName(const std::string& name) const; 
      bool HasInput(const std::string& name) const; 

      ~JobData() {
	indices.clear();
	variables.clear();
      }
    };

    struct JobQueue {
#if 0
      std::vector<MPI_Request> requests;
      std::vector<MPI_Success> responses;
#endif
    };

    typedef JobData JobDescription;
    typedef JobData JobOutput;
    typedef void (*CompletionHandler)(const slib::mpi::JobOutput&, const int&);

    typedef std::map<int, MPI_Request>::iterator RequestIterator;

    class JobController {
    public:
      JobController();

      // You should almost always set a completion handler or jobs may
      // never actually complete correctly. In some cases you can omit
      // this, but if you ever want to handle outputs from nodes you
      // must implement and set such a method.
      // 
      // The signature for the CompletionHandler is: 
      // void CompletionHandler(const slib::mpi::JobOutput&, const int&);
      void SetCompletionHandler(CompletionHandler handler);

      // Starts a job on the specified node. Non-blocking. 
      void StartJobOnNode(const JobDescription& description, const int& node,
			  const std::map<std::string, VariableType>& variable_types);
      // Almost always use this method unless you know what you're
      // doing and understand VariableTypes.
      inline void StartJobOnNode(const JobDescription& description, const int& node) {
	StartJobOnNode(description, node, std::map<std::string, VariableType>());
      }

      // You have to run this method periodically to have the
      // CompletionHandler method called appropriately.
      //
      // TODO(sarietta): This shouldn't be necessary, but without
      // spawning a new thread to poll MPI it must remain this way.
      void CheckForCompletion();
      static void PrintMPICommunicationError(const int& state);

      void CancelPendingRequests();

    private:
      CompletionHandler _completion_handler;
      std::map<int, MPI_Request> _request_handlers;
      int _completion_status;
      MPI_Errhandler _error_handler;

      // When a node completes, it should send a completion message
      // via SendCompletionMessage. The master receives this message
      // and then sends a completion response
      // automatically. Typically, on the node, immediately after
      // calling SendCompletionMessage you call the blocking method
      // WaitForCompletionResponse. The method below sends the
      // response to the node and unblocks that call to
      // WaitForCompletion Response.
      //
      // ------------- Time t -----------------------
      // Node: <SendCompletionMessage>
      // Master: <>
      // ------------- Time t + 1 -------------------
      // Node: <WaitForCompletionResponse> (blocking)
      // Master: <ReceiveCompletionMessage>
      // ------------- Time t + 2 -------------------
      // Node: <WaitForCompletionResponse> (blocking)
      // Master: <SendCompletionResponse>
      // ------------- Time t + 3 -------------------
      // Node: Execution Continues
      // Master: <>
      void SendCompletionResponse(const int& node);
    };

    class JobNode {
    public:
      static JobData WaitForJobData(const int& node = MPI_ROOT_NODE);   
      static std::string WaitForString(const int& node = MPI_ROOT_NODE);

      static void SendJobDataToNode(const JobData& data, const int& node,
				    const std::map<std::string, VariableType>& variable_types);
      inline static void SendJobDataToNode(const JobData& data, const int& node) {
	SendJobDataToNode(data, node, std::map<std::string, VariableType>());
      }
      static void SendStringToNode(const std::string& message, const int& node);

      // Alert the master that this node is done with an operation.
      static void SendCompletionMessage(const int& node);
      // Blocking call to wait for the master to acknowledge the
      // previous method's message. Almost always call this directly
      // after previous method.
      static void WaitForCompletionResponse(const int& node);

    private:
      static bool _initialized;
      static bool CheckInitialized();
    };
  }  // namespace util
}  // namespace slib

#endif

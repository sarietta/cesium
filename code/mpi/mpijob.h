#ifndef __SLIB_UTIL_MPI_H__
#define __SLIB_UTIL_MPI_H__

#include <map>
#include <mpi.h>
#include <string>
#include <vector>

#define MPI_ROOT_NODE 0
#define MPI_COMPLETION_TAG 1025

namespace slib {
  namespace util {
    class MatlabMatrix;
  }
}

namespace slib {
  namespace mpi {

    struct JobData {
      std::string command;
      std::vector<int> indices;

      std::map<std::string, slib::util::MatlabMatrix> variables;

      const slib::util::MatlabMatrix& GetInputByName(const std::string& name) const; 
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

      // MUST always call MPI_Init before calling any of these
      // routines. This assumes that the master node is ALWAYS node #
      // 0.
      void SetCompletionHandler(CompletionHandler handler);
      void StartJobOnNode(const JobDescription& description, const int& node);
      void CheckForCompletion();

    private:
      CompletionHandler _completion_handler;
      std::map<int, MPI_Request> _request_handlers;
      int _completion_status;
    };

    class JobNode {
    public:
      // MUST always call MPI_Init before calling any of these
      // routines. This assumes that the master node is ALWAYS node #
      // 0.
      static JobData WaitForJobData(const int& node = MPI_ROOT_NODE);   
      static std::string WaitForString(const int& node = MPI_ROOT_NODE);

      static void SendJobDataToNode(const JobData& data, const int& node);
      static void SendStringToNode(const std::string& message, const int& node);

      static void SendCompletionMessage(const int& node);
    };
  }  // namespace util
}  // namespace slib

#endif

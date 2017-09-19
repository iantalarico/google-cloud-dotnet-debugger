// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef I_EVAL_COORDINATOR_H_
#define I_EVAL_COORDINATOR_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "ccomptr.h"
#include "cor.h"
#include "cordebug.h"
#include "i_portablepdbfile.h"

namespace google_cloud_debugger {

class BreakpointCollection;
class DbgBreakpoint;

// TODO(quoct): Add a switch to turn off function evaluation by default.
// Also, we have to investigate function evaluation for multi-threading case.
//
// An EvalCoordinator object is used by DebuggerCallback object to evaluate
// and print out variables. It does so by creating a StackFrame on a new
// thread and coordinates between the StackFrame and DebuggerCallback.
//
// We need an EvalCoordinator for coordination because if we want to print
// out properties and perform function evaluation, we would have to do it
// from a different thread. This is because for an evaluation to succeed,
// the DebuggerCallback object has to call ICorDebugController->Continue
// method and return control to the debuggee by returning from whatever
// callback it is in.
//
// For example, if the DebuggerCallback is in the Break
// callback method when it uses EvalCoordinator to print out variables,
// then it will have to call appdomain->Continue(FALSE) and exits the method.
// When the evaluation is finished, the EvalComplete or EvalException callback
// of DebuggerCallback class will be invoked and that is when we know that
// the evaluation has finished.
//
// For this reason, we have to do the variable enumeration and value
// inspection on a different thread than the thread that the DebuggerCallback
// is on. Otherwise, the DebuggerCallback thread will be blocked and
// cannot perform evaluation.
class IEvalCoordinator {
 public:
  // Class' destructor.
  virtual ~IEvalCoordinator() = default;

  // This method is used to create an ICorDebugEval object
  // from the active thread.
  virtual HRESULT CreateEval(ICorDebugEval **eval) = 0;

  // StackFrame calls this to get evaluation result.
  // This method will block until an evaluation is complete.
  virtual HRESULT WaitForEval(BOOL *exception_thrown, ICorDebugEval *eval,
                              ICorDebugValue **eval_result) = 0;

  // DebuggerCallback calls this function to signal that an evaluation is
  // finished.
  virtual void SignalFinishedEval(ICorDebugThread *debug_thread) = 0;

  // DebuggerCallback calls this function to signal that an exception has
  // occurred.
  virtual void HandleException() = 0;

  // Prints out the stack frames at DbgBreakpoint breakpoint based on
  // debug_stack_walk.
  virtual HRESULT PrintBreakpoint(
      ICorDebugStackWalk *debug_stack_walk, ICorDebugThread *debug_thread,
      BreakpointCollection *breakpoint_collection, DbgBreakpoint *breakpoint,
      const std::vector<
          std::unique_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
          &pdb_files) = 0;

  // StackFrame calls this to signal that it already processed all the
  // variables and it is just waiting to perform evaluation (if necessary) and
  // print them out.
  virtual void WaitForReadySignal() = 0;

  // StackFrame calls this to signal to the DebuggerCallback that it
  // finished all the evaluation.
  virtual void SignalFinishedPrintingVariable() = 0;

  // Returns the active debug thread.
  virtual HRESULT GetActiveDebugThread(ICorDebugThread **debug_thread) = 0;

  // Returns true if we are waiting for an evaluation result.
  virtual BOOL WaitingForEval() = 0;

  // Sets this to stop property evaluation.
  virtual void SetPropertyEvaluation(BOOL eval) = 0;

  // Returns whether property evaluation should be performed.
  virtual BOOL PropertyEvaluation() = 0;
};

}  //  namespace google_cloud_debugger

#endif  // EVAL_COORDINATOR_H_
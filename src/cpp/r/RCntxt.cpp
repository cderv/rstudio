/*
 * RCntxt.cpp
 *
 * Copyright (C) 2022 by RStudio, PBC
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */


#include <r/RCntxt.hpp>
#include <r/RExec.hpp>
#include <r/RIntCntxt.hpp>
#include <r/RCntxtUtils.hpp>
#include <r/RInterface.hpp>

#include <boost/make_shared.hpp>

#include <shared_core/Error.hpp>

using namespace rstudio::core;

namespace rstudio {
namespace r {
namespace context {

RCntxt::RCntxt()
{ }

// constructs an RCntxt object from a raw RNTCXT* pointer; this is where the 
// we set up the appropriate interface pointer based on the R version
RCntxt::RCntxt(void *rawCntxt)
{
   if (rawCntxt == nullptr)
      return;
   else if (contextVersion() == RVersion40)
      pCntxt_ = boost::make_shared<RIntCntxt<RCNTXT_40> >(
                                   static_cast<RCNTXT_40*>(rawCntxt));
   else if (contextVersion() == RVersion34)
      pCntxt_ = boost::make_shared<RIntCntxt<RCNTXT_34> >(
                                   static_cast<RCNTXT_34*>(rawCntxt));
   else if (contextVersion() == RVersion33)
      pCntxt_ = boost::make_shared<RIntCntxt<RCNTXT_33> >(
                                   static_cast<RCNTXT_33*>(rawCntxt));
   else
      pCntxt_ = boost::make_shared<RIntCntxt<RCNTXT_32> >(
                                   static_cast<RCNTXT_32*>(rawCntxt));
}

Error RCntxt::callSummary(std::string* pCallSummary) const
{
   return invokeFunctionOnCall(".rs.callSummary", pCallSummary);
}

Error RCntxt::functionName(std::string* pFunctionName) const
{
   return invokeFunctionOnCall(".rs.functionNameFromCall", pFunctionName);
}

bool RCntxt::isDebugHidden() const
{
   SEXP hideFlag = r::sexp::getAttrib(callfun(), "hideFromDebugger");
   return TYPEOF(hideFlag) != NILSXP && r::sexp::asLogical(hideFlag);
}

bool RCntxt::isErrorHandler() const
{
   SEXP errFlag = r::sexp::getAttrib(callfun(), "errorHandlerType");
   return TYPEOF(errFlag) == INTSXP;
}

bool RCntxt::hasSourceRefs() const
{
   SEXP refs = callFunSourceRefs();
   return refs && TYPEOF(refs) != NILSXP;
}

SEXP RCntxt::contextSourceRefs() const
{
   // retrieve the source reference tagged on the context
   SEXP ref = srcref();
   
   // if this is a byte-code context, we need to do some
   // extra work to resolve the "real" source reference (if any)
   if (isByteCodeSrcRef(ref))
   {
      r::sexp::Protect protect;
   
      // attempt to resolve context
      Error error = r::exec::RFunction(".rs.resolveContextSourceRefs")
            .addParam(callfun())
            .call(&ref, &protect);
      
      // errors are somewhat expected here, so don't log them and
      // instead just set the source references to NULL
      if (error)
         ref = R_NilValue;
   }
   
   return ref;
}

SEXP RCntxt::callFunSourceRefs() const
{
   return r::sexp::getAttrib(originalFunctionCall(), "srcref");
}

std::string RCntxt::shinyFunctionLabel() const
{
   std::string label;
   SEXP s = r::sexp::getAttrib(originalFunctionCall() , "_rs_shinyDebugLabel");
   if (s != nullptr && TYPEOF(s) != NILSXP)
   {
      r::sexp::extract(s, &label);
   }
   return label;
}

// enabling tracing on a function turns it into an S4 object with an 'original'
// slot that includes the function's original contents. use this instead if
// it's set up. (consider: is it safe to assume that S4 objects here are always
// traced functions, or do we need to compare classes to be safe?)
SEXP RCntxt::originalFunctionCall() const
{
   SEXP callObject = callfun();

   if (Rf_isS4(callObject))
   {
      callObject = r::sexp::getAttrib(callObject, "original");
   }
   return callObject;
}

Error RCntxt::fileName(std::string* pFileName) const
{
   SEXP ref = contextSourceRefs();
   
   if (ref && TYPEOF(ref) != NILSXP)
   {
      r::sexp::Protect protect;
      SEXP fileName;
      Error error = r::exec::RFunction(".rs.sourceFileFromRef")
            .addParam(ref)
            .call(&fileName, &protect);
      if (error)
          return error;

      return r::sexp::extract(fileName, pFileName, true);
   }
   else
   {
      // If no source references, that's OK--just set an empty filename.
      pFileName->clear();
      return Success();
   }
}

// call objects can't be passed as primary values through our R interface
// (early evaluation can be triggered) so we wrap them in an attribute attached
// to a dummy value when we need to pass them through
Error RCntxt::invokeFunctionOnCall(const char* rFunction,
                                   std::string* pResult) const
{
   SEXP result;
   r::sexp::Protect protect;
   SEXP val = r::sexp::create("_rs_callval", &protect);
   r::sexp::setAttrib(val, "_rs_call", call());
   Error error = r::exec::RFunction(rFunction, val)
                            .call(&result, &protect);
   if (!error && r::sexp::length(result) > 0)
   {
      error = r::sexp::extract(result, pResult, true);
   }
   else
   {
      pResult->clear();
   }
   return error;
}

bool RCntxt::operator==(const RCntxt& other) const
{
   // Equivalent if they refer to the same underlying object
   if (other.pCntxt_ == pCntxt_)
      return true;

   // Also equivalent if they refer to the same call at the same stack position and have the same
   // source references
   if (!other.isNull() &&
       other.call() == call() &&
       other.evaldepth() == evaldepth() &&
       other.srcref() == srcref())
      return true;

   return false;
}

bool RCntxt::operator!=(const RCntxt& other) const
{
   return !(*this == other);
}

RCntxt::iterator RCntxt::begin()
{
   return RCntxt::iterator(globalContext());
}

RCntxt::iterator RCntxt::end()
{
   return RCntxt::iterator(RCntxt());
}

bool RCntxt::isNull() const
{
   return pCntxt_ == nullptr;
}

SEXP RCntxt::callfun() const
{
   return pCntxt_ ? pCntxt_->callfun() : R_NilValue;
}

int RCntxt::callflag() const
{
   return pCntxt_ ? pCntxt_->callflag() : 0;
}

SEXP RCntxt::call() const
{
   return pCntxt_ ? pCntxt_->call() : R_NilValue;
}

int RCntxt::evaldepth() const
{
   return pCntxt_ ? pCntxt_->evaldepth() : 0;
}

SEXP RCntxt::srcref() const
{
   return pCntxt_ ? pCntxt_->srcref() : R_NilValue;
}

SEXP RCntxt::cloenv() const
{
   return pCntxt_ ? pCntxt_->cloenv() : R_NilValue;
}

RCntxt RCntxt::nextcontext() const
{
   return pCntxt_ ? pCntxt_->nextcontext() : RCntxt(nullptr);
}

} // namespace context
} // namespace r
} // namespace rstudio

/*
 * SessionActiveSessionsStorage.cpp
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

#include <session/SessionActiveSessionsStorage.hpp>

#include <shared_core/system/User.hpp>

#include <session/SessionOptions.hpp>
#include <session/SessionServerRpc.hpp>

using namespace rstudio::core;
using namespace rstudio::core::r_util;

namespace rstudio {
namespace session {
namespace storage {

Error activeSessionsStorage(std::shared_ptr<IActiveSessionsStorage>* pStorage) 
{
   std::shared_ptr<IActiveSessionsStorage> storage;
   if (options().sessionUseFileStorage())
      storage = std::shared_ptr<IActiveSessionsStorage>(new FileActiveSessionsStorage(options().userScratchPath()));
   else
   {
      system::User user;
      Error error = system::User::getCurrentUser(user);
      if (error)
      {
         LOG_ERROR(error);
         return error;
      }

      pStorage->reset(new RpcActiveSessionsStorage(
            user,
            [](const json::JsonRpcRequest& request, json::JsonRpcResponse* pResponse)
            {
               return server_rpc::invokeServerRpc(request, pResponse);
            }));
   }

   return Success();
}

} // namespace storage
} // namespace session
} // namespace rstudio

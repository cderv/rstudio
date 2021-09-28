/*
 * RActiveSessionStorage.hpp
 *
 * Copyright (C) 2021 by Rstudio, PBC
 *
 * Unless you have received this program directly from Rstudio pursuant
 * to the terms of a commercial license agreement with Rstudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#ifndef CORE_R_UTIL_R_ACTIVE_SESSION_STORAGE
#define CORE_R_UTIL_R_ACTIVE_SESSION_STORAGE

#include <shared_core/FilePath.hpp>

namespace rstudio {
namespace core {
namespace r_util {

   class IActiveSessionStorage 
   {
   public:
      Error virtual readProperty(const std::string& id, const std::string& name, std::string* pValue) = 0;
      Error virtual writeProperty(const std::string& id, const std::string& name, const std::string& value) = 0;

   protected:
      virtual ~IActiveSessionStorage() = default;
      IActiveSessionStorage() = default;
   };

   class LegacySessionStorage : public IActiveSessionStorage
   {
   public:
      explicit LegacySessionStorage(const FilePath& location);

      Error readProperty(const std::string& id, const std::string& name, std::string* pValue) override;   
      Error writeProperty(const std::string& id, const std::string& name, const std::string& value) override;

   private:
      FilePath activeSessionsDir_;
      const std::string propertiesDirName_ = "properites";
      const std::string legacySessionDirPrefix_ = "session-";

      FilePath buildPropertyPath(const std::string& id, const std::string& name);

      static const std::string& getLegacyName(const std::string& name)
      {
         static const std::map<std::string, std::string> legacyNames = 
         {
            { "last_used" , "last-used" },
            { "r_version" , "r-version" },
            { "r_version_label" , "r-version-label" },
            { "r_version_home" , "r-version-home" },
            { "working_directory" , "working-dir" },
            { "launch_parameters" , "launch-parameters" }
         };

         if (legacyNames.find(name) != legacyNames.end())
            return legacyNames.at(name);

         return name;
      }
   };

   class ActiveSessionStorageFactory
   {
   public:
      static std::shared_ptr<IActiveSessionStorage> getActiveSessionStorage();
      static std::shared_ptr<IActiveSessionStorage> getLegacyActiveSessionStorage();
   };
} // namespace r_util
} // namespace core
} // namespace rstudio

#endif // CORE_R_UTIL_ACTIVE_SESSIONS_STORAGE_HPP

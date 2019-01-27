#include "FAPIContext.h"

#include "FAPIModule.h"
#include "FAPIWrapper.h"
#include "FAPIUtil.h"
#include <algorithm>

#include "Expected.h"
#include "Exceptions.h"
#include "FAPILogger.h"

namespace CPPFAPIWrapper {
   using namespace std;

   FAPIContext::FAPIContext()
      : is_connected(false) { TRACE_FNC("")
      d2fctx * ctx_ = nullptr;
      attr.mask_d2fctxa = (ub4)0;
      int status = d2fctxcr_Create(&ctx_, &attr);

      if ( status != D2FS_SUCCESS )
         throw FAPIException(Reason::INTERNAL_ERROR, __FILE__, __LINE__, "", status);

      auto deleter = [](d2fctx * data) { if(data) d2fctxde_Destroy(data); };
      ctx = unique_ptr<d2fctx, function<void(d2fctx *)>>(ctx_, deleter);
   }

   void FAPIContext::loadSourceModules(const FAPIModule * _module, const bool _ignore_missing_libs, const bool _ignore_missing_sub) { TRACE_FNC(to_string(_ignore_missing_libs) + " | " + to_string(_ignore_missing_sub))
      unordered_set<string> to_process(_module->getSourceModules());
      unordered_set<string> processed(_module->getSourceModules());
      processed.insert(_module->getName());

      while( true ) {
         unordered_set<string> to_load;

         for( auto & source_mod : to_process ) {
            string path = modulePathFromName(source_mod);

            if( hasModule(path) )
               continue;

            try {
               loadModule(path, _ignore_missing_libs, _ignore_missing_sub);
               auto parent_module = getModule(path);

               for( auto & source_mod2 : parent_module->getSourceModules() ) {
                  if( processed.find(source_mod2) == processed.end() ) {
                     to_load.insert(source_mod2);
                     processed.insert(source_mod2);

                     FAPILogger::debug("To process: " + source_mod2);
                  }
               }
            } catch( exception & ex ) { FAPILogger::error(ex.what()); }
         }

         if( to_load.empty() )
            break;

         to_process.insert(to_load.begin(), to_load.end());
      }
   }

   void FAPIContext::loadModuleWithSources(const string & _filepath, const bool _ignore_missing_libs, const bool _ignore_missing_sub) { TRACE_FNC(_filepath + " | " + to_string(_ignore_missing_libs) + " | " + to_string(_ignore_missing_sub))
      loadModule(_filepath, _ignore_missing_libs, _ignore_missing_sub);
      auto module = getModule(_filepath);
      loadSourceModules(module, _ignore_missing_libs, _ignore_missing_sub);
      module->checkOverriden();
   }

   void FAPIContext::loadModule(const string & _filepath, const bool _ignore_missing_libs, const bool _ignore_missing_sub) { TRACE_FNC(_filepath + " | " + to_string(_ignore_missing_libs) + " | " + to_string(_ignore_missing_sub))
      if( hasModule(_filepath) )
         throw FAPIException(Reason::OTHER, __FILE__, __LINE__, _filepath);

      if( !fileExists(_filepath) )
         throw FAPIException(Reason::OTHER, __FILE__, __LINE__, _filepath);

      d2ffmd * mod = nullptr;
      int status = d2ffmdld_Load(ctx.get(), &mod, (text *)_filepath.c_str(), FALSE);

      if ( (status != D2FS_SUCCESS && status != D2FS_MISSINGLIBMOD && status != D2FS_MISSINGSUBCLMOD)
      || (!_ignore_missing_libs && status == D2FS_MISSINGLIBMOD)
      || (!_ignore_missing_sub && status == D2FS_MISSINGSUBCLMOD) )
         throw FAPIException(Reason::INTERNAL_ERROR, __FILE__, __LINE__, _filepath, status); // "operation failed", when trying to load, already loaded, module

      auto module = make_unique<FAPIModule>(this, mod, _filepath);
      status = module->traverseObjects();

      if( status != D2FS_SUCCESS )
         throw FAPIException(Reason::INTERNAL_ERROR, __FILE__, __LINE__, "", status);

      modules[toUpper(_filepath)] = move(module);
   }

   FAPIModule * FAPIContext::getModule(const string & _filepath) { TRACE_FNC(_filepath)
      return Expected<FAPIModule>(modules[toUpper(_filepath)].get()).get();
   }

   bool FAPIContext::hasModule(const string & _filepath) { TRACE_FNC(_filepath)
      return modules.find(toUpper(_filepath)) != modules.end();
   }

   void FAPIContext::createModule(const string & _filepath) { TRACE_FNC(_filepath)
      d2ffmd * mod = nullptr;
      string name = moduleNameFromPath(_filepath);
      int status = d2ffmdcr_Create(ctx.get(), &mod, stringToText(name));

      FAPILogger::debug(name);

      if ( status != D2FS_SUCCESS )
         throw FAPIException(Reason::INTERNAL_ERROR, __FILE__, __LINE__, name, status);

      modules[toUpper(_filepath)] = make_unique<FAPIModule>(this, mod, _filepath);
   }

   void FAPIContext::removeModule(const string & _filepath) { TRACE_FNC(_filepath)
      modules.erase(toUpper(_filepath));
   }

   bool FAPIContext::connectContextToDB(const string & _connstring) { TRACE_FNC(_connstring)
      if( !is_connected ) {
         int status = d2fctxcn_Connect(ctx.get(), stringToText(_connstring), nullptr);

         if( status != D2FS_SUCCESS )
            throw FAPIException(Reason::INTERNAL_ERROR, __FILE__, __LINE__, _connstring, status);

         is_connected = true;
         connstring = _connstring;
      }

      return is_connected;
   }

   bool FAPIContext::disconnectContextFromDB() { TRACE_FNC("")
      if( is_connected ) {
         int status = d2fctxdc_Disconnect(ctx.get());

         if( status != D2FS_SUCCESS )
            throw FAPIException(Reason::INTERNAL_ERROR, __FILE__, __LINE__, "", status);

         is_connected = false;
         connstring = "";
      }

      return is_connected;
   }
}
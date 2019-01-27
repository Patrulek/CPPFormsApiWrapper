#include "FormsObject.h"

#include "Property.h"
#include "FAPIWrapper.h"
#include "FAPIModule.h"
#include "FAPIContext.h"
#include "FAPIUtil.h"

#include "D2FPR.H"

#include "Exceptions.h"
#include "FAPILogger.h"

#include <algorithm>

namespace CPPFAPIWrapper {
   using namespace std;

   FormsObject::FormsObject(FAPIModule * _module, int _type_id, d2fob * _forms_obj, int _level)
      : module(_module), parent(nullptr), type_id(_type_id), forms_obj(_forms_obj), level(_level) { TRACE_FNC("")
   }

   bool FormsObject::isSubclassed() const { TRACE_FNC("")
      auto ctx = getContext()->getContext();
      return d2fobis_IsSubclassed(ctx, forms_obj) == D2FS_YES;
   }

   vector<FormsObject *> FormsObject::findSources() { TRACE_FNC("")// TODO when testing is done
      auto ctx = getContext()->getContext();
      vector<FormsObject *> sources = { this };

      while ( true ) {  // TODO
         auto f_obj = sources.back()->forms_obj;
         auto & source_props = sources.back()->properties;

         if( d2fobis_IsSubclassed(ctx, f_obj) != D2FS_YES )
            break;

         string mod_name = source_props.find(D2FP_PAR_FLNAM) != source_props.end() ? source_props[D2FP_PAR_FLNAM]->getValue() : "";
         string mod_path = source_props.find(D2FP_PAR_FLPATH) != source_props.end() ? source_props[D2FP_PAR_FLPATH]->getValue() : "";

         string mod = source_props.find(D2FP_PAR_MODULE) != source_props.end() ? source_props[D2FP_PAR_MODULE]->getValue() : "";
         string name = source_props.find(D2FP_PAR_NAM) != source_props.end() ? source_props[D2FP_PAR_NAM]->getValue() : "";
         int typ = stoi(source_props.find(D2FP_PAR_TYP) != source_props.end() ? source_props[D2FP_PAR_TYP]->getValue() : "0");

         if( mod_name.empty() && mod_path.empty() /* local reference */ ) {
            if( name != "" && typ != D2FFO_ANY ) {
               auto new_source = module->getObject(typ, name);

               if( new_source )
                  sources.emplace_back(new_source);
            } else
               break; // TODO ??
         } else /* other module */ {
            auto fapi_ctx = getContext();
            string path = modulePathFromName(mod_name);

            if( !fapi_ctx->hasModule(path) )
               fapi_ctx->loadModule(path);

            auto module_ = fapi_ctx->getModule(path);
            string name2 = source_props.find(D2FP_PAR_SL2OBJ_NAM) != source_props.end() ? source_props[D2FP_PAR_SL2OBJ_NAM]->getValue() : "";
            int typ2 = stoi(source_props.find(D2FP_PAR_SL2OBJ_TYP) != source_props.end() ? source_props[D2FP_PAR_SL2OBJ_TYP]->getValue() : "0");
            string name1 = source_props.find(D2FP_PAR_SL1OBJ_NAM) != source_props.end() ? source_props[D2FP_PAR_SL1OBJ_NAM]->getValue() : "";
            int typ1 = stoi(source_props.find(D2FP_PAR_SL1OBJ_TYP) != source_props.end() ? source_props[D2FP_PAR_SL1OBJ_TYP]->getValue() : "0");

            if( name != "" && typ != D2FFO_ANY ) {
               auto new_source = module_->getRootObject(typ, name);

               if( name1 != "" && typ1 != D2FFO_ANY ) {
                  new_source = new_source->getObject(typ1, name1).get();

                  if( name2 != "" && typ2 != D2FFO_ANY )
                     new_source = new_source->getObject(typ2, name2).get();
               }

               if( new_source )
                  sources.emplace_back(new_source);
            }
         }
      }

      sources.erase(sources.begin());
      return sources;
   }

   bool FormsObject::hasChild(FormsObject * _fo) { TRACE_FNC("")
      auto & children_ = children[_fo->getId()];
      return find_if(children_.begin(), children_.end(), [_fo](const auto & child_) { return child_.get() == _fo; }) != children_.end();
   }

   void FormsObject::addChild(FormsObject * _fo) { TRACE_FNC("")
      if( hasChild(_fo) )
         return;

      _fo->parent = this;
      auto child = unique_ptr<FormsObject>(_fo);
      children[_fo->getId()].emplace_back(move(child));
   }

   void FormsObject::removeChild(FormsObject * _fo) { TRACE_FNC("")
      auto & children_ = children[_fo->getId()];
      auto child = find_if(children_.begin(), children_.end(), [_fo](const auto & child_) { return child_.get() == _fo; });

      if( child != children_.end() )
         children_.erase(child);
   }

   void FormsObject::changeParent(FormsObject * _parent) { TRACE_FNC("")
      throw FAPIException(Reason::OTHER, __FILE__, __LINE__, "Not implemented yet!");
   }

   bool FormsObject::hasObject(const int _type_id, const string & _name) { TRACE_FNC(to_string(_type_id) + " | " + _name)
      return getObject(_type_id, _name).isValid();
   }

   Expected<FormsObject> FormsObject::getObject(const int _type_id, const string & _name) { TRACE_FNC(to_string(_type_id) + " | " + _name)
      auto & objects = children[_type_id];
      auto child = find_if(objects.begin(), objects.end(), [_name](const auto & child_) { return toUpper(child_->getName()) == toUpper(_name); });

      return Expected<FormsObject>(child != objects.end() ? (*child).get() : nullptr);
   }

   vector<FormsObject *> FormsObject::getObjects(const int _type_id) { TRACE_FNC(to_string(_type_id))
      vector<FormsObject *> objects;
      auto & children_ = children[_type_id];

      for( auto & child : children_ )
         objects.emplace_back(child.get());

      return objects;
   }

   void FormsObject::markProperty(Property * _property) { TRACE_FNC("")
      if( find(marked_properties.begin(), marked_properties.end(), _property) == marked_properties.end() ) {
         marked_properties.emplace_back(_property);
         module->markObject(this);
      }
   }

   void FormsObject::unmarkProperty(Property * _property) { TRACE_FNC("")
      auto idx = find(marked_properties.begin(), marked_properties.end(), _property);

      if( idx != marked_properties.end() ) {
         marked_properties.erase(idx);

         if( marked_properties.empty() )
            module->unmarkObject(this);
      }
   }

   void FormsObject::setProperty(Property * _property) { TRACE_FNC("")
      if( !_property->isDirty() )
         return;

      int status = D2FS_SUCCESS;

      if( _property->getType() == D2FP_TYP_TEXT )
         status = d2fobst_SetTextProp(module->getContext()->getContext(), forms_obj, _property->getId(), stringToText(_property->getValue()));
      else if( _property->getType() == D2FP_TYP_NUMBER )
         status = d2fobsn_SetNumProp(module->getContext()->getContext(), forms_obj, _property->getId(), stoi(_property->getValue()));
      else if( _property->getType() == D2FP_TYP_BOOLEAN )
         status = d2fobsb_SetBoolProp(module->getContext()->getContext(), forms_obj, _property->getId(), stoi(_property->getValue()));

      if( status != D2FS_SUCCESS )
         throw FAPIException(Reason::INTERNAL_ERROR, __FILE__, __LINE__, prop_names[_property->getId()] + " - " + _property->getValue(), status);
      else
         _property->accept();
   }

   void FormsObject::inheritAllProp() { TRACE_FNC("")
      for( auto & entry : properties )
         entry.second->inherit();
   }

   void FormsObject::inheritProps(const std::vector<int> & _prop_nums) { TRACE_FNC("")
      for( auto prop_num : _prop_nums )
         inheritProp(prop_num);
   }

   void FormsObject::inheritProp(Property * _property) { TRACE_FNC("")
      _property->inherit();
   }

   void FormsObject::inheritProp(const int _prop_num) { TRACE_FNC(to_string(_prop_num))
      if( properties.find(_prop_num) != properties.end() )
         properties[_prop_num]->inherit();
   }

   string FormsObject::getName()  { TRACE_FNC("")
      return properties[D2FP_NAME]->getValue();
   }

   FAPIContext * FormsObject::getContext() const { TRACE_FNC("")
      return module->getContext();
   }
}
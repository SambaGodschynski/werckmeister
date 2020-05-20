#include "AddInstrumentConfig.h"
#include <compiler/context/AContext.h>
#include <compiler/metaCommands.h>
#include <fm/tools.h>
#include <compiler/error.hpp>
#include <fm/werckmeister.hpp>
#include "ICanSpecifyInstrument.h"

namespace sheet {
    namespace compiler {
        const std::vector<fm::String> AddInstrumentConfig::SupportedConfigCommands = {
            SHEET_META__SET_INSTRUMENT_CONFIG_VOLUME,
            SHEET_META__SET_INSTRUMENT_CONFIG_PAN,
            SHEET_META__SET_VOICING_STRATEGY,
            SHEET_META__SET_MOD
        }; 	

        void AddInstrumentConfig::execute(AContext* context)
        {
            if (this->_configCommands.empty()) {
                return;
            }
            auto uname = parameters[argumentNames.InstrumentConf.ForInstrument].value<fm::String>();
			auto instrumentDef = context->getInstrumentDef(uname);
			if (instrumentDef == nullptr) {
				FM_THROW(Exception, "instrument not found: " + uname);
			}            
            for (auto configCommand : this->_configCommands) {
                const auto &name = configCommand.first;
                auto commandObject = configCommand.second;
                auto canSpecifyInstrument = std::dynamic_pointer_cast<ICanSpecifyInstrument>(commandObject);
                if (canSpecifyInstrument) {
                    canSpecifyInstrument->setInstrument(instrumentDef);
                }
                try {
                    commandObject->execute(context);
                } catch (const std::exception &ex) {
                    fm::StringStream ss;
                    ss << "failed to process: " << name << std::endl;
                    ss << ex.what();
                    FM_THROW(Exception, ss.str()); 
                }
            }
           
        }

        void AddInstrumentConfig::setArguments(const Arguments &args) 
        {
            auto &wm = fm::getWerckmeister();
            Base::setArguments(args);
            auto uname = parameters[argumentNames.InstrumentConf.ForInstrument].value<fm::String>();
            if (args.size() < 3) {
				FM_THROW(Exception, "not enough values for " + fm::String(SHEET_META__SET_INSTRUMENT_CONFIG) +  ": " + uname);
			}
            auto argsMappedByKeyword = fm::mapArgumentsByKeywords(args, SupportedConfigCommands);
            for (auto configCommandName : SupportedConfigCommands) {
                auto argsRange = argsMappedByKeyword.equal_range(configCommandName);
                if (argsRange.first == argsRange.second) {
                    continue;
                }
                auto it = argsRange.first;
				for (; it != argsRange.second; ++it) {
					const auto &args = it->second;
                    auto commandObject = wm.createOrDefault<ACommand>(configCommandName);
                    if (!commandObject) {
                        continue;
                    }
                    commandObject->setArguments(args);
                    _configCommands.push_back(std::make_pair(configCommandName, commandObject));
				}
            }
        }
    }
}
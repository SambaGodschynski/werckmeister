#ifndef ICOMPILER_H
#define ICOMPILER_H

#include <fm/common.hpp>
#include "forward.hpp"
#include <memory>

namespace sheet {
	struct Event;
	namespace compiler {
		class ICompiler {
		public:
			virtual void compile(DocumentPtr document) = 0;
		};
		typedef std::shared_ptr<ICompiler> ICompilerPtr;
	}
	
}

#endif
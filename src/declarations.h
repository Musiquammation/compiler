#ifndef COMPILER_DECLARATIONS_H_
#define COMPILER_DECLARATIONS_H_

#include "tools/tools.h"



structdef(Scope);
typedef union ScopeBuffer ScopeBuffer;
structdef(ScopeFile);
structdef(ScopeFolder);

structdef(Module);

structdef(Variable);
structdef(Class);
structdef(Function);

structdef(FunctionAssembly);
structdef(Trace);
structdef(TraceStackHandler);
    
structdef(ScopeClass);
structdef(ScopeFunction);

structdef(ScopeSearchArgs);

structdef(Expression);

structdef(LabelPool);
structdef(CommonLabels);

structdef(Parser);
structdef(Annotation);

structdef(Prototype);
structdef(ProtoSetting);
structdef(Type);
structdef(TypeFollower);



#endif

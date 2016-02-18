#include "typematcher.h"
#include "codegencontext.h"

namespace ccoscope{

const OperatorEntry* TypeMatcher::MatchOperator(std::string name, Type t1, Type t2){
    // TODO: implicit conversions and stuff

    // Currently the matcher does a first fit search, and it ignores possible implicit conversions.

    auto fitit = ctx.BinOpCreator.find(name);

    if(fitit == ctx.BinOpCreator.end()) {
        ctx.AddError("Operator's '" + name + "' codegen is not implemented!");
        return nullptr;
    }
    else{
        for(const OperatorEntry& oe : fitit->second){
            // First-fit
            if(oe.t1 == t1 && oe.t2 == t2)
                return &oe;
        }
        // TODO: Printout type names
        ctx.AddError("No match found for operator '" + name + "' and these types.");
        return nullptr;
    }
}

}

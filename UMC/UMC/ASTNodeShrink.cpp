#include "ASTNodes.h"

void CASTfnShrink::FoldConstants()
{
    CASTfnExpand::FoldConstantsExpandShrink(this);
}
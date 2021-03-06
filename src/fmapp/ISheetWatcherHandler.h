#ifndef FMAPP_ISHEETWATCHER_HANDLER_HPP
#define FMAPP_ISHEETWATCHER_HANDLER_HPP


#include "DiContainerWrapper.h"
#include <memory>

namespace fmapp {
    /**
     * watches a sheetfile and notify if any changes where done
     */
    class ISheetWatcherHandler {
    public:
        virtual void onSheetChanged() = 0;
    };
    typedef DiContainerWrapper<ISheetWatcherHandler*> SheetWatcherHandlers;
    typedef std::shared_ptr<SheetWatcherHandlers> SheetWatcherHandlersPtr;
}
#endif
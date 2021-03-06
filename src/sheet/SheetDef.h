#ifndef SHEET_SHEETDEF_H
#define SHEET_SHEETDEF_H

#include <fm/common.hpp>
#include "objects/Track.h"
#include "objects/Event.h"
#include <vector>
#include "DocumentUsing.h"
#include "objects/ASheetObject.h"
#include "objects/DocumentConfig.h"

namespace sheet {

	struct SheetDef {
		typedef std::vector<Track> Tracks;
		typedef std::vector<DocumentConfig> DocumentConfigs;
		DocumentUsing documentUsing;
		DocumentConfigs documentConfigs;
		Tracks tracks;
	};

}

#endif
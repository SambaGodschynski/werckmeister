#ifndef SHEET_TRACK_H
#define SHEET_TRACK_H

#include <fm/common.hpp>
#include <vector>
#include "Event.h"
#include "ASheetObject.hpp"
#include "Argument.h"

namespace sheet {

	struct Voice : public ASheetObject {
		typedef std::vector<Event> Events;
		Events events;
	};

	struct TrackConfig : public ASheetObjectWithSourceInfo {
		typedef std::vector<sheet::Argument> Args;
		fm::String name;
		Args args;
	};

	struct Track : ASheetObjectWithSourceInfo {
		typedef ASheetObjectWithSourceInfo Base;
		typedef std::vector<Voice> Voices;
		typedef std::vector<TrackConfig> TrackConfigs;
		TrackConfigs trackConfigs;
		Voices voices;
	};

}

#endif
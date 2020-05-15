#include <fm/werckmeister.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <exception>
#include "compiler/context/MidiContext.h"
#include <fstream>
#include <fm/common.hpp>
#include "sheet/Document.h"
#include "sheet.h"
#include "fmapp/midiplayer.h"
#include <algorithm>
#include <thread>
#include <chrono>
#include "fmapp/os.hpp"
#include <thread>
#include <fm/config.hpp>
#include <ctime>
#include "fmapp/boostTimer.h"
#include <boost/format.hpp>
#include <map>
#include <fm/config/configServer.h>
#include <fmapp/udpSender.hpp>
#include "fmapp/json.hpp"
#include "fmapp/MidiAndTimeline.hpp"
#include <set>
#include <fm/tools.h>
#include <time.h>


#define ARG_HELP "help"
#define ARG_INPUT "input"
#define ARG_LIST "list"
#define ARG_MIDI_OUTPUT "device"
#define ARG_LOOP "loop"
#define ARG_BEGIN "begin"
#define ARG_END "end"
#define ARG_WATCH "watch"
#define ARG_UDP "funkfeuer"
#define ARG_NOSTDOUT "notime"
#define ARG_INFO "info"
#define ARG_PRINT_EVENTINFOS_JSON "print-events"
#define ARG_WIN32_SIGINT_WORKAROUND "win32-sigint-workaround"
#define ARG_VERSION "version"
#ifdef WIN32
#include "fmapp/os_win_ipc_kill_handler.hpp"
#define SIGINT_WORKAROUND
#endif

#define UPDATE_THREAD_SLEEPTIME 70

typedef int MidiOutputId;
typedef std::unordered_map<fm::String, time_t> Timestamps;
typedef std::vector<fmapp::EventInfo> EventInfos;

namespace {
	std::unique_ptr<funk::UdpSender> funkfeuer;
	fmapp::JsonWriter jsonWriter;
	fmapp::EventTimeline timeline;
	fmapp::EventTimeline::const_iterator lastTimelineEvent = timeline.end();
	unsigned long lastUpdateTimestamp = 0;
}

struct Settings {
	typedef boost::program_options::variables_map Variables;
	typedef boost::program_options::options_description OptionsDescription;
	OptionsDescription optionsDescription;
	Variables variables;

	Settings(int argc, const char** argv) 
		: optionsDescription("Allowed options")
	{
		namespace po = boost::program_options;
		optionsDescription.add_options()
			(ARG_HELP, "produce help message")
			(ARG_INPUT, po::value<std::string>(), "input file")
			(ARG_LIST, "list midi devices")
			(ARG_LOOP, "activates playback loop")
			(ARG_BEGIN, po::value<double>(), "start postition in quarter notes. E.g.: 1.2")
			(ARG_END, po::value<double>(), "estop postition in quarter notes. E.g.: 1.2")
			(ARG_MIDI_OUTPUT, po::value<MidiOutputId>(), "select midi device (default = 0)")
			(ARG_WATCH, "checks the input file for changes and recompiles if any")
			(ARG_UDP, po::value<std::string>(), "activates an udp sender, which sends sheet info periodically to to given host")
			(ARG_NOSTDOUT, "disable printing time on stdout")
			(ARG_INFO, "prints sheet info as json")
			(ARG_PRINT_EVENTINFOS_JSON, "prints the sheet events as json")
			(ARG_VERSION, "prints the werckmeister version")
#ifdef SIGINT_WORKAROUND
			(ARG_WIN32_SIGINT_WORKAROUND, "uses a ipc workaround for the lack of a proper SIGINT signal handling in windows. \
(an ipc handler will be created before the player starts. If the handler will receive a ipc message, called by a separate program, the player will be stopped.)")

#endif
			;
		po::positional_options_description p;
		p.add(ARG_INPUT, -1);
		po::store(po::command_line_parser(argc, argv).
			options(optionsDescription).positional(p).run(), variables);
		po::notify(variables);
	}


	bool help() const {
		return !!variables.count(ARG_HELP);
	}

	bool listDevices() const {
		return !!variables.count(ARG_LIST);
	}

	bool input() const {
		return !!variables.count(ARG_INPUT);
	}

	auto getInput() const {
		return variables[ARG_INPUT].as<std::string>();
	}

	bool device() const {
		return !!variables.count(ARG_MIDI_OUTPUT);
	}

	auto deviceId() const {
		return variables[ARG_MIDI_OUTPUT].as<MidiOutputId>();
	}

	bool loop() const {
		return !!variables.count(ARG_LOOP);
	}

	bool begin() const {
		return !!variables.count(ARG_BEGIN);
	}

	auto getBegin() const {
		return variables[ARG_BEGIN].as<double>();
	}

	bool end() const {
		return !!variables.count(ARG_END);
	}

	auto getEnd() const {
		return variables[ARG_END].as<double>();
	}

	bool watch() const {
		return !!variables.count(ARG_WATCH);
	}

	bool udp() const {
		return !!variables.count(ARG_UDP);
	}

	auto getUdpHostname() const {
		return variables[ARG_UDP].as<std::string>();
	}

	bool nostdout() const {
		return !!variables.count(ARG_NOSTDOUT);
	}

	bool documentInfoJSON() const {
		return !!variables.count(ARG_INFO);
	}

	bool eventInfosJSON() const {
		return !!variables.count(ARG_PRINT_EVENTINFOS_JSON);
	}

	bool sigintWorkaround() const {
		return !!variables.count(ARG_WIN32_SIGINT_WORKAROUND);
	}

	bool version() const {
		return !!variables.count(ARG_VERSION);
	}

};

void updateLastChangedTimestamp() {
	lastUpdateTimestamp = (unsigned long)time(NULL);
}

void printWarnings(const sheet::DocumentPtr document, sheet::Warnings &warnings)
{
	if (warnings.empty()) {
		return;
	}
	for (const auto &warning : warnings) {
		std::cout << warning.toString(document) << std::endl;
	}
}

int listDevices() {
	auto &player = fmapp::getMidiplayer();
	auto outputs = player.getOutputs();
	for (const auto &output : outputs) {
		std::cout << output.id << ": " << output.name << std::endl;
	}
	return 0;
}

fmapp::Midiplayer::Output findOutput(MidiOutputId id) 
{
	auto &player = fmapp::getMidiplayer();
	auto outputs = player.getOutputs();
	auto it = std::find_if(outputs.begin(), outputs.end(), [id](const auto &x) { return x.portid == id; });
	if (it == outputs.end()) {
		throw std::runtime_error("device not found: " + std::to_string(id));
	}
	return *it;
}

time_t getTimestamp(const fm::String &input) {
	auto path = boost::filesystem::path(input);
	return boost::filesystem::last_write_time(path);
}

bool hasChanges(sheet::DocumentPtr document, Timestamps &timestamps)
{
	auto changed = [document, &timestamps](const fm::String &path) {
		time_t new_timestamp = getTimestamp(path);
		auto it = timestamps.find(path);
		if (it == timestamps.end()) {
			timestamps.emplace(std::make_pair(path, new_timestamp));
			return true;
		}
		if (it->second != getTimestamp(path)) {
			timestamps[path] = new_timestamp;
			return true;
		}
		return false;
	};
	bool result = changed(document->path);
	// check all files, even if a file has changed already
	for(const auto &p : document->sheetDef.documentUsing.usings) {
		auto fullPath = fm::getWerckmeister().resolvePath(p, document);
		result |= changed(fullPath);
	}
	return result;
}

void printElapsedTime(fm::Ticks elapsed) 
{
	using boost::format;
	using boost::str;
	using boost::io::group;

	std::string strOut = "[" + str(format("%.3f") % (elapsed / (double)fm::PPQ)) + "]";
	static std::string lastOutput;
	for (std::size_t i=0; i<lastOutput.size(); ++i) {
		std::cout << "\b";
	}
	std::cout 
		<< strOut 
		<< std::flush;
	lastOutput = strOut;
}

void updatePlayer(fmapp::Midiplayer &player, const std::string &inputfile)
{
	
	auto pos = player.elapsed();
	player.stop();
	try {
		timeline.clear();
		lastTimelineEvent = timeline.end();
		sheet::Warnings warnings;
		auto result = sheet::processFile(inputfile, warnings);
		player.midi(result.first);
		printWarnings(result.second, warnings);
	}
	catch (const fm::Exception &ex)
	{
		sheet::onCompilerError(ex);
		throw;
	}
	catch (const std::exception &ex)
	{
		sheet::onCompilerError(ex);
		throw;
	}
	catch (...)
	{
		sheet::onCompilerError();
		throw;
	}
	player.play(pos);

}


void sendFunkfeuerIfNeccessary(sheet::DocumentPtr document, fm::Ticks elapsed)
{
	if (!funkfeuer) {
		return;
	}
	auto elapsedQuarter = elapsed / (double)fm::PPQ;
	auto ev = timeline.find(elapsed);
	if (ev == lastTimelineEvent || ev == timeline.end()) {
		std::string bff = jsonWriter.funkfeuerToJSON(elapsedQuarter, lastUpdateTimestamp);
		funkfeuer->send(bff.data(), bff.size());
		return;
	}
	lastTimelineEvent = ev;
	EventInfos eventInfos;
	eventInfos.reserve(ev->second.size());
	for (const auto &x : ev->second) {
		eventInfos.push_back(x);
	}
	std::string bff = jsonWriter.funkfeuerToJSON(elapsedQuarter, lastUpdateTimestamp, eventInfos);
	funkfeuer->send(bff.data(), bff.size());
}

std::string eventInfosAsJson(sheet::DocumentPtr document) 
{
	std::stringstream ss;
	ss << "[" << std::endl;
	for (const auto &timelineEntry : timeline) {
		EventInfos eventInfos;
		fm::Ticks eventsBeginTime = timelineEntry.first.lower() / (double)fm::PPQ;
		eventInfos.reserve(timelineEntry.second.size());
		for (const auto &x : timelineEntry.second) {
			eventInfos.push_back(x);
		}
		ss << jsonWriter.funkfeuerToJSON(eventsBeginTime, lastUpdateTimestamp, eventInfos) << ", ";
	}
	ss << "]";
	return ss.str();
}

std::string getDocumentsInfoJSON(sheet::DocumentPtr document, fm::Ticks duration, const sheet::Warnings &warnings)
{
	return jsonWriter.documentInfosToJSON(document, duration / fm::PPQ, warnings);
}
void play(sheet::DocumentPtr document, fm::midi::MidiPtr midi, MidiOutputId midiOutput, fm::Ticks begin, fm::Ticks end, const Settings &settings) 
{
	auto &player = fmapp::getMidiplayer();
	player.updateOutputMapping(fm::getConfigServer().getDevices());
	auto output = findOutput(midiOutput);
	player.setOutput(output);
	player.midi(midi);
	player.play(begin);
	bool playing = true;
	bool watch = settings.watch();
	bool stdout_ = !settings.nostdout();
	Timestamps timestamps;
	hasChanges(document, timestamps);	// init timestamps
	updateLastChangedTimestamp();
	auto inputfile = settings.getInput();

	fmapp::os::setSigtermHandler([&playing, &player]{
		playing = false;
		player.panic();
	});

#ifdef SIGINT_WORKAROUND
	std::unique_ptr<fmapp::os::InterProcessMessageQueue> ipcMessageQueue = nullptr;
	if (settings.sigintWorkaround()) {
		ipcMessageQueue = std::make_unique<fmapp::os::InterProcessMessageQueue>();
	}
#endif

#ifdef SHEET_USE_BOOST_TIMER
	std::thread boost_asio_([] {
		fmapp::BoostTimer::io_run();
	});
#endif

	while (playing) {
		auto elapsed = player.elapsed();
		if (stdout_) {
			printElapsedTime(elapsed);
		}
		sendFunkfeuerIfNeccessary(document, elapsed);
		std::this_thread::sleep_for( std::chrono::milliseconds(UPDATE_THREAD_SLEEPTIME) );
#ifdef SIGINT_WORKAROUND
		if (ipcMessageQueue && ipcMessageQueue->sigintReceived()) {
			playing = false;
			player.panic();
		}
#endif
		if (watch) {
			if (hasChanges(document, timestamps)) {
				try {
					updatePlayer(player, inputfile);
					updateLastChangedTimestamp();
				} catch(...) {
					player.panic();
					break;
				}
			}
			
		}
		if (elapsed > end) {
			if (!settings.loop()) {
				break;
			}
			player.play(begin);
		}
	}
	std::cout << std::endl;
	player.stop();
	player.panic();

#ifdef SHEET_USE_BOOST_TIMER
	fmapp::BoostTimer::io_stop();
	boost_asio_.join();
#endif

	player.Backend:: tearDown();
}

void startSender() {

}

int main(int argc, const char** argv)
{
	bool showDocumentInfo = false;
	try {
		Settings settings(argc, argv);
		showDocumentInfo = settings.documentInfoJSON();
		bool needsTimeline = settings.udp() || settings.eventInfosJSON();
		if (settings.help()) 
		{
			std::cout << settings.optionsDescription << "\n";
			return 0;
		}
		if (settings.version()) {
			std:: cout << SHEET_VERSION << std::endl;
			return 0;
		}
		if (settings.listDevices()) 
		{
			return listDevices();
		}

		if (!settings.input()) 
		{
			throw std::runtime_error("missing input file");
		}

		int midi_out = 0;
		if (settings.device()) 
		{
			midi_out = settings.deviceId();
		}
		if (settings.udp()) 
		{
			funkfeuer = std::make_unique<funk::UdpSender>(settings.getUdpHostname());
			funkfeuer->start();
		}
		if (needsTimeline) {
			auto &wm = fm::getWerckmeister();
			wm.createContextHandler([](){
				auto context = std::make_shared<fmapp::MidiAndTimelineContext>();
				context->intervalContainer(&timeline);
				return context;
			});
		}
		
		std::string infile = settings.getInput();
		auto doc = sheet::createDocument(infile);
		auto showWarnings = !showDocumentInfo;
		sheet::Warnings warnings;
		auto midi = sheet::processFile(doc, warnings);
		if (showWarnings) {
			printWarnings(doc, warnings);
		}
		
		fm::Ticks begin = 0;

		auto end = midi->duration();
		if (settings.begin()) 
		{
			begin = fm::Ticks((double)fm::PPQ * settings.getBegin());
		}

		if (settings.end()) 
		{
			end = fm::Ticks((double)fm::PPQ * settings.getEnd());
		}

		if (begin >= end) 
		{
			throw std::runtime_error("invalid begin/end range");
		}
		if (showDocumentInfo) 
		{
			std::cout << getDocumentsInfoJSON(doc, end, warnings) << std::endl;
		}
		else if(settings.eventInfosJSON())
		{
			std::cout << eventInfosAsJson(doc) << std::endl;
		}
		else 
		{
			play(doc, midi, midi_out, begin, end, settings);
		}
		if (funkfeuer) {
			funkfeuer->stop();
			funkfeuer.reset();
		}

		return 0;
	}
	catch (const fm::Exception &ex)
	{
		if (showDocumentInfo)  {
			std::cout << jsonWriter.exceptionToJSON(ex) << std::endl;
		} else {
			sheet::onCompilerError(ex);
		}
	}
	catch (const std::exception &ex)
	{
		if (showDocumentInfo)  {
			std::cout << jsonWriter.exceptionToJSON(ex) << std::endl;
		} else {
			sheet::onCompilerError(ex);
		}
	}
	catch (...)
	{
		if (showDocumentInfo)  {
			std:: cout << jsonWriter.exceptionToJSON(std::runtime_error("unknown error")) << std::endl;
		} else {
			sheet::onCompilerError();
		}
	}
	return -1;
}
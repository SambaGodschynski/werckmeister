#include "json.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <sheet/Document.h>
#include <fmapp/TimelineVisitor.hpp>
#include <compiler/error.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <fm/midi.hpp>
#include <fmapp/os.hpp>

namespace {
    std::string toString(const rapidjson::Document &doc) 
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        return buffer.GetString();
    }

    std::string midiToBase64(fm::midi::MidiPtr midi)
    {
        size_t nbytes = midi->byteSize();
        size_t nBase64 = boost::beast::detail::base64::encoded_size(nbytes);
        fm::Byte *bff = new fm::Byte[nbytes];
        char *base64 = new char[nBase64];
        midi->write(bff, nbytes);
        auto writtenBytes = boost::beast::detail::base64::encode(base64, bff, nbytes);
        std::string result(base64, writtenBytes);
        delete []bff;
        delete []base64;
        return result;
    }

    rapidjson::Document documentInfosToJSONDoc(sheet::DocumentPtr sheetDoc, fm::Ticks duration, const sheet::Warnings &warnings)
    {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Value array(rapidjson::kArrayType);
        rapidjson::Value warningsArray(rapidjson::kArrayType);
        rapidjson::Value durationValue((double)duration);
        for (const auto &source : sheetDoc->sources.left) {
            rapidjson::Value object(rapidjson::kObjectType);
            rapidjson::Value sourceId(source.first);
            object.AddMember("sourceId", sourceId, doc.GetAllocator());
            rapidjson::Value path;
            path.SetString(rapidjson::StringRef(source.second.c_str()));
            object.AddMember("path", path, doc.GetAllocator());
            array.PushBack(object, doc.GetAllocator());
	    }
        for (const auto &warning : warnings) {
            rapidjson::Value object(rapidjson::kObjectType);
            rapidjson::Value message;
            rapidjson::Value path;
            path.SetString(warning.getSourceFile(sheetDoc).c_str(), doc.GetAllocator());
            rapidjson::Value sourceId(warning.sourceObject.sourceId);
            rapidjson::Value positionBegin(warning.sourceObject.sourcePositionBegin);
            message.SetString(warning.message.c_str(), doc.GetAllocator());
            object.AddMember("message", message, doc.GetAllocator());
            object.AddMember("sourceFile", path, doc.GetAllocator());
            object.AddMember("sourceId", sourceId, doc.GetAllocator());
            object.AddMember("positionBegin", positionBegin, doc.GetAllocator());
            warningsArray.PushBack(object, doc.GetAllocator());
        }
        doc.AddMember("sources", array, doc.GetAllocator());
        doc.AddMember("duration", durationValue, doc.GetAllocator());
        doc.AddMember("warnings", warningsArray, doc.GetAllocator());
        return doc;
    }
}


namespace fmapp {
    std::string base64Encode(const std::string &data)
    {
        size_t nbytes = data.size();
        size_t nBase64 = boost::beast::detail::base64::encoded_size(nbytes);
        char *base64 = new char[nBase64];
        auto writtenBytes = boost::beast::detail::base64::encode(base64, data.c_str(), nbytes);
        std::string result(base64, writtenBytes);
        delete []base64;
        return result;
    }
    std::string base64Decode(const std::string &base64)
    {
        size_t nBase64 = base64.size();
        size_t nbytes = boost::beast::detail::base64::decoded_size(nBase64);
        char *data = new char[nbytes];
        auto writtenBytes = boost::beast::detail::base64::decode(data, base64.c_str(), nBase64);
        std::string result(data, writtenBytes.first);
        delete []data;
        return result;
    }
    ///////////////////////////////////////////////////////////////////////////
    std::string JsonWriter::funkfeuerToJSON(fm::Ticks elapsedTime, unsigned long lastUpdateTimestamp)
    {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Value elapsed;
        rapidjson::Value pid;
        elapsed.SetDouble(elapsedTime);
        pid.SetInt(fmapp::os::getPId());
        doc.AddMember("sheetTime", elapsed, doc.GetAllocator());
        doc.AddMember("pid", pid, doc.GetAllocator());
        return toString(doc);
    }

    std::string JsonWriter::funkfeuerToJSON(fm::Ticks elapsedTime, unsigned long lastUpdateTimestamp, const std::vector<EventInfo> &eventInfos, bool ignoreTimestamp)
    {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Value elapsed;
        elapsed.SetDouble(elapsedTime);
        rapidjson::Value lastUpdate;
		lastUpdate.SetUint(lastUpdateTimestamp);
        rapidjson::Value pid;
        pid.SetInt(fmapp::os::getPId());
        doc.AddMember("sheetTime", elapsed, doc.GetAllocator());
        doc.AddMember("pid", pid, doc.GetAllocator());
        if (!ignoreTimestamp) {
            doc.AddMember("lastUpdateTimestamp", lastUpdate, doc.GetAllocator());
        }
        rapidjson::Value array(rapidjson::kArrayType);
        for (const auto &eventInfo : eventInfos) {
            rapidjson::Value object(rapidjson::kObjectType);
            rapidjson::Value beginPosition(eventInfo.beginPosition);
            object.AddMember("beginPosition", beginPosition, doc.GetAllocator());
            if (eventInfo.endPosition > 0) {
                rapidjson::Value endPosition(eventInfo.endPosition);
                object.AddMember("endPosition", endPosition, doc.GetAllocator());
            }
            rapidjson::Value sourceId(eventInfo.sourceId);
            object.AddMember("sourceId", sourceId, doc.GetAllocator());
            array.PushBack(object, doc.GetAllocator());
        }
        doc.AddMember("sheetEventInfos", array, doc.GetAllocator());
        return toString(doc);
    }

    std::string JsonWriter::documentInfosToJSON(sheet::DocumentPtr document, fm::Ticks duration, const sheet::Warnings &warnings)
    {
        rapidjson::Document json = documentInfosToJSONDoc(document, duration, warnings);
        return toString(json);
    }

    std::string JsonWriter::exceptionToJSON(const std::exception &ex)
    {
        typedef sheet::compiler::Exception SheetException;
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Value errorMessage;
        const SheetException *fmex                       = dynamic_cast<const SheetException*>(&ex);
        const sheet::ASheetObjectWithSourceInfo *docInfo = fmex ? fmex->getSourceInfo() : nullptr;
        if (fmex && docInfo) {
            const auto sourceFile = fmex->getSourceFile(); 
            rapidjson::Value sourceId(docInfo->sourceId);
            rapidjson::Value positionBegin(docInfo->sourcePositionBegin);
            rapidjson::Value positionEnd(docInfo->sourcePositionEnd);
            errorMessage.SetString(fmex->what(), doc.GetAllocator());
            doc.AddMember("sourceId", sourceId, doc.GetAllocator());
            if (docInfo->sourcePositionBegin != sheet::ASheetObjectWithSourceInfo::UndefinedPosition) {
                doc.AddMember("positionBegin", positionBegin, doc.GetAllocator());
            }
            if (docInfo->sourcePositionEnd != sheet::ASheetObjectWithSourceInfo::UndefinedPosition) {
                doc.AddMember("positionEnd", positionEnd, doc.GetAllocator());
            }
            if (true) {
                rapidjson::Value jsonSourceFile;
                jsonSourceFile.SetString(sourceFile.c_str(), doc.GetAllocator());
                doc.AddMember("sourceFile", jsonSourceFile, doc.GetAllocator());
            }
        } else {
            errorMessage.SetString(ex.what(), doc.GetAllocator());
        }
        doc.AddMember("errorMessage", errorMessage, doc.GetAllocator());
        return toString(doc);
    }

    std::string JsonWriter::midiToJSON(sheet::DocumentPtr sheetDoc, fm::midi::MidiPtr midi, const sheet::Warnings& warnings)
    {
        rapidjson::Document doc = documentInfosToJSONDoc(sheetDoc, midi->duration(), warnings);
        rapidjson::Value midiData;
        rapidjson::Value bpm(midi->bpm());
        midiData.SetString(midiToBase64(midi).c_str(), doc.GetAllocator());
        doc.AddMember("midiData", midiData, doc.GetAllocator());
        doc.AddMember("bpm", bpm, doc.GetAllocator());
        return toString(doc);
    }


    ///////////////////////////////////////////////////////////////////////////

    std::list<VirtualFileDto> JsonReader::readVirtualFS(const std::string & jsonData)
    {
        std::list<VirtualFileDto> result;
        rapidjson::Document document;
        document.Parse(jsonData.c_str());
        if (document.HasParseError()) {
            auto errCode = document.GetParseError();
            std::stringstream ss;
            ss << "failed parsing input json: "  << errCode;
            throw std::runtime_error(ss.str());
        }
        if (!document.IsArray()) {
            throw std::runtime_error("failed to load input json: expected an array");
        }
        for (rapidjson::SizeType i = 0; i < document.Size(); i++) {
            if (!document[i].IsObject()) {
                std::stringstream ss;
                ss << "failed to load input json: expected an object at index: "  << i;
                throw std::runtime_error(ss.str());
            }
            const auto &obj = document[i].GetObject();
            VirtualFileDto dto;
            if (!obj.HasMember("path")) {
                std::stringstream ss;
                ss << "failed to load input json: expected a member \"path\" with object at index: "  << i;
                throw std::runtime_error(ss.str());
            }
            if (!obj.HasMember("data")) {
                std::stringstream ss;
                ss << "failed to load input json: expected a member \"data\" with object at index: "  << i;
                throw std::runtime_error(ss.str());
            }
            dto.path = obj["path"].GetString();
            dto.data = obj["data"].GetString();
            result.emplace_back(dto);
        }
        return result;
    }
}
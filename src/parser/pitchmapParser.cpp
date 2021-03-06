#include "parser.h"
#include "lexer.h"
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include "parserSymbols.h"
#include "error.hpp"

BOOST_FUSION_ADAPT_STRUCT(
	sheet::PitchDef,
	(sheet::PitchDef::Pitch, pitch)
	(sheet::PitchDef::Octave, octave)
)

BOOST_FUSION_ADAPT_STRUCT(
	sheet::Pitchmap,
	(fm::String, name)
	(sheet::PitchDef, pitch)
)


namespace sheet {
	namespace compiler {

		namespace {
			PitchSymbols pitchSymbols_;
			OctaveSymbols octaveSymbols_;
		}

		namespace {
			namespace qi = boost::spirit::qi;
			namespace ascii = boost::spirit::ascii;

			template <typename Iterator>
			struct _PitchmapParser : qi::grammar<Iterator, Pitchmap(), ascii::space_type>
			{
				typedef ChordDef::Intervals Intervals;
				_PitchmapParser() : _PitchmapParser::base_type(start, "chord def")
				{
					using qi::int_;
					using qi::lit;
					using qi::double_;
					using qi::lexeme;
					using ascii::char_;
					using qi::on_error;
					using qi::fail;
					using qi::attr;

					alias_ %= lexeme['"' >> +(char_ - '"') >> '"'];

					pitch_.name("pitch");
					pitch_ %= pitchSymbols_ >> (octaveSymbols_ | attr(PitchDef::DefaultOctave));

					start %= alias_ > ':' > pitch_;

					auto onError = boost::bind(&handler::errorHandler<Iterator>, _1);
					on_error<fail>(start, onError);
				}
				qi::rule<Iterator, Pitchmap(), ascii::space_type> start;
				qi::rule<Iterator, PitchDef(), ascii::space_type> pitch_;
				qi::rule<Iterator, fm::String(), ascii::space_type> alias_;
			};


			void _parse(const fm::String &defStr, Pitchmap &def)
			{
				using boost::spirit::ascii::space;
				typedef _PitchmapParser<fm::String::const_iterator> PitchmapParserType;
				PitchmapParserType g;
				phrase_parse(defStr.begin(), defStr.end(), g, space, def);
			}
		}


		PitchmapParser::PitchmapDefs PitchmapParser::parse(fm::CharType const* first, fm::CharType const* last)
		{

			PitchmapDefs result;
			PitchmapTokenizer<LexerType> pitchDefTok;
			LexerType::iterator_type iter = pitchDefTok.begin(first, last);
			LexerType::iterator_type end = pitchDefTok.end();
			boost::spirit::lex::tokenize(first, last, pitchDefTok);
			for (const auto& defStr : pitchDefTok.pitchdefs) {
				Pitchmap def;
				_parse(defStr, def);
				result.push_back(def);
			}
			return result;
		}
	}
}

#include "Normal.h"
#include "compiler/AContext.h"

namespace sheet {
    namespace compiler {

        void Normal::addEvent(AContext *ctx, const Event::Pitches &pitches, fm::Ticks duration, bool tying)
        {
            auto meta = ctx->voiceMetaData(ctx->voice());
			auto tmpExpression = meta->expression;
			if (meta->singleExpression != fm::expression::Default) {
				tmpExpression = meta->expression;
				meta->expression = meta->singleExpression;
				meta->singleExpression = fm::expression::Default;
			}
			for (const auto &pitch : pitches)
			{
				ctx->addEvent(pitch, duration, tying);
			}
			meta->expression = tmpExpression;
			ctx->seek(duration);
        }

    }
}
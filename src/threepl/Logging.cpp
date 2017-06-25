//
// Created by harold on 20/05/17.
//

#include "Logging.h"

#include <l3pp/l3pp.h>
#include <libpurple/debug.h>

namespace threepl {
    static l3pp::SinkPtr sink;

    /**
     * Logging sink that uses purple's logging facilities
     */
    class PrplSink: public l3pp::Sink {
    public:
        /**
         * Create a PrplSink from some output stream.
         * @param os Output stream.
         */
        static l3pp::SinkPtr create() {
            return l3pp::SinkPtr(new PrplSink());
        }

    protected:
        void logEntry(l3pp::LogLevel level, std::string const& entry) const override {
            switch(level) {
                default:
                case l3pp::LogLevel::TRACE:
                case l3pp::LogLevel::DEBUG:
                    purple_debug(PURPLE_DEBUG_MISC, "threepl", entry.c_str());
                    break;
                case l3pp::LogLevel::INFO:
                    purple_debug(PURPLE_DEBUG_INFO, "threepl", entry.c_str());
                    break;
                case l3pp::LogLevel::WARN:
                    purple_debug(PURPLE_DEBUG_WARNING, "threepl", entry.c_str());
                    break;
                case l3pp::LogLevel::ERR:
                    purple_debug(PURPLE_DEBUG_ERROR, "threepl", entry.c_str());
                    break;
                case l3pp::LogLevel::FATAL:
                    purple_debug(PURPLE_DEBUG_FATAL, "threepl", entry.c_str());
                    break;
            }
        }

    private:
        explicit PrplSink() {}
    };

    void init_logging() {
        sink = PrplSink::create();
        l3pp::Logger::getRootLogger()->addSink(sink);
        l3pp::Logger::getRootLogger()->setLevel(l3pp::LogLevel::ALL);
    }

    void deinit_logging() {
        l3pp::Logger::getRootLogger()->removeSink(sink);
    }
}
#include "core/logger/Logger.h"
#include "core/logger/Logger.h"

int main() {
    farm::Logger::init();
    FARM_LOG_INFO("Test message");
    farm::Logger::shutdown();
    return 0;
}

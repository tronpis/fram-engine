#include "core/logger/Logger.h"
#include <iostream>

int main() {
    farm::Logger::init();
    FARM_LOG_INFO("Test message");
    farm::Logger::shutdown();
    return 0;
}

#include "core/logger.h"

int main() {
    logger_init();
    log_info("Hello friends.");
    logger_quit();
    return 0;
}

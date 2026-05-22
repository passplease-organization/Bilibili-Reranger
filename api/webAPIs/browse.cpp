#include "browse.h"

using namespace webAPI;

BrowseWorker NullWorker(nullptr);

const BrowseWorker &webAPI::nullWorker() {
    return NullWorker;
}

BrowseController BrowseController::controller("");

bool BrowseWorker::valid() const {
    return context != nullptr;
}
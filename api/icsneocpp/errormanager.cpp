#include "icsneo/api/errormanager.h"
#include <memory>

using namespace icsneo;

static std::unique_ptr<ErrorManager> singleton;

ErrorManager& ErrorManager::GetInstance() {
	if(!singleton)
		singleton = std::unique_ptr<ErrorManager>(new ErrorManager());
	return *singleton.get();
}

void ErrorManager::get(std::vector<APIError>& errorOutput, size_t max, ErrorFilter filter) {
	std::lock_guard<std::mutex> lk(mutex);

	if(max == 0) // A limit of 0 indicates no limit
		max = (size_t)-1;

	size_t count = 0;
	errorOutput.clear();
	auto it = errors.begin();
	while(it != errors.end()) {
		if(filter.match(*it)) {
			errorOutput.push_back(*it);
			errors.erase(it++);
			if(count++ >= max)
				break; // We now have as many written to output as we can
		} else {
			it++;
		}
	}
}

bool ErrorManager::getOne(APIError& errorOutput, ErrorFilter filter) {
	std::vector<APIError> output;
	get(output, filter, 1);
	
	if(output.size() == 0)
		return false;
	
	errorOutput = output[0];
	return true;
}

void ErrorManager::discard(ErrorFilter filter) {
	std::lock_guard<std::mutex> lk(mutex);
	errors.remove_if([&filter](const APIError& error) {
		return filter.match(error);
	});
}

size_t ErrorManager::count_internal(ErrorFilter filter) const {
	size_t ret = 0;
	for(auto& error : errors)
		if(filter.match(error))
			ret++;
	return ret;
}

bool ErrorManager::beforeAddCheck(APIError::ErrorType type) {
	if(enforceLimit()) { // The enforceLimit will add the "TooManyErrors" error for us if necessary
		// We need to decide whether to add this error or drop it
		// We would have to remove something if we added this error
		if(APIError::SeverityForType(type) < lowestCurrentSeverity())
			return false; // Don't add this one, we are already full of higher priority items
	}
	return true;
}

bool ErrorManager::enforceLimit() {
	if(errors.size() + 1 < errorLimit)
		return false;
	
	bool hasTooManyWarningAlready = count_internal(ErrorFilter(APIError::TooManyErrors)) != 0;
	size_t amountToRemove = (errors.size() + (hasTooManyWarningAlready ? 0 : 1)) - errorLimit;

	discardLeastSevere(amountToRemove);
	if(!hasTooManyWarningAlready)
		add_internal(APIError::TooManyErrors);
	return true;
}

APIError::Severity ErrorManager::lowestCurrentSeverity() {
	if(errors.empty())
		return APIError::Severity(0);

	APIError::Severity lowest = APIError::Severity::Error;
	auto it = errors.begin();
	while(it != errors.end()) {
		if((*it).getSeverity() < lowest)
			lowest = (*it).getSeverity();
	}
	return lowest;
}

void ErrorManager::discardLeastSevere(size_t count) {
	if(count == 0)
		return;

	ErrorFilter infoFilter(APIError::Severity::Info);
	auto it = errors.begin();
	while(it != errors.end()) {
		if(infoFilter.match(*it)) {
			errors.erase(it++);
			if(--count == 0)
				break;
		}
	}

	if(count != 0) {
		ErrorFilter warningFilter(APIError::Severity::Warning);
		it = errors.begin();
		while(it != errors.end()) {
			if(warningFilter.match(*it)) {
				errors.erase(it++);
				if(--count == 0)
					break;
			}
		}
	}

	if(count != 0) {
		ErrorFilter warningFilter(APIError::Severity::Warning);
		it = errors.begin();
		while(it != errors.end()) {
			if(warningFilter.match(*it)) {
				errors.erase(it++);
				if(--count == 0)
					break;
			}
		}
	}
}
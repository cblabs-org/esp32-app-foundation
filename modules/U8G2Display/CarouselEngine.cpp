/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/CarouselEngine.cpp
 * Description: Implementation of CarouselEngine.
 */

#include "CarouselEngine.hpp"
#include "platform/LogShim.h"

static const char* TAG = "CarouselEngine";

void CarouselEngine::add(IDisplayBoard* board) {
    if (_locked) {
        ESP_LOGE(TAG, "Cannot add board: pool is locked");
        return;
    }
    if (board != nullptr) {
        _pool.push_back(board);
    }
}

void CarouselEngine::lock() {
    _locked = true;
    ESP_LOGI(TAG, "Board pool locked with %d boards", (int)_pool.size());
}

void CarouselEngine::show(IDisplayBoard* board, unsigned long currentMs) {
    if (board == nullptr) return;
    
    for (size_t i = 0; i < _pool.size(); ++i) {
        if (_pool[i] == board) {
            _activeIndex = static_cast<int>(i);
            _lastRotationMs = currentMs;
            break;
        }
    }
}

void CarouselEngine::suspend() {
    _suspended = true;
}

void CarouselEngine::resume() {
    _suspended = false;
}

void CarouselEngine::setInterval(unsigned long ms) {
    _intervalMs = ms;
}

unsigned long CarouselEngine::getInterval() const {
    return _intervalMs;
}

void CarouselEngine::tick(unsigned long currentMs) {
    if (!_suspended && (currentMs - _lastRotationMs) >= _intervalMs) {
        _advance();
        _lastRotationMs = currentMs;
    }
}

IDisplayBoard* CarouselEngine::currentBoard() const {
    if (_pool.empty() || _activeIndex < 0 || _activeIndex >= static_cast<int>(_pool.size())) {
        return nullptr;
    }
    return _pool[_activeIndex];
}

int CarouselEngine::boardCount() const {
    return (int)_pool.size();
}

IDisplayBoard* CarouselEngine::getBoard(int index) const {
    if (index < 0 || index >= (int)_pool.size()) {
        return nullptr;
    }
    return _pool[index];
}

int CarouselEngine::activeIndex() const {
    if (_pool.empty()) {
        return -1;
    }
    return _activeIndex;
}

void CarouselEngine::_advance() {
    if (_pool.empty()) return;

    // Remove expired boards
    for (auto it = _pool.begin(); it != _pool.end(); ) {
        if ((*it)->isExpired()) {
            ESP_LOGI(TAG, "Removing expired board: %s", (*it)->getBoardName());
            it = _pool.erase(it);
            // Adjust active index if necessary
            if (it - _pool.begin() <= _activeIndex && _activeIndex > 0) {
                _activeIndex--;
            }
        } else {
            ++it;
        }
    }

    if (_pool.empty()) {
        _activeIndex = 0;
        return;
    }

    // Advance to next board
    _activeIndex = (_activeIndex + 1) % _pool.size();
}

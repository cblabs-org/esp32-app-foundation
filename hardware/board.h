/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: hardware/board.h
 * Description: Top-level hardware facade selector. Resolves BOARD_HEADER, executes
 *              the contract check, and echoes the resolved path at compile time.
 */

#pragma once

#ifndef BOARD_HEADER
#error "BOARD_HEADER is not defined! Specify -DBOARD_HEADER=\"hardware/<board_id>/board.h\" in platformio.ini"
#endif

#include BOARD_HEADER
#include "board_contract.h"

#pragma message("board: using " BOARD_HEADER)

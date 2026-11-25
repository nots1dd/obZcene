#pragma once

/**
 * @file Env-Vars.hpp
 * @brief Central list of environment variables the utilities read.
 *
 * Kept together so the debug/tracing tooling and its documentation stay in
 * sync. Consumed by core::is_stack_trace_enabled in Backtrace.hpp.
 *
 * @see Backtrace.hpp (OBZCENE_STACK_TRACE_DUMP_ENV)
 */

/** @brief Enables dumping the backtrace event log when set to 1/true/yes/on. */
#define OBZCENE_STACK_TRACE_DUMP_ENV "OBZCENE_STACK_TRACE"

/**
 * @brief Prepends an asset root to every obzcene::Assets registry.
 *
 * Set to a directory path (e.g. `OBZCENE_ASSETS_DIR=/usr/share/mygame/assets`)
 * to override where models/audio are searched without recompiling; the
 * directory is searched before the configured root and the cwd fallback.
 * Consumed by the Assets constructor (src/Engine/Assets.cpp).
 */
#define OBZCENE_ASSETS_DIR_ENV "OBZCENE_ASSETS_DIR"
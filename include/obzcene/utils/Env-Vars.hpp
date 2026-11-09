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
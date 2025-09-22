#pragma once

#include <cstdint>

/**
 * @brief Incrementally decodes UTF-8 encoded bytes into Unicode code points.
 *
 * The decoder keeps track of its progress while bytes are streamed in so that
 * complete code points can be retrieved once all required bytes are available.
 */
struct Utf8Decoder {
    uint32_t codepoint = 0; ///< Accumulates the current Unicode code point.
    uint8_t needed = 0;     ///< Remaining continuation bytes for the sequence.

    /**
     * @brief Feed a single byte of UTF-8 input into the decoder.
     *
     * @param byte Next byte of the UTF-8 encoded stream.
     * @param out  Receives the decoded Unicode code point once complete.
     * @return True if a full code point was decoded and stored in @p out;
     *         otherwise false meaning more bytes are required or the input
     *         sequence was invalid and has been discarded.
     */
    bool feed(uint8_t byte, uint32_t& out);
};

/**
 * @brief Global decoder instance used for handling serial input.
 */
extern Utf8Decoder gUtf;

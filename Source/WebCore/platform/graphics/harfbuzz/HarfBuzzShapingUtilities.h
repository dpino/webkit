/*
 * Copyright (C) 2026 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if USE(HARFBUZZ)

#include <array>
#include <hb-ot.h>
#include <hb.h>

namespace WebCore {

// Finds a script under which the font's GSUB table lists the 'vert' or 'vrt2' feature, so that
// buffers shaped for vertical text pick up the font's vertical glyph substitutions even when that
// feature isn't listed under the default ('dflt') script.
inline hb_script_t findScriptForVerticalGlyphSubstitution(hb_face_t* face)
{
    static const unsigned maxCount = 32;

    unsigned scriptCount = maxCount;
    std::array<hb_tag_t, maxCount> scriptTags;
    hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, &scriptCount, scriptTags.data());
    for (unsigned scriptIndex = 0; scriptIndex < scriptCount; ++scriptIndex) {
        unsigned languageCount = maxCount;
        std::array<hb_tag_t, maxCount> languageTags;
        hb_ot_layout_script_get_language_tags(face, HB_OT_TAG_GSUB, scriptIndex, 0, &languageCount, languageTags.data());
        for (unsigned languageIndex = 0; languageIndex < languageCount; ++languageIndex) {
            unsigned featureIndex;
            if (hb_ot_layout_language_find_feature(face, HB_OT_TAG_GSUB, scriptIndex, languageIndex, HB_TAG('v', 'e', 'r', 't'), &featureIndex)
                || hb_ot_layout_language_find_feature(face, HB_OT_TAG_GSUB, scriptIndex, languageIndex, HB_TAG('v', 'r', 't', '2'), &featureIndex))
                return hb_ot_tag_to_script(scriptTags[scriptIndex]);
        }
    }
    return HB_SCRIPT_INVALID;
}

} // namespace WebCore

#endif // USE(HARFBUZZ)

/*
 * Copyright (C) 2017-2023 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "AlignedMemoryAllocator.h"

#include "BlockDirectory.h"

namespace JSC { 

AlignedMemoryAllocator::AlignedMemoryAllocator() = default;

AlignedMemoryAllocator::~AlignedMemoryAllocator() = default;

void AlignedMemoryAllocator::registerDirectory(BlockDirectory* directory)
{
    RELEASE_ASSERT(!directory->nextDirectoryInAlignedMemoryAllocator());

    m_directories.append(std::mem_fn(&BlockDirectory::setNextDirectoryInAlignedMemoryAllocator), directory);
    m_directoryForEmptyAllocation = m_directories.first();
}

void AlignedMemoryAllocator::prepareForAllocation()
{
    m_directoryForEmptyAllocation = m_directories.first();
}

MarkedBlock::Handle* AlignedMemoryAllocator::findEmptyBlockToSteal()
{
    for (; m_directoryForEmptyAllocation; m_directoryForEmptyAllocation = m_directoryForEmptyAllocation->nextDirectoryInAlignedMemoryAllocator()) {
        if (MarkedBlock::Handle* block = m_directoryForEmptyAllocation->findEmptyBlockToSteal())
            return block;
    }
    return nullptr;
}

} // namespace JSC



//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//
// ReaderShim.h: Currently we are preserving the old interface in SGD. So this shim exposes the old interface and calls into the 
// reader implemented with the new interfaces (reader/packer/transforms/serializers)
//

#pragma once

#include <map>
#include <string>
#include "DataReader.h"
#include <future>
#include "Reader.h"

namespace CNTK
{
    class CompositeMinibatchSource;
}

namespace Microsoft { namespace MSR { namespace CNTK {

typedef ReaderPtr (*ReaderFactory)(const ConfigParameters& parameters);

template <class ElemType>
class ReaderShim : public IDataReader
{
    friend class ::CNTK::CompositeMinibatchSource;
public:
    explicit ReaderShim(ReaderFactory factory);
    virtual ~ReaderShim() { }

    virtual void Init(const ScriptableObjects::IConfigRecord& /*config*/) override
    {
        assert(false);
    }
    virtual void Init(const ConfigParameters& config) override;

    virtual void Destroy() override
    {
        // Make sure there are no outstanding reads.
        if (m_prefetchTask.valid())
        {
            // If there are some, give them time to finish.
            m_prefetchTask.wait_for(std::chrono::seconds(5));
        }

        delete this;
    }

    virtual void StartMinibatchLoop(size_t mbSize, size_t epoch, const std::unordered_set<InputStreamDescription>& inputs, size_t requestedEpochSamples = requestDataSize) override;
    virtual void StartDistributedMinibatchLoop(size_t requestedMBSize, size_t epoch, size_t subsetNum, size_t numSubsets, const std::unordered_set<InputStreamDescription>& inputs, size_t requestedEpochSamples) override;

    virtual void StartMinibatchLoop(size_t, size_t, size_t) override
    {
        LogicError("Not implemented");
    }

    virtual void StartDistributedMinibatchLoop(size_t, size_t, size_t, size_t, size_t) override
    {
        LogicError("Not implemented");
    }

    virtual bool SupportsDistributedMBRead() const override
    {
        return true;
    }

    virtual bool GetMinibatch(StreamMinibatchInputs& matrices) override;

    virtual bool DataEnd() override;

    void CopyMBLayoutTo(MBLayoutPtr) override;

    virtual size_t GetNumParallelSequencesForFixingBPTTMode() override;

private:
    std::pair<bool, bool> PrefetchMinibatch();

    std::future<std::pair<bool, bool>> m_prefetchTask;
    ReaderPtr m_reader;
    ReaderFactory m_factory;
    bool m_endOfEpoch;

    size_t m_numParallelSequences;

    std::map<std::wstring, size_t> m_nameToStreamId;
    std::vector<StreamDescriptionPtr> m_streams;
    launch m_launchType;

    std::map<std::wstring, std::shared_ptr<Matrix<ElemType>>> m_prefetchBuffer;
    std::map<std::wstring, MBLayoutPtr> m_prefetchMbLayouts;

    int m_deviceId;
    bool m_outstandingRead;

    static void FillMatrixFromStream(StorageType type, Matrix<ElemType>* matrix, size_t numRows, const StreamMinibatchPtr& stream);
};

}}}

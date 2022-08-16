#pragma once

#include "handle.hpp"
#include "loader.hpp"
#include "record.hpp"

#include <utils/uuid.hpp>

namespace aln
{
class AssetService;

struct AssetRequest
{
    enum class Type : uint8_t
    {
        Load,
        Unload,
        Invalid,
    };

    enum class State : uint8_t
    {
        Invalid,
        Pending,
        Loading,
        Unloading,
        WaitingForDependencies,
        Installing,
        Complete,
        Failed
    };

    GUID m_requesterEntityID;
    AssetRecord* m_pAssetRecord = nullptr;
    AssetLoader* m_pLoader = nullptr;
    AssetService* m_pAssetService = nullptr;

    Type m_type = Type::Invalid;
    State m_status = State::Invalid;

    std::vector<IAssetHandle> m_dependencies;

    bool IsValid() const { return m_type != Type::Invalid; }
    bool IsLoadingRequest() const { return m_type == Type::Load; }
    bool IsUnloadingRequest() const { return m_type == Type::Unload; }

    void Load();
    void WaitForDependencies();
    void Install();
    void Unload();

    bool IsComplete() { return m_status == State::Complete; }
};
} // namespace aln
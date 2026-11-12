#pragma once

#include "EditorCredentialStore.hpp"

// Compatibility aliases preserve the existing OpenAI helper API for focused tests.
namespace workphone::openai_credentials::detail
{
    using api_credentials::detail::maximumKeySize;
    using api_credentials::detail::openAI;
    using api_credentials::detail::wipeString;
    using api_credentials::detail::SecretGuard;
    using api_credentials::detail::validateKey;
    using api_credentials::detail::CredentialStore;
    using api_credentials::detail::resolve;
}

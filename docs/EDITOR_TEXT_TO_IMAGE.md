# Text to Image in the Editor

Open **Window > Text to Image** to generate a PNG from a description. Open or
create a project first so the Editor has a destination for the new asset.

## Setup

On Windows, open **File > Settings > Editor**, paste your key into the masked
**OpenAI API key** field, and click **Save API Key**. It is available immediately for
new image and AI assistant requests and is remembered after restarting the Editor.
The saved value is not displayed again; use the same field to replace it.

The key is stored in Windows Credential Manager for your Windows user on this
computer, separately from project files and settings exports. **Remove Saved Key**
removes that credential. Existing requests keep the key they started with.

`OPENAI_API_KEY` remains an optional fallback when no Editor key is saved. A saved
Editor key takes precedence. Standalone games and other non-Editor clients use
the environment variable and do not load the personal Editor credential. On
other platforms, continue using `OPENAI_API_KEY`.

The default image model is `gpt-image-2.5-flare`. Set `OPENAI_IMAGE_MODEL` before
launching to use another GPT Image model compatible with the Images API and the
options below. This is independent of the assistant's `OPENAI_MODEL` setting.

Generation sends the prompt to OpenAI and uses your API account. Your account
must have access to the selected model and available API quota.

## Generate an asset

1. Describe the desired image in the prompt field.
2. Enter an asset name, then choose square (1024 x 1024), landscape (1536 x 1024),
   or portrait (1024 x 1536), and low, medium, or high quality.
3. Click **Generate Image**. You can continue using the Editor while the request runs.
4. The generated PNG is saved under the project's `Assets/Generated` directory
   and the project asset browser is refreshed. Existing files are preserved;
   repeated names receive a numbered suffix. A preview appears in the window.

The window reports missing credentials, provider failures and save/import
errors. Generation can take several minutes. **Discard Pending Image** discards
the result; a request already sent to OpenAI can still finish and incur API usage. Another
request can start after the outstanding request finishes. Switching projects
discards the old project's pending result. Closing the Editor waits for an
outstanding request to finish or time out before releasing the AI manager.

## Implementation and verification

`IAiManager::generateImage` returns validated PNG bytes or an error and never
applies scene actions. The Editor owns background execution and project asset
publication. The HTTPS request uses `/v1/images/generations` with one PNG result,
a five-minute Windows transport timeout, a 32 MiB response limit and a 24 MiB
decoded PNG limit. The service accepts prompts up to 32,000 UTF-8 bytes and the
size/quality choices above; the Editor uses its existing text-entry control.

Offline regression coverage uses a mocked transport and temporary project
directories, so it needs no credentials and makes no paid requests. It covers
request construction, input validation, provider errors, malformed responses,
PNG decoding and safe asset filenames/publication.

Build the `AiImageGenerationTests` and `TextToImageAssetTests` targets, then run
the CTest entries `WPAI.image_generation` and `WPEditor.text_to_image_assets`.
`OpenAICredentialTests` / `WPAI.editor_credentials` checks credential precedence,
validation, replacement and removal. Its persistence fixture uses a unique test
credential and removes it afterward; it never accesses your saved Editor key.

API contract: [OpenAI Images API reference](https://developers.openai.com/api/reference/python/resources/images/methods/generate).

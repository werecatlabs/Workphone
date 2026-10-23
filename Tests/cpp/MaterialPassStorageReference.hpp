#ifndef WP_TEST_MATERIAL_PASS_STORAGE_REFERENCE_HPP
#define WP_TEST_MATERIAL_PASS_STORAGE_REFERENCE_HPP

// Frozen immediately preceding storage shape. This reproduces the inline
// rendered values, texture owners and publication bookkeeping without creating
// another live material implementation. The optional payloads and publications
// are charged separately in population comparisons.
struct InlinePassStateStorageBeforeSurfaceMigration : workphone::StateData
{
    workphone::FixedArray<workphone::SmartPtr<workphone::render::ITexture>, 32> textures;
    workphone::ColourF colours[5];
    workphone::f32 renderedFloats[18];
    workphone::u32 renderedSelectors[12];
    workphone::MaterialType materialType;
    workphone::hash_type renderTechnique;
    std::unique_ptr<workphone::u32[]> numericAuthoring;
    std::unique_ptr<char[]> stringAuthoring;
    workphone::MaterialRenderState::Ptr publication;
    workphone::u32 writeDepth;
};

#endif

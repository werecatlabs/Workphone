#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Memory/MemoryPool.hpp>

namespace workphone
{

    MemoryPool::MemoryPool()
    {
    }

    MemoryPool::MemoryPool( u32 block_size, u32 block_count ) :
        block_size_( block_size ),
        block_count_( block_count ),
        pool_( new char[block_size * block_count] ),
        head_( pool_ )
    {
        for( std::size_t i = 0; i < block_count_ - 1; ++i )
        {
            char *block = pool_ + ( i * block_size_ );
            char **next_block = reinterpret_cast<char **>( block );
            *next_block = block + block_size_;
        }

        char *last_block = pool_ + ( ( block_count_ - 1 ) * block_size_ );
        char **next_block = reinterpret_cast<char **>( last_block );
        *next_block = nullptr;
    }

    MemoryPool::~MemoryPool()
    {
        delete[] pool_;
    }

    auto MemoryPool::allocate() -> void *
    {
        if( head_ == nullptr )
        {
            return nullptr;
        }

        void *block = head_;
        head_ = *reinterpret_cast<char **>( head_ );
        return block;
    }

    void MemoryPool::deallocate( void *block )
    {
        *reinterpret_cast<char **>( block ) = head_;
        head_ = static_cast<char *>( block );
    }

}  // namespace workphone

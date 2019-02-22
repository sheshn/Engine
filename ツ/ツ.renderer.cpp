#include "ツ.renderer.h"
#include "ツ.math.h"
#include "ツ.intrinsic.h"

Renderer_Buffer renderer_create_buffer_reference(u32 id)
{
    Renderer_Buffer buffer = {id};
    return buffer;
}
Renderer_Texture renderer_create_texture_reference(u32 id, u32 width, u32 height)
{
    Renderer_Texture texture = {id, (u16)width, (u16)height};
    return texture;
}

void renderer_init_transfer_queue(Renderer_Transfer_Queue* queue, u8* memory, u64 memory_size)
{
    *queue = {memory, memory_size};
}

Renderer_Transfer_Operation* renderer_request_transfer(Renderer_Transfer_Queue* queue, Renderer_Transfer_Operation_Type type, u64 transfer_size)
{
    Renderer_Transfer_Operation* operation = NULL;
    if (queue->operation_count < array_count(queue->operations) && queue->transfer_memory_used + transfer_size <= queue->transfer_memory_size)
    {
        // TODO: Proper alignment! May need to move to Vulkan side
        u64 size = transfer_size;
        if (queue->enqueue_location + transfer_size >= queue->transfer_memory_size)
        {
            if (transfer_size < queue->dequeue_location)
            {
                queue->enqueue_location = 0;
                size += queue->transfer_memory_size - queue->enqueue_location;
            }
            else
            {
                return NULL;
            }
        }

        operation = &queue->operations[(queue->operation_index++ % array_count(queue->operations))];
        assert(operation->state == RENDERER_TRANSFER_OPERATION_STATE_UNLOADED || operation->state == RENDERER_TRANSFER_OPERATION_STATE_FINISHED);

        atomic_increment(&queue->operation_count);

        operation->memory = queue->transfer_memory + queue->enqueue_location;
        operation->size = size;
        operation->state = RENDERER_TRANSFER_OPERATION_STATE_UNLOADED;
        operation->type = type;

        queue->enqueue_location = (queue->enqueue_location + transfer_size) % queue->transfer_memory_size;
        atomic_add((s64*)&queue->transfer_memory_used, size);

        assert(queue->transfer_memory_used <= queue->transfer_memory_size);
    }

    return operation;
}

void renderer_queue_transfer(Renderer_Transfer_Queue* queue, Renderer_Transfer_Operation* operation)
{
    operation->state = RENDERER_TRANSFER_OPERATION_STATE_READY;
}
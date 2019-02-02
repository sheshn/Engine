#include "ツ.renderer.h"

void renderer_init_transfer_queue(Renderer_Transfer_Queue* queue, u8* memory, u64 memory_size)
{
    *queue = {memory, memory_size};
}

Renderer_Transfer_Operation* renderer_request_transfer(Renderer_Transfer_Queue* queue, u64 transfer_size)
{
    Renderer_Transfer_Operation* operation = NULL;
    if (queue->operation_count < array_count(queue->operations) && queue->transfer_memory_used + transfer_size <= queue->transfer_memory_size)
    {
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
#include "simulator.h"

int refer_page(Memory* memory, PageMap page_map_table[], int page_index, int time);
void assign_page(Memory* memory, PageMap page_map_table[], int page_index, int page_frame_index, int time);
void release_page(Memory* memory, PageMap page_map_table[], int page_index, int page_frame_index);
int _MIN_find_victim_page_frame_index(Memory memory, Input input, int current_index);
int _FIFO_find_victim_page_frame_index(Memory memory, PageMap page_map_table[]);
int _LRU_find_victim_page_frame_index(Memory memory, PageMap page_map_table[]);
int _LFU_find_victim_page_frame_index(Memory memory, PageMap page_map_table[]);
void _WS_increase_page_frame(Memory* memory);
void _WS_decrease_page_frame(Memory* memory, PageMap page_map_table[], Input input, int time);
int _find_empty_page_frame_index(Memory memory);
void copy_memory(Memory* target, Memory* source);
void _print_memory(Memory memory);

SimulationResult* simulate(Input input, const char* replacement_technique)
{
    SimulationResult* simulation_result = (SimulationResult*) malloc(sizeof(SimulationResult));
    simulation_result->number_of_page_reference = input.number_of_page_reference;
    simulation_result->page_fault_history = (char *) malloc(sizeof(char) * input.number_of_page_reference);
    simulation_result->memory_history = (Memory *) malloc(sizeof(Memory) * input.number_of_page_reference);
    simulation_result->page_references = (int *) malloc(sizeof(int) * input.number_of_page_reference);

    int i;
    for (i = 0; i < input.number_of_assigned_page_frame; i++)
    {
        simulation_result->page_fault_history[i] = 0;
    }

    Memory memory;

    if (strcmp(replacement_technique, "WS") == 0)
    {   
        memory.number_of_page_frame = 0;
        memory.page_frames = (int *) malloc(sizeof(int) * 0);
    }
    else
    {
        memory.number_of_page_frame = input.number_of_assigned_page_frame;
        memory.page_frames = (int *) malloc(sizeof(int) * input.number_of_assigned_page_frame);
    }
    PageMap page_map_table[input.number_of_page_in_process];

    for (i = 0; i < input.number_of_assigned_page_frame; i++)
    {
        memory.page_frames[i] = -1;
    }

    for (i = 0; i < input.number_of_page_in_process; i++)
    {
        page_map_table[i].assigned_page_frame_index = -1;
        page_map_table[i].assigned_time = -1;
        page_map_table[i].reference_time = -1;
        page_map_table[i].reference_count = 0;
    }

    for (i = 0; i < input.number_of_page_reference; i++)
    {
        int referenced_page_index = input.page_references[i];

        if (strcmp(replacement_technique , "WS") == 0)
        {
            _WS_decrease_page_frame(&memory, page_map_table, input, i);
        }

        int page_frame_index = refer_page(&memory, page_map_table, referenced_page_index, i);

        if (page_frame_index == -1)
        {
            if (strcmp(replacement_technique, "WS") == 0)
            {
                _WS_increase_page_frame(&memory);
            }

            int empty_page_frame_index = _find_empty_page_frame_index(memory);
        
            if (empty_page_frame_index == -1)
            {
                int victim_page_frame_index;
                if (strcmp(replacement_technique, "MIN") == 0) victim_page_frame_index = _MIN_find_victim_page_frame_index(memory, input, i);
                else if (strcmp(replacement_technique, "FIFO") == 0) victim_page_frame_index = _FIFO_find_victim_page_frame_index(memory, page_map_table);
                else if (strcmp(replacement_technique, "LRU") == 0) victim_page_frame_index = _LRU_find_victim_page_frame_index(memory, page_map_table);
                else if (strcmp(replacement_technique, "LFU") == 0) victim_page_frame_index = _LFU_find_victim_page_frame_index(memory, page_map_table);
                int victim_page_index = memory.page_frames[victim_page_frame_index];


                release_page(&memory, page_map_table, victim_page_index, victim_page_frame_index);
                assign_page(&memory, page_map_table, referenced_page_index, victim_page_frame_index, i);
            }
            else
            {
                assign_page(&memory, page_map_table, referenced_page_index, empty_page_frame_index, i);
            }    
        }
        
        simulation_result->page_references[i] = referenced_page_index;
        if (page_frame_index == -1)
        {
            simulation_result->page_fault_history[i] = 1;
        }
        Memory record_memory;
        copy_memory(&record_memory, &memory);
        simulation_result->memory_history[i] = record_memory;
    }

    return simulation_result;
}

int _MIN_find_victim_page_frame_index(Memory memory, Input input, int current_index)
{
    int max_length = 0;
    int max_page_frame_index = -1;
    int i;
    for (i = 0; i < input.number_of_assigned_page_frame; i++)
    {
        int page_index = memory.page_frames[i];
        
        int j;
        for (j = current_index + 1; j < input.number_of_page_reference; j++)
        {
            int referenced_page_index = input.page_references[j];

            if (referenced_page_index == page_index) break;
        }
        
        if (j > max_length)
        {
            max_length = j;
            max_page_frame_index = i;
        }
    }
    return max_page_frame_index;
}

int _FIFO_find_victim_page_frame_index(Memory memory, PageMap page_map_table[])
{
    int min_assigned_time = page_map_table[memory.page_frames[0]].assigned_time;
    int min_page_frame_index = 0;
    int i;
    for (i = 0; i < memory.number_of_page_frame; i++)
    {
        int page_index = memory.page_frames[i];

        if (page_map_table[page_index].assigned_time < min_assigned_time)
        {
            min_assigned_time = page_map_table[page_index].assigned_time;
            min_page_frame_index = i;
        }
    }

    return min_page_frame_index;
}

int _LRU_find_victim_page_frame_index(Memory memory, PageMap page_map_table[])
{
    int min_reference_time = page_map_table[memory.page_frames[0]].reference_time;
    int min_page_frame_index = 0;
    int i;
    for (i = 0; i < memory.number_of_page_frame; i++)
    {
        int page_index = memory.page_frames[i];

        if (page_map_table[page_index].reference_time < min_reference_time)
        {
            min_reference_time = page_map_table[page_index].reference_time;
            min_page_frame_index = i;
        }
    }

    return min_page_frame_index;
}

int _LFU_find_victim_page_frame_index(Memory memory, PageMap page_map_table[])
{
    int min_reference_count = page_map_table[memory.page_frames[0]].reference_count;
    int min_page_frame_index = 0;
    int min_reference_time = page_map_table[memory.page_frames[0]].reference_time;
    int i;
    for (i = 0; i < memory.number_of_page_frame; i++)
    {
        int page_index = memory.page_frames[i];

        if (
            page_map_table[page_index].reference_count < min_reference_count
            || (
                page_map_table[page_index].reference_count == min_reference_count
                && page_map_table[page_index].reference_time < min_reference_time
            )
        )
        {
            min_reference_count = page_map_table[page_index].reference_count;
            min_page_frame_index = i;
            min_reference_time = page_map_table[page_index].reference_time;
        }
    }

    return min_page_frame_index;
}

void _WS_increase_page_frame(Memory* memory)
{
    memory->number_of_page_frame += 1;
    memory->page_frames = realloc(memory->page_frames, sizeof(int) * memory->number_of_page_frame);
    memory->page_frames[memory->number_of_page_frame - 1] = -1;
}

void _WS_decrease_page_frame(Memory* memory, PageMap page_map_table[], Input input, int time)
{
    if (time > input.window_size)
    {
        int oldest_time = time - input.window_size - 1;

        int oldest_page_index = input.page_references[oldest_time];

        int need_decrease = 1;
        int i;
        for (i = oldest_time + 1; i <= time && i > 0 ; i++)
        {
            if (input.page_references[i] == oldest_page_index)
            {
                need_decrease = 0;
                break;
            }
        }

        if (need_decrease == 1)
        {
            int target_page_frame_index = page_map_table[oldest_page_index].assigned_page_frame_index;
            memory->number_of_page_frame -= 1;
            int* new_page_frames = (int*) malloc(sizeof(int) * memory->number_of_page_frame);

            for (i = 0; i < memory->number_of_page_frame; i++)
            {
                if (i < target_page_frame_index)
                {
                    new_page_frames[i] = memory->page_frames[i];
                }
                else
                {
                    new_page_frames[i] = memory->page_frames[i + 1];
                }
                page_map_table[new_page_frames[i]].assigned_page_frame_index = i;
            }

            memory->page_frames = new_page_frames;
            page_map_table[oldest_page_index].assigned_page_frame_index = -1;
        }
    } 
}

int _find_empty_page_frame_index(Memory memory)
{
    int finded_index = -1;
    int i;
    for (i = 0; i < memory.number_of_page_frame; i++)
    {
        if (memory.page_frames[i] == -1)
        {
            finded_index = i;
            break;
        }
    }

    return finded_index;
}

void copy_memory(Memory* target, Memory* source)
{
    target->number_of_page_frame = source->number_of_page_frame;
    target->page_frames = (int *) malloc(sizeof(int *) * source->number_of_page_frame);
    
    int i;
    for (i = 0; i < source->number_of_page_frame; i++)
    {
        target->page_frames[i] = source->page_frames[i];
    }
}

void _print_memory(Memory memory)
{
    int i;
    for (i = 0; i < memory.number_of_page_frame; i++)
    {
        printf(" %d | ", memory.page_frames[i]);
    }
    printf("\n");
}

int refer_page(Memory* memory, PageMap page_map_table[], int page_index, int time)
{
    int page_frame_index = page_map_table[page_index].assigned_page_frame_index;
    page_map_table[page_index].reference_time = time;
    page_map_table[page_index].reference_count += 1;

    return page_frame_index;
}

void assign_page(Memory* memory, PageMap page_map_table[], int page_index, int page_frame_index, int time)
{
    memory->page_frames[page_frame_index] = page_index;
    page_map_table[page_index].assigned_page_frame_index = page_frame_index;
    page_map_table[page_index].assigned_time = time;
}

void release_page(Memory* memory, PageMap page_map_table[], int page_index, int page_frame_index)
{
    memory->page_frames[page_frame_index] = -1;
    page_map_table[page_index].assigned_page_frame_index = -1;
}

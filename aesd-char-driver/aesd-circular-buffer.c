/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */
#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/slab.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"


/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    uint8_t index = buffer->out_offs;
    struct aesd_buffer_entry *entry;
    size_t total_bytes =0;
    int j =0 ;
    size_t each_entry_size =0; 
    for(j = 0; ((index != buffer->in_offs) || buffer->full) && (j < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED); j++)
    {
    	entry = &buffer->entry[index];
    	each_entry_size = entry->size;
    	if (char_offset < total_bytes + each_entry_size) {
        	*entry_offset_byte_rtn = char_offset - total_bytes;
        	return entry;
    	}
    	total_bytes = total_bytes + each_entry_size;
    	index = (index + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }


    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
	const char *overflow =NULL;
    //Implemented as per description
    if( (add_entry != NULL) && (buffer != NULL) )
    {
	if( (buffer->full) && (buffer->out_offs == buffer->in_offs ))
    	{
     	   overflow = buffer->entry[buffer->in_offs].buffptr;
    	}
        if(buffer->full)
        {
	   
            buffer->out_offs = (buffer->out_offs + 1 ) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED ;
            buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
            buffer->entry[buffer->in_offs].size = add_entry->size;
	    buffer->in_offs = (buffer->in_offs + 1) %  AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED ;
        } 
        else 
        {
            buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
            buffer->entry[buffer->in_offs].size = add_entry->size;
	    buffer->in_offs = (buffer->in_offs + 1) %  AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
            if (buffer->in_offs == buffer->out_offs) 
            {
                buffer->full = true;  
            }
        }
    }
    else 
    {
        // Invalid arguments
	return NULL;
    }
    return overflow;
}


/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}

/**
 *  clean up function
 */

void cbuffer_exit_cleanup(struct aesd_circular_buffer *buf)
{
        struct aesd_buffer_entry *entry;
        uint8_t iter;

        AESD_CIRCULAR_BUFFER_FOREACH(entry, buf, iter)
        {
                if(entry->buffptr != NULL)
                {
#ifdef __KERNEL__
                        kfree(entry->buffptr);

#endif
                }
        }
}
